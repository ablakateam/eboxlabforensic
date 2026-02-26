; EBOXLAB Installer Script for Inno Setup 6
; Digital Forensics & eDiscovery Platform
; Law Stars - Trial Lawyers and Legal Services Colorado LLC

#define MyAppName "EBOXLAB"
#define MyAppVersion "1.0"
#define MyAppPublisher "Law Stars - Trial Lawyers and Legal Services Colorado LLC"
#define MyAppExeName "eboxlab.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppCopyright=Copyright (C) 2026 {#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=installer_output
OutputBaseFilename=EBOXLAB_Setup_v1.0
SetupIconFile=
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
LicenseFile=
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName={#MyAppName} - Digital Forensics Platform
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "eboxlab.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Comment: "EBOXLAB Digital Forensics Platform"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; Comment: "EBOXLAB Digital Forensics Platform"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{userappdata}\EBOXLAB"

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nEBOXLAB is a digital forensics and eDiscovery platform designed for forensic examiners and legal professionals.%n%nFeatures include case management, evidence tracking, chain of custody, file browsing, and SHA-256/MD5 hash calculation.%n%nData is stored in %APPDATA%\EBOXLAB\ and will persist across reinstalls.
FinishedLabel=Setup has completed installing [name] on your computer.%n%nThe application may be launched by selecting the installed shortcut.%n%nCase data is stored in: %APPDATA%\EBOXLAB\
