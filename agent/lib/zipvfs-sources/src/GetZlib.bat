@ECHO OFF

::
:: GetZlib.bat --
::
:: zlib Download Tool
::

SETLOCAL

REM SET __ECHO=ECHO
REM SET __ECHO2=ECHO
REM SET __ECHO3=ECHO
IF NOT DEFINED _AECHO (SET _AECHO=REM)
IF NOT DEFINED _CECHO (SET _CECHO=REM)
IF NOT DEFINED _VECHO (SET _VECHO=REM)

SET DUMMY1=%1

IF DEFINED DUMMY1 (
  GOTO usage
)

SET ROOT=%~dp0\..
SET ROOT=%ROOT:\\=\%

%_VECHO% Root = '%ROOT%'

SET TOOLS=%~dp0
SET TOOLS=%TOOLS:~0,-1%

%_VECHO% Tools = '%TOOLS%'

IF NOT DEFINED windir (
  ECHO The windir environment variable must be set first.
  GOTO errors
)

%_VECHO% WinDir = '%windir%'

IF NOT DEFINED TEMP (
  ECHO The TEMP environment variable must be set first.
  GOTO errors
)

%_VECHO% Temp = '%TEMP%'

IF NOT DEFINED ZLIBSRCDIR (
  IF EXIST "%TOOLS%\zlibSrcDir.bat" (
    CALL "%TOOLS%\zlibSrcDir.bat"

    IF ERRORLEVEL 1 (
      ECHO Calling "%TOOLS%\zlibSrcDir.bat" failed.
      GOTO errors
    )
  )

  IF NOT DEFINED ZLIBSRCDIR (
    ECHO The ZLIBSRCDIR environment variable must be set first.
    GOTO errors
  )
)

%_VECHO% ZlibSrcDir = '%ZLIBSRCDIR%'

IF EXIST "%ZLIBSRCDIR%" (
  ECHO The directory "%ZLIBSRCDIR%" already exists, skipping...
  GOTO no_errors
)

IF NOT DEFINED UNZIP_URI (
  SET UNZIP_URI=https://urn.to/r/unzip
)

%_VECHO% UnZipUri = '%UNZIP_URI%'

IF NOT DEFINED ZLIB_URI (
  SET ZLIB_URI=https://urn.to/r/zlib
)

%_VECHO% ZlibUri = '%ZLIB_URI%'

CALL :fn_ResetErrorLevel

FOR %%T IN (csc.exe) DO (
  SET %%T_PATH=%%~dp$PATH:T
)

%_VECHO% Csc.exe_PATH = '%csc.exe_PATH%'

IF DEFINED csc.exe_PATH (
  GOTO skip_addToPath
)

IF DEFINED FRAMEWORKDIR (
  REM Use the existing .NET Framework directory...
) ELSE IF EXIST "%windir%\Microsoft.NET\Framework64\v2.0.50727" (
  SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework64\v2.0.50727
) ELSE IF EXIST "%windir%\Microsoft.NET\Framework64\v3.5" (
  SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework64\v3.5
) ELSE IF EXIST "%windir%\Microsoft.NET\Framework64\v4.0.30319" (
  SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework64\v4.0.30319
) ELSE IF EXIST "%windir%\Microsoft.NET\Framework\v2.0.50727" (
  SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework\v2.0.50727
) ELSE IF EXIST "%windir%\Microsoft.NET\Framework\v3.5" (
  SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework\v3.5
) ELSE IF EXIST "%windir%\Microsoft.NET\Framework\v4.0.30319" (
  SET FRAMEWORKDIR=%windir%\Microsoft.NET\Framework\v4.0.30319
) ELSE (
  ECHO No suitable version of the .NET Framework appears to be installed.
  GOTO errors
)

%_VECHO% FrameworkDir = '%FRAMEWORKDIR%'

IF NOT EXIST "%FRAMEWORKDIR%\csc.exe" (
  ECHO The file "%FRAMEWORKDIR%\csc.exe" is missing.
  GOTO errors
)

CALL :fn_PrependToPath FRAMEWORKDIR

:skip_addToPath

IF NOT EXIST "%TEMP%\GetFile.exe" (
  %__ECHO% csc.exe "/out:%TEMP%\GetFile.exe" /target:exe "%TOOLS%\GetFile.cs"

  IF ERRORLEVEL 1 (
    ECHO Compilation of "%TOOLS%\GetFile.cs" failed.
    GOTO errors
  )
)

IF NOT EXIST "%TEMP%\unzip.exe" (
  %__ECHO% "%TEMP%\GetFile.exe" "%UNZIP_URI%" unzip.exe

  IF ERRORLEVEL 1 (
    ECHO Download of "%UNZIP_URI%" failed.
    GOTO errors
  )
)

IF NOT EXIST "%TEMP%\zlib.zip" (
  %__ECHO% "%TEMP%\GetFile.exe" "%ZLIB_URI%" zlib.zip

  IF ERRORLEVEL 1 (
    ECHO Download of "%ZLIB_URI%" failed.
    GOTO errors
  )
)

%__ECHO% "%TEMP%\unzip.exe" -n "%TEMP%\zlib.zip" -d "%ZLIBSRCDIR%"

IF ERRORLEVEL 1 (
  ECHO Could not unzip "%TEMP%\zlib.zip" to "%ZLIBSRCDIR%".
  GOTO errors
)

GOTO no_errors

:fn_PrependToPath
  IF NOT DEFINED %1 GOTO :EOF
  SETLOCAL
  SET __ECHO_CMD=ECHO %%%1%%
  FOR /F "delims=" %%V IN ('%__ECHO_CMD%') DO (
    SET VALUE=%%V
  )
  SET VALUE=%VALUE:"=%
  REM "
  ENDLOCAL && SET PATH=%VALUE%;%PATH%
  GOTO :EOF

:fn_ResetErrorLevel
  VERIFY > NUL
  GOTO :EOF

:fn_SetErrorLevel
  VERIFY MAYBE 2> NUL
  GOTO :EOF

:usage
  ECHO.
  ECHO Usage: %~nx0
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%
