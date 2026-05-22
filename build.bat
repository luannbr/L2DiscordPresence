@echo off
setlocal

cd /d "%~dp0"

set "CONFIG=%~dp0presence.ini"
set "MAIN=%~dp0L2DiscordPresence\main.cpp"
set "SLN=%~dp0L2DiscordPresence.sln"
set "PATCH=%~dp0patch.ps1"
set "OUTDIR=%~dp0bin\VS_L2DiscordPresence_Release-Win32"

if not exist "%CONFIG%" (
    echo [ERRO] presence.ini nao encontrado: %CONFIG%
    exit /b 1
)
if not exist "%MAIN%" (
    echo [ERRO] main.cpp nao encontrado: %MAIN%
    exit /b 1
)
if not exist "%PATCH%" (
    echo [ERRO] patch.ps1 nao encontrado: %PATCH%
    exit /b 1
)

echo [1/2] Aplicando presence.ini em main.cpp...
powershell -NoProfile -ExecutionPolicy Bypass -File "%PATCH%" -Config "%CONFIG%" -Main "%MAIN%"
if errorlevel 1 (
    echo [ERRO] falha ao aplicar patch
    exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERRO] vswhere nao encontrado. Instale o Visual Studio 2022.
    exit /b 1
)

set "MSBPATHFILE=%TEMP%\_l2dp_msbuild.txt"
"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" > "%MSBPATHFILE%"
set /p MSBUILD=<"%MSBPATHFILE%"
del "%MSBPATHFILE%" >nul 2>&1

if not exist "%MSBUILD%" (
    echo [ERRO] MSBuild nao encontrado.
    exit /b 1
)

echo [2/2] Compilando Release^|x86...
"%MSBUILD%" "%SLN%" /t:Rebuild /p:Configuration=Release /p:Platform=x86 /m /nologo /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [ERRO] build falhou
    exit /b 1
)

echo.
echo [OK] DLL gerada:
echo   %OUTDIR%\L2DiscordPresence.dll
echo.
endlocal
