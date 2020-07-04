@ECHO OFF

REM
REM Keep our environment variable changes local.
REM
SETLOCAL

REM
REM These commands must be executed in a normal Command Prompt window ^(i.e.
REM not a Visual Studio one^) while in the root CEROD checkout directory.
REM
SET COREONLY=1
SET STATICONLY=
SET INTEROPONLY=
..\..\dotnet\Externals\Eagle\bin\netFramework40\EagleShell.exe -file buildSds.eagle "" "" ReleaseMono

SET COREONLY=
SET STATICONLY=1
SET INTEROPONLY=1
..\..\dotnet\Externals\Eagle\bin\netFramework40\EagleShell.exe -file buildSds.eagle "" "" "" Win32
..\..\dotnet\Externals\Eagle\bin\netFramework40\EagleShell.exe -file buildSds.eagle "" "" "" x64

REM
REM Copy the built System.Data.SQLite binaries to the location where Eagle
REM can easily find them.
REM
FOR %%Y IN (2017 2015 2013 2012 2010 2008 2005) DO (
  IF EXIST ..\..\dotnet\bin\%%Y\ReleaseMonoOnPosix\bin\System.Data.SQLite.dll (
    IF NOT EXIST ..\..\dotnet\Externals\Eagle\bin\netFramework40\System.Data.SQLite.dll (
      COPY /Y ..\..\dotnet\bin\%%Y\ReleaseMonoOnPosix\bin\System.Data.SQLite.dll ..\..\dotnet\Externals\Eagle\bin\netFramework40\
    )
  )

  IF EXIST ..\..\dotnet\bin\%%Y\Win32\ReleaseNativeOnlyStatic\SQLite.Interop.dll (
    IF NOT EXIST ..\..\dotnet\Externals\Eagle\bin\netFramework40\x86\SQLite.Interop.dll (
      COPY /Y ..\..\dotnet\bin\%%Y\Win32\ReleaseNativeOnlyStatic\SQLite.Interop.dll ..\..\dotnet\Externals\Eagle\bin\netFramework40\x86\
    )
  )

  IF EXIST ..\..\dotnet\bin\%%Y\x64\ReleaseNativeOnlyStatic\SQLite.Interop.dll (
    IF NOT EXIST ..\..\dotnet\Externals\Eagle\bin\netFramework40\x64\SQLite.Interop.dll (
      COPY /Y ..\..\dotnet\bin\%%Y\x64\ReleaseNativeOnlyStatic\SQLite.Interop.dll ..\..\dotnet\Externals\Eagle\bin\netFramework40\x64\
    )
  )
)

REM
REM Exit our environment variable scope.
REM
ENDLOCAL
