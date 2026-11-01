#ifndef PackageDir
  #error PackageDir must point to the validated portable Windows folder
#endif
#ifndef ReleaseDir
  #error ReleaseDir must point to the release output folder
#endif

[Setup]
AppId={{65CE7487-4FA9-42D3-90A1-5DD841726AC8}
AppName=GotlandRingUnreal
AppVersion=0.1.0
AppVerName=GotlandRingUnreal v0.1.0
AppPublisher=Mannetroll Solutions AB
AppPublisherURL=https://github.com/mannetroll/GotlandRingUnreal
AppSupportURL=https://github.com/mannetroll/GotlandRingUnreal/issues
AppUpdatesURL=https://github.com/mannetroll/GotlandRingUnreal/releases
DefaultDirName={localappdata}\Programs\GotlandRing
DefaultGroupName=GotlandRing
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#ReleaseDir}
OutputBaseFilename=GotlandRingUnreal-v0.1.0-Setup-Win64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\..\PortableLauncher\Impreza.ico
UninstallDisplayIcon={app}\GotlandRingUnreal.exe
InfoBeforeFile=InstallInfo.txt
VersionInfoVersion=0.1.0.0
CloseApplications=yes
RestartApplications=no

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#PackageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\GotlandRing"; Filename: "{app}\GotlandRingUnreal.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\GotlandRing Standalone"; Filename: "{app}\GotlandRingUnreal.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\GotlandRingUnreal.exe"; Description: "Launch GotlandRing"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent
