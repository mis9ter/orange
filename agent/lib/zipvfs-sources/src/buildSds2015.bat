@ECHO OFF

REM
REM Keep our environment variable changes local.
REM
SETLOCAL

REM
REM These commands must be executed in a normal Command Prompt window ^(i.e.
REM not a Visual Studio one^) while in the root CEROD checkout directory.
REM
SET NETFX46ONLY=1
SET COREONLY=1
SET STATICONLY=
SET INTEROPONLY=
..\..\dotnet\Externals\Eagle\bin\netFramework40\EagleShell.exe -file buildSds.eagle "" "" "" msil 2015

SET VisualStudioVersion=14.0
SET COREONLY=
SET STATICONLY=1
SET INTEROPONLY=1
..\..\dotnet\Externals\Eagle\bin\netFramework40\EagleShell.exe -file buildSds.eagle "" "" "" Win32 2015
..\..\dotnet\Externals\Eagle\bin\netFramework40\EagleShell.exe -file buildSds.eagle "" "" "" x64 2015

REM
REM Copy the built System.Data.SQLite binaries to the location where Eagle
REM can easily find them.
REM
COPY /Y ..\..\dotnet\bin\2015\Release\bin\System.Data.SQLite.dll ..\..\dotnet\Externals\Eagle\bin\netFramework40\
COPY /Y ..\..\dotnet\bin\2015\Win32\ReleaseNativeOnlyStatic\SQLite.Interop.dll ..\..\dotnet\Externals\Eagle\bin\netFramework40\x86\
COPY /Y ..\..\dotnet\bin\2015\x64\ReleaseNativeOnlyStatic\SQLite.Interop.dll ..\..\dotnet\Externals\Eagle\bin\netFramework40\x64\

REM
REM Exit our environment variable scope.
REM
ENDLOCAL
