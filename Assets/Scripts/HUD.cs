using UnityEngine;
using TMPro;
using System.Linq;
using UnityEngine.PlayerLoop;
using System;
using System.Collections.Generic;

public class HUD : MonoBehaviour
{
    // UI instances
    [Header("UI Instances")]

    public TMP_Text repelForceInput;
    public TMP_Text interactForceInput;
    public TMP_Text repelRadiusInput;
    public TMP_Text interactRadiusInput;

    public enum ConstType
    {
        RepelForce,
        InteractForce,
        RepelRadius,
        InteractRadius,
        WrappedToggle
    }
    private Dictionary<string, ConstType> inputLookup;


    // Consts
    [Header("Constants")]

    private char[] nums = new char[10];

    // Initializes variables & runs __init__ funcs
    private void Start()
    {
        for (int i = 0; i < nums.Length; i++) nums[i] = (char)(i + 48); // Numbers start at 48(in ASCII)

        inputLookup = new Dictionary<string, ConstType>()
        {
            { "RepelForceConst", ConstType.RepelForce },
            { "InteractForceConst", ConstType.InteractForce },
            { "RepelRadius", ConstType.RepelRadius },
            { "InteractRadius", ConstType.InteractRadius }
        };
    }

    // Makes sure new text pasts all requirements
    public void RemoveCharacters(TMP_InputField reference)
    {
        // Removes extra characters
        for (int i = 0; i < reference.text.Length; i++)
        {
            if ((!nums.Contains(reference.text[i])) && !(reference.text[i] == '.') && !(reference.text[i] == '-'))
            {
                reference.text = reference.text.Remove(i);
                return;
            }
        }

        // Sets blank to zero
        if(reference.text.Length == 0)
        {
            reference.text = "0";
        }

        // Removes access variables
        if(reference.text[0] == '0' && reference.text.Length > 1)
        {
            print("Unneccessary 0!");

            for(int i = 0; i < reference.text.Length; i++)
            {
                print("Checking " + reference.text[i]);

                if(reference.text[i] == '0')
                {
                    print("Found!");
                    reference.text = reference.text.Remove(i);
                }
                else
                {
                    break;
                }
            }
        }

        UpdateValues(inputLookup[reference.name], reference);
    }

    public void SwitchToggle(string toggle, bool value)
    {
        switch(toggle.ToLower())
        {
            case "applywrappedforces":
                GameObject.FindAnyObjectByType<ParticleManager>().GetComponent<ParticleManager>().enableWrapped = value;

                break;
        }
    }

    private void UpdateValues(ConstType type, TMP_InputField text)
    {
        ParticleManager script = GameObject.FindAnyObjectByType<ParticleManager>().GetComponent<ParticleManager>();

        float newValue = 0;

        if(text != null)
        {
            newValue = float.Parse(text.text);
        }

        switch (type)
        {
            case ConstType.RepelForce:
                script.repelForceConst = newValue;
                break;

            case ConstType.InteractForce:
                script.interactForceConst = newValue;
                break;

            case ConstType.RepelRadius:
                script.repelRadius = newValue;
                break;

            case ConstType.InteractRadius:
                script.interactRadius = newValue;
                break;
        }

        script.UpdateConsts((int)type);
    }
}
