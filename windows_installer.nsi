; NSIS installer script for dfterm2

; REMEMBER to change these at every release


;-------------------------------------------
; Version

!define VERSION_MAJOR 0
!define VERSION_MINOR 1

;-------------------------------------------

;-------------------------------------------
; Names and directories
Name "Dfterm2 ${VERSION_MAJOR}.${VERSION_MINOR}"
OutFile "Dfterm2-installer-${VERSION_MAJOR}.${VERSION_MINOR}.exe"
InstallDir $PROGRAMFILES\dfterm2
InstallDirRegKey HKLM "Software\dfterm2" "Install_Dir"
RequestExecutionLevel admin
;-------------------------------------------

!include "MUI2.nsh"

Page license
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "Installer Section"

    SectionIn RO
    SetOutPath $INSTDIR

    WriteRegStr HKLM SOFTWARE\dfterm2 "Install_Dir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dfterm2" "DisplayName" "Dfterm2"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dfterm2" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dfterm2" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dfterm2" "NoRepair" 1

    ; List of files to distribute
    File "dfterm2.exe"
    File "dfterm2_configure.exe"
    File "dfterm_injection_glue.dll"
    File "icudt44.dll"
    File "icuin44.dll"
    File "icuio44.dll"
    File "icule44.dll"
    File "iculx44.dll"
    File "icutest.dll"
    File "icutu44.dll"
    File "icuuc44.dll"
    File "libeay32.dll"
    File "license.txt"
    File "license_other.txt"
    File "mingwm10.dll"
    File "msvcp100.dll"
    File "msvcr100.dll"
    File "ssleay32.dll"
    File "testplug.dll"
    File "version.txt"
    CreateDirectory $INSTDIR\soiled
    File /oname=soiled\AC_OETags.js "soiled\AC_OETags.js"
    File /oname=soiled\beep.mp3 "soiled\beep.mp3"
    File /oname=soiled\soiled.swf "soiled\soiled.swf"
    File /oname=soiled\soiled.txt "soiled\soiled.txt"
    File /oname=soiled\soiled_LICENSE "soiled\soiled_LICENSE"

    WriteUninstaller "uninstall.exe"
SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\dfterm2"
  
  CreateShortCut "$SMPROGRAMS\dfterm2\Uninstall.lnk" \
                 "$INSTDIR\uninstall.exe" \
                 "" \
                 "$INSTDIR\uninstall.exe" \
                 0
                 
  CreateShortCut "$SMPROGRAMS\dfterm2\Dfterm2.lnk" \
                 "$INSTDIR\dfterm2" \
                 "--create-appdir --logfile %APPDATA%\dfterm2\dfterm2.log --database %APPDATA%\dfterm2\dfterm2.database" \
                 "$INSTDIR\dfterm2.exe" \
                 0
  
SectionEnd


Section "un.Uninstaller Section"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\dfterm2"
    DeleteRegKey HKLM SOFTWARE\dfterm2

    Delete $INSTDIR\*.*
    Delete $INSTDIR\soiled\*.*

    RMDir $INSTDIR\soiled
    RMDir $INSTDIR
SectionEnd
