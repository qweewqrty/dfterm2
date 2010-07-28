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
!include "nsDialogs.nsh"
!include "LogicLib.nsh"

XPStyle on

Page license
Page directory
Page instfiles
Page custom adminAccountSetPage

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

;---- The admin account creation dialog

Var EDIT_USERNAME
Var EDIT_PASSWORD
Var EDIT_PASSWORD_RETYPE
Var BUTTON

Function adminAccountSetPage
    nsDialogs::Create 1018
    Pop $0
    
	GetFunctionAddress $0 OnBack
	nsDialogs::OnBack $0
    
    ${NSD_CreateLabel} 0 50u 100% 10u "Username"
    Pop $0
    
    ${NSD_CreateLabel} 0 70u 100% 10u "Password"
    Pop $0
    
    ${NSD_CreateLabel} 0 90u 100% 10u "Retype password"
    Pop $0
    
	${NSD_CreateText} 0 60u 100% 10u ""
	Pop $EDIT_USERNAME
	GetFunctionAddress $0 OnUsernameChange
	nsDialogs::OnChange $EDIT_USERNAME $0
    
	${NSD_CreatePassword} 0 80u 100% 10u ""
	Pop $EDIT_PASSWORD
	GetFunctionAddress $0 OnPasswordChange
	nsDialogs::OnChange $EDIT_PASSWORD $0
    
 	${NSD_CreatePassword} 0 100u 100% 10u ""
	Pop $EDIT_PASSWORD_RETYPE
	GetFunctionAddress $0 OnPasswordRetypeChange
	nsDialogs::OnChange $EDIT_PASSWORD_RETYPE $0
    
	${NSD_CreateButton} 10u 115u 30% 12u "Create account"
	Pop $BUTTON
	GetFunctionAddress $0 OnAccountCreate
	nsDialogs::OnClick $BUTTON $0
    
	${NSD_CreateLabel} 0 0 100% 50u "To be able to configure dfterm2, you need an administrator account to it. If you are updating from an older version of dfterm2, you can just continue using your old account. In other case, fill these in. You can also add administrator accounts later with the command line tool dfterm2_configure. You can create more than one administrator account if you want. The user account information is stored in current user %APPDATA%\dfterm2 directory."
	Pop $0
    
    nsDialogs::Show

FunctionEnd

Function OnBack

	MessageBox MB_YESNO "Are you sure?" IDYES +2
	Abort

FunctionEnd

Function OnUsernameChange

	Pop $0 # HWND

FunctionEnd

Function OnPasswordChange

	Pop $0 # HWND

FunctionEnd

Function OnPasswordRetypeChange

	Pop $0 # HWND

FunctionEnd


Function OnAccountCreate

	Pop $0 # HWND
    
    System::Call user32::GetWindowText(i$EDIT_USERNAME,t.r0,i${NSIS_MAX_STRLEN})
    ${If} $0 == ""
        MessageBox MB_OK "Fill something in to username."
        Return
    ${EndIf}
    
    System::Call user32::GetWindowText(i$EDIT_PASSWORD,t.r1,i${NSIS_MAX_STRLEN})
    ${If} $1 == ""
        MessageBox MB_OK "Empty passwords are not allowed."
        Return
    ${EndIf}
    
    System::Call user32::GetWindowText(i$EDIT_PASSWORD_RETYPE,t.r2,i${NSIS_MAX_STRLEN})
    ${If} $2 != $1
        MessageBox MB_OK "Passwords don't match each other."
        Return
    ${EndIf}
    
    CreateDirectory "$APPDATA\dfterm2"
    ExecWait "$INSTDIR\dfterm2_configure.exe --database $APPDATA\dfterm2\dfterm2.database --adduser $\"$0$\" $\"$1$\" admin"

FunctionEnd
