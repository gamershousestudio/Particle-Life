#ifndef BODY_H
#define BODY_H

#include <array>

// Helper functions for objects with physics
namespace body
{
    // Actual body class
    class Body
    {
        private:
            int interactionsCount; // Number of interactions since last update
        protected:
            std::array<float, 2> position; // Stores current position of body
            std::array<float, 2> velocity; // Stores current velocity of body

            bool stable = true; // Should particles remain stable over large numbers of interactions
            int maxInteractions = 30; // Not actual max; just when destabalizing starts

        public:
            float maxVelocity = 1.0f; // Highest speed a particle can be

            Body() = default; // Default constructor; creates body at 0, 0

            Body(const float &x, const float &y); // Creates body at given position
            Body(const std::array<float, 2> &position); // Creates body at given position

            void Interact(const Body &other, const float &radius, const float &repelRadius, const float &factor, const float &repelFactor, const float deltaTime=1); // Updates body's velocity based on given body

            void Update(const float &deltaTime); // Updates position based on velocity

            void SetPosition(const float &x, const float &y); // Sets body's position
            [[nodiscard]] const std::array<float, 2> &GetPosition() const; // Returns body's position

            // Apply an immediate velocity impulse (used by optimized interaction loop)
            void ApplyImpulse(const float &ix, const float &iy);

            void Damp(const float &factor);  // Applies damping to velocity
    };

    // Any functions related to Body

    // Distance between two points/bodies
    [[nodiscard]] const float GetDistance(std::array<float, 2> const &a, std::array<float, 2> const &b);
    [[nodiscard]] const float GetDistance(const Body &a, const Body &b);
}

#endif
