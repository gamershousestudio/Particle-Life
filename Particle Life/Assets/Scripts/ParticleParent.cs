using UnityEngine;

public class ParticleParent : MonoBehaviour
{
    // Interaction variables
    [Header("Interaction Variables")]

    public float repelForce;
    public float repelRadius;
    public float interactForce;
    public float interactRadius;

    // Updates all children's variables
    private void Update()
    {
        Particle[] particles = transform.GetComponentsInChildren<Particle>();

        foreach (Particle particle in particles)
        {
            particle.repelForce = this.repelForce;
            particle.repelRadius = this.repelRadius;
            particle.interactForce = this.interactForce;
            particle.interactRadius = this.interactRadius;
        }
    }
}
