; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------
;Include Modern UI
!include "MUI2.nsh"

!define Project dwindow
!define SHCNE_ASSOCCHANGED 0x8000000
!define SHCNF_IDLIST 0
!include "FileAssoc.nsi"

;--------------------------------
;General

SetCompressor /SOLID lzma

; The name of the installer
Name "3DVPlayer"

; The file to write
OutFile "dwindow_setup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\DWindow

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\DWindow" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Language Selection Dialog Settings

  ;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
  !define MUI_LANGDLL_REGISTRY_KEY "Software\DWindow" 
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------

; Pages
;  !insertmacro MUI_PAGE_LICENSE "License.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;Page components
;Page directory
;Page instfiles

;UninstPage uninstConfirm
;UninstPage instfiles

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English" ;first language is the default language
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "SpanishInternational"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Latvian"
  !insertmacro MUI_LANGUAGE "Macedonian"
  !insertmacro MUI_LANGUAGE "Estonian"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Lithuanian"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Serbian"
  !insertmacro MUI_LANGUAGE "SerbianLatin"
  !insertmacro MUI_LANGUAGE "Arabic"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Hebrew"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Mongolian"
  !insertmacro MUI_LANGUAGE "Luxembourgish"
  !insertmacro MUI_LANGUAGE "Albanian"
  !insertmacro MUI_LANGUAGE "Breton"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Icelandic"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Bosnian"
  !insertmacro MUI_LANGUAGE "Kurdish"
  !insertmacro MUI_LANGUAGE "Irish"
  !insertmacro MUI_LANGUAGE "Uzbek"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "Afrikaans"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Esperanto"
  
;--------------------------------
; The stuff to install

LangString MAINPROGRAM_LANG ${LANG_ENGLISH} "3DVPlayer Program(required)"
LangString MAINPROGRAM_LANG ${LANG_SIMPCHINESE} "3DVPlayer主程序(必选)"
Section $(MAINPROGRAM_LANG)

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "StereoPlayer.exe"
  File "dwindow.ini"
  File "alpha.raw"
  File "logo.raw"
  File "Fonts.conf"
  File "MediaInfo.dll"
  File "DevIL.dll"
  File "ILU.dll"
  SetOutPath $INSTDIR\codec
  File "codec\*"
  SetOutPath $INSTDIR\skin
  File "skin\*"
  SetOutPath $INSTDIR\Language
  File "Language\*"
  SetOutPath $INSTDIR

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\DWindow "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "DisplayName" "3DVPlayer"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
LangString STARTMENU_LANG ${LANG_ENGLISH} "Start Menu Shortcuts"
LangString STARTMENU_LANG ${LANG_SIMPCHINESE} "开始菜单快捷方式"
Section $(STARTMENU_LANG)

  CreateDirectory "$SMPROGRAMS\3DVPlayer"
  CreateShortCut "$SMPROGRAMS\3DVPlayer\3DVPlayer.lnk" "$INSTDIR\StereoPlayer.exe" "" "$INSTDIR\StereoPlayer.exe" 0
  CreateShortCut "$SMPROGRAMS\3D影音\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

; Optional section (can be disabled by the user)
LangString DESKTOP_LANG ${LANG_ENGLISH} "Desktop Shortcuts"
LangString DESKTOP_LANG ${LANG_SIMPCHINESE} "桌面快捷方式"
Section $(DESKTOP_LANG)

  CreateShortCut "$DESKTOP\3DVPlayer.lnk" "$INSTDIR\StereoPlayer.exe" "" "$INSTDIR\StereoPlayer.exe" 0
  
SectionEnd

LangString ASSOC_LANG ${LANG_ENGLISH} "File association"
LangString ASSOC_LANG ${LANG_SIMPCHINESE} "文件关联"
SectionGroup /e $(ASSOC_LANG)

Section "mkv"
!insertmacro Assoc mkv "mkv" "MKV File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "mkv3d"
!insertmacro Assoc mkv3d "mkv3d" "3D MKV File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "ssif"
!insertmacro Assoc ssif "ssif" "SSIF File" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd

Section "mpls"
!insertmacro Assoc mpls "mpls" "MPLS Playlist" "$INSTDIR\StereoPlayer.exe" "$INSTDIR\StereoPlayer.exe,0"
SectionEnd


SectionGroupEnd

Section "-Refresh File Icon"
System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'
SectionEnd


;--------------------------------
; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow"
  DeleteRegKey HKCU "SOFTWARE\DWindow"

  ; Remove files and uninstaller
  Delete $INSTDIR\codec\*.dll
  Delete $INSTDIR\codec\*.ax
  Delete $INSTDIR\dwindow.exe
  Delete $INSTDIR\StereoPlayer.exe
  Delete $INSTDIR\detoured.dll
  Delete $INSTDIR\kill_ssaver.dll
  Delete $INSTDIR\launcher.exe
  Delete $INSTDIR\regsvr.exe
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\xvidcore.dll
  Delete $INSTDIR\alpha.raw
  Delete $INSTDIR\logo.raw
  Delete $INSTDIR\*.exe
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\fonts.conf
  Delete $INSTDIR\dxva2.dll

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\3DVPlayer\*.*"
  RMDir "$SMPROGRAMS\3DVPlayer"
  Delete "$DESKTOP\DWindow.lnk"

  ; Remove directories used
  RMDir "$SMPROGRAMS\DWindow"
  RMDir "$INSTDIR\codec"
  RMDir "$INSTDIR\Language"
  RMDir "$INSTDIR"

  ; Revese file association
  !insertmacro UnAssoc mkv
  !insertmacro UnAssoc mkv3d
  !insertmacro UnAssoc ssif
  !insertmacro UnAssoc mpls

  System::Call 'Shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, i 0, i 0)'

SectionEnd
