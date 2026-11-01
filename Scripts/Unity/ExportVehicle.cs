using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Globalization;
using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;

public static class ExportVehicle {
 [Serializable] public class Item { public string name, material, materialPath, texture, shader; public float[] color, bounds; public float roughness, metallic; public int objects, triangles; public int wheel=-1; public float[] pivot; }
 [Serializable] public class Manifest { public string source; public List<Item> meshes=new List<Item>(); public float wheelRadius; public string displayName,credit; public string[] paintNames; public float[] paintColors,cockpit,bonnet,steeringAxis; public int initialPaint,paintSlot=-1; }
 class Group { public Item item; public List<Vector3> vertices=new List<Vector3>(), normals=new List<Vector3>(); public List<Vector2> uv=new List<Vector2>(); public List<int> triangles=new List<int>(); }
 static string F(float v)=>v.ToString("R",CultureInfo.InvariantCulture);
 public static void Run(){
  string output=Environment.GetEnvironmentVariable("GOTLAND_VEHICLE_OUTPUT");Directory.CreateDirectory(output);Directory.CreateDirectory(output+"/Textures");
  EditorSceneManager.NewScene(NewSceneSetup.EmptyScene,NewSceneMode.Single);
  var car=PrefabUtility.LoadPrefabContents("Assets/Resources/"+Environment.GetEnvironmentVariable("GOTLAND_VEHICLE_MODEL")+".prefab");
  var model=car.GetComponent<ImprezaModel>();
  var manifest=new Manifest{source="Unity "+Environment.GetEnvironmentVariable("GOTLAND_VEHICLE_MODEL")+" prefab; CC BY-NC 4.0",wheelRadius=model.wheelRadius};var groups=new Dictionary<string,Group>();
  manifest.displayName=model.displayName;manifest.credit=model.credit;manifest.paintNames=model.paintNames;manifest.initialPaint=model.initialPaint;manifest.paintColors=model.paintColors.SelectMany(c=>new[]{c.r,c.g,c.b,c.a}).ToArray();manifest.cockpit=new[]{model.cockpitView.position.z*100,model.cockpitView.position.x*100,model.cockpitView.position.y*100};manifest.bonnet=new[]{model.bonnetView.position.z*100,model.bonnetView.position.x*100,model.bonnetView.position.y*100};var axis=model.steeringWheel.forward;manifest.steeringAxis=new[]{axis.z,axis.x,axis.y};
  foreach(var renderer in car.GetComponentsInChildren<Renderer>().OrderBy(f=>f.name)){
   if(!renderer.enabled)continue;
   Mesh mesh=null;
   if(renderer is SkinnedMeshRenderer skinned){mesh=new Mesh();skinned.BakeMesh(mesh);}
   else {var mf=renderer.GetComponent<MeshFilter>();if(mf)mesh=mf.sharedMesh;}
   if(!mesh)continue;
   var filter=renderer;
   int wheel=-1;Vector3 pivot=Vector3.zero;
   for(int wi=0;wi<4;wi++)if(renderer.transform.IsChildOf(model.wheelSpin[wi])){wheel=wi;pivot=model.wheelSpin[wi].position;break;}
   if(renderer==model.cockpitHead)wheel=-2;
   if(renderer==model.driverSteering.body)wheel=-4;
   if(renderer.transform.IsChildOf(model.steeringWheel)){wheel=-3;pivot=model.steeringWheel.position;}
   var v=mesh.vertices;var normals=mesh.normals;var uv=mesh.uv;

   for(int part=0;part<mesh.subMeshCount;part++){

    var materials=renderer.sharedMaterials;var mat=materials[Math.Min(part,materials.Length-1)];
    string key=mat.GetInstanceID()+"_"+wheel;
    if(!groups.TryGetValue(key,out var group)){
     string name="Vehicle_"+groups.Count.ToString("D2");Color color=mat.HasProperty("_Color")?mat.color:Color.white;
     var item=new Item{wheel=wheel,pivot=new[]{pivot.z*100,pivot.x*100,pivot.y*100},name=name,material=mat.name,materialPath=AssetDatabase.GetAssetPath(mat),shader=mat.shader.name,color=new[]{color.r,color.g,color.b,color.a},roughness=mat.HasProperty("_Glossiness")?1-mat.GetFloat("_Glossiness"):.8f,metallic=mat.HasProperty("_Metallic")?mat.GetFloat("_Metallic"):0};
     if(mat.mainTexture){string path=AssetDatabase.GetAssetPath(mat.mainTexture);if(!string.IsNullOrEmpty(path)){item.texture="Tex_"+AssetDatabase.AssetPathToGUID(path)+Path.GetExtension(path);File.Copy(path,output+"/Textures/"+item.texture,true);}}
     if(mat==model.bodyPaint)manifest.paintSlot=groups.Count;
     group=new Group{item=item};groups.Add(key,group);manifest.meshes.Add(item);
    }
    group.item.objects++;
     var indices=mesh.GetTriangles(part);var remap=new Dictionary<int,int>();
     foreach(int index in indices.Distinct()){
      var point=v[index];
      point=filter.transform.TransformPoint(point)-pivot;
      Vector3 normal=filter.transform.localToWorldMatrix.inverse.transpose.MultiplyVector(normals[index]).normalized;
      remap[index]=group.vertices.Count;group.vertices.Add(new Vector3(point.z,-point.x,point.y)*100);
      group.normals.Add(new Vector3(normal.z,-normal.x,normal.y));group.uv.Add(uv.Length==v.Length?uv[index]:Vector2.zero);
     }
     for(int t=0;t<indices.Length;t+=3)group.triangles.AddRange(new[]{remap[indices[t+2]],remap[indices[t+1]],remap[indices[t]]});
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
  File.WriteAllText(output+"/vehicle.json",JsonUtility.ToJson(manifest,true));
  PrefabUtility.UnloadPrefabContents(car);Debug.Log($"VEHICLE_EXPORTED groups={groups.Count}");
 }
}
