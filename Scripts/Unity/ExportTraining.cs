using System;
using System.IO;
using System.Reflection;
using System.Globalization;
using UnityEngine;
public static class ExportTraining {
 static string F(float x)=>x.ToString("R",CultureInfo.InvariantCulture);
 public static void Run(){
  string output=Environment.GetEnvironmentVariable("GOTLAND_TRAINING_OUTPUT");Directory.CreateDirectory(output);
  var track=new DrivingSurface(null,null){Training=true}.Route;
  foreach(bool awd in new[]{true,false}){
   var settings=awd?new DrivingSettings{grip=8.8f,braking=6.5f,acceleration=.7f,response=90f/42f,offRoadGrip=3.5f}:new DrivingSettings();
   var pilot=new AutopilotController(track,settings);var speeds=(float[])typeof(AutopilotController).GetField("speedPlan",BindingFlags.Instance|BindingFlags.NonPublic).GetValue(pilot);
   using(var w=new StreamWriter(output+(awd?"/awd.csv":"/arcade.csv"))){w.WriteLine("cx,cy,rx,ry,left,right,lx,ly,z,speed,distance");for(int i=0;i<track.Points.Count;i++){var r=track.Sections[i];var p=pilot.RacingLine[i];w.WriteLine(string.Join(",",F(r.Center.z),F(r.Center.x),F(r.Right.z),F(r.Right.x),F(r.LeftWidth),F(r.RightWidth),F(p.z),F(p.x),F(p.y),F(speeds[i]),F(r.Distance)));}}
  }
 }
}
