

# Page size used for database. And the root directory of a hierachy of 
# text files used as a data source.
#
set G(page_size) 8192
set G(maildir) /home/dan/tmp/enron/maildir 

# Number of rows added to db by the "build" command.
#
set G(dbrows)     1000000

# Number of rows updated/inserted by the "update" and "insert" commands.
#
set G(updaterows)   10000
set G(insertrows)   10000

# End of configurable parameters.
##########################################################################

# Schema for test databases.
#
set G(test_schema_1) {
  CREATE TABLE data(file, line, text);
}
set G(test_schema_2) {
  CREATE INDEX i1 ON data(file, line);
  CREATE INDEX i2 ON data(line, file);
}

proc usage {msg nArg} {
  set preamble [concat $::argv0 [lrange $::argv 0 [expr $nArg-1]]]
  puts stderr "Usage: $preamble $msg"
  exit -1
}

proc foreach_file {varname root script {nested 0}} {
  foreach f [lsort [glob -nocomplain -directory $root *]] {
    if {[file isdir $f]} {
      set rc [catch [list \
          uplevel [list foreach_file $varname $f $script 1]
      ] msg]
    } else {
      uplevel [list set $varname $f]
      set rc [catch [list uplevel $script] msg]
    }

    if {$rc == 1} { error $msg }
    if {$rc == 3} {
      if {$nested} {return -code $rc}
      break
    }
  }
}

proc foreach_line {fvar lvar tvar root args} {
  if {[llength $args]==1} {
    set script [lindex $args 0]
    set skip 0
  } elseif {[llength $args]==3 && [lindex $args 0]=="-skip"} {
    set skip [lindex $args 1]
    set script [lindex $args 2]
  }

  foreach_file f $root {
    if {$skip>0} { incr skip -1 ; continue }

    set fd [open $f]
    set l 1
    while {![eof $fd]} {
      uplevel [list set $fvar $f]
      uplevel [list set $lvar $l]
      uplevel [list set $tvar [gets $fd]]
      incr l

      set rc [catch [list uplevel $script] msg]
      if {$rc != 0 && $rc != 4} break
    }
    close $fd

    switch -- $rc {
      0 {}
      1 { error $msg } 
      2 { return -code return }
      3 { break }
    }
  }
}

proc count_files {db} {
  set n 0
  set f ""
  $db eval {SELECT file FROM data ORDER BY file} {
    if {$file == $f} continue
    set f $file
    incr n
  }
  
  return $n
}

zip_register 0
if {[llength $argv]<1} { usage "SUB-PROGRAM ?ARGS?" 1 }

set command(build) {

  # Process command line arguments.
  #
  set nArg [llength $argv]
  if {$nArg!=2 && $nArg!=3} { usage "DB-NAME ?ROWS?" 2 }
  set dbname [lindex $argv 1]
  set nRow $::G(dbrows)
  if {$nArg==3} { set nRow [lindex $argv 2] }

  puts "Building $nRow row db (page size = $G(page_size))"
  puts "Database name is $dbname"

  file delete -force $dbname
  sqlite3 db $dbname
  db eval "PRAGMA page_size = $::G(page_size)"
  db eval "PRAGMA cache_size = 100000"
  db eval $::G(test_schema_1)
  db eval BEGIN

  set nDot [expr $nRow / 50]

  set n 0
  foreach_line f l t $G(maildir) {
    db eval {INSERT INTO data VALUES($f, $l, $t)}
    incr n
    if {$n >= $nRow} break
    if {($n % $nDot)==0} { puts -nonewline . ; flush stdout }
  }

  puts ""
  puts "Building indexes..."
  db eval $::G(test_schema_2)

  db eval COMMIT
  puts "$n rows. [db one {PRAGMA page_count}] pages."
}

set command(compress) {
  if {[llength $argv]!=2} { usage "DB-NAME" 2 }
  set input [lindex $argv 1]
  set output "${input}.zdb"
  puts "Creating zipdb $output..."

  file delete -force $output
  sqlite3 db $input
  sqlite3 db2 $output -vfs zip
  open_write_log db2

  db eval {SELECT * FROM sqlite_master}

  db2 eval "PRAGMA page_size = [db eval {PRAGMA page_size}]"
  db2 eval "PRAGMA user_version = 10"

  sqlite3_backup B db2 main db main
  while {[B step 1000] != "SQLITE_DONE"} {
    puts -nonewline .
    flush stdout
  }
  B finish
}

set command(uncompress) {
  if {[llength $argv]!=2} { usage "DB-NAME" 2 }
  set input [lindex $argv 1]
  set output "${input}.db"
  puts "Creating uncompressed $output..."

  file delete -force $output
  sqlite3 db $input -vfs zip
  sqlite3 db2 $output

  db eval {SELECT * FROM sqlite_master}

  db2 eval "PRAGMA page_size = [db eval {PRAGMA page_size}]"
  db2 eval "PRAGMA user_version = 10"

  sqlite3_backup B db2 main db main
  while {[B step 100000000] != "SQLITE_DONE"} {
    puts -nonewline .
    flush stdout
  }
  B finish
}

set command(stats) {
  if {[llength $argv]!=2} { usage "DB-NAME" 2 }
  set dbname [lindex $argv 1]
  sqlite3 db $dbname -vfs zip
  set freeslots [zip_control db main free_slots]
  set freebytes [zip_control db main free_bytes]
  set fragbytes [zip_control db main fragmented_bytes]
  set filesize [file size $dbname]
  set origsize [expr [db one {PRAGMA page_count}]*[db one {PRAGMA page_size}]]
  set percent [format %.2f [expr {100.0*($freebytes+$fragbytes)/$filesize}]]
  set percent2 [format %.2f [expr {100.0*($freebytes+$fragbytes)/$origsize}]]

  puts "Free slots: $freeslots"
  puts "Free bytes: $freebytes"
  puts "Frag bytes: $fragbytes"
  puts "File size:  $filesize"
  puts "Orig size:  $origsize"
  puts "Percentage overhead: $percent% ($percent2%)"
  puts "Compression ratio: [format %.2f [expr {100.0*$filesize/$origsize}]]%"
}

set command(update) {
  set nArg [llength $argv]
  if {$nArg!=2 && $nArg!=3} { usage "DB-NAME ?ROWS?" 2 }
  set dbname [lindex $argv 1]
  set nRow $::G(updaterows)
  if {$nArg==3} { set nRow [lindex $argv 2] }

  puts "Updating $nRow arbitrarily selected rows"

  if {[string match *zdb $dbname]} {
    sqlite3 db $dbname -vfs zip
    open_write_log db
  } else {
    sqlite3 db $dbname
  }
  set skip [count_files db]
  puts "skipping $skip files"
  set n 0
  set nMax [db one {SELECT max(oid) FROM data}]
  set nDot [expr $nRow/50]

  db eval "PRAGMA secure_delete = on"
  db transaction {
    foreach_line f l t $G(maildir) -skip $skip { 
      set row [expr {int(rand()*$nMax)}]
      db eval {UPDATE data set file=$f, line=$l, text=$t WHERE oid=$row}

      incr n
      if {($n%$nDot)==0} { puts -nonewline . ; flush stdout }
      if {$n>$nRow} break
    }
  }
}

set command(insert) {

  set nArg [llength $argv]
  if {$nArg!=2 && $nArg!=3} { usage "DB-NAME ?ROWS?" 2 }
  set dbname [lindex $argv 1]
  set nRow $::G(insertrows)
  if {$nArg==3} { set nRow [lindex $argv 2] }

  if {[string match *zdb $dbname]} {
    sqlite3 db $dbname -vfs zip
    open_write_log db
  } else {
    sqlite3 db $dbname
  }
  set skip [count_files db]
  puts "skipping $skip files"
  set n 0
  puts "Inserting $nRow more rows"

  set nDot [expr $nRow/50]

  db eval "PRAGMA secure_delete = on"
  db transaction {
    foreach_line f l t $G(maildir) -skip $skip { 
      db eval {INSERT INTO data VALUES($f, $l, $t)}
      incr n
      if {($n%$nDot)==0} { puts -nonewline . ; flush stdout }
      if {$n>$nRow} break
    }
  }
  puts ""
}

set command(dump) {
  if {[llength $argv]!=2} { usage "DB-NAME" 2 }
  set dbname [lindex $argv 1]

  sqlite3 db $dbname -vfs zip
  proc x {pg off nbyte npadding} {
    if {$pg==0} { 
      incr ::freeslotarray($nbyte) 
    } else {
      set ::offset($off) "page $pg $nbyte bytes"
    }
  }
  zip_control db main structure x
  foreach k [lsort -integer [array names ::offset]] {
    puts "offset $k - $::offset($k)"
  }
  foreach k [lsort -integer [array names ::freeslotarray]] {
    puts "$k bytes - $::freeslotarray($k) slots"
  }
}

set command(compact) {
  if {[llength $argv]!=2} { usage "DB-NAME" 2 }
  set dbname [lindex $argv 1]
  sqlite3 db $dbname -vfs zip
  zip_control db main compact
}

set command(integrity_check) {
  if {[llength $argv]!=2} { usage "DB-NAME" 2 }
  set dbname [lindex $argv 1]
  sqlite3 db $dbname -vfs zip
  set res [zip_control db main integrity_check]
  if {$res == ""} {set res "Ok."}

  puts "Integrity check says: $res"
}

proc open_write_log {db} {
  set ::write_log_fd [open speedtest.log w]
  zip_control $db main write_hook write_log
}
proc close_write_log {} {
  catch { close $::write_log_fd }
}
proc write_log {iPg nByte} {
  puts $::write_log_fd "$iPg $nByte"
}

switch -- [lindex $argv 0] [concat [array get command] {
  default {
    puts stderr "Unrecognized command: \"[lindex $argv 0]\""
    puts stderr "Should be one of: [array names command]"
    exit -1
  }
}]
close_write_log


