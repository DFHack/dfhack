using DFHack;
using System.Collections.Generic;
using UnityEngine;

public class TestScript : MonoBehaviour {
    RemoteClient remoteClient = new RemoteClient();
	// Use this for initialization
	void Start () {
            if (!remoteClient.connect())
                Debug.LogError("Could not connect");
            else
            {
                remoteClient.run_command("ls", new List<string>());
            }
	}
	
	// Update is called once per frame
	void Update () {
	
	}
}
