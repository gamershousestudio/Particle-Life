using UnityEditor.SearchService;
using UnityEngine;

public class ParticleSpawner : MonoBehaviour
{
    // Spawning variables
    [Header("Spawning Variables")]

    public float xRange;
    public float yRange;

    public float sceneSize;

    public float particleSize;

    // Numbers of each particles
    [Header("Numbers of Each Particle")]

    public int numberOfAquaParticles;
    public int numberOfBlueParticles;
    public int numberOfBrownParticles;
    public int numberOfDarkGreenParticles;
    public int numberOfDarkRedParticles;
    public int numberOfGreenParticles;
    public int numberOfMagentaParticles;
    public int numberOfNavyBlueParticles;
    public int numberOfOrangeParticles;
    public int numberOfPinkParticles;
    public int numberOfPurpleParticles;
    public int numberOfRedParticles;
    public int numberOfTealParticles;
    public int numberOfWhiteParticles;
    public int numberOfYellowParticles;

    // Particle prefabs
    [Header("Particle Prefabs")]

    public GameObject aquaParticle;
    public GameObject blueParticle;
    public GameObject brownParticle;
    public GameObject darkGreenParticle;
    public GameObject darkRedParticle;
    public GameObject greenParticle;
    public GameObject magentaParticle;
    public GameObject navyBlueParticle;
    public GameObject orangeParticle;
    public GameObject pinkParticle;
    public GameObject purpleParticle;
    public GameObject redParticle;
    public GameObject tealParticle;
    public GameObject whiteParticle;
    public GameObject yellowParticle;

    private ParticleManager manager;

    // Initialize init variables and functions    
    private void Start()
    {
        manager = GameObject.FindAnyObjectByType<ParticleManager>();

        manager.particles = new GameObject[
            numberOfAquaParticles + numberOfBlueParticles + numberOfBrownParticles + numberOfDarkGreenParticles + numberOfDarkRedParticles + numberOfGreenParticles + numberOfMagentaParticles +
            numberOfNavyBlueParticles + numberOfOrangeParticles + numberOfPinkParticles + numberOfPurpleParticles + numberOfRedParticles + numberOfTealParticles + numberOfWhiteParticles + numberOfYellowParticles
        ];

        SpawnParticles();

        SizeScene();
    }

    private void SizeScene()
    {
        Camera cam = Camera.main;
        cam.orthographicSize = sceneSize;
    }

    // Spawns every particle
    private void SpawnParticles()
    {
        // Creates parents to organize each particle
        GameObject aqua = null;
        GameObject blue = null;
        GameObject brown = null;
        GameObject darkGreen = null;
        GameObject darkRed = null;
        GameObject green = null;
        GameObject magenta = null;
        GameObject navyBlue = null;
        GameObject orange = null;
        GameObject pink = null;
        GameObject purple = null;
        GameObject red = null;
        GameObject teal = null;
        GameObject white = null;
        GameObject yellow = null;

        if (numberOfAquaParticles > 0) { aqua = new GameObject("Aqua"); aqua.transform.parent = manager.transform; }
        if (numberOfBlueParticles > 0) { blue = new GameObject("Blue"); blue.transform.parent = manager.transform; }
        if (numberOfBrownParticles > 0) { brown = new GameObject("Brown"); brown.transform.parent = manager.transform; }
        if (numberOfDarkGreenParticles > 0) { darkGreen = new GameObject("Dark-Green"); darkGreen.transform.parent = manager.transform; }
        if (numberOfDarkRedParticles > 0) { darkRed = new GameObject("Dark-Red"); darkRed.transform.parent = manager.transform; }
        if (numberOfGreenParticles > 0) { green = new GameObject("Green"); green.transform.parent = manager.transform; }
        if (numberOfMagentaParticles > 0) { magenta = new GameObject("Magenta"); magenta.transform.parent = manager.transform; }
        if (numberOfNavyBlueParticles > 0) { navyBlue = new GameObject("Navy-Blue"); navyBlue.transform.parent = manager.transform; }
        if (numberOfOrangeParticles > 0) { orange = new GameObject("Orange"); orange.transform.parent = manager.transform; }
        if (numberOfPinkParticles > 0) { pink = new GameObject("Pink"); pink.transform.parent = manager.transform; }
        if (numberOfPurpleParticles > 0) { purple = new GameObject("Purple"); purple.transform.parent = manager.transform; }
        if (numberOfRedParticles > 0) { red = new GameObject("Red"); red.transform.parent = manager.transform; }
        if (numberOfTealParticles > 0) { teal = new GameObject("Teal"); teal.transform.parent = manager.transform; }
        if (numberOfWhiteParticles > 0) { white = new GameObject("White"); white.transform.parent = manager.transform; }
        if (numberOfYellowParticles > 0) { yellow = new GameObject("Yellow"); yellow.transform.parent = manager.transform; }

        // Creates every particle
        int c = 0;

        // Aqua
        for (int i = 0; i < numberOfAquaParticles; i++)
        {
            GameObject inst = Instantiate(aquaParticle, aqua.transform);
            inst.name = "Aqua Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Blue
        for (int i = 0; i < numberOfBlueParticles; i++)
        {
            GameObject inst = Instantiate(blueParticle, blue.transform);

            inst.name = "Blue Particle " + (c + 1).ToString();
            inst.layer = 3; // Three is the layer for particles

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;

        }

        // Brown
        for (int i = 0; i < numberOfBrownParticles; i++)
        {
            GameObject inst = Instantiate(brownParticle, brown.transform);
            inst.name = "Brown Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Dark green
        for (int i = 0; i < numberOfDarkGreenParticles; i++)
        {
            GameObject inst = Instantiate(darkGreenParticle, darkGreen.transform);
            inst.name = "Dark-Green Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Dark red
        for (int i = 0; i < numberOfDarkRedParticles; i++)
        {
            GameObject inst = Instantiate(darkRedParticle, darkRed.transform);
            inst.name = "Dark-Red Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Green
        for (int i = 0; i < numberOfGreenParticles; i++)
        {
            GameObject inst = Instantiate(greenParticle, green.transform);
            inst.name = "Green Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Magenta
        for (int i = 0; i < numberOfMagentaParticles; i++)
        {
            GameObject inst = Instantiate(magentaParticle, magenta.transform);
            inst.name = "Magenta Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Navy blue
        for (int i = 0; i < numberOfNavyBlueParticles; i++)
        {
            GameObject inst = Instantiate(navyBlueParticle, navyBlue.transform);
            inst.name = "Navy Blue Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Orange
        for (int i = 0; i < numberOfOrangeParticles; i++)
        {
            GameObject inst = Instantiate(orangeParticle, orange.transform);
            inst.name = "Orange Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Pink
        for (int i = 0; i < numberOfPinkParticles; i++)
        {
            GameObject inst = Instantiate(pinkParticle, pink.transform);
            inst.name = "Pink Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Purple
        for (int i = 0; i < numberOfPurpleParticles; i++)
        {
            GameObject inst = Instantiate(purpleParticle, purple.transform);
            inst.name = "Purple Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Red
        for (int i = 0; i < numberOfRedParticles; i++)
        {
            GameObject inst = Instantiate(redParticle, red.transform);
            inst.name = "Red Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Teal
        for (int i = 0; i < numberOfTealParticles; i++)
        {
            GameObject inst = Instantiate(tealParticle, teal.transform);
            inst.name = "Teal Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // White
        for (int i = 0; i < numberOfWhiteParticles; i++)
        {
            GameObject inst = Instantiate(whiteParticle, white.transform);
            inst.name = "White Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }

        // Yellow
        for (int i = 0; i < numberOfYellowParticles; i++)
        {
            GameObject inst = Instantiate(yellowParticle, yellow.transform);
            inst.name = "Yellow Particle " + (c + 1).ToString();

            manager.particles[c] = inst;

            Vector2 pos = new Vector2(Random.Range(-xRange + (.5f * particleSize), xRange - (.5f * particleSize)), Random.Range(-yRange + (.5f * particleSize), yRange - (.5f * particleSize)));
            inst.transform.position = pos;

            inst.transform.localScale = Vector2.one * particleSize;

            c++;
        }
    }
}
