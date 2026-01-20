using System.Collections.Generic;
using UnityEngine;

public class Particle : MonoBehaviour
{

    // Interaction variables
    [Header("Interaction Variables")]

    public List<float> repelForce;
    public float repelRadius;
    public List<float> interactForce;
    public float interactRadius;
    public int index;

    public float Interact(float distance, int otherID)
    {
        if (distance >= interactRadius) return 0;
        float t = distance / interactRadius;
        return -SmootherStep(t) * interactForce[otherID];
    }

    public float Repel(float distance, int otherID)
    {
        if (distance >= repelRadius) return 0;
        float t = distance / repelRadius;
        return SmootherStep(t) * repelForce[otherID];
    }

    // Decreases the derivative to minimize sudden change
    float SmootherStep(float t)
    {
        // Clamped to [0,1]
        t = Mathf.Clamp01(t);
        return 1 - (6 * Mathf.Pow(t, 5) - 15 * Mathf.Pow(t, 4) + 10 * Mathf.Pow(t, 3));
    }
}
