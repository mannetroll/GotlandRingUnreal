$project=Split-Path $PSScriptRoot -Parent
$started=Get-Date
& 'D:/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' "$project/GotlandRingUnreal.uproject" /Game/Migration/Prototype -game -windowed -ResX=1600 -ResY=900 -NoSplash -RingPilotTest -benchmark -fps=60
$report=Get-Item "$project/Saved/pilot_validation.json" -ErrorAction Stop
if($report.LastWriteTime -lt $started){throw 'No fresh pilot validation report'}
$result=Get-Content $report.FullName -Raw | ConvertFrom-Json
$result | ConvertTo-Json
if(-not $result.passed){throw 'Autopilot lap failed'}
