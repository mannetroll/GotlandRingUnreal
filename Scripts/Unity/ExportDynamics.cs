using System;
using System.IO;
using System.Globalization;
using UnityEngine;
public static class ExportDynamics {
 public static void Run() {
  string output=Environment.GetEnvironmentVariable("GOTLAND_DYNAMICS_OUTPUT");Directory.CreateDirectory(output);
  var engine=new SubaruEngine(new SubaruVehicleSetup());
  using(var w=new StreamWriter(output+"/engine_reference.csv")) {
   w.WriteLine("step,pedal,shaft_rpm,open,torque,rpm");
   for(int n=0;n<4000;n++) {
    float pedal=n<100?0:n<1600?1:n<2300?0:n<3200?.75f:1;
    float shaft=n<800?0:n<1200?2500:n<1600?4500:n<2800?0:3500;
    bool open=n<800||(n>=1600&&n<2300);
    float torque=engine.DriveTorque(.01f,pedal,shaft,open);engine.Integrate(.01f,torque);
    w.WriteLine(string.Join(",",n,pedal.ToString("R",CultureInfo.InvariantCulture),shaft,open?1:0,torque.ToString("R",CultureInfo.InvariantCulture),engine.Rpm.ToString("R",CultureInfo.InvariantCulture)));
   }
  }
  var mixer=BoxerAudio.CreateMixer(48000);
  using(var w=new BinaryWriter(File.Create(output+"/audio_reference.f32"))) {
   var buffer=new float[480*2];
   for(int n=0;n<1200;n++) {
    float rpm=n<200?900:n<800?900+(n-200)*9:6300-(n-800)*13.5f;
    mixer.Render(buffer,2,rpm,n>=200&&n<800?1:0,n<200?0:25,n>=1100);
    foreach(float sample in buffer)w.Write(sample);
   }
  }
  Debug.Log("DYNAMICS_REFERENCE_EXPORTED");
 }
}
