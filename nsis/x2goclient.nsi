;NSIS Modern User Interface
;Multilingual Example Script
;Written by Joost Verburg


!include "MUI.nsh"
!include "FileFunc.nsh"
!insertmacro Locate
;--------------------------------
;General

  RequestExecutionLevel admin
 !define VERSION "4.0.1.0-pre02"
  Name "x2goclient-${VERSION}"
  Caption "x2goclient-${VERSION}"
  OutFile "x2goclient-${VERSION}-setup.exe"
  !define MUI_ICON icons\win-install.ico
  !define MUI_UNICON icons\win-uninstall.ico
  ;Default installation folder
  InstallDir "$PROGRAMFILES\x2goclient"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\x2goclient" ""

;--------------------------------
;Check if user is admin
Section
    userInfo::getAccountType
    pop $0
    strCmp $0 "Admin" +3
    messageBox MB_OK $(ADM_RIGHT)
    abort
SectionEnd

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Language Selection Dialog Settings

  ;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "HKLM" 
  !define MUI_LANGDLL_REGISTRY_KEY "Software\x2goclient" 
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;------------------------------
;Variables

  Var STARTMENU_FOLDER
  Var MUI_TEMP

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "gpl.txt"
  !insertmacro MUI_PAGE_DIRECTORY
  !define MUI_STARTMENUPAGE_DEFAULTFOLDER "X2Go Client for Windows"
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\x2goclient"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"  
  !insertmacro MUI_PAGE_STARTMENU "Application" $STARTMENU_FOLDER
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English" 
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Russian"
  
;--------------------------------
;Reserve Files
  
  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.
  
  !insertmacro MUI_RESERVEFILE_LANGDLL


;--------------------------------
;Installer Sections
Section "x2goclient" Section1

  SetShellVarContext all 
  SectionIn RO

  SetOutPath "$INSTDIR"
  File /a "x2goclient\*.*"
  File /r "x2goclient\pulse"
  File /r "x2goclient\VcXsrv"
  
  ;Store installation folder
  WriteRegStr HKLM "Software\x2goclient" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$INSTDIR\X2goClient.lnk" "$INSTDIR\x2goclient.exe"
  CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\X2goClient.lnk" "$INSTDIR\x2goclient.exe"
  CreateShortCut "$DESKTOP\X2goClient.lnk" "$INSTDIR\x2goclient.exe"
  CreateShortCut "$DESKTOP\X2goClient.lnk" "$INSTDIR\x2goclient.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

;-------------------------------------------
;Descriptions


LangString ADM_RIGHT ${LANG_ENGLISH} "You have to be Administrator on this computer to install X2Go Client"
LangString ADM_RIGHT ${LANG_GERMAN} "Sie brauchen Administratorenrechte um X2go Client zu installieren"
LangString ADM_RIGHT ${LANG_RUSSIAN} "��� ����, ����� ���������� X2Go Client, ���������� ���� ��������������� ����� ����������"


!Insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!Insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  SetShellVarContext all 

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\X2goClient.lnk"
  Delete "$DESKTOP\X2goClient.lnk"
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"
  startMenuDeleteLoop:
	ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."
    IfErrors startMenuDeleteLoopDone
    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  RMDir /r "$INSTDIR"

  DeleteRegKey  HKLM "Software\x2goclient"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE
  
FunctionEnd
