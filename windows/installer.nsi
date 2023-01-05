;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"

SetCompressor lzma
!define MUI_ICON "logo.ico"
!define MUI_PRODUCT "ALPACA"

;--------------------------------
;General

  Unicode true

  ;Name and file
  Name "${MUI_PRODUCT}"
  OutFile "../${MUI_PRODUCT} Setup.exe"
  BrandingText "Team Sushi"

  ;Default installation folder
  InstallDir "$LOCALAPPDATA\Programs\${MUI_PRODUCT}"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\${MUI_PRODUCT}" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel user

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

  !define MUI_FINISHPAGE_RUN "$INSTDIR\bin\pac.exe"
  !insertmacro MUI_PAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "German"

;--------------------------------
;Reserve Files

  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.

  !insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
;Installer Sections

Section "" SecUninstallPrevious
  Call UninstallPrevious
SectionEnd

Section ""

  Call UninstallPrevious

  SetOutPath "$INSTDIR\bin"

  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libepoxy-0.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libboost_filesystem-mt-x32.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libboost_system-mt-x32.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libboost_thread-mt-x32.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libbz2-1.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libdl.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libfreetype-6.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libgcc_s_dw2-1.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libjpeg-62.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libogg-0.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libvorbis-0.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libvorbisfile-3.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libwebp-7.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/zlib1.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/libpng16-16.dll
  File /usr/i686-w64-mingw32/sys-root/mingw/bin/OpenAL32.dll

  File ..\build-windows\pac.exe

  CreateShortcut "$desktop\${MUI_PRODUCT}.lnk" "$instdir\bin\pac.exe"

  SetOutPath "$INSTDIR\data"
  File /r ..\data\*

  SetOutPath "$INSTDIR"

  ;Store installation folder
  WriteRegStr HKCU "Software\${MUI_PRODUCT}" "" $INSTDIR

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" \
                   "DisplayName" "${MUI_PRODUCT}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" \
                   "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" \
                   "DisplayIcon" "$INSTDIR\bin\pac.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" \
                   "Publisher" "Team Sushi"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" \
                   "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

Function UninstallPrevious

  DetailPrint "Checking"
  ; Check for uninstaller.
  ReadRegStr $R0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}" \
                      "QuietUninstallString"

  ${If} $R0 == ""
    Goto Done
  ${EndIf}

  DetailPrint "Removing previous installation."

  ; Run the uninstaller silently.
  ExecWait $R0

  Done:

FunctionEnd

;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY

FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\Uninstall.exe"
  Delete "$desktop\${MUI_PRODUCT}.lnk"

  RMDir /r "$INSTDIR"

  DeleteRegKey /ifempty HKCU "Software\${MUI_PRODUCT}"
  DeleteRegKey /ifempty HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${MUI_PRODUCT}"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd
