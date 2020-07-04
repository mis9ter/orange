#!/bin/tclsh
#
# Run this script to build a ZIPVFS-capable "sqlite3.c" file that can be
# dropping into an existing application in place of public-domain sqlite3.c
# to create a ZIPVFS-capable application.
#
proc readFile {name} {
  set fd [open $name rb]
  set x [read $fd]
  close $fd
  return $x
}
set out [open sqlite3-zipvfs.c wb]
set copyright \
{/******************************************************************************
** Copyright (c) 2010-2016 Hipp, Wyrick & Company, Inc                       **
** 6200 Maple Cove Lane, Charlotte, NC 28269                                 **
** +1.704.948.4565                                                           **
**                                                                           **
** All Right Reserved                                                        **
******************************************************************************/}
set x [readFile sqlite3.c]
set i [string first {#define} $x]
puts $out [string trim [string range $x 0 [expr {$i-1}]]]
puts $out $copyright
puts $out "#define SQLITE_EXTRA_INIT zipvfsExtraInit"
puts $out "#define SQLITE_ENABLE_ZIPVFS 1"
puts $out "#define SQLITE_ENABLE_ZIPVFS_VTAB 1"
puts $out "#define SQLITE_ENABLE_ZLIB_FUNCS 1"
puts $out "#define SQLITE_SECURE_DELETE 1"
puts $out [string range $x $i end]
set x {}
puts $out "/************** Begin file zipvfs.c ******************************************/"
puts $out [readFile zipvfs.c]
puts $out "/************** End file zipvfs.c ********************************************/"
puts $out $copyright
puts $out "/************** Begin file zipvfs_vtab.c *************************************/"
puts $out [readFile zipvfs_vtab.c]
puts $out "/************** End file zipvfs_vtab.c ***************************************/"
puts $out $copyright
puts $out "/************** Begin file algorithms.c **************************************/"
puts $out [readFile algorithms.c]
puts $out "/************** End file algorithms.c ****************************************/"
puts $out {int zipvfsExtraInit(const char *notUsed){
  zipvfsInit_v3(1);
  sqlite3_auto_extension((void(*)(void))zipvfs_sqlcmd_autoinit);
  return 0;
}}
puts $out $copyright
close $out
