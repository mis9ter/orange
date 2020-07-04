
setlocal

if not defined ZLIBSRCDIR (
  if exist "zlibSrcDir.bat" (
    call "zlibSrcDir.bat"

    if errorlevel 1 (
      echo Calling "zlibSrcDir.bat" failed.
      goto :EOF
    )
  )

  if not defined ZLIBSRCDIR (
    ECHO The ZLIBSRCDIR environment variable must be set first.
    goto :EOF
  )
)

call GetZlib.bat
if errorlevel 1 goto :EOF

pushd "%ZLIBSRCDIR%"
nmake -f win32/Makefile.msc clean
nmake -f win32/Makefile.msc zlib.lib
popd

rmdir /S /Q bld
mkdir bld

copy sqlite3.c bld
copy sqlite3.h bld
copy zipvfs.c bld
copy zipvfs.h bld
copy zipvfs_vtab.c bld
copy algorithms.c bld
copy shell.c bld

pushd bld
copy /B sqlite3.c + /B zipvfs.c + /B zipvfs_vtab.c zsqlite3.c /B

set CC=cl.exe -MD -Zi -Os -W4 -wd4210
set CC=%CC% -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS
set CC=%CC% -DSQLITE_THREADSAFE=0 -DSQLITE_ENABLE_ZIPVFS -DSQLITE_SMALL_STACK
set CC=%CC% -DSQLITE_ENABLE_ZIPVFS -DSQLITE_ENABLE_ZLIB_FUNCS
set CC=%CC% -DSQLITE_ENABLE_ZIPVFS_VTAB
set CC=%CC% -I. "-I%ZLIBSRCDIR%"
set LD=link.exe "/LIBPATH:%ZLIBSRCDIR%"

%CC% -Fozsqlite3.obj -c zsqlite3.c
%CC% -Foalgorithms.obj -c algorithms.c
%CC% -Foshell.obj -c shell.c

%LD% /OUT:..\zsqlite3.exe zsqlite3.obj algorithms.obj shell.obj zlib.lib

popd
dir zsqlite3.exe

endlocal
