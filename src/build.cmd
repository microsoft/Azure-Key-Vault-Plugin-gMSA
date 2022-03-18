setlocal EnableExtensions EnableDelayedExpansion


cd /D "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\Tools\VsDevCmd.bat"

call msbuild .\CCGAKVPlugin.sln /p:Configuration=Release /t:CCGAKVPlugin /p:Platform=x64 || exit /b 1
