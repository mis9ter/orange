# How To Test ZIPVFS

## You will need

  1.  The ZIPVFS source tree (in which this README.md is located)
  2.  The SQLite source tree in a separate directory.

Let the root of the ZIPVFS source tree be called $ZIPVFS.  And let
the root of the SQLite source tree be $SQLITE.

## Procedure:

  1.  Generate the canonical "sqlite3.c" source file

~~~~
      cd $SQLITE
      ./configure
      make sqlite3.c
~~~~

  2.   Modify the "sqlite3.c" source file to include ZIPVFS

~~~~
      cat sqlite3.c >tmp.c  
      cat $ZIPVFS/src/zipvfs.c >>tmp.c  
      echo '#ifdef SQLITE_TEST' >>tmp.c  
      cat $ZIPVFS/src/libzipvfs.c >>tmp.c  
      echo '#endif' >>tmp.c  
      mv tmp.c sqlite3.c  
~~~~

  3.  Copy in additional files needed

~~~~
      cp $ZIPVFS/src/zipvfs.h .  
      cp $ZIPVFS/src/algorithms.c .  
~~~~

  4.  Set up the environment:

~~~~
      export SQLITE_TEST_DIR=$SQLITE/test  
~~~~

  5.  Run ZIPVFS-specific tests:

~~~~
      CFLAGS='-Os -DSQLITE_ENABLE_ZIPVFS' make -e clean testfixture  
      ./testfixture $ZIPVFS/test/zipvfs.test  
~~~~

  6.  Run generic SQLite tests:

~~~~
      CFLAGS='-Os -DSQLITE_ENABLE_ZIPVFS' make -e test  
~~~~
