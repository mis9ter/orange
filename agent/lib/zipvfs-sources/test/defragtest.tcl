
set G(docs) [subst {
Usage: $argv0 <path to shell executable> <path to defrag executable>

}]
##########################################################################

if {[llength $argv] != 2} {
  puts stderr $G(docs)
  exit -1
}

set G(shell)  [lindex $argv 0]
set G(defrag) [lindex $argv 1]
set G(modes) [list -rle -bsr -zlib -lrr {}]

set G(schema) {
  CREATE TABLE IF NOT EXISTS t1(x, y);
}

proc run_shell {file sql} {
  exec $::G(shell) -zlib $file << $sql
}

proc run_defrag {file1 file2} {
  puts -nonewline "defragmenting...."
  flush stdout
  exec $::G(defrag) $file1 $file2 
  puts " done"
}

proc checksum {file} {
  #set cksum 0
  #set data [run_shell $file "SELECT quote(x), quote(y) FROM t1 ORDER BY oid;"]
  #binary scan $data c* ints
  #foreach i $ints { incr cksum $i }
  #return $cksum

  return [string map {"\n" " "} [
    run_shell $file "SELECT count(*) FROM t1 ; PRAGMA integrity_check;"
  ]]
}

file delete -force test.db
file delete -force test.db2

puts -nonewline "building db"
flush stdout
run_shell test.db $G(schema)
run_shell test.db {
  INSERT INTO t1 VALUES(randomblob(500), randomblob(400));
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   --  2
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   --  4
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   --  8
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   -- 16
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   -- 32
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   -- 64
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   -- 128
  INSERT INTO t1 SELECT randomblob(500), randomblob(400) FROM t1;   -- 256
}

for {set i 0} {$i < 13} {incr i} {
  run_shell test.db { INSERT INTO t1 SELECT * FROM t1; }
  puts -nonewline .
  flush stdout
}
puts " done"

set cksum1 [checksum test.db]
run_defrag test.db test.db2
set cksum2 [checksum test.db2]

if {$cksum1==$cksum2} {
  set res "ok"
} else {
  set res "FAILED"
}
puts "cksum1=$cksum1 cksum2=$cksum2 $res"

if {[info commands zip_register] != ""} {
  zip_register "" 0 zlib
  sqlite3 db test.db2 -vfs zlib

  puts -nonewline "Zipvfs integrity check..."
  flush stdout

  set rc [catch { zip_control db main integrity_check }]
  set res ok
  if {$rc} { set res FAILED }
  puts $res

  db close
  zip_unregister
} else {
  puts "If you run this test using a zipvfs testfixture, it checks the "
  puts "integrity of the output database using zipvfs integrity check too."
}

