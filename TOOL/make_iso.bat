@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "REPO=%ROOT%.."
set "ISO_DIR=%REPO%\iso"
set "BUILD_DIR=%REPO%\build"
set "OUTPUT=%BUILD_DIR%\snu.iso"
set "EFI_FILE=%ISO_DIR%\EFI\BOOT\BOOTX64.EFI"

if not exist "%EFI_FILE%" (
  echo ERROR: Missing EFI boot file: %EFI_FILE%
  exit /b 1
)

set "TOOL="
for %%T in (xorriso.exe genisoimage.exe mkisofs.exe) do (
  if not defined TOOL (
    where /Q %%~T
    if !ERRORLEVEL! EQU 0 set "TOOL=%%~T"
  )
)

if defined TOOL (
  echo Using ISO tool: %TOOL%
  pushd "%ISO_DIR%"
  if /I "%~1"=="--debug" echo Creating hybrid ISO with %TOOL%...
  if /I "%TOOL:~-9%"=="xorriso.exe" (
    %TOOL% -as mkisofs -r -J -joliet-long -o "%OUTPUT%" -V SNU -c boot.catalog -b snu.img -no-emul-boot -boot-load-size 4 -boot-info-table -eltorito-alt-boot -e EFI/BOOT/BOOTX64.EFI -no-emul-boot "%ISO_DIR%"
  ) else (
    %TOOL% -o "%OUTPUT%" -V SNU -r -J -joliet-long -c boot.catalog -b snu.img -no-emul-boot -boot-load-size 4 -boot-info-table -eltorito-alt-boot -e EFI/BOOT/BOOTX64.EFI -no-emul-boot "%ISO_DIR%"
  )
  popd
  if errorlevel 1 (
    echo ERROR: ISO creation with %TOOL% failed.
    exit /b 1
  )
  echo Created ISO: %OUTPUT%
  exit /b 0
)

if defined PYTHONHOME (
  set "PY=%PYTHONHOME%\python.exe"
) else (
  set "PY=python.exe"
)

where /Q %PY%
if errorlevel 1 (
  echo ERROR: No ISO tool found and Python is not available.
  exit /b 1
)

echo Falling back to Python ISO creation.
"%PY%" -u "%ROOT%\make_iso.py"
exit /b %ERRORLEVEL%
