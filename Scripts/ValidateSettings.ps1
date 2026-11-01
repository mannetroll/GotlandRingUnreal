param([string]$EngineRoot='C:\UE_5.8')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$started=Get-Date
$arguments='"'+(Join-Path $root 'GotlandRingUnreal.uproject')+'" /Game/Migration/Prototype -game -windowed -ResX=1600 -ResY=900 -NoSplash -RingSettingsTest -ExecCmds="t.MaxFPS 60"'
$process=Start-Process (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe') -ArgumentList $arguments -WorkingDirectory $root -Wait -PassThru
if($process.ExitCode -ne 0){throw "Unreal exited with $($process.ExitCode)"}
$report=Get-Item (Join-Path $root 'Saved\settings_validation.json')
if($report.LastWriteTime -lt $started){throw 'No fresh settings report was produced'}
$result=Get-Content $report.FullName -Raw | ConvertFrom-Json
$result | ConvertTo-Json
if(-not $result.passed){throw 'Settings validation failed'}
