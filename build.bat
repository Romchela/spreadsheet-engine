set MSBuildPath="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
@echo off
cls
%MSBuildPath% "spreadsheet-engine.sln" /p:configuration=release
pause
