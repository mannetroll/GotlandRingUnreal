@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /fp:strict /I"%~dp0..\Source\GotlandRingUnreal" "%~dp0..\Tests\dynamics.cpp" /Fe:"%~dp0..\Saved\dynamics-tests.exe" /Fo:"%~dp0..\Saved\dynamics-tests.obj"
if errorlevel 1 exit /b 1
"%~dp0..\Saved\dynamics-tests.exe" "%~dp0..\Validation\UnityReference"
