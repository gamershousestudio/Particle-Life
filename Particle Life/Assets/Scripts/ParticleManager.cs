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

    // Initialize update functions
    private void Update()
    {
        ParticleInteract();

        RandomizeInteractions();
    }

    private void ParticleInteract()
    {
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

                Vector2 distance = inst.transform.position - other.transform.position;
                Vector2 direction = distance.normalized;

                float hypotenuse = Mathf.Sqrt(Mathf.Pow(distance.x, 2) + Mathf.Pow(distance.y, 2));

                float force = 0f;

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

                vel += direction * force;
            }

            // Moves the particle based on it's final force
            // Prevents invalid addforce
            if (vel != Vector2.zero || vel != null)
            {
                Debug.DrawRay(inst.transform.position, vel, Color.green, 0.1f);
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

                int id = GetId(inst.name);

                if (inst.GetComponent<ParticleParent>() == null)
                {
                    inst.AddComponent<ParticleParent>();
                }

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
        }
    }

    // Returns an int for a particle's name
    // Organized in alphabetical order
    public static int GetId(string name)
    {
        int id = 0;

        name = name.ToLower();

        if (name == "aqua")
        {
            id = 0;
        }
        else if (name == "blue")
        {
            id = 1;
        }
        else if (name == "brown")
        {
            id = 2;
        }
        else if (name == "dark-green")
        {
            id = 3;
        }
        else if (name == "dark-red")
        {
            id = 4;
        }
        else if (name == "green")
        {
            id = 5;
        }
        else if (name == "magenta")
        {
            id = 6;
        }
        else if (name == "navy")
        {
            id = 7;
        }
        else if (name == "orange")
        {
            id = 8;
        }
        else if (name == "pink")
        {
            id = 9;
        }
        else if (name == "purple")
        {
            id = 10;
        }
        else if (name == "red")
        {
            id = 11;
        }
        else if (name == "teal")
        {
            id = 12;
        }
        else if (name == "white")
        {
            id = 13;
        }
        else if (name == "yellow")
        {
            id = 14;
        }

        return id;
    }
}
