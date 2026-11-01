using System;
using System.IO;
using System.Linq;
using UnityEngine;
using UnityEditor;
public static class ExportDriver {
 [Serializable] public class Vertex { public Vector3 p,n; public Vector2 uv; public int[] bones; public float[] weights; }
 [Serializable] public class Rig { public Vector3 centre,axis,cockpit,bonnet; public Quaternion cockpitRotation; public Vector3[] joints; public Vertex[] vertices; public int[] triangles; }
 public static void Run(){
  var car=PrefabUtility.LoadPrefabContents("Assets/Resources/RallyCar.prefab");
  var model=car.GetComponent<ImprezaModel>();var rig=model.driverSteering;var mesh=rig.body.sharedMesh;
  var v=mesh.vertices;var n=mesh.normals;var uv=mesh.uv;var w=mesh.boneWeights;
  var data=new Rig{centre=rig.wheelCentre,axis=rig.wheelAxis,cockpit=model.cockpitView.localPosition,bonnet=model.bonnetView.localPosition,cockpitRotation=model.cockpitView.localRotation,joints=rig.arms.SelectMany(a=>new[]{a.shoulder,a.elbow,a.wrist}).ToArray(),triangles=mesh.triangles,
   vertices=Enumerable.Range(0,v.Length).Select(i=>new Vertex{p=v[i],n=n[i],uv=uv[i],bones=new[]{w[i].boneIndex0,w[i].boneIndex1,w[i].boneIndex2,w[i].boneIndex3},weights=new[]{w[i].weight0,w[i].weight1,w[i].weight2,w[i].weight3}}).ToArray()};
  File.WriteAllText(Environment.GetEnvironmentVariable("GOTLAND_DRIVER_OUTPUT"),JsonUtility.ToJson(data));
  Debug.Log($"DRIVER_EXPORTED vertices={v.Length} triangles={data.triangles.Length/3}");PrefabUtility.UnloadPrefabContents(car);
 }
}
