setlocal EnableExtensions EnableDelayedExpansion
ECHO Running %~f0 
pushd "%~dp0"

SET "vcpkgDir=%~dp0%..\vcpkg"
ECHO Using "%vcpkgDir%" as vcpkg destination directory

IF "%~1"=="boost-log" SET "InstallBoostLog=y"

ECHO "Running nuget restore"
CALL nuget restore %~dp0DND-FleetAgent.sln

REM validate vcpkg does not already exist (no error)
IF EXIST %vcpkgDir%\ ( 
	ECHO vcpkg already installed	
) ELSE (
	ECHO Installing vcpkg
	CALL git clone https://github.com/microsoft/vcpkg.git %vcpkgDir% --single-branch -b 2021.05.12
	CALL %vcpkgDir%\bootstrap-vcpkg.bat -disableMetrics
)

ECHO Installing vcpkg user-wide integration
CALL %vcpkgDir%\vcpkg integrate install

IF EXIST %vcpkgDir%\packages\cpprestsdk\ (
	ECHO boost-test already installed	
) ELSE (
	ECHO Installing Boost Test packages
	CALL %vcpkgDir%\vcpkg install cpprestsdk:x64-windows-static
)

ECHO Rebuilding all outdated packages
CALL %vcpkgDir%\vcpkg upgrade

cd /D "%~dp0"

nuget restore . || exit /b 1

:exit_clean
ECHO Exiting %~f0 
popd
endlocal & exit /B 0

:exit_error
ECHO Exiting %~f0 
popd
endlocal & exit /B %errorlevel%

