using UnityEngine;
using TMPro;
using System.Linq;
using UnityEngine.PlayerLoop;

public class HUD : MonoBehaviour
{
    // UI instances
    [Header("UI Instances")]

    public TMP_Text repelForceInput;
    public TMP_Text interactForceInput;
    public TMP_Text repelRadiusInput;
    public TMP_Text interactRadiusInput;

    // Consts
    [Header("Constants")]

    private char[] nums = new char[10];

    // Initializes variables & runs __init__ funcs
    private void Start()
    {
        for (int i = 0; i < nums.Length; i++) nums[i] = (char)(i + 48); // Numbers start at 48(in ASCII)
    }

    // Makes sure new text pasts all requirements
    public void RemoveCharacters(TMP_InputField reference)
    {
        for (int i = 0; i < reference.text.Length; i++)
        {
            if ((!nums.Contains(reference.text[i])) && !(reference.text[i] == '.'))
            {
                reference.text = reference.text.Remove(i);
                return;
            }
        }

        if (reference.name == "RepelForceConst")
        {
            UpdateValues(0, reference);
        }
        else if (reference.name == "InteractForceConst")
        {
            UpdateValues(1, reference);
        }
        else if (reference.name == "RepelRadius")
        {
            UpdateValues(2, reference);
        }
        else if(reference.name == "InteractRadius")
        {
            UpdateValues(3, reference);
        }
    }

    private void UpdateValues(int i, TMP_InputField text)
    {
        ParticleManager script = FindAnyObjectByType<ParticleManager>().GetComponent<ParticleManager>();

        float newValue = float.Parse(text.text);

        if (i == 0)
        {
            script.repelForceConst = newValue;
        }
        else if (i == 1)
        {
            script.interactForceConst = newValue;
        }
        else if (i == 2)
        {
            script.repelRadius = newValue;
        }
        else if (i == 3)
        {
            script.interactRadius = newValue;
        }

        script.UpdateInts(i);
    }
}
