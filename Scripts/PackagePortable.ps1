param(
 [string]$EngineRoot='C:\UE_5.8',
 [string]$OutputDirectory='C:\Dev\GotlandRing\Releases\v0.1.0-portable',
 [switch]$SkipBuildAndCook
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vs){throw 'Visual Studio C++ redistributables were not found'}
$redist=Get-ChildItem "$vs/VC/Redist/MSVC" -Directory | Where-Object {$_.Name -match '^\d+\.'} | Sort-Object {[version]$_.Name} -Descending | Select-Object -First 1
$dependencies=Join-Path $root 'Saved/PortableDependencies'
$crt=Join-Path $dependencies 'Win64/x64/Microsoft.VC143.CRT'
New-Item -ItemType Directory -Force $crt | Out-Null
Copy-Item "$($redist.FullName)/x64/Microsoft.VC143.CRT/*.dll" $crt -Force
$arguments=@('BuildCookRun',"-project=$root/GotlandRingUnreal.uproject",'-noP4','-platform=Win64','-clientconfig=Shipping','-map=/Game/Migration/Prototype','-stage','-pak','-iostore','-archive',"-archivedirectory=$OutputDirectory",'-nodebuginfo',"-applocaldirectory=$dependencies",'-utf8output','-unattended')
if($SkipBuildAndCook){$arguments+=@('-skipbuild','-skipcook')}else{$arguments+=@('-build','-cook')}
& "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" @arguments
if($LASTEXITCODE -ne 0){throw "Packaging failed: $LASTEXITCODE"}
$package=Join-Path $OutputDirectory 'Windows'
foreach($file in @('LICENSE','DATA_LICENSES.md','VEHICLE_ASSET_CREDITS.md','ENGINE_AUDIO_CREDITS.md')){Copy-Item (Join-Path $root $file) $package -Force}
@'
GotlandRingUnreal v0.1.0 - portable Windows game

Launch GotlandRingUnreal.exe. Keep the complete folder together.
The game and runtime DLLs are included. No Unreal Editor is required.
Windows x64; Windows 11 validated, Windows 10 not runtime-tested.

W/Up: throttle  S/Down: brake  A/D or arrows: steer  X: reverse
Space: handbrake  P: autopilot  C: cockpit/bonnet/chase/drone
O: reference lap  V: 15 scenery views  H: AWD/Arcade
Esc: menu  F3/F1: settings  U: mouse steering  F2: screenshot
Replay: O returns, Space/Esc pauses, Home restarts, M mutes.

Preserve the included licenses and asset credits.
'@ | Set-Content (Join-Path $package 'START-HERE.txt') -Encoding UTF8
$revision=git -C $root rev-parse HEAD
@{revision=$revision;version='0.1.0';tag='v0.1.0';configuration='Shipping';platform='Win64';built_utc=(Get-Date).ToUniversalTime().ToString('o')} | ConvertTo-Json | Set-Content (Join-Path $package 'build-info.json') -Encoding UTF8
Write-Output "Portable build: $package/GotlandRingUnreal.exe"
