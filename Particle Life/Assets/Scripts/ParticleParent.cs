using System.Collections.Generic;
using UnityEngine;

public class ParticleParent : MonoBehaviour
{
    // Interaction variables
    [Header("Interaction Variables")]

    public List<float> repelForce = new List<float>(0);
    public float repelRadius;
    public List<float> interactForce = new List<float>(0);
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
