#!/bin/sh
#
# With the latest public-domain SQLite checked out in a sibling directory
# "../../sqlite", and after having run "configure; make sqlite3.c" in that
# sibling directory, run this script to update the build-in SQLite source
# code.
#
cp ../../sqlite/sqlite3.[ch] .
cp ../../sqlite/src/sqlite3ext.h .
fossil 3-way-merge baseline/shell.c shell.c ../../sqlite/shell.c shell.c
cp ../../sqlite/shell.c baseline
tclsh mksqlite3zipvfsc.tcl
