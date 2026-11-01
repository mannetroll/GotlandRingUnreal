using System;
using System.IO;
// Compile alongside Unity's unmodified ImprezaEngineMixer.cs.
public static class TyreAudioReference {
 static float[] ReadWave(string path) {
  using(var r=new BinaryReader(File.OpenRead(path))) {
   r.ReadBytes(12);
   while(r.BaseStream.Position<r.BaseStream.Length) {
    string id=new string(r.ReadChars(4));int size=r.ReadInt32();
    if(id=="data") {var samples=new float[size/2];for(int i=0;i<samples.Length;i++)samples[i]=r.ReadInt16()/32768f;return samples;}
    r.BaseStream.Position+=size+(size&1);
   }
  }
  throw new InvalidDataException(path);
 }
 public static void Main(string[] args) {
  var loops=new float[3][];var names=new[]{"ImprezaLow","ImprezaMid","ImprezaHigh"};
  for(int i=0;i<3;i++)loops[i]=ReadWave(Path.Combine(args[0],names[i]+".wav"));
  using(var writer=new BinaryWriter(File.Create(args[1]))) {
   foreach(int rate in new[]{44100,48000,96000}) {
    var mixer=new ImprezaEngineMixer(loops,rate);var data=new float[960];
    for(int n=0;n<400;n++) {
     mixer.Render(data,2,3500,.6f,n<25?0:30,n>=350);
     foreach(float value in data)writer.Write(value);
    }
   }
  }
 }
}
