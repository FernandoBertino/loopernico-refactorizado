#define MyAppName "Loopernico"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Loopernico"
#define MyAppExeName "Loopernico Refactorizado.exe"

[Setup]
AppId={{6FBEA249-985B-48D7-AE13-C85F4C58AE3D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\Loopernico
DefaultGroupName=Loopernico
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=Loopernico-Setup-{#MyAppVersion}
SetupIconFile=..\Assets\Brand\Icons\loopernico-app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
WizardStyle=modern

[Files]
Source: "..\Builds\VisualStudio2026\x64\Release\App\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Assets\*"; DestDir: "{app}\Assets"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Assets\Brand\Icons\loopernico-session.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autodesktop}\Loopernico"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Comment: "Loopernico"
Name: "{group}\Loopernico"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"

[Registry]
Root: HKA; Subkey: "Software\Classes\.loopernico"; ValueType: string; ValueName: ""; ValueData: "Loopernico.Session"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\Loopernico.Session"; ValueType: string; ValueName: ""; ValueData: "Archivo de Loopernico"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Loopernico.Session"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "Archivo de Loopernico"
Root: HKA; Subkey: "Software\Classes\Loopernico.Session\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\loopernico-session.ico,0"
Root: HKA; Subkey: "Software\Classes\Loopernico.Session\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Abrir Loopernico"; Flags: nowait postinstall skipifsilent
