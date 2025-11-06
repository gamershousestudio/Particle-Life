using UnityEngine;

public class Particle : MonoBehaviour
{

    // Interaction variables
    [Header("Interaction Variables")]

    public float repelForce;
    public float repelRadius;
    public float interactForce;
    public float interactRadius;
    public int index;

    public float Interact(float distance)
    {
        if (distance >= interactRadius) return 0;
        float t = distance / interactRadius;
        return -SmootherStep(t) * interactForce;
    }

    public float Repel(float distance)
    {
        if (distance >= repelRadius) return 0;
        float t = distance / repelRadius;
        return SmootherStep(t) * repelForce;
    }

    // Decreases the derivative to minimize sudden change
    float SmootherStep(float t)
    {
        // Clamped to [0,1]
        t = Mathf.Clamp01(t);
        return 1 - (6 * Mathf.Pow(t, 5) - 15 * Mathf.Pow(t, 4) + 10 * Mathf.Pow(t, 3));
    }
}
