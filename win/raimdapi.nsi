; NSIS installer for the Rai market data api, built on Linux with makensis
; (mingw32-nsis) by "make port_extra=-mingw dist_win".  Defines passed in:
;   NAME VERSION VER_BUILD SRC (staged dist dir) OUT (installer path)
Unicode true
!include "MUI2.nsh"
!include "StrFunc.nsh"
${StrStr}
${UnStrRep}

Name "Rai MD API ${VER_BUILD}"
OutFile "${OUT}"
InstallDir "$PROGRAMFILES64\RaiMdApi"
InstallDirRegKey HKLM "Software\RaiTechnology\RaiMdApi" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!define REGUNINST "Software\Microsoft\Windows\CurrentVersion\Uninstall\RaiMdApi"
!define REGENV    "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Rai MD API (programs, dlls, headers, jars, .NET)" SecMain
  SectionIn RO
  SetOutPath "$INSTDIR"
  File /r "${SRC}\*.*"
  WriteRegStr HKLM "Software\RaiTechnology\RaiMdApi" "InstallDir" "$INSTDIR"
  WriteRegStr HKLM "Software\RaiTechnology\RaiMdApi" "Version" "${VER_BUILD}"
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr   HKLM "${REGUNINST}" "DisplayName" "Rai MD API"
  WriteRegStr   HKLM "${REGUNINST}" "DisplayVersion" "${VER_BUILD}"
  WriteRegStr   HKLM "${REGUNINST}" "Publisher" "Rai Technology"
  WriteRegStr   HKLM "${REGUNINST}" "InstallLocation" "$INSTDIR"
  WriteRegStr   HKLM "${REGUNINST}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "${REGUNINST}" "NoModify" 1
  WriteRegDWORD HKLM "${REGUNINST}" "NoRepair" 1
SectionEnd

Section "Add bin directory to the system PATH" SecPath
  ReadRegStr $0 HKLM "${REGENV}" "Path"
  ${StrStr} $1 $0 "$INSTDIR\bin"
  StrCmp $1 "" 0 +3
    WriteRegExpandStr HKLM "${REGENV}" "Path" "$0;$INSTDIR\bin"
    SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
SectionEnd

LangString DESC_SecMain ${LANG_ENGLISH} "raisub2/raipub2/raiping2/raireplay2 (C++ and .NET), raimdapi.dll + import library, headers, java jars and JNI dlls.  The .NET programs need the .NET 9 runtime, the java ones a JRE 21."
LangString DESC_SecPath ${LANG_ENGLISH} "Append $INSTDIR\bin to the machine PATH so the programs and raimdapi.dll are found from any shell."
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecPath} $(DESC_SecPath)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
  ReadRegStr $0 HKLM "${REGENV}" "Path"
  ${UnStrRep} $1 $0 ";$INSTDIR\bin" ""
  StrCmp $0 $1 +3 0
    WriteRegExpandStr HKLM "${REGENV}" "Path" $1
    SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\lib"
  RMDir /r "$INSTDIR\include"
  Delete "$INSTDIR\README.md"
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "${REGUNINST}"
  DeleteRegKey HKLM "Software\RaiTechnology\RaiMdApi"
SectionEnd
