using isoworldremote;
using UnityEditor;
using UnityEngine;
using System;

[CustomEditor(typeof(MapBlock))]
[CanEditMultipleObjects]
public class MapBlockEditor : Editor
{
    static Color32 selectedColor = Color.white;
    static BasicShape selectedShape = BasicShape.WALL;

    public override void OnInspectorGUI()
    {
        MapBlock[] targetBlocks = Array.ConvertAll(targets, element => (MapBlock)element);
        if(targets.Length == 1)
            EditorGUILayout.LabelField(targets.Length + " Map Block selected.");
        else
            EditorGUILayout.LabelField(targets.Length + " Map Blocks selected.");

        selectedColor = EditorGUILayout.ColorField("Material Color", selectedColor);
        selectedShape = (BasicShape)EditorGUILayout.EnumPopup("Terrain Shape ", selectedShape);
        EditorGUILayout.BeginVertical();
        DFHack.DFCoord2d tempCoord = new DFHack.DFCoord2d();
        for (int i = 0; i < 16; i++)
        {
            EditorGUILayout.BeginHorizontal();
            for (int j = 0; j < 16; j++)
            {
                tempCoord.x = j;
                tempCoord.y = i;
                Color currentColor = targetBlocks[0].GetColor(tempCoord);
                for (int index = 1; index < targetBlocks.Length; index++)
                {
                    if (currentColor != targetBlocks[index].GetColor(tempCoord))
                    {
                        currentColor = Color.white;
                        break;
                    }
                }
                currentColor.a = 1.0f;
                GUI.color = currentColor;
                string buttonIcon = "\u00A0";
                BasicShape tile = targetBlocks[0].GetSingleTile(tempCoord);
                for (int index = 1; index < targetBlocks.Length; index++)
                {
                    if (tile != targetBlocks[index].GetSingleTile(tempCoord))
                    {
                        tile = BasicShape.NONE;
                        break;
                    }

                }
                switch (tile)
                {
                    case BasicShape.WALL:
                        buttonIcon = "▓";
                        break;
                    case BasicShape.FLOOR:
                        buttonIcon = "+";
                        break;
                    case BasicShape.NONE:
                        buttonIcon = "?";
                        break;
                    case BasicShape.OPEN:
                        buttonIcon = "\u00A0";
                        break;
                    case BasicShape.RAMP_UP:
                        buttonIcon = "▲";
                        break;
                    case BasicShape.RAMP_DOWN:
                        buttonIcon = "▼";
                        break;
                    default:
                        buttonIcon = "?";
                        break;
                }
                if (GUILayout.Button(buttonIcon))
                {
                    for (int index = 0; index < targetBlocks.Length; index++)
                    {
                        targetBlocks[index].SetSingleTile(tempCoord, selectedShape);
                        targetBlocks[index].SetColor(tempCoord, selectedColor);
                        targetBlocks[index].Regenerate();
                        EditorUtility.SetDirty(targetBlocks[index]);
                    }
                }
            }
            EditorGUILayout.EndHorizontal();
        }
        EditorGUILayout.EndVertical();
        GUI.color = Color.white;
        if (GUILayout.Button("Fill"))
        {
            for (int index = 0; index < targetBlocks.Length; index++)
            {
                for (int i = 0; i < 16; i++)
                    for (int j = 0; j < 16; j++)
                    {
                        DFHack.DFCoord2d here = new DFHack.DFCoord2d(j, i);
                        targetBlocks[index].SetSingleTile(here, selectedShape);
                        targetBlocks[index].SetColor(here, selectedColor);

                    }
                targetBlocks[index].Regenerate();
                EditorUtility.SetDirty(targetBlocks[index]);
            }
        }
    }
}
