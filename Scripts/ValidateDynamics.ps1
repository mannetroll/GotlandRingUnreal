$projectRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $projectRoot 'GotlandRingUnreal.uproject'
$started = Get-Date
& 'D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' $project '/Game/Migration/Prototype' -game -windowed -ResX=1600 -ResY=900 -NoSplash -RingDynamicsTest -ExecCmds='t.MaxFPS 60'
foreach ($name in @('collision_validation.json','dynamics_validation.json','audio_validation.json')) {
    $report = Get-Item (Join-Path $projectRoot "Saved\$name") -ErrorAction Stop
    if ($report.LastWriteTime -lt $started) { throw "No fresh $name was produced" }
    $result = Get-Content $report.FullName -Raw | ConvertFrom-Json
    $result | ConvertTo-Json
    if (-not $result.passed) { throw "$name failed" }
}
