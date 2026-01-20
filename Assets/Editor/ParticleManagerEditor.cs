using UnityEditor;
using UnityEngine;

[CustomEditor(typeof(ParticleManager))]
public class ParticleManagerEditor : Editor
{
    public override void OnInspectorGUI()
    {
        // Keeps rest of inspector intact
        DrawDefaultInspector();

        // Defines where GUI appears
        ParticleManager manager = (ParticleManager)target;
        
        #region Repel Force
        // Defines space for GUI
        EditorGUILayout.Space();

        // Hides section when list is empty
        if (manager.repelForce.Count != 0)
        {
            EditorGUILayout.LabelField("Repel Force Matrix", EditorStyles.boldLabel);
        }
        else
        {
            return;
        }

        // Gets amount of rows in the matrix
        int rowsRepel = manager.repelForce.Count;

        // Gets the amount of columns in the matrix
        int colsRepel = manager.repelForce[0].values.Count;

        // Draws matrix in GUI
        for (int i = 0; i < rowsRepel; i++) // Loops through all rows
        {
            EditorGUILayout.BeginHorizontal(); // Starts drawing on x axis
            for (int j = 0; j < colsRepel; j++) // Loops through all columns in the row
            {
                manager.repelForce[i].values[j] = EditorGUILayout.FloatField(
                    manager.repelForce[i].values[j],
                    GUILayout.Width(50)
                ); // Appends the new row at the end of every column
            }
            EditorGUILayout.EndHorizontal(); // Stops drawing on x axis
        }

        #endregion

        #region Interact Force

        // Defines space for GUI
        EditorGUILayout.Space();
        EditorGUILayout.Space();

        // Hides section when list is empty
        if (manager.repelForce.Count != 0)
        {
            EditorGUILayout.LabelField("Interact Force Matrix", EditorStyles.boldLabel);
        }
        else
        {
            return;
        }

        // Gets amount of rows in the matrix
        int rowsAttract = manager.interactForce.Count;

        // Gets the amount of columns in the matrix
        int colsAttract = manager.interactForce[0].values.Count;

        // Draws matrix in GUI
        for (int i = 0; i < rowsAttract; i++) // Loops through all rows
        {
            EditorGUILayout.BeginHorizontal(); // Starts drawing on x axis
            for (int j = 0; j < colsAttract; j++) // Loops through all columns in the row
            {
                manager.interactForce[i].values[j] = EditorGUILayout.FloatField(
                    manager.interactForce[i].values[j],
                    GUILayout.Width(50)
                ); // Appends the new row at the end of every column
            }
            EditorGUILayout.EndHorizontal(); // Stops drawing on x axis
        }

        #endregion

        // Marks as dirty if the values are changed
        if (GUI.changed)
        {
            EditorUtility.SetDirty(manager);
        }
    }
}