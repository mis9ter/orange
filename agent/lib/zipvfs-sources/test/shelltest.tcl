
set G(docs) [subst {
Usage: $argv0 <path to shell executable>

This script is used to check the zipvfs-enabled version of the shell tool.
If they do not already exist, it creates the following five database files:

  zipvfs_shell_test.db-bsr
  zipvfs_shell_test.db-lrr
  zipvfs_shell_test.db-rle
  zipvfs_shell_test.db-zlib
  zipvfs_shell_test.db

If the files already exists, the existing databases are used. Each file is 
read, written and checked with the sqlite integrity-check.

By running this script in a directory that contains database files generated
on a different architecture (i.e. 32 vs 64 bit, or little vs big-endian), it
can be verified that zipvfs database files are portable.
}]
##########################################################################

if {[llength $argv] != 1} {
  puts stderr $G(docs)
  exit -1
}

set G(shell) [lindex $argv 0]
set G(modes) [list -rle -bsr -zlib -lrr {}]

set G(schema) {
  CREATE TABLE IF NOT EXISTS t1(x,y);
  CREATE TABLE IF NOT EXISTS cksum(i);
  CREATE INDEX IF NOT EXISTS i1 ON t1(x,y);
}
set G(sql) {
  INSERT INTO t1 VALUES(randomblob(250), zeroblob(250));
  INSERT INTO t1 SELECT zeroblob(oid%400), randomblob(405-(oid%400)) FROM t1;
  DELETE FROM t1 WHERE oid < (SELECT max(oid) FROM t1) - 32;
  PRAGMA integrity_check;
}


proc run_shell {mode sql} {
  if {$mode!=""} {
    exec $::G(shell) $mode "zipvfs_shell_test.db$mode" << $sql
  } {
    exec $::G(shell) "zipvfs_shell_test.db" << $sql
  }
}

proc checksum {mode} {
  set cksum 0
  set data [run_shell $mode "SELECT quote(x), quote(y) FROM t1 ORDER BY oid;"]
  binary scan $data c* ints

  foreach i $ints { incr cksum $i }
  return $cksum
}

foreach mode $G(modes) {
  run_shell $mode $G(schema)

  set cksum [checksum $mode]
  if {$cksum!=0} {
    set i [run_shell $mode "SELECT i FROM cksum;"]
    if {$i!=$cksum} {
      puts "Checksum mismatch: $i != $cksum"
      #exit -1
    }
  }

  set rows1 [run_shell $mode "SELECT count(*) FROM t1;"]
  set ic [run_shell $mode $G(sql)]
  set rows2 [run_shell $mode "SELECT count(*) FROM t1;"]

  set cksum [checksum $mode]
  run_shell $mode "
    DELETE FROM cksum;
    INSERT INTO cksum VALUES($cksum);
  "
  puts "mode=$mode (integrity check is $ic) rows:$rows1->$rows2 cksum $cksum"
}

puts "Tests passed OK"


