$project = Join-Path $PSScriptRoot '..\GotlandRingUnreal.uproject'
& 'D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' $project '/Game/Migration/Prototype' -game -windowed -ResX=1600 -ResY=900 -NoSplash
