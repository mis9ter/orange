#!/bin/sh
#
# This shell script demonstrates how to build a command-line shell using
# the zipvfs_create_vfs_v2() interface.  This script runs on ubuntu linux,
# but it should be easy to port to other systems.
#
gcc -g -Werror -Wall $* -o zsqlite3 \
  -DSQLITE_THREADSAFE=0 \
  -DHAVE_USLEEP=1 \
  -DSQLITE_ENABLE_LOCKING_STYLE=0 \
  -DSQLITE_ENABLE_EXPLAIN_COMMENTS \
  -DOS_UNIX=1 \
  -DSQLITE_ENABLE_STMTVTAB \
  -DSQLITE_ENABLE_DBPAGE_VTAB \
  -DSQLITE_ENABLE_DBSTAT_VTAB \
  -DSQLITE_HAVE_ZLIB \
  -DHAVE_EDITLINE=1 \
  -I. \
  sqlite3-zipvfs.c \
  shell.c \
  -lm -lz -ldl -ledit
