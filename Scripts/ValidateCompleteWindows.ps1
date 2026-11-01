$ErrorActionPreference='Stop'
$root='C:\Dev\GotlandRing\GotlandRingUnreal'
$package='C:\Dev\GotlandRing\Releases\complete-portable\Windows'
$userDir="$root\Saved\Complete Validation"
$log="$root\Saved\complete-validation.log"
'Complete Unity migration validation started' | Set-Content $log
function RunGameCheck($exe,$mode,$user){
 $report=Join-Path "$user\Saved" ($(if($mode -eq 'FrameRate'){'frame_rate'}elseif($mode -eq 'WheelView'){'driving'}else{$mode.ToLower()})+'_validation.json')
 if(Test-Path $report){Remove-Item $report}
 $p=Start-Process $exe -ArgumentList "-Ring${mode}Test -windowed -ResX=1600 -ResY=900 -UserDir=`"$user`"" -PassThru
 $watch=[Diagnostics.Stopwatch]::StartNew()
 do {Start-Sleep -Milliseconds 500;$games=@(Get-CimInstance Win32_Process -Filter "Name='GotlandRingUnreal-Win64-Shipping.exe'" | Where-Object {$_.CommandLine -like "*-Ring${mode}Test*" -and $_.CommandLine -like "*$user*"});$done=(Test-Path $report) -and $games.Count -eq 0} while(-not $done -and $watch.Elapsed.TotalSeconds -lt 150)
 if(-not $done){throw "$mode timed out"}
 $data=Get-Content $report -Raw | ConvertFrom-Json
 if(-not $data.passed){throw "$mode failed"}
 if($mode -eq 'Migration' -and $data.game_version -ne 'v0.1.0'){throw 'Wrong in-game version'}
 return $report
}
foreach($mode in @('Complete','Migration','Instruments','Settings','Driving','Wind','FrameRate')){
 $report=RunGameCheck "$package\GotlandRingUnreal.exe" $mode $userDir
 Copy-Item $report "$root\Validation\$($mode.ToLower())_complete_fractal.json" -Force
 "$mode passed=True" | Add-Content $log
}
$report=Get-Content "$root\Validation\complete_complete_fractal.json" -Raw|ConvertFrom-Json
$png=[IO.File]::ReadAllBytes($report.desktop_png)
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Collections.Generic;
public static class RingPngMetadata {
 static uint UInt(byte[] b,int p){return ((uint)b[p]<<24)|((uint)b[p+1]<<16)|((uint)b[p+2]<<8)|b[p+3];}
 public static Dictionary<string,string> Read(byte[] b){
  byte[] signature={137,80,78,71,13,10,26,10};for(int i=0;i<8;i++)if(b[i]!=signature[i])throw new Exception("PNG signature");
  var meta=new Dictionary<string,string>();int p=8;
  while(p<b.Length){int n=(int)UInt(b,p);uint crc=0xffffffff;
   for(int i=p+4;i<p+8+n;i++){crc^=b[i];for(int k=0;k<8;k++)crc=(crc>>1)^((crc&1)!=0?0xedb88320u:0u);}
   if(~crc!=UInt(b,p+8+n))throw new Exception("PNG CRC");
   if(Encoding.ASCII.GetString(b,p+4,4)=="iTXt"){
    int end=Array.IndexOf(b,(byte)0,p+8,n);string key=Encoding.ASCII.GetString(b,p+8,end-p-8);
    for(int i=1;i<=4;i++)if(b[end+i]!=0)throw new Exception("Unexpected compressed/language metadata");
    meta[key]=Encoding.UTF8.GetString(b,end+5,p+8+n-end-5);
   }p+=n+12;
  }return meta;
 }
}
'@
$metadata=[RingPngMetadata]::Read($png)
foreach($field in @('Software','Creation Time','Description','HUD','Vehicle','Camera')){if(!$metadata.ContainsKey($field)){throw "PNG missing $field"}}
@{passed=$true;crc_verified=$true;metadata=$metadata}|ConvertTo-Json -Depth 4|Set-Content "$root\Validation\desktop_png_fractal.json" -Encoding UTF8
Copy-Item $report.desktop_png "$root\Renders\DesktopCaptureFractal.png" -Force
Remove-Item $report.desktop_png
foreach($name in @('RallyPaintRed','RallyPaintBlue','RallyPaintWhite','SubaruBlueGold','TrainingArea')){Copy-Item "$userDir\Saved\$name.png" "$root\Renders\${name}Fractal.png" -Force}
'ALL COMPLETE WINDOWS CHECKS PASSED' | Add-Content $log
