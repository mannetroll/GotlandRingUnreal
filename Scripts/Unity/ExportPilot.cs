using System;
using System.IO;
using System.Reflection;
using System.Globalization;
using UnityEngine;
using UnityEditor;
public static class ExportPilot {
 static string F(float x)=>x.ToString("R",CultureInfo.InvariantCulture);
 public static void Run(){
  string output=Environment.GetEnvironmentVariable("GOTLAND_PILOT_OUTPUT");Directory.CreateDirectory(output);
  var track=TrackData.Load(Resources.Load<TextAsset>("Track/Surface").text);
  var settings=new DrivingSettings{grip=8.8f,braking=6.5f,acceleration=.7f,response=90f/42f,offRoadGrip=3.5f};
  var pilot=new AutopilotController(track,settings);
  var speeds=(float[])typeof(AutopilotController).GetField("speedPlan",BindingFlags.Instance|BindingFlags.NonPublic).GetValue(pilot);
  using(var w=new StreamWriter(output+"/route.csv")){
   w.WriteLine("cx,cy,rx,ry,left,right,lx,ly,z,speed,distance");
   for(int i=0;i<track.Points.Count;i++){var r=track.Sections[i];var p=pilot.RacingLine[i];w.WriteLine(string.Join(",",F(r.Center.z),F(r.Center.x),F(r.Right.z),F(r.Right.x),F(r.LeftWidth),F(r.RightWidth),F(p.z),F(p.x),F(p.y),F(speeds[i]),F(r.Distance)));}
  }
  using(var w=new StreamWriter(output+"/pilot_reference.csv")){
   w.WriteLine("x,y,yaw,vx,vy,speed,segment,onroad,steer,throttle,brake,target,error,curvature");
   for(int n=0;n<500;n++){
    int i=n*37%track.Points.Count;var r=track.Sections[i];var p=pilot.RacingLine[i]+r.Right*Mathf.Sin(n*.7f)*1.5f;
    var direction=(pilot.RacingLine[(i+1)%track.Points.Count]-pilot.RacingLine[i]).normalized;float yaw=Mathf.Atan2(direction.x,direction.z)*Mathf.Rad2Deg+Mathf.Sin(n*.3f)*8;
    var velocity=direction*(8+n%35);var road=track.Sample(p.x,p.z);
    pilot.ResetControls();var c=pilot.Drive(p,yaw,velocity,road,.016666667f);
    w.WriteLine(string.Join(",",F(p.z),F(p.x),F(yaw),F(velocity.z),F(velocity.x),F(velocity.magnitude),road.Segment,road.OnRoad?1:0,F(c.Steering),F(c.Throttle),F(c.Brake),F(c.TargetSpeed),F(c.LineError),F(c.Curvature)));
   }
  }
  var car=PrefabUtility.LoadPrefabContents("Assets/Resources/SubaruImpreza.prefab");var model=car.GetComponent<ImprezaModel>();
  using(var w=new StreamWriter(output+"/cockpit.csv")){
   w.WriteLine("name,x,y,z,fx,fy,fz,ux,uy,uz");
   foreach(var t in new[]{model.cockpitView,model.bonnetView,model.steeringWheel}){
    var p=t.position;var f=t.forward;var u=t.up;w.WriteLine(string.Join(",",t==model.cockpitView?"cockpit":t==model.bonnetView?"bonnet":"wheel",F(p.z*100),F(p.x*100),F(p.y*100),F(f.z),F(f.x),F(f.y),F(u.z),F(u.x),F(u.y)));
   }
  }
  PrefabUtility.UnloadPrefabContents(car);Debug.Log("PILOT_EXPORTED");
 }
}
