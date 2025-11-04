using UnityEngine;

public class Boundary : MonoBehaviour
{
    // Boundary variables
    [Header("Boundary Variables")]

    public float buffer;
    [HideInInspector] public float size;

    Vector2 newPos = new Vector2();

    // Initializes variables and init functions
    private void Start()
    {
        transform.localScale = Vector2.one * (size + buffer);
    }

    private void OnTriggerEnter2D(Collider2D other)
    {
        // Moves the particle to the other side of the boundary
        ParticleSpawner script = GameObject.FindAnyObjectByType<ParticleSpawner>();

        double boundary = script.xRange * script.sceneSize - double.Epsilon;

        if ((other.transform.position.x > boundary || other.transform.position.x < -boundary) ||
            other.transform.position.x > boundary || other.transform.position.x < -boundary)
        {
            newPos = new Vector2(-other.transform.position.x, other.transform.position.y);
        }
        else
        {
            newPos = new Vector2(other.transform.position.x, -other.transform.position.y);
        }

        other.transform.position = newPos * Vector2.one + (buffer * (other.transform.position / new Vector2 (Mathf.Abs(other.transform.position.x), Mathf.Abs(other.transform.position.y))));
    }
}