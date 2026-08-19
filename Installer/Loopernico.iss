#define MyAppName "Loopernico"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Loopernico"
#define MyAppExeName "Loopernico Refactorizado.exe"
#define UserDownloads GetEnv("USERPROFILE") + "\Downloads"
#define StemRoot GetEnv("LOCALAPPDATA") + "\Loopernico\StemSeparation"

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
PrivilegesRequired=admin

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "full"; Description: "Completa (recomendada)"
Name: "compact"; Description: "Solo Loopernico"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "plugins"; Description: "Plugins VST3"; Types: full
Name: "plugins\direct"; Description: "MT Power Drum Kit y Malibu"; Types: full
Name: "plugins\bertom"; Description: "Bertom Denoiser Classic 3.0.11 (instalador firmado)"; Types: full
Name: "plugins\vztec"; Description: "VZtec Overdrive, Fuzz y NAM Studio (instaladores no firmados)"; Types: full
Name: "stems"; Description: "Separacion real de stems Demucs CPU (runtime y modelo)"; Types: full

[Files]
Source: "..\Builds\VisualStudio2026\x64\Release\App\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Assets\*"; DestDir: "{app}\Assets"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Assets\Brand\Icons\loopernico-session.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#UserDownloads}\MTPDK-2.1.5.0-VST3-64bit-Windows-FULL\MTPDK-2.1.5-VST3-64bit-Windows-FULL\MT-PowerDrumKit.vst3"; DestDir: "{commoncf64}\VST3\Loopernico"; Components: plugins\direct; Flags: ignoreversion
Source: "{#UserDownloads}\Malibu-plugin 0.1.0-beta Files\Plugin\VST3\Malibu.vst3"; DestDir: "{commoncf64}\VST3\Loopernico"; Components: plugins\direct; Flags: ignoreversion
Source: "{#UserDownloads}\Bertom_DenoiserClassic_3.0.11_WIN\Bertom_DenoiserClassic_3.0.11_WIN\Bertom_DenoiserClassic_3.0.11_setup.exe"; DestDir: "{tmp}"; DestName: "Bertom_DenoiserClassic_3.0.11_setup.exe"; Components: plugins\bertom; Flags: deleteafterinstall
Source: "{#UserDownloads}\VZtec - Overdrive Setup 0.1.1\VZtec - Overdrive Setup 0.1.1.exe"; DestDir: "{tmp}"; DestName: "VZtec-Overdrive-Setup.exe"; Components: plugins\vztec; Flags: deleteafterinstall
Source: "{#UserDownloads}\VZtec - Fuzz Setup 0.1.0\VZtec - Fuzz Setup 0.1.0.exe"; DestDir: "{tmp}"; DestName: "VZtec-Fuzz-Setup.exe"; Components: plugins\vztec; Flags: deleteafterinstall
Source: "{#UserDownloads}\VZtec - NAM Studio Setup 0.1.0\VZtec - NAM Studio Setup 0.1.0.exe"; DestDir: "{tmp}"; DestName: "VZtec-NAM-Studio-Setup.exe"; Components: plugins\vztec; Flags: deleteafterinstall
Source: "{#StemRoot}\Runtime\demucs.cpp.main.exe"; DestDir: "{app}\StemSeparation\Runtime"; Components: stems; Flags: ignoreversion
Source: "{#StemRoot}\Models\ggml-model-htdemucs-6s-f16.bin"; DestDir: "{app}\StemSeparation\Models"; Components: stems; Flags: ignoreversion

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
Filename: "{tmp}\Bertom_DenoiserClassic_3.0.11_setup.exe"; Parameters: "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-"; StatusMsg: "Instalando Bertom Denoiser Classic..."; Components: plugins\bertom; Flags: waituntilterminated
Filename: "{tmp}\VZtec-Overdrive-Setup.exe"; Parameters: "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-"; StatusMsg: "Instalando VZtec Overdrive..."; Components: plugins\vztec; Flags: waituntilterminated
Filename: "{tmp}\VZtec-Fuzz-Setup.exe"; Parameters: "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-"; StatusMsg: "Instalando VZtec Fuzz..."; Components: plugins\vztec; Flags: waituntilterminated
Filename: "{tmp}\VZtec-NAM-Studio-Setup.exe"; Parameters: "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-"; StatusMsg: "Instalando VZtec NAM Studio..."; Components: plugins\vztec; Flags: waituntilterminated
Filename: "{app}\{#MyAppExeName}"; Description: "Abrir Loopernico"; Flags: nowait postinstall skipifsilent
