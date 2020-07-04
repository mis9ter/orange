#!/bin/sh
#
# With the ZIPVFS shell compiled in the sibline src/ directory, run this
# script to ensure that all the test databases are readable.
#
../src/zsqlite3 demo01.zlib .selftest
../src/zsqlite3 demo01.rle .selftest
../src/zsqlite3 demo01.bsr .selftest
../src/zsqlite3 demo01.lrr .selftest
../src/zsqlite3 'file:./demo01-xyzzy.zlib?password=xyzzy' .selftest
../src/zsqlite3 'file:./demo01-xyzzy256.zlib?password256=xyzzy' .selftest
