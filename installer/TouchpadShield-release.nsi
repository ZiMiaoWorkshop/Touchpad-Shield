; Touchpad Shield release installer skeleton



!include "MUI2.nsh"

!include "common.nsh"



!define APP_NAME "Touchpad Shield"

!define APP_PUBLISHER "ZiMiaoWorkshop"

!define APP_SEMVER "1.1.0"

!define APP_BUILD "0098"

!define APP_VERSION "1.1.0 build 0098"

!define MUI_ICON "TouchpadShield.ico"

!define MUI_UNICON "TouchpadShield.ico"

!define MUI_WELCOMEFINISHPAGE_BITMAP "welcome-finish.bmp"

!define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH

!define APP_EXE "TouchpadShield.exe"



Name "${APP_NAME}"

OutFile "..\Touchpad Shield App\release\TouchpadShield-${APP_SEMVER}-build${APP_BUILD}-setup.exe"

InstallDir "$PROGRAMFILES64\${APP_NAME}"

RequestExecutionLevel admin



!insertmacro MUI_PAGE_WELCOME

!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_PAGE_FINISH



!insertmacro MUI_LANGUAGE "SimpChinese"



Section "Install"

  SetOutPath "$INSTDIR"

  File /r "..\Touchpad Shield App\release\app\*.*"

  !insertmacro CreateAppShortcuts

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro RegisterUninstallEntry

SectionEnd



Section "Uninstall"

  !insertmacro RemoveAppShortcuts

  !insertmacro UnregisterUninstallEntry

  Delete "$INSTDIR\Uninstall.exe"

  RMDir /r "$INSTDIR"

SectionEnd

