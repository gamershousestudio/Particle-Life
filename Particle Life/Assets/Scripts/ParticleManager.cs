using System.Runtime.CompilerServices;
using UnityEngine;

public class ParticleManager : MonoBehaviour
{
    [HideInInspector] public GameObject[] particles;

    // Algorithm variables
    [Header("Algorithm Variables")]

    public bool randomizeInteractions;

    // FOLLOWING FIVE LINES ARE ONLY UNTIL MORE CUSTOMIZATION IS ADDED TO CUSTOM INTERACTIONS
    public float repelForce;
    public float repelRadius;
    public float interactForce;
    public float interactRadius;
    public float wrappedBuffer;

    // Initialize update functions
    private void Update()
    {
        ParticleInteract();

        RandomizeInteractions();
    }

    private void ParticleInteract()
    {
        ParticleSpawner script = GameObject.FindAnyObjectByType<ParticleSpawner>();

        // For each loop variables
        // Variables created here because in today's economy memory ain't free
        Vector2 distance;
        Vector2 direction;

        float hypotenuse;
        float force = 0f;

        Gradient gradient = new Gradient();

        GradientColorKey[] keys = new GradientColorKey[2];
        GradientAlphaKey[] alpha = new GradientAlphaKey[2];

        Vector2 ghost;
        Vector2 main; // NOTE: not actually the main particle, I just suck at coming up with names

        Vector3 size = new Vector2(script.xRange * script.sceneSize, script.yRange * script.sceneSize);

        // Loops through every particle to see how it should move
        foreach (GameObject inst in particles)
        {
            Vector2 vel = new Vector2(0, 0);

            // Checks for nearby particles
            Collider2D[] nearby = Physics2D.OverlapCircleAll(inst.transform.position, interactRadius); // 3 is layermask for particles

            // Finds how the nearby should impact the particle
            foreach (Collider2D other in nearby)
            {
                if (other.gameObject == inst) continue;
                if (other.tag != "Particle") continue;

                distance = inst.transform.position - other.transform.position;
                direction = distance.normalized;

                hypotenuse = Mathf.Sqrt(Mathf.Pow(distance.x, 2) + Mathf.Pow(distance.y, 2));

                // Particles are close enough to repel
                if (hypotenuse <= repelRadius)
                {
                    force = inst.GetComponent<Particle>().Repel(hypotenuse);
                }
                // Particles should go based on interaction force
                else
                {
                    force = inst.GetComponent<Particle>().Interact(hypotenuse);
                }

                // Creates a gradient to display interactions
                keys[0] = new GradientColorKey(Color.green, -1.0f);
                keys[1] = new GradientColorKey(Color.red, 1.0f);

                alpha[0] = new GradientAlphaKey(1.0f, -1.0f);
                alpha[1] = new GradientAlphaKey(1.0f, 1.0f);

                gradient.SetKeys(keys, alpha);

                Debug.DrawLine(inst.transform.position, other.transform.position, gradient.Evaluate(force / Mathf.Abs(force + .001f)), 0.1f);

                vel += direction * force;
            }

            // Checks if wrapped interactions should be applied
            float bufferX = script.xRange * script.sceneSize - interactRadius * wrappedBuffer;
            float bufferY = script.yRange * script.sceneSize - interactRadius * wrappedBuffer;

            if (inst.transform.position.x > bufferX || inst.transform.position.x < -bufferX ||
                inst.transform.position.y > bufferY || inst.transform.position.y < -bufferY)
            {
                foreach (GameObject other in particles)
                {
                    if ((inst.transform.position.x > bufferX && other.transform.position.x < -bufferX) ||
                        (inst.transform.position.x < -bufferX && other.transform.position.x > bufferX))
                    {
                        if (inst.transform.position.x < other.transform.position.x) // inst == lesser
                        {
                            main = inst.transform.position + 2 * new Vector3(size.x, 0, 0);
                            ghost = other.transform.position;
                        }
                        else // other == lesser
                        {
                            ghost = other.transform.position + 2 * new Vector3(size.x, 0, 0);
                            main = inst.transform.position;
                        }
                    }
                    else if ((inst.transform.position.y > bufferY && other.transform.position.y < -bufferY) ||
                        (inst.transform.position.y < -bufferY && other.transform.position.y > bufferY))
                    {
                        if (inst.transform.position.y < other.transform.position.y) // inst == lesser
                        {
                            main = inst.transform.position + 2 * new Vector3(0, size.y, 0);
                            ghost = other.transform.position;
                        }
                        else // other == lesser
                        {
                            ghost = other.transform.position + 2 * new Vector3(0, size.y, 0);
                            main = inst.transform.position;
                        }
                    }
                    else
                    {
                        continue;
                    }

                    // Normal interaction calculations, just with "ghost particle"s this time
                    distance = main - ghost;
                    direction = distance.normalized;

                    hypotenuse = Mathf.Sqrt(Mathf.Pow(distance.x, 2) + Mathf.Pow(distance.y, 2));

                    // Particles are close enough to repel
                    if (hypotenuse <= repelRadius)
                    {
                        force = inst.GetComponent<Particle>().Repel(hypotenuse);
                    }
                    // Particles should go based on interaction force
                    else
                    {
                        force = inst.GetComponent<Particle>().Interact(hypotenuse);
                    }

                    if (force != 0)
                    {
                        // Creates a gradient to display interactions
                        keys[0] = new GradientColorKey(Color.gray, -1.0f); // Attract
                        keys[1] = new GradientColorKey(Color.black, 1.0f); // Repel

                        alpha[0] = new GradientAlphaKey(1.0f, -1.0f);
                        alpha[1] = new GradientAlphaKey(1.0f, 1.0f);

                        gradient.SetKeys(keys, alpha);

                        Debug.DrawLine(inst.transform.position, other.transform.position, gradient.Evaluate(force / Mathf.Abs(force + .001f)), 0.1f); // force / Mathf.Abs(force + .001f) -- Generates a value either 1 or -1 (like binary)

                        // if (wrapX) { direction = new Vector2(-direction.x, direction.y); }
                        // else { direction = new Vector2(direction.x, -direction.y); }

                        vel += direction * force; // TODO: WRAPPED FORCES ARE NOT BEING APPLIED CORRECTLY (one goes wrong in x, both in y)
                    }
                }
            }

            // Moves the particle based on it's final force
            // Prevents invalid addforce
            if (!float.IsNaN(vel.x) && !float.IsNaN(vel.x))
            {
                inst.GetComponent<Rigidbody2D>().AddForce(vel * Time.deltaTime);
            }
        }
    }

    private void RandomizeInteractions()
    {
        // Randomizes how each particle should interact
        if (randomizeInteractions)
        {

        }
        // Allows user to manually set interactions
        // TODO: ADD MORE CUSTOMIZATION FOR SPECIFIC INTERACTIONS
        else
        {
            for (int i = 0; i < transform.childCount; i++)
            {
                GameObject inst = transform.GetChild(i).gameObject;
                ParticleParent child = inst.GetComponent<ParticleParent>();

                int id = GetNumeric(inst.name);

                if (inst.GetComponent<ParticleParent>() == null)
                {
                    inst.AddComponent<ParticleParent>();
                }

                // Breaks if the particle doesn't exist yet
                try
                {
                    if (child.repelForce != this.repelForce)
                    {
                        child.repelForce = this.repelForce;
                    }
                    if (child.repelRadius != this.repelRadius)
                    {
                        child.repelRadius = this.repelRadius;
                    }
                    if (child.interactForce != this.interactForce)
                    {
                        child.interactForce = this.interactForce;
                    }
                    if (child.interactRadius != this.interactRadius)
                    {
                        child.interactRadius = this.interactRadius;
                    }
                }
                catch
                {
                    break;
                }
            }
        }
    }

    // Returns an int for a particle's name
    // Organized in alphabetical order
    public static int GetNumeric(string color)
    {
        int id = 0;

        color = color.ToLower();

        if (color == "aqua")
        {
            id = 0;
        }
        else if (color == "blue")
        {
            id = 1;
        }
        else if (color == "brown")
        {
            id = 2;
        }
        else if (color == "dark-green")
        {
            id = 3;
        }
        else if (color == "dark-red")
        {
            id = 4;
        }
        else if (color == "green")
        {
            id = 5;
        }
        else if (color == "magenta")
        {
            id = 6;
        }
        else if (color == "navy")
        {
            id = 7;
        }
        else if (color == "orange")
        {
            id = 8;
        }
        else if (color == "pink")
        {
            id = 9;
        }
        else if (color == "purple")
        {
            id = 10;
        }
        else if (color == "red")
        {
            id = 11;
        }
        else if (color == "teal")
        {
            id = 12;
        }
        else if (color == "white")
        {
            id = 13;
        }
        else if (color == "yellow")
        {
            id = 14;
        }

        return id;
    }

    // Gets an int id for each particle(based on amount of particles)
    public int GetID(string color)
    {
        for (int i = 0; i < this.transform.childCount; i++)
        {
            if (transform.GetChild(i).name == color)
            {
                return i;
            }
        }

        Debug.LogError("Invalid color, no ID found.");
        return 15; // Can never be an actual id(bigger then the number of total colors), so makes sure that code will give error
    }
}

public class RandomInteraction // TODO: figure out what actually needs to go in here(write a sudo code for it)
{
    
}