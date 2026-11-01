using System;
using System.IO;
public static class SurfaceAudioReference {
 public static void Main(string[] args){
  using(var writer=new BinaryWriter(File.Create(args[0])))foreach(int rate in new[]{44100,48000,96000}){
   var mixer=new SurfaceAudioMixer();var data=new float[960];
   for(int n=0;n<400;n++){
    Array.Clear(data,0,data.Length);mixer.Render(data,2,rate,n<25?0:30,n<100?1:0,n>=100&&n<200?1:0,n>=200?1:0,n>=350);
    foreach(float value in data)writer.Write(value);
   }
  }
 }
}
