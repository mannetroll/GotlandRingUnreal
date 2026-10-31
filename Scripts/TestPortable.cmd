@echo off
setlocal
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%i"
call "%VSROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /fp:strict /I"%~dp0..\Source\GotlandRingUnreal" "%~dp0..\Tests\dynamics.cpp" /Fe:"%~dp0..\Saved\dynamics-tests.exe" /Fo:"%~dp0..\Saved\dynamics-tests.obj"
if errorlevel 1 exit /b 1
"%~dp0..\Saved\dynamics-tests.exe" "%~dp0..\Validation\UnityReference"
if errorlevel 1 exit /b 1
"%VSROOT%\MSBuild\Current\Bin\Roslyn\csc.exe" /nologo /out:"%~dp0..\Saved\tyre-reference.exe" "%~dp0..\Tests\TyreAudioReference.cs" "%~dp0..\..\GotlandRingUnity\Assets\Scripts\ImprezaEngineMixer.cs"
if errorlevel 1 exit /b 1
"%~dp0..\Saved\tyre-reference.exe" "%~dp0..\..\GotlandRingUnity\Assets\Resources\Audio" "%~dp0..\Saved\tyre-reference.f32"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /fp:strict /I"%~dp0..\Source\GotlandRingUnreal" "%~dp0..\Tests\tyre_audio.cpp" /Fe:"%~dp0..\Saved\tyre-audio-tests.exe" /Fo:"%~dp0..\Saved\tyre-audio-tests.obj"
if errorlevel 1 exit /b 1
"%~dp0..\Saved\tyre-audio-tests.exe" "%~dp0..\Saved\tyre-reference.f32"
exit /b %errorlevel%
