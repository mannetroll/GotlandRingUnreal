using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;

public static class ExportScenery {
 [Serializable] public class Item { public string name, material, texture, shader; public float[] color, bounds; public float roughness, metallic; public bool masked; public int objects, triangles; }
 [Serializable] public class Label { public string text; public float[] position, forward; public float height; }
 [Serializable] public class Manifest { public List<Label> labels=new List<Label>(); public string source="Existing Unity runtime scenery; approximate ground, not surveyed DTM"; public List<Item> meshes=new List<Item>(); public int trees; }
 class Group { public Item item; public List<Vector3> vertices=new List<Vector3>(), normals=new List<Vector3>(); public List<Vector2> uv=new List<Vector2>(); public List<int> triangles=new List<int>(); }
 static string F(float v)=>v.ToString("R",CultureInfo.InvariantCulture);
 public static void Run(){
  string output=Environment.GetEnvironmentVariable("GOTLAND_SCENERY_OUTPUT");Directory.CreateDirectory(output);Directory.CreateDirectory(output+"/Textures");
  EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
  var drive=new GameObject("Scenery export").AddComponent<RingDrive>();
  typeof(RingDrive).GetMethod("Awake",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(drive,null);
  var manifest=new Manifest();var groups=new Dictionary<Material,Group>();
  foreach(var filter in UnityEngine.Object.FindObjectsByType<MeshFilter>(FindObjectsSortMode.None).OrderBy(f=>f.name)){
   var renderer=filter.GetComponent<MeshRenderer>();var mesh=filter.sharedMesh;if(!renderer||!mesh)continue;
   var v=mesh.vertices;var normals=mesh.normals;var uv=mesh.uv;var offsets=mesh.uv2;
   bool forest=filter.GetComponentInParent<TrackForest>()!=null;
   if(forest)manifest.trees+=v.Length/4;
   for(int part=0;part<mesh.subMeshCount;part++){
    // The validated road and aprons already exist; migrate its outer shoulders only.
    if(filter.name=="Estimated banked track surface"&&part>=1&&part<=3)continue;
    var materials=renderer.sharedMaterials;var mat=materials[Math.Min(part,materials.Length-1)];
    if(!groups.TryGetValue(mat,out var group)){
     string name="Scenery_"+groups.Count.ToString("D2");Color color=mat.HasProperty("_Color")?mat.color:Color.white;
     var item=new Item{name=name,material=mat.name,shader=mat.shader.name,color=new[]{color.r,color.g,color.b,color.a},roughness=mat.HasProperty("_Glossiness")?1-mat.GetFloat("_Glossiness"):.8f,metallic=mat.HasProperty("_Metallic")?mat.GetFloat("_Metallic"):0,masked=forest};
     if(mat.mainTexture){string path=AssetDatabase.GetAssetPath(mat.mainTexture);if(!string.IsNullOrEmpty(path)){item.texture=Path.GetFileName(path);File.Copy(path,output+"/Textures/"+item.texture,true);}}
     group=new Group{item=item};groups.Add(mat,group);manifest.meshes.Add(item);
    }
    group.item.objects++;
    int copies=forest?2:1;
    for(int copy=0;copy<copies;copy++){
     var indices=mesh.GetTriangles(part);var remap=new Dictionary<int,int>();
     foreach(int index in indices.Distinct()){
      var point=v[index];if(forest)point+=(copy==0?Vector3.right:Vector3.forward)*offsets[index].x+Vector3.up*offsets[index].y;
      point=filter.transform.TransformPoint(point);
      Vector3 normal=forest?(copy==0?Vector3.back:Vector3.right):filter.transform.localToWorldMatrix.inverse.transpose.MultiplyVector(normals[index]).normalized;
      remap[index]=group.vertices.Count;group.vertices.Add(new Vector3(point.z,-point.x,point.y)*100);
      group.normals.Add(new Vector3(normal.z,-normal.x,normal.y));group.uv.Add(uv.Length==v.Length?uv[index]:Vector2.zero);
     }
     for(int t=0;t<indices.Length;t+=3)group.triangles.AddRange(new[]{remap[indices[t+2]],remap[indices[t+1]],remap[indices[t]]});
    }
   }
  }
  foreach(var g in groups.Values){
   using(var w=new StreamWriter(output+"/"+g.item.name+".obj")){
    foreach(var p in g.vertices)w.WriteLine($"v {F(p.x)} {F(p.y)} {F(p.z)}");
    foreach(var p in g.uv)w.WriteLine($"vt {F(p.x)} {F(p.y)}");
    foreach(var p in g.normals)w.WriteLine($"vn {F(p.x)} {F(p.y)} {F(p.z)}");
    for(int i=0;i<g.triangles.Count;i+=3)w.WriteLine("f "+string.Join(" ",g.triangles.Skip(i).Take(3).Select(n=>$"{n+1}/{n+1}/{n+1}")));
   }
   g.item.triangles=g.triangles.Count/3;
   g.item.bounds=new[]{g.vertices.Min(p=>p.x),g.vertices.Max(p=>p.x),-g.vertices.Max(p=>p.y),-g.vertices.Min(p=>p.y),g.vertices.Min(p=>p.z),g.vertices.Max(p=>p.z)};
  }
  foreach(var text in UnityEngine.Object.FindObjectsByType<TextMesh>(FindObjectsSortMode.None)){
   var p=text.transform.position;var f=text.transform.TransformDirection(Vector3.back);var size=text.GetComponent<MeshRenderer>().localBounds.size;
   manifest.labels.Add(new Label{text=text.text,position=new[]{p.z*100,p.x*100,p.y*100},forward=new[]{f.z,f.x,f.y},height=size.y*text.transform.lossyScale.y*100/text.text.Split('\n').Length});
  }
  File.WriteAllText(output+"/scenery.json",JsonUtility.ToJson(manifest,true));Debug.Log($"SCENERY_EXPORTED groups={groups.Count} trees={manifest.trees}");
 }
}
