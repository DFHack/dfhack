using DFHack;
using System.Collections.Generic;
using UnityEngine;
using isoworldremote;

public class MapBlock : MonoBehaviour
{
    public static float floorHeight = 0.1f;
    DFCoord coordinates;
    GameMap parent;

    [SerializeField]
    BasicShape[] terrain = new BasicShape[256];
    [SerializeField]
    Color32[] colors = new Color32[256];

    List<Vector3> finalVertices = new List<Vector3>();
    List<int> finalFaces = new List<int>();
    List<Color32> finalVertexColors = new List<Color32>();
    List<Vector2> finalUVs = new List<Vector2>();

    public enum Openness
    {
        air,
        mixed,
        stone
    }
    Openness openness;

    public void SetOpenness()
    {
        int air = 0;
        int solid = 0;
        for (int x = 0; x < 16; x++)
            for (int y = 0; y < 16; y++)
            {
                if (terrain[y * 16 + x] == BasicShape.OPEN || terrain[y * 16 + x] == BasicShape.NONE)
                    air++;
                else if (terrain[y * 16 + x] == BasicShape.WALL)
                    solid++;
            }
        if (air == 256)
            openness = Openness.air;
        else if (solid == 256)
            openness = Openness.stone;
        else openness = Openness.mixed;
    }

    public Openness GetOpenness()
    {
        return openness;
    }

    public Color32 GetColor(DFCoord2d position)
    {
        return colors[position.x + position.y * 16];
    }

    public void SetColor(DFCoord2d position, Color32 input)
    {
        colors[position.x + position.y * 16] = input;
    }

    public void SetSingleTile(DFCoord2d position, BasicShape tile)
    {
        terrain[position.x + position.y * 16] = tile;
        SetOpenness();
    }

    public BasicShape GetSingleTile(DFCoord2d position)
    {
        if (position.x >= 0 && position.x < 16 && position.y >= 0 && position.y < 16)
            return terrain[position.x + position.y * 16];
        else
            return BasicShape.NONE;
    }

    public void Regenerate()
    {
        finalVertices.Clear();
        finalFaces.Clear();
        finalVertexColors.Clear();
        finalUVs.Clear();

        if (openness == Openness.air)
        {
        }
        else
        {
            for (int i = 0; i < 16; i++)
                for (int j = 0; j < 16; j++)
                {
                    DFCoord2d here = new DFCoord2d(i, j);
                    switch (GetSingleTile(here))
                    {
                        case BasicShape.WALL:
                            AddTopFace(here, 1.0f);
                            break;
                        case BasicShape.FLOOR:
                            AddTopFace(here, floorHeight);
                            break;
                    }
                    AddSideFace(here, FaceDirection.North);
                    AddSideFace(here, FaceDirection.South);
                    AddSideFace(here, FaceDirection.East);
                    AddSideFace(here, FaceDirection.West);
                }
        }
        MeshFilter mf = GetComponent<MeshFilter>();
        Mesh mesh = new Mesh();
        mf.mesh = mesh;
        mesh.vertices = finalVertices.ToArray();
        mesh.uv = finalUVs.ToArray();
        mesh.colors32 = finalVertexColors.ToArray();
        mesh.triangles = finalFaces.ToArray();
        mesh.RecalculateBounds();
        mesh.RecalculateNormals();
    }

    enum FaceDirection
    {
        Up,
        Down,
        North,
        South,
        East,
        West
    }

    BasicShape GetRelativeTile(DFCoord2d position, FaceDirection direction)
    {
        DFCoord2d relativePosition = new DFCoord2d(position.x, position.y);
        switch (direction)
        {
            case FaceDirection.North:
                relativePosition.y--;
                break;
            case FaceDirection.South:
                relativePosition.y++;
                break;
            case FaceDirection.East:
                relativePosition.x++;
                break;
            case FaceDirection.West:
                relativePosition.x--;
                break;
        }
        return GetSingleTile(relativePosition);
    }

    void AddSideFace(DFCoord2d position, FaceDirection direction)
    {
        float currentFloorHeight = -0.5f;
        float adjacentFloorHeight = -0.5f;
        switch (GetSingleTile(position))
        {
            case BasicShape.WALL:
                currentFloorHeight = 0.5f;
                break;
            case BasicShape.FLOOR:
                currentFloorHeight = floorHeight - 0.5f;
                break;
            default:
                break;
        }
        switch (GetRelativeTile(position, direction))
        {
            case BasicShape.WALL:
                adjacentFloorHeight = 0.5f;
                break;
            case BasicShape.FLOOR:
                adjacentFloorHeight = floorHeight - 0.5f;
                break;
            default:
                break;
        }
        if (currentFloorHeight <= adjacentFloorHeight)
            return;
        int startindex = finalVertices.Count;
        int uvPos = 0;
        switch (direction)
        {
            case FaceDirection.North:
                finalVertices.Add(new Vector3(position.x + 0.5f, currentFloorHeight, -(position.y - 0.5f)));
                finalVertices.Add(new Vector3(position.x - 0.5f, currentFloorHeight, -(position.y - 0.5f)));
                finalVertices.Add(new Vector3(position.x + 0.5f, adjacentFloorHeight, -(position.y - 0.5f)));
                finalVertices.Add(new Vector3(position.x - 0.5f, adjacentFloorHeight, -(position.y - 0.5f)));
                uvPos = position.x;
                break;
            case FaceDirection.South:
                finalVertices.Add(new Vector3(position.x - 0.5f, currentFloorHeight, -(position.y + 0.5f)));
                finalVertices.Add(new Vector3(position.x + 0.5f, currentFloorHeight, -(position.y + 0.5f)));
                finalVertices.Add(new Vector3(position.x - 0.5f, adjacentFloorHeight, -(position.y + 0.5f)));
                finalVertices.Add(new Vector3(position.x + 0.5f, adjacentFloorHeight, -(position.y + 0.5f)));
                uvPos = 16 - position.x;
                break;
            case FaceDirection.East:
                finalVertices.Add(new Vector3(position.x + 0.5f, currentFloorHeight, -(position.y + 0.5f)));
                finalVertices.Add(new Vector3(position.x + 0.5f, currentFloorHeight, -(position.y - 0.5f)));
                finalVertices.Add(new Vector3(position.x + 0.5f, adjacentFloorHeight, -(position.y + 0.5f)));
                finalVertices.Add(new Vector3(position.x + 0.5f, adjacentFloorHeight, -(position.y - 0.5f)));
                uvPos = position.y;
                break;
            case FaceDirection.West:
                finalVertices.Add(new Vector3(position.x - 0.5f, currentFloorHeight, -(position.y - 0.5f)));
                finalVertices.Add(new Vector3(position.x - 0.5f, currentFloorHeight, -(position.y + 0.5f)));
                finalVertices.Add(new Vector3(position.x - 0.5f, adjacentFloorHeight, -(position.y - 0.5f)));
                finalVertices.Add(new Vector3(position.x - 0.5f, adjacentFloorHeight, -(position.y + 0.5f)));
                uvPos = 16 - position.y;
                break;
            default:
                break;
        }
        finalUVs.Add(new Vector2(-(float)(uvPos + 1) / 16.0f, -(float)(0) / 16.0f));
        finalUVs.Add(new Vector2(-(float)(uvPos) / 16.0f, -(float)(0) / 16.0f));
        finalUVs.Add(new Vector2(-(float)(uvPos + 1) / 16.0f, -(float)(0 + 1) / 16.0f));
        finalUVs.Add(new Vector2(-(float)(uvPos) / 16.0f, -(float)(0 + 1) / 16.0f));

        finalVertexColors.Add(GetColor(position));
        finalVertexColors.Add(GetColor(position));
        finalVertexColors.Add(GetColor(position));
        finalVertexColors.Add(GetColor(position));

        finalFaces.Add(startindex);
        finalFaces.Add(startindex + 1);
        finalFaces.Add(startindex + 2);

        finalFaces.Add(startindex + 1);
        finalFaces.Add(startindex + 3);
        finalFaces.Add(startindex + 2);
    }

    void AddTopFace(DFCoord2d position, float height)
    {
        height -= 0.5f;
        //Todo: Weld vertices that should be welded
        //On second though, not with vertex colors there.
        int startindex = finalVertices.Count;
        finalVertices.Add(new Vector3(position.x - 0.5f, height, -(position.y - 0.5f)));
        finalVertices.Add(new Vector3(position.x + 0.5f, height, -(position.y - 0.5f)));
        finalVertices.Add(new Vector3(position.x - 0.5f, height, -(position.y + 0.5f)));
        finalVertices.Add(new Vector3(position.x + 0.5f, height, -(position.y + 0.5f)));

        finalUVs.Add(new Vector2((float)(position.x) / 16.0f, -(float)(position.y) / 16.0f));
        finalUVs.Add(new Vector2((float)(position.x + 1) / 16.0f, -(float)(position.y) / 16.0f));
        finalUVs.Add(new Vector2((float)(position.x) / 16.0f, -(float)(position.y + 1) / 16.0f));
        finalUVs.Add(new Vector2((float)(position.x + 1) / 16.0f, -(float)(position.y + 1) / 16.0f));

        finalVertexColors.Add(GetColor(position));
        finalVertexColors.Add(GetColor(position));
        finalVertexColors.Add(GetColor(position));
        finalVertexColors.Add(GetColor(position));

        finalFaces.Add(startindex);
        finalFaces.Add(startindex + 1);
        finalFaces.Add(startindex + 2);

        finalFaces.Add(startindex + 1);
        finalFaces.Add(startindex + 3);
        finalFaces.Add(startindex + 2);
    }
}
