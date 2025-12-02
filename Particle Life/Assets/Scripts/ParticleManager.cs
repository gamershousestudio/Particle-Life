using System.Runtime.CompilerServices;
using UnityEngine;
using System.Collections.Generic;
using System.Linq.Expressions;
using System.Linq;
using UnityEditorInternal;
using System.ComponentModel;

[System.Serializable] // Allows float lists to show up in inspector
public class RepelForce
{
    public List<float> values = new List<float>();
}

[System.Serializable] // Allows float lists to show up in inspector
public class InteractForce
{
    public List<float> values = new List<float>();
}

public class ParticleManager : MonoBehaviour
{
    [HideInInspector] public GameObject[] particles;

    // Algorithm variables
    [Header("Algorithm Variables")]

    // public bool randomizeInteractions; TODO: ADD RANDOM INTERACTIONS

    // X axis = how each particle interacts with other particles
    public float repelForceConst;
    public float interactForceConst;
    [HideInInspector] public List<RepelForce> repelForce = new List<RepelForce>(0);
    [HideInInspector] public List<InteractForce> interactForce = new List<InteractForce>(0);
    public float repelRadius;
    public float interactRadius;
    public float wrappedBuffer;

    // Toggles
    [Header("Toggles")]

    public bool drawLines;
    public bool enableWrapped;
    public bool showWrapped;
    public bool showNormal;
    public bool showAttract;
    public bool showRepel;

    // Initialize __init__ functions
    private void Start()
    {
        while (!FindAnyObjectByType<ParticleSpawner>().isComplete) // Prevents code from running until particles are fully initialized
        print("Particle Initialization complete.");

        InitializeValues();

        // Debug.();

        UpdateValues();
    }

    // Initialize update functions
    private void Update()
    {
        ParticleInteract();

        // RandomizeInteractions(); NOTE: DOES NOT DO ANYTHING RIGHT NOW

        // Updates particle variables in real time
        // List<int> toFix = new List<int>();

        // Loops through each value in the matrix to see if it needs to be updated
        // for (int x = 0; x < transform.childCount; x++)
        // {
        //     if (transform.GetChild(x).name == "Walls") continue;

        //     for (int y = 0; y < (transform.childCount - 1); y++)
        //     {
        //         ParticleParent inst = transform.GetChild(x).GetComponent<ParticleParent>();

        //         if (inst.repelForce[y] != (this.repelForce[x - 1].values[y] * repelForceConst) ||
        //            inst.interactForce[y] != (this.interactForce[x - 1].values[y] * interactForceConst) ||
        //            inst.repelRadius != this.repelRadius ||
        //            inst.interactRadius != this.interactRadius)
        //         {
        //             toFix.Add(x);

        //             print("Proccessing change to color type " + transform.GetChild(x).name + "...");
        //         }
        //     }
        // }
        
        // if(toFix.Count > 0)
        // {
        //     UpdateValues(toFix.ToArray());
        //     print("Change complete");
        // }

        UpdateValues();
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
            Particle particle = inst.GetComponent<Particle>(); // Can be further optimized, but this will do for now

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

                hypotenuse = distance.magnitude;

                // Particles are close enough to repel
                if (hypotenuse <= repelRadius)
                {
                    force = particle.Repel(hypotenuse, GetID(other.transform.parent.name));
                }
                // Particles should go based on interaction force
                else
                {
                    force = particle.Interact(hypotenuse, GetID(other.transform.parent.name));
                }

                // Creates a gradient to display interactions
                keys[0] = new GradientColorKey(Color.green, -1.0f);
                keys[1] = new GradientColorKey(Color.red, 1.0f);

                alpha[0] = new GradientAlphaKey(1.0f, -1.0f);
                alpha[1] = new GradientAlphaKey(1.0f, 1.0f);

                gradient.SetKeys(keys, alpha);

                if(force != 0 && drawLines && showNormal)
                {
                    if (force < 0 && showAttract)
                    {
                        Debug.DrawLine(inst.transform.position, other.transform.position, gradient.Evaluate(force / Mathf.Abs(force + .001f)), 0.1f); // force / Mathf.Abs(force + .001f) -- Generates a value either 1 or -1 (like binary)
                    }
                    else if(force > 0 && showRepel)
                    {
                        Debug.DrawLine(inst.transform.position, other.transform.position, gradient.Evaluate(force / Mathf.Abs(force + .001f)), 0.1f); // force / Mathf.Abs(force + .001f) -- Generates a value either 1 or -1 (like binary)
                    }
                } 

                vel += direction * force;
            }

            // Checks if wrapped interactions should be applied
            float bufferX = script.xRange * script.sceneSize - interactRadius * wrappedBuffer;
            float bufferY = script.yRange * script.sceneSize - interactRadius * wrappedBuffer;

            if ((inst.transform.position.x > bufferX || inst.transform.position.x < -bufferX ||
                inst.transform.position.y > bufferY || inst.transform.position.y < -bufferY) && enableWrapped)
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

                    hypotenuse = distance.magnitude;

                    // Particles are close enough to repel
                    if (hypotenuse <= repelRadius)
                    {
                        force = particle.Repel(hypotenuse, GetID(other.transform.parent.name));
                    }
                    // Particles should go based on interaction force
                    else
                    {
                        force = particle.Interact(hypotenuse, GetID(other.transform.parent.name));
                    }

                    if (force != 0)
                    {
                        // Creates a gradient to display interactions
                        keys[0] = new GradientColorKey(Color.gray, -1.0f); // Attract
                        keys[1] = new GradientColorKey(Color.black, 1.0f); // Repel

                        alpha[0] = new GradientAlphaKey(1.0f, -1.0f);
                        alpha[1] = new GradientAlphaKey(1.0f, 1.0f);

                        gradient.SetKeys(keys, alpha);

                        if(drawLines && showWrapped)
                        {
                            if (force < 0 && showAttract)
                            {
                                Debug.DrawLine(inst.transform.position, other.transform.position, gradient.Evaluate(force / Mathf.Abs(force + .001f)), 0.1f); // force / Mathf.Abs(force + .001f) -- Generates a value either 1 or -1 (like binary)
                            }
                            else if(force > 0 && showRepel)
                            {
                                Debug.DrawLine(inst.transform.position, other.transform.position, gradient.Evaluate(force / Mathf.Abs(force + .001f)), 0.1f); // force / Mathf.Abs(force + .001f) -- Generates a value either 1 or -1 (like binary)
                            }
                        } 

                        // if (wrapX) { direction = new Vector2(-direction.x, direction.y); }
                        // else { direction = new Vector2(direction.x, -direction.y); }

                        vel += direction * force;
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

    private void InitializeValues()
    {
        // Creates arrays based on number of particle types present
        // Following three lines are old debug code.  From now on I will preserve old debug to act as a makeshift "archive" (and if I need them in the future, of course)
        // ("Rows: " + interactForce.Count);
        // try { ("Cols: " + interactForce[0].values.Count); }
        // catch { ("Cols: 0 (By default)"); }

        // Finds how much particle types there are
        int needed = transform.childCount - 1;

        // Initializes matricies for particles
        for (int i = interactForce.Count; i < needed; i++) interactForce.Add(new InteractForce());

        for (int i = repelForce.Count; i < needed; i++) repelForce.Add(new RepelForce());

        foreach (var repelF in repelForce)
            while (repelF.values.Count < needed) repelF.values.Add(0f);

        foreach (var interactF in interactForce)
            while (interactF.values.Count < needed) interactF.values.Add(0f);

        // Initializes values for children
        for(int x = 0; x < transform.childCount; x++)
        {
            if(transform.GetChild(x).name == "Walls") continue;

            ParticleParent inst = transform.GetChild(x).gameObject.AddComponent<ParticleParent>();

            for(int y = 0; y < (transform.childCount - 1); y++)
            {
                inst.interactForce.Add(this.interactForce[x - 1].values[y] * interactForceConst);
                inst.repelForce.Add(this.repelForce[x - 1].values[y] * repelForceConst);
            }   

            inst.repelRadius = this.repelRadius;
            inst.interactRadius = this.interactRadius;
        }
    }

    private void UpdateValues() // NOTE: this function will become useless once hud is fully complete, as it is not efficient to run this every frame & HUD.cs will fix that
    {
        // Loops through all particles and updates their values
        for (int j = 0; j < transform.childCount; j++)
        {
            if (transform.GetChild(j).name == "Walls") continue;

            int i = j;


            GameObject inst = transform.GetChild(i).gameObject;
            ParticleParent child = inst.GetComponent<ParticleParent>();

            for (int x = 0; x < transform.childCount - 1; x++)
            {
                if (child.repelForce != this.repelForce[x].values)
                {
                    for (int y = 0; y < this.repelForce[GetID(inst.name)].values.Count; y++)
                    {
                        try
                        {
                            child.repelForce[y] = this.repelForce[GetID(inst.name)].values[y] * repelForceConst;
                        }
                        catch
                        {
                            child.repelForce.Add(this.repelForce[GetID(inst.name)].values[y] * repelForceConst);
                        }
                    }
                }

                if (child.repelRadius != this.repelRadius)
                {
                    child.repelRadius = this.repelRadius;
                }

                if (child.interactForce != this.interactForce[x].values)
                {
                    for (int y = 0; y < this.interactForce[GetID(inst.name)].values.Count; y++)
                    {
                        try
                        {
                            child.interactForce[y] = this.interactForce[GetID(inst.name)].values[y] * interactForceConst;
                        }
                        catch
                        {
                            child.interactForce.Add(this.interactForce[GetID(inst.name)].values[y] * interactForceConst);
                        }
                    }
                }

                if (child.interactRadius != this.interactRadius)
                {
                    child.interactRadius = this.interactRadius;
                }
            }
        }
    }

    public void UpdateConsts(int index) // NOTE: name temp while UpdateValues() is still in use
    {
        for(int x = 0; x < transform.childCount; x++)
        {
            if (transform.GetChild(x).name == "Walls") continue;

            ParticleParent inst = transform.GetChild(x).GetComponent<ParticleParent>();

            for(int y = 0; y < transform.childCount - 1; y++)
            {
                if (index == 0)
                {
                    inst.repelForce[y] = this.repelForce[x - 1].values[y] * this.repelForceConst;
                }
                else if (index == 1)
                {
                    inst.interactForce[y] = this.interactForce[x - 1].values[y] * this.interactForceConst;
                }
                else if (index == 2)
                {
                    inst.repelRadius = this.repelRadius;
                }
                else if(index == 3)
                {
                    inst.interactRadius = this.interactRadius;
                }
            }
        }
    }

    // Returns an int for a particle's name
    // Organized in alphabetical order
    private static readonly Dictionary<string, int> colorMap = new Dictionary<string, int>()
{
    {"aqua", 0}, {"blue", 1}, {"brown", 2}, {"dark-green", 3}, {"dark-red", 4},
    {"green", 5}, {"magenta", 6}, {"navy", 7}, {"orange", 8}, {"pink", 9},
    {"purple", 10}, {"red", 11}, {"teal", 12}, {"white", 13}, {"yellow", 14}
    };

    // Gets an int id for each particle(based on amount of particles)
    public int GetID(string color)
    {
        for (int i = 0; i < this.transform.childCount; i++)
        {
            if (transform.GetChild(i).name == color)
            {
                return i - 1;
            }
        }

        Debug.LogError("Invalid color, no ID found.");
        return 15; // Can never be an actual id(bigger then the number of total colors), so makes sure that code will give error
    }

    // GetID uses transform, so following allows GetID to be used globally
    // Most likely necessary in future code, but is possible that it may not be
    public static int GetIDGlobal(string color)
    {
        return GameObject.FindAnyObjectByType<ParticleManager>().GetID(color);
    }
}

// NOTE: probably don't need a class for this, but just in case I will keep it for now
public class RandomInteraction // TODO: figure out what actually needs to go in here(write a sudo code for it)
{

}