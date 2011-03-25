; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------
SetCompressor /SOLID lzma

; The name of the installer
Name "DWindow"

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

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "DWindow(required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "dwindow.exe"
  File "dwindow.ini"
  ;File "launcher.exe"
  ;File "regsvr.exe"
  SetOutPath $INSTDIR\codec
  File "codec\*"
  SetOutPath $INSTDIR

  ;ExecWait '"$INSTDIR\regsvr.exe" -silent'

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\DWindow "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "DisplayName" "DWindow"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\DWindow"
  ;CreateShortCut "$SMPROGRAMS\DWindow\DWindow.lnk" "$INSTDIR\launcher.exe" "" "$INSTDIR\launcher.exe" 0
  ;CreateShortCut "$SMPROGRAMS\DWindow\Re-Register modules.lnk" "$INSTDIR\regsvr.exe" "" "$INSTDIR\regsvr.exe" 0
  CreateShortCut "$SMPROGRAMS\DWindow\DWindow.lnk" "$INSTDIR\dwindow.exe" "" "$INSTDIR\dwindow.exe" 0
  CreateShortCut "$SMPROGRAMS\DWindow\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DWindow"
  DeleteRegKey HKLM SOFTWARE\DWindow

  ; Remove files and uninstaller
  Delete $INSTDIR\codec\*.dll
  Delete $INSTDIR\codec\*.ax
  Delete $INSTDIR\dwindow.exe
  Delete $INSTDIR\kill_ssaver.dll
  Delete $INSTDIR\launcher.exe
  Delete $INSTDIR\regsvr.exe
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\DWindow\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\DWindow"
  RMDir "$INSTDIR\codec"
  RMDir "$INSTDIR"

SectionEnd
