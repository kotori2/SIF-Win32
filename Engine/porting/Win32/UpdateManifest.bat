@echo off
rem Because VS2010 RTM cannot recognize the "compatibility" tag,
rem this script need to be executed after building with VS2012 command prompt or VS2010 SP1

if "%VSINSTALLDIR%" == "" (
	echo Run from VS2012 command prompt or VS2010 SP1
	exit /b 1
)

if exist "Output/Debug/SampleProject.exe" (
	echo Updating debug build manifest
	mt -nologo -manifest SampleProject.manifest -outputresource:Output/Debug/SampleProject.exe
)
if exist "Output/Release/SampleProject_Release.exe" (
	echo Updating release build manifest
	mt -nologo -manifest SampleProject.manifest -outputresource:Output/Release/SampleProject_Release.exe
)

echo Done