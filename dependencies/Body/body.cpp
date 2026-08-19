#include <cmath>
#include <random>

#include <iostream> // TEMP

#include "body.h"

namespace body
{
    /* Body Class */
    #pragma region Body Class

    // Creates body at given position (as two numbers)
    Body::Body(const float &x, const float &y): position {x, y}, velocity {0, 0} {}

    // Creates body at given position (as an array)
    Body::Body(const std::array<float, 2> &position): position(position), velocity{0, 0} {}

    /* Interactions Between Two Bodies */
    // Updates body's velocity based on given body
    void Body::Interact(const Body &other, const float &radius, const float &repelRadius, const float &factor, const float &repelFactor, const float deltaTime)
    {
            const auto &otherPos = other.GetPosition();
        float dx = otherPos[0] - position[0];
        float dy = otherPos[1] - position[1];
        float distSq = dx*dx + dy*dy; // squared distance, avoids sqrt until we need a real length

        if (distSq < 1e-8f)
        {
            // Near or exact overlap: just skip interaction to avoid random forces
            return;
        }

        float dist = std::sqrt(distSq); // actual separation distance
        dx /= dist; // normalize direction to unit length
        dy /= dist;

        // Bodies too close
        if (dist < repelRadius)
        {    
            // Repulsion strength increases the closer the bodies are
            float strength = (repelRadius - dist) / repelRadius;
            strength *= strength; // Exponential growth

            float impulse = strength * repelFactor * deltaTime;
            float maxImpulse = 0.10f * deltaTime;
            if (impulse > maxImpulse)
                impulse = maxImpulse;

            // Repels particles who are too close
            velocity[0] -= dx * impulse;
            velocity[1] -= dy * impulse;
        }
        else if (dist < radius)
        {
            // Pulls particles closer together with stronger short-range attraction
            float normalized = (radius - dist) / radius;
            float strength = normalized * normalized; // stronger as they get closer

            float impulse = strength * factor * deltaTime;
            float maxImpulse = 0.15f * deltaTime;
            if (impulse > maxImpulse)
                impulse = maxImpulse;

            velocity[0] += dx * impulse;
            velocity[1] += dy * impulse;
        }
        // Wrapping: if the other particle is outside the direct interaction radius,
        // we still have to check the toroidal boundary by comparing against a mirrored position.
        else
        {
            // How far this particle is from each edge of the normalized [-1,1] world.
            std::array<float, 2> distFromEdge = {1.0f - std::abs(position[0]), 1.0f - std::abs(position[1])}; // 1 - |axis|

            int xSign = (position[0] > 0.0f) ? 1 : ((position[0] < 0.0f) ? -1 : 0);
            int ySign = (position[1] > 0.0f) ? 1 : ((position[1] < 0.0f) ? -1 : 0);

            std::array<float, 2> shadowPos;

            if(distFromEdge[0] <= radius && !(distFromEdge[1] <= radius)) // Body is near the edge of the x axis
            {
                // Create a shadow position on the opposite x side, same y, to simulate wrap-around forces
                shadowPos = {-xSign - (xSign * distFromEdge[0]), position[1]};
            }
            else if(distFromEdge[1] <= radius && !(distFromEdge[0] <= radius)) // Body is near the edge of the y axis
            {
                // Create a shadow position on the opposite y side, same x
                shadowPos = {position[0], -ySign - (ySign * distFromEdge[1])};
            }
            else if(distFromEdge[0] <= radius && distFromEdge[1] <= radius) // Body is near the edge of the x AND y axis
            {
                // Wrap on both axes for a corner case near the world boundary
                shadowPos = {-xSign - (xSign * distFromEdge[0]), -ySign - (ySign * distFromEdge[1])};
            }
            else // None of these are true; particles shouldn't interact with one another across the boundary
                return;

            // Normal force calculation, but using the wrapped shadow position across the boundary
                const auto &otherPos = other.GetPosition();
            float dx = otherPos[0] - shadowPos[0];
            float dy = otherPos[1] - shadowPos[1];
            float distSq = dx*dx + dy*dy; // squared distance for performance
            if (distSq == 0.0f)
                return;

            float dist = std::sqrt(distSq);
            dx /= dist; // normalize the direction vector
            dy /= dist;

            // Bodies too close
            if (dist < repelRadius)
            {
                // Repulsion strength increases the closer the bodies are
                float strength = (repelRadius - dist) / repelRadius;
                strength *= strength;

                float impulse = strength * repelFactor * deltaTime;
                float maxImpulse = 0.10f * deltaTime;
                if (impulse > maxImpulse)
                    impulse = maxImpulse;

                velocity[0] -= dx * impulse;
                velocity[1] -= dy * impulse;
            }
            else if (dist < radius)
            {
                // Pulls particles closer together with stronger short-range attraction
                float normalized = (radius - dist) / radius;
                float strength = normalized * normalized; // stronger as they get closer

                float impulse = strength * factor * deltaTime;
                float maxImpulse = 0.15f * deltaTime;
                if (impulse > maxImpulse)
                    impulse = maxImpulse;

                velocity[0] += dx * impulse;
                velocity[1] += dy * impulse;
            }
        }

        // Count this particle interaction so the update step can detect unstable particles
        interactionsCount++;
    }

    /* Per-Frame Particle Updates */
    // Updates position based on velocity
    void Body::Update(const float &deltaTime)
    {
        // Limits particles max velocity to keep them from shooting too fast
        // Apply quadratic drag: stronger damping at high speeds, weaker at low speeds.
        // This prevents overshoot and scatter-bomb bouncing while allowing smooth settling.
        const float dragCoefficient = 0.8f;
        if (deltaTime > 0.0f)
        {
            float speed = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);
            if (speed > 0.0f)
            {
                float dragFactor = 1.0f - (dragCoefficient * speed * deltaTime);
                if (dragFactor < 0.0f) dragFactor = 0.0f;  // Prevent velocity reversal
                velocity[0] *= dragFactor;
                velocity[1] *= dragFactor;
            }
        }

        // Enforce an overall velocity magnitude cap instead of per-axis clamping.
        float clampedSpeed = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);
        if (clampedSpeed > maxVelocity)
        {
            float scale = maxVelocity / clampedSpeed;
            velocity[0] *= scale;
            velocity[1] *= scale;
        }

        // Cluster punishment disabled: damping alone handles settling now without random jerks.

        // Toroidal world wrap: if the particle goes past one edge, it reappears on the opposite side
        if(position[0] < -1 || (std::isnan(position[0]) && std::signbit(position[0])))
            position[0] = 1;
        else if(1 < position[0] || (std::isnan(position[0]) && !std::signbit(position[0])))
            position[0] = -1;

        if(position[1] < -1 || (std::isnan(position[1]) && std::signbit(position[1])))
            position[1] = 1;
        else if(1 < position[1] || (std::isnan(position[1]) && !std::signbit(position[1])))
            position[1] = -1;

        // Updates particle position based on current velocity
        position[0] += velocity[0] * deltaTime;
        position[1] += velocity[1] * deltaTime;

        // Resets interaction counter for the next frame
        interactionsCount = 0;
    }

    /* Position Setting */
    // Sets body's position
    void Body::SetPosition(const float &x, const float &y)
    {
        position[0] = x;
        position[1] = y;
    }

    /* Position Getting */
    // Returns body's position
    [[nodiscard]] const std::array<float, 2> &Body::GetPosition() const
    {
        return position;
    }

    /* Velocity Damping */
    // Applies damping to velocity
    void Body::Damp(const float &factor)
    {
        velocity[0] -= factor * sqrt(velocity[0]);
        velocity[1] -= factor * sqrt(velocity[1]);
    }

    /* Apply an immediate velocity impulse */
    void Body::ApplyImpulse(const float &ix, const float &iy)
    {
        velocity[0] += ix;
        velocity[1] += iy;
    }

    #pragma endregion

    // Any functions related to Body

    /* Distance Between Bodies */
    // Helper function for getting distance between two points/bodies
    [[nodiscard]] const float GetDistance(std::array<float, 2> const &a, std::array<float, 2> const &b)
    {
        float dx = b[0] - a[0];
        float dy = b[1] - a[1];
        return std::sqrt(dx*dx + dy*dy);
    }

    /* Distance Between Bodies */
    // Helper function for getting distance between two points/bodies
    [[nodiscard]] const float GetDistance(const Body &a, const Body &b)
    {
        const auto &apos = a.GetPosition();
        const auto &bpos = b.GetPosition();
        float dx = bpos[0] - apos[0];
        float dy = bpos[1] - apos[1];
        return std::sqrt(dx*dx + dy*dy);
    }
}