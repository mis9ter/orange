INTRODUCTION

  The SQLite ZIPVFS Extension allows SQLite to read and write
  database files in which individual pages are compressed using
  application-supplied compression and decompression functions.
  A version of SQLite that includes ZIPVFS is also able to read and
  write normal database files created with a public domain version
  of SQLite.

  This demonstration release supports four compression algorithms:

    (1)  zlib - The same algorithm used by ZIP and gzip.  (recommended)

    (2)  rle  - Run-length encoding.  Fast, but gives only limited compression.

    (3)  bsr  - Blank-Space-Removal.  A single block of zeros is removed from
                each page.

    (4)  lrr  - Longest-Run-Removal.  Similar to bsr in that a single block
                of repeated characters is removed, but with the added
                flexibility that the characters need not be zeros.

  To be clear: the above algorithms are just the algorithms supported
  by this demonstration program.  Once you purchase a license to ZIPVFS,
  you can use whatever other compression algorithms you want with ZIPVFS.
  You can even substitute an encryption algorithm in place of the
  compression algorithm if you like.

  The zlib algorithm above is the only one that gives impressive amounts
  of compression.  The other three make interesting demos but are not
  really very practical.


LICENSE

  The core SQLite library is in the public domain.  However, the
  software included with this file is proprietary and licensed.
  You are authorized to use the software included with this file
  for evaluation purposes only.  You may not redistribute the 
  software or use it for any reason other than to evaluate whether
  or not you want to purchase a perpetual license to use the software.

  You are authorized to use this demonstration software for no more 
  than 60 calendar days.  After 60 days, you must destroy all copies 
  of this software.
  

COMMAND-LINE USAGE

  The demonstration command-line shell program (called "zipvfs.exe"
  or "zipvfs") that was included with this README file takes optional
  command-line argument to active the four supported compression modes:

     -zlib
     -rle
     -bsr
     -lrr

  If no extra command-line arguments are provided, then the shell works
  just like an ordinary public-domain SQLite shell and reads and writes
  ordinary uncompressed database files.  If one (and only one) of the
  above arguments is provided, then the shell applies the selected
  compression algorithm when reading and writing the database.

  If a database is created using any one of the above compression algorithms,
  then it can be accessed using any of the other algorithms.  ZIPVFS 
  includes an algorithm auto-detection mechanism such that if you attempt
  to open an existing database file using the wrong compression algorithm it
  will automatically switch over to the correct compression algorithm.  But
  this auto-detection mechanism only works to switch between various
  compression algorithms; it will not automatically switch between compressed
  and uncompressed.  So if you have an existing compressed database, you
  must specify one of the compression algorithms to read it, though you do
  not have to select exactly the right algorithm.

  You can convert an existing uncompressed database file into a
  compressed database file as follows:

     zipvfs old-database .dump | zipvfs -zlib new-database

  When evaluating the compression performance, you may want to
  experiment with different database page sizes in order to find
  a page size that gives good performance.

URI FILENAMES

  The filename argument to the zipvfs.exe command-line tool can be
  either a simple filename or a "file:" URI.  For example, the following
  are equivalent:

     zipvfs.exe test.db
     zipvfs.exe file:test.db

  When the filename is a URI, query parameters can be appended to modify
  the behavior.  For example, the -zlib, -rle, -bsr, and -lrr command-line
  options can be replaced by a "vfs=" query parameter.  The following two
  commands are equivalent:

     zipvfs.exe -zlib test.db
     zipvfs.exe file:test.db?vfs=zlib


ENCRYPTION

  All of the demonstration compression algorithms feature an optional
  encryption capability.

  To enable encryption using AES128, simply specify an encryption password
  using the "password=" query parameter with a URI filename.  For example:

     zipvfs.exe file:test.db?vfs=zlib&password=abcdef

  To enable encryption using AES256, use the "password256=" query parameter
  inplace of "password=".

  The example above would cause the "test.db" database file to be compressed
  using the zlib algorithm and encrypted using a password "abcdef".  The
  AES128 algorithm is used with the "password=" query parameter.  For
  AES256, use "password256=" instead.

  Note that only compressed content is encrypted.  Temporary files used
  by SQLite such as rollback journals or materialized views are
  not compressed and hence are not encrypted.  And those files might
  contain sensitive data.  Use caution and good judgment in your
  application design and do not rely too heavily on encryption provided
  by ZIPVFS to protect your content from sophisticated adversaries.
  A separate extension, the SQLite Encryption Extension or SEE, provides
  more robust encryption and also encrypts temporary files and metadata
  that ZIPVFS leaves in the clear.  For sensitive applications, consider
  using SEE instead of ZIPVFS.  Unfortunately, it is not currently possible 
  to both compress a database with ZIPVFS and encrypt it using SEE.


PURCHASE OPTIONS

  If you chose to acquire a license for ZIPVFS, you will be issued
  a username and password which allows you to log onto the configuration
  management system used to hold all of the historical ZIPVFS source
  code.  From there, you can download the latest ZIPVFS source code
  (the source code consists of a single file that is appended to
  an ordinary public-domain sqlite3.c source file.) and read instructions
  on how to compile and use ZIPVFS.  Your license will be perpetual.
  Your username and password will not expire.  So you can log on
  again in the future to download updates and enhancements to ZIPVFS,
  if you like.

  The cost of a perpetual license is US $2000.00.  You may ship as
  many copied of the software to your customers as you want so long
  as the software has been compiled and statically linked with your
  product.

  Once you have a valid license, you can create multiple products 
  that use this software as long as all products are developed and
  maintained by the same team.  For the purposes of this paragraph, 
  a "team" is a work unit where everybody knows each others names.  
  If you are in a large company where this product is used by multiple
  teams, then each team should acquire their own separate license.
