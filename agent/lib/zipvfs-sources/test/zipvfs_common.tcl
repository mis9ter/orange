
set testdir $env(SQLITE_TEST_DIR)
source $testdir/tester.tcl

# Return true if SQLite is configured to use 8.3 filenames by default.
#
proc uses_8_3_names {} {
  forcedelete 83.db 83.db-journal 83.nal
  sqlite3 db83test 83.db
  db83test eval {
    PRAGMA journal_mode = delete;
    CREATE TABLE t1(x);
    BEGIN;
      INSERT INTO t1 VALUES(1);
  }

  set res [file exists 83.nal]
  db83test eval ROLLBACK
  db83test close

  return $res
}


#-------------------------------------------------------------------------
# This command tests that the compressed database size returned by
# ZIPVFS_CTRL_STAT is the same as that returned (indirectly) by
# ZIPVFS_CTRL_OFFSET_AND_STAT.
#
proc do_contentsize_test {tn} {
  # Figure out how large the db is.
  set nPg [db one {PRAGMA page_count}]

  array set A [zip_control db main stat]
  set n1 $A(nContentByte)

  set n2 0
  for {set iPg 1} {$iPg <= $nPg} {incr iPg} {
    set d [zip_control db main offset_and_size $iPg]
    incr n2 [lindex $d 1]
  }

  uplevel "do_test $tn { expr $n1==$n2 } 1"
}


proc do_zip_faultsim_test {args} {
#if {[lindex $args 0] != "9"} return
  catch { zip_unregister }
  uplevel do_faultsim_test $args [list -install {
    zip_register "" 1
  } -uninstall {
    catch { db close }
    zip_unregister
  }]
  zip_register "" 1
}

proc do_noop_faultsim_test {args} {
#if {[lindex $args 0] != "9"} return
  catch { noop_unregister }
  uplevel do_faultsim_test $args [list -install {
    noop_register "" 1
  } -uninstall {
    catch { db close }
    noop_unregister
  }]
}
