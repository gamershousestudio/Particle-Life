using Unity.VisualScripting.Dependencies.Sqlite;
using UnityEngine;

public class Particle : MonoBehaviour
{

    // Interaction variables
    [Header("Interaction Variables")]
    
    [HideInInspector] public float repelForce;
    [HideInInspector] public float repelRadius;
    [HideInInspector] public float interactForce;
    [HideInInspector] public float interactRadius;

    public float Interact(float distance)
    {
        // Calculates the interaction force
        if (distance <= interactForce / 2)
        {
            float slope = repelRadius / interactForce;

            return distance * slope;
        }
        else
        {
            float slope = interactForce / interactRadius;

            return distance * slope;
        }
    }

    // Repels particles when they get too close
    public float Repel(float distance)
    {
        // Calculates the repel force
        float slope = repelForce / repelRadius;

        return distance * slope;
    }
}
