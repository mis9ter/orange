@ECHO OFF
REM https://m.blog.naver.com/PostView.nhn?blogId=neimd&logNo=140141019240&proxyReferer=https%3A%2F%2Fwww.google.com%2F
REM https://platformengineer.tistory.com/39
SET SYMBOLS=\\172.29.59.170\SYMBOLS\ORANGE
SET NOOP=NoDebugSymbol

@ECHO ON
pushd .
cd "%~dp0"
SET /p SYMBOLPATH=<"SYMBOLPATH"
if exist "%NOOP%" (
    goto NOOP
) 
if not exist "%SYMBOLPATH%" (
    goto NOOP
)
if "" == "%SYMBOLPATH%" (
    set SYMBOLPATH=%SYMBOLS%
)
if not exist "%NOOP%" (
    cd .\binary\utility
    echo SymStore.exe add /f %1 /s %SYMBOLPATH% /t "orange"
    SymStore.exe add /f %1 /s %SYMBOLPATH% /t "orange"
) 
popd

:NOOP
SET %ERRORLEVEL%=0
EXIT 0