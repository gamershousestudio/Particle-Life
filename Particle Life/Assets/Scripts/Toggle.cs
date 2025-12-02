using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;


public class CustomToggle : MonoBehaviour, IPointerClickHandler
{
    private bool enable = true;
    private GameObject child;

    private void Start()
    {
        child = this.transform.GetChild(0).gameObject;
    }

    public void OnPointerClick(PointerEventData eventData)
    {
        enable = !enable;
        transform.GetChild(0).gameObject.SetActive(enable);

        GameObject.FindAnyObjectByType<HUD>().SwitchToggle(name, enable);
    }
}
