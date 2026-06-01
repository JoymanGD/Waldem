@echo off
setlocal

set ROOT_DIR=%~dp0
set PHYSX_ROOT=%ROOT_DIR%Vendor\PhysX\physx
set PHYSX_LIB_DIR=%PHYSX_ROOT%\bin\win.x86_64.vc143.md\release
set PHYSX_PRESET=vc17win64-waldem-md-cpu-only
set PHYSX_PRESET_FILE=%PHYSX_ROOT%\buildtools\presets\%PHYSX_PRESET%.xml

git submodule update --init --recursive Vendor/PhysX
if errorlevel 1 exit /b %errorlevel%

if not exist "%PHYSX_LIB_DIR%\PhysX_64.lib" (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "$preset = [xml](Get-Content '%PHYSX_ROOT%\buildtools\presets\public\vc17win64-cpu-only.xml'); $preset.preset.name = '%PHYSX_PRESET%'; foreach ($switch in $preset.preset.CMakeSwitches.cmakeSwitch) { if ($switch.name -eq 'NV_USE_STATIC_WINCRT') { $switch.value = 'False' } }; $preset.preset.CMakeParams.cmakeParam.value = 'install/%PHYSX_PRESET%/PhysX'; $preset.Save('%PHYSX_PRESET_FILE%')"
    if errorlevel 1 exit /b %errorlevel%
    pushd "%PHYSX_ROOT%"
    call generate_projects.bat %PHYSX_PRESET%
    if errorlevel 1 exit /b %errorlevel%
    cmake --build compiler\%PHYSX_PRESET% --config release --target PhysX PhysXCommon PhysXFoundation PhysXCooking PhysXExtensions PhysXPvdSDK
    if errorlevel 1 exit /b %errorlevel%
    popd
)

call Vendor\premake\premake5.exe vs2022
