/*
** Copyright (c) 2010-2011 Hipp, Wyrick & Company, Inc.
** 6200 Maple Cove Lane, Charlotte, NC 28269 USA
** +1.704.948.4565
**
** All rights reserved.
**
*/

#ifndef _ZIPVFS_H
#define _ZIPVFS_H

#ifndef SQLITE_AMALGAMATION
# include "sqlite3.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ZipvfsMethods ZipvfsMethods;

/*
** CAPI: Legacy Procedures to Create a New Zipvfs VFS
**
** New Zipvfs VFSes should be created using zipvfs_create_vfs_v3().
** The interfaces described here are legacy and are retained for
** backwards compatibility only.
*/
int zipvfs_create_vfs(
  char const *zName,
  char const *zParent,
  void *pCtx,
  int (*xCompressBound)(void*, int nSrc),
  int (*xCompress)(void*, char *aDest, int *pnDest, const char *aSrc, int nSrc),
  int (*xUncompress)(void*, char *aDest, int *pnDest, const char *aSrc,int nSrc)
);
int zipvfs_create_vfs_v2(
  char const *zName,
  char const *zParent,
  void *pCtx,
  int (*xCompressBound)(void *, int nSrc),
  int (*xCompress)(void*, char *aDest, int *pnDest, const char *aSrc, int nSrc),
  int (*xUncompress)(void*, char *aDest, int *pnDest,const char *aSrc,int nSrc),
  int (*xCompressOpen)(void*, const char*, void**),
  int (*xCompressClose)(void*)
);

/*
** CAPI: Create a new Zipvfs VFS
**
** This interface creates a new Zipvfs VFS.
**
** The name of the new VFS is as specified by parameter zName.
** The new VFS accesses the file system using an existing VFS named
** by parameter zParent.
**
** The third argument to this function, pCtx, is a context pointer passed
** through into callback functions
**
** The fourth argument is this interface is the xAutoDetect callback
** function.  The xAutoDetect callback is invoked whenever a new file is
** opened by the Zipvfs VFS.  The fourth argument to the xAutoDetect callback
** is a pointer to a ZipvfsMethods objects.  Initially, most fields
** of the ZipvfsMethods object are zero. If the xAutoDetect callback may
** leave the ZipvfsMethods object unchanged, in which case the database
** file is opened as if it were a normal uncompressed database. (This is
** called "pass-through" mode.)  Or, the xAutoDetect implementation may
** populate the ZipvfsMethods structure with pointers to the methods
** that will be used to compress and uncompress pages of the database
** file.
** 
** If no error occurs, the xAutoDetect implementation should return SQLITE_OK.
** Otherwise, it may return any SQLite error code and the error will be
** propagated back to the user.
**
** The first argument passed to the xAutoDetect invocation is a copy of the
** context pointer, pCtx, passed to zipvfs_create_vfs3_v3().
** The second argument is passed a pointer to a buffer containing the
** database file name.  The second argument to xAutoDetect is a copy
** of the pointer passed to the created VFS's xOpen method, and so the
** sqlite3_uri_parameter() function may be used with it to extract any URI
** parameters specified by the SQLite user. If a new database is being 
** created, the third argument to xAutoDetect is NULL. Otherwise, the
** third argument to xAutoDetect is a buffer containing a copy of the 13
** byte database header string identifying the compression algorithm used
** by the database. The implementation of xAutoDetect should interpret
** this string and provide compatible compression routines by filling in
** appropriate fields of the ZipvfsMethods object.
*/
int zipvfs_create_vfs_v3(
  char const *zName,
  char const *zParent,
  void *pCtx,
  int (*xAutoDetect)(void*, const char *zFile, const char *zHdr, ZipvfsMethods*)
);

/*
** CAPI: Container for Zipvfs Callback Pointers
**
** An instance of the ZipvfsMethods object describes the compression
** procedures used to read and writes a Zipvfs database file.
**
** A pointer to an instance of this object is passed to the xAutoDetect
** callback whenever a new database file is first opened.  The method
** callback pointers are initially all zero.  The job of the xAutoDetect
** callback is to fill in the ZipvfsMethods method pointer with routines
** appropriate for doing compression and decompression of individual pages
** on the database file being opened.
**
** The xAutoDetect callback must populate all of the following method pointers:
**
**   xCompressBound(X,N):
**     The xCompressBound(X,N) function returns the maximum possible size 
**     (in bytes) of the output produced by compressing a buffer containing
**     N bytes of data.  The value returned by xCompressBound(X,N) must be
**     the worst-case compression.  One expects that the actually length of
**     the compressed buffer will usually be shorter.  The xCompressBound(X,N)
**     method is used by Zipvfs to allocate a buffer sufficient to hold the
**     results of compression.
**
**   xCompress(X,D,M,S,N):
**     The xCompress(X,D,M,S,N) method compresses a single page of content.
**     The input content is N bytes at S.  Output must be written into a
**     buffer D.  (The size of D will be at least xCompressBound(X,N) bytes.)
**     M must be filled with the number of bytes in D after compression.
**     
**   xUncompress(X,D,M,S,N):
**     The xUncompress(X,D,M,S,N) method uncompressions N bytes of content
**     from the S buffer and writes the result into buffer D.  M is filled
**     with the number of bytes in D after decompression.
**
**   xCompressClose(X):
**     The xCompressClose(X) method is called when the ZIPVFS database
**     connection is closed.  The xCompressClose(X) method is responsible
**     for freeing any resources (ex: memory) held by the ZIPVFS instance.
**
** The zHdr field should be set to point to a nul-terminated string. If the
** string is longer than 13 bytes, not including the nul-terminator, it is
** truncated. If a new database is created, the string that zHdr points to
** is written into the header file of the new database. This string will 
** then be read from the database and passed as the third parameter to
** the xAutoDetect() invocation next time it is opened. It follows that the 
** first 13 bytes of the string that zHdr points to should identify the
** compression functions that the other fields are set to, so that future
** invocations of xAutoDetect can supply Zipvfs with compatible functions.
**
** The zAuxHdr field, if not NULL, is a zero-terminated string that is
** appended after the zero-terminator on zHdr, assuming there is space.
** The total number of bytes consumed by zHdr, the zero terminator on zHdr,
** and zAuxHdr may not exceed 13 bytes.  Any excess is silently truncated.
*/
struct ZipvfsMethods {
  const char *zHdr;
  void *pCtx;
  int (*xCompressBound)(void *, int nSrc);
  int (*xCompress)(void*, char *aDest, int *pnDest, const char *aSrc, int nSrc);
  int (*xUncompress)(void*, char *aDest, int *pnDest,const char *aSrc,int nSrc);
  int (*xCompressClose)(void*);
  const char *zAuxHdr;  /* Extra header information */
};

/*
** CAPI: Destroy a Zipvfs VFS - zipvfs_destroy_vfs()
**
** Deregister and destroy a ZIP vfs previously created by zipvfs_create_vfs().
**
** VFS objects are not reference counted. If a VFS object is destroyed
** before all database handles that use it have been closed, the results 
** are undefined.
*/
void zipvfs_destroy_vfs(const char *zName);

/*
** CAPI: Obtain an English Language Error Message - zipvfs_errmsg()
**
** Return a pointer to a static buffer containing an English language
** interpretation of the error code passed as the only argument. The error
** code must be one returned by either zipvfs_create_vfs(), or a call to
** sqlite3_file_control() with one of the ZIPVFS_CTRL_*** verbs.
*/
const char *zipvfs_errmsg(int rc);

/*
** CAPI: File-control Operations Supported by Zipvfs
**
** Values interpreted by the xFileControl method of a zipvfs db file-handle.
**
** ZIPVFS_CTRL_COMPACT:
**   This control is used to compact a database (rebuild it with zero 
**   free-slots and zero fragmented bytes). The argument may either be
**   NULL, or a pointer to a variable of type sqlite3_int64.
**
**   If the argument is a NULL, then zipvfs attempts to compact the 
**   database. If successful, SQLITE_OK is returned and the database 
**   file truncated before returning. For small database files, this is
**   fine. For larger database files it has two disadvantages: (1) 
**   It may take a long time, during which time an exclusive lock is held
**   on the database preventing other processes from accessing it, 
**   and (2) a journal file as large as the database itself may be 
**   created during the compact operation (it is deleted before the 
**   call returns, but may still be a problem if disk space is scarce).
**
**   If the argument is a pointer to a variable of type sqlite3_int64,
**   then only a section of the database is compacted before returning.
**   Subsequent ZIPVFS_CTRL_COMPACT operations continue on from where the
**   previous one finished. The initial value of the sqlite3_int64 variable 
**   is used as a rough limit to the size of the section of the database
**   compacted (and therefore a rough limit on the size of the journal file
**   created during the partial compact). If this limit is less than the
**   total size of the database file, then multiple calls may be required to
**   compact the file. If the limit is zero or less than zero, then the 
**   entire file is compacted. The file is not truncated until the entire 
**   database has been compacted.
**
**   Before returning, the value of the sqlite3_int64 variable pointed to by
**   the file-control argument is set to the number of bytes in the part of
**   the database file yet to be compacted. If the file has been completely
**   compacted, zipvfs sets the value to 0.
**
** ZIPVFS_CTRL_OFFSET_AND_SIZE:
**   The argument is a pointer to an array of two sqlite3_int64 values.  
**   The first element of the array holds a page number when called.  The
**   first element is overwritten with the offset from the beginning of the
**   compressed file to where the page is stored.  The second element of the
**   array is overwritten with the size of the page as stored in the compressed
**   database files.
**
** ZIPVFS_CTRL_MAXFREE:
**   The argument to this file-control is a pointer to a variable of type
**   "int". It is used to set a limit on the number of free-slots allowed
**   to accumulate within the zipvfs file before zipvfs becomes more 
**   aggressive in its attempts to reuse them. 
**
**   When there are less free-slots in the file than the configured limit, a 
**   free-slot is only reused if it is the exact size required by a new 
**   compressed page image. Once the configured limit is reached or exceeded,
**   when a new compressed page record is written to the file a free-slot is
**   reused if there exists one large enough that is not more than MAXFRAG
**   bytes larger than required, where MAXFRAG is the limit configured by the
**   ZIPVFS_CTRL_MAXFRAG file-control. If there is more than one free-slot 
**   that fits these constraints, the smallest is used.
**
** ZIPVFS_CTRL_MAXFRAG:
**   This file control is used to set a limit on the maximum amount of space
**   that will be wasted when reusing a free-slot for a new record (see 
**   ZIPVFS_CTRL_MAXFREE for details). The argument to this file-control
**   should be a pointer to a variable of type "int" that contains the new
**   limit value.
**
** ZIPVFS_CTRL_CACHESIZE:
**   Set the size of the page-cache used by the VFS when writing to the 
**   database file. The argument should be a pointer to a value of type int.
**
** ZIPVFS_CTRL_INTEGRITY_CHECK:
**   Check that a zipvfs database file appears to be internally consistent.
**   This is not the same as running "PRAGMA integrity_check".
**
** ZIPVFS_CTRL_LOCKING_MODE:
**   This file-control is similar to the "PRAGMA locking_mode" command 
**   supported by SQLite. The argument should be a pointer to a value of
**   type "int". If the value is initially set to 0, then an attempt is
**   made to set the pager used to write to the zipvfs file to 
**   locking_mode=normal mode. If the value is initially 1, then an attempt
**   is made to set it to locking_mode=exclusive. Other values do not modify
**   the locking-mode of the pager, but may be used to query for the same.
**
**   Before returning, the value of the int variable pointed to by the
**   argument is set to 0 if the pager is left in locking_mode=normal mode,
**   or 1 if the pager is left in locking_mode=exclusive mode.
**
** ZIPVFS_CTRL_STAT:
**   The argument to this file-control must be a pointer to an instance
**   of struct ZipvfsStat (see below). If successful, the fields of the
**   structure are populated according to the current size and structure
**   of the zipvfs database file before returning SQLITE_OK. If an error
**   is encountered while attempting to read the database file, an error
**   code is returned and the final values of the structure fields are 
**   undefined.
**
** ZIPVFS_CTRL_FILE_POINTER:
**   Return the underlying sqlite3_file pointer.  This works in either
**   a normal ZIPVFS or in pass-through mode.
**
** ZIPVFS_CTRL_CACHE_USED:
**   The argument must be a pointer to an integer of type sqlite3_int64.
**   Before returning, this file-control sets the value of the output
**   integer to the number of bytes of memory used by the underlying 
**   zipvfs pager (the same value as is returned by DBSTATUS_CACHE_USED
**   for a top level pager). 
**
** ZIPVFS_CTRL_CACHE_HIT:
**   The argument must be a pointer to an integer of type sqlite3_int64.
**   Before returning, this file-control sets the value of the output
**   integer to the number of cache hits that have occurred since the
**   counter was reset (the same value as is returned by DBSTATUS_CACHE_HITS
**   for a top level pager). If the value of the output integer is initially
**   non-zero, the counter is reset before returning.
**
** ZIPVFS_CTRL_CACHE_MISS:
**   As for ZIPVFS_CTRL_CACHE_HIT, except for DBSTATUS_CACHE_MISS.
**
** ZIPVFS_CTRL_CACHE_WRITE:
**   As for ZIPVFS_CTRL_CACHE_HIT, except for DBSTATUS_CACHE_WRITE.
**
** ZIPVFS_CTRL_DIRECT_READ:
**   The argument must be a pointer to an integer of type sqlite3_int64.
**   Before returning, this file-control sets the value of the output
**   integer to the number of read operations performed by zipvfs directly
**   on the underlying database file (bypassing the zipvfs pager). If the 
**   value of the output integer is initially non-zero, the counter is 
**   reset before returning.
**
** ZIPVFS_CTRL_DIRECT_BYTES:
**   As for ZIPVFS_CTRL_CACHE_HIT, except the output value is the total
**   number of bytes read directly, not the number of xRead calls.
**
** ZIPVFS_CTRL_REPLACE:
**   The (void*) argument must actually be a pointer to an SQLite handle
**   (type sqlite3*) for which the main database is a ZipVFS database
**   that uses the same compression schema and block_size as the database
**   ZIPVFS_CTRL_REPLACE is called on. There must not be a transaction
**   open on either database. The contents of the argument database are
**   copied over the top of the current database (i.e. the current db is
**   "replaced" by the argument).
**
**   If successful, SQLITE_OK is returned. Otherwise, if the argument
**   database is not a ZipVFS database, SQLITE_NOTFOUND is returned. Or,
**   if the argument is a ZipVFS db, but the compression algorithm or
**   page-size does not match, SQLITE_ERROR is returned. If an OOM or 
**   IO error occurs, the usual SQLite error code is returned.
**
** ZIPVFS_CTRL_REPLACE_INIT:
**   This operation, along with REPLACE_STEP and REPLACE_FINISH, 
**   provides the same functionality as ZIPVFS_CTRL_REPLACE. To replace
**   a database using these file controls, REPLACE_INIT is invoked once
**   to begin the operation, then REPLACE_STEP is invoked one or more
**   times to copy page data, and then REPLACE_FINISH is invoked once
**   to commit or rollback the change and to release resources.
**
**   The (void*) argument passed to this file-control must actually be a
**   pointer to an SQLite handle (type sqlite3*) for which the main database 
**   is a ZipVFS database that uses the same compression schema and block_size
**   as the database ZIPVFS_CTRL_REPLACE is called on. There must not be a
**   transaction open on either database.
**
**   If SQLITE_OK is returned, then the replace operation has been started.
**   From this point on it is an error to attempt to read or write to the
**   database. Writing to the source database (the one passed via the
**   (void*) pointer to this file-control) is also illegal, but no error
**   will be returned. Instead, it is likely that the destination database
**   will be silently corrupted. Otherwise, if an error occurs, an SQLite
**   error code is returned. If this file-control does not first return 
**   SQLITE_OK, it is an error (SQLITE_MISUSE) to invoke either REPLACE_STEP
**   or REPLACE_FINISH.
**   
**   A successful call to this file control must eventually be matched by 
**   a call to REPLACE_FINISH, even if an error occurs within REPLACE_STEP.
**
** ZIPVFS_CTRL_REPLACE_STEP:
**   This file-control may only be invoked following a successful call to
**   REPLACE_INIT. The (void*) argument should point to an integer 
**   (type int) with value N, where N is greater than zero. This 
**   file-control copies up to N pages from the source database to the
**   main database. If the copy is complete, SQLITE_DONE is returned. Or,
**   if there are still more pages to copy but no error occurred, SQLITE_OK.
**   Finally, if an error occurs, an SQLite error code is returned.
**
** ZIPVFS_CTRL_REPLACE_FINISH:
**   This file-control is invoked to commit or rollback a replace operation
**   started by REPLACE_INIT. The (void*) argument must point to an integer
**   (type int). If the value of the integer is non-zero and the most recent
**   call to REPLACE_STEP returned SQLITE_DONE, then the replace operation
**   is committed. Otherwise it is rolled back.
**
**   If no error occurs, SQLITE_OK is returned, regardless of whether the
**   transaction is committed or rolled back. Or, if an error occurs or
**   has already occurred within an earlier invocation of REPLACE_STEP,
**   an error code is returned. In all cases all resources related to
**   the replace operation are released.
**
** ZIPVFS_CTRL_REPLACE_NTOTAL:
**   This file-control may only be invoked while there is an ongoing
**   replace operation (i.e. between a REPLACE_INIT and REPLACE_FINISH).
**   The (void*) argument must point to an integer (type int). Before
**   returning, this file-control sets the value of that integer to
**   the total number of pages that will be copied by the replace operation.
**
** ZIPVFS_CTRL_REPLACE_DONE:
**   This file-control may only be invoked while there is an ongoing
**   replace operation (i.e. between a REPLACE_INIT and REPLACE_FINISH).
**   The (void*) argument must point to an integer (type int). Before
**   returning, this file-control sets the value of that integer to
**   the total number of pages that have already been copied from source
**   to destination by the replace operation.
**
** ZIPVFS_CTRL_FREELIST:
**   For this file control, the (void*) argument should be a pointer to
**   a variable of type (sqlite3_int64*). This command allocates an array
**   of (sqlite3_int64) values and sets the indicated variable to point
**   to it. It is the responsibility of the caller to free this array
**   using sqlite3_free() when it is no longer required.
**
**   The first value in the array is the number of free-slots in the file.
**   Then, for each free-slot in the file there are four values, as follows:
**
**      <ul>
**        <li> The byte offset of the free-slot.
**        <li> The size in bytes of the free-slot.
**        <li> The byte offset of the parent free-slot, or 0 for the root.
**        <li> The index of the current slot within the parent.
**      <ul>
**   
*/
#define ZIPVFS_CTRL_COMPACT          230437
#define ZIPVFS_CTRL_FILE_POINTER     230439
#define ZIPVFS_CTRL_OFFSET_AND_SIZE  230440
#define ZIPVFS_CTRL_MAXFREE          230441
#define ZIPVFS_CTRL_MAXFRAG          230442
#define ZIPVFS_CTRL_CACHESIZE        230443
#define ZIPVFS_CTRL_INTEGRITY_CHECK  230444
#define ZIPVFS_CTRL_LOCKING_MODE     230445
#define ZIPVFS_CTRL_STAT             230446

#define ZIPVFS_CTRL_CACHE_USED       231454      /* Like DBSTATUS_CACHE_USED */
#define ZIPVFS_CTRL_CACHE_HIT        231455      /* Like DBSTATUS_CACHE_HIT */
#define ZIPVFS_CTRL_CACHE_MISS       231456      /* Like DBSTATUS_CACHE_MISS */
#define ZIPVFS_CTRL_CACHE_WRITE      231457      /* Like DBSTATUS_CACHE_WRITE */
#define ZIPVFS_CTRL_DIRECT_READ      231458
#define ZIPVFS_CTRL_DIRECT_BYTES     231459
#define ZIPVFS_CTRL_REPLACE          230460
#define ZIPVFS_CTRL_REPLACE_INIT     230461
#define ZIPVFS_CTRL_REPLACE_STEP     230462
#define ZIPVFS_CTRL_REPLACE_FINISH   230463
#define ZIPVFS_CTRL_REPLACE_NTOTAL   230464
#define ZIPVFS_CTRL_REPLACE_NDONE    230465
#define ZIPVFS_CTRL_FREELIST         230466

/*
** CAPI: File Space Usage Report - struct ZipvfsStat
**
** An instance of the ZipvfsStat structure is used as an output parameter by
** the ZIPVFS_CTRL_STAT file-control. ZIPVFS_CTRL_STAT is used to query for
** information regarding the size and structure of a zipvfs database file.
** Successfully invoking the file-control populates the fields of an instance
** of this structure as follows:
**
**   nFreeSlot:
**     The total number of slots in the file that do not currently contain
**     compressed database content.
**
**   nFileByte:
**     Number of bytes of space used by the zipvfs database image. The
**     actual size of the file on disk may be slightly larger than this, 
**     as the file-size is always an integer multiple of the page-size.
**
**   nContentByte:
**     The total size of all compressed user database pages currently 
**     stored in the zipvfs database file.
**
**   nFreeByte:
**     The total size of all free space on free-slots in the zipvfs 
**     database file.
**
**   nFragByte:
**     The total size of all unused space at the end of used slots in the
**     zipvfs database file (i.e. space wasted because a slots payload size
**     is larger than the size of the compressed page currently stored in
**     it).
**
**   nGapByte:
**     Total size of the unused block of space created by an incremental
**     compact operation. See the ZIPVFS_CTRL_COMPACT file-control operation
**     for more details.
**
** The total amount of free space in bytes that may be reclaimed by a
** ZIPVFS_CTRL_COMPACT operation is the sum of the values written to the
** nFreeByte, nFragByte and nGapByte structure members as part of a 
** ZIPVFS_CTRL_STAT request.
*/
typedef struct ZipvfsStat ZipvfsStat;
struct ZipvfsStat {
  int nFreeSlot;                  /* Number of free slots */
  sqlite3_int64 nFileByte;        /* Size of zipvfs database image */
  sqlite3_int64 nContentByte;     /* Bytes of compressed content in file */
  sqlite3_int64 nFreeByte;        /* Total size of all free slots in bytes */
  sqlite3_int64 nFragByte;        /* Total size of all fragments */
  sqlite3_int64 nGapByte;         /* Size "gap" produced by incr-compact */
};

/*
** CAPI: Register the zipvfs_dictstore module
**
** This function registers the "zipvfs_dictstore" virtual table module with
** the database handle passed as the only argument. The zipvfs_dictstore
** module may be used to create virtual tables interfaces to read or write 
** a ZipVFS dictionary store.
**
** SQLITE_OK is returned if the module is successfully registered, or an SQLite
** error code otherwise.
*/
int sqlite3_zipvfs_dictstore_register(sqlite3*);

/*
** CAPI: Read an entry from the dictionary store
**
** This function may be used from within an xAutoDetect, xCompress
** or xUncompress callback to read an entry from the dictionary store of
** the current ZipVFS database. The first argument must be a copy of the 
** pMethods pointer passed as the xAutoDetect function by the ZipVFS
** core. If the second argument is a non-negative integer, then it is
** the id of the entry to retrieve. Or, if the second argument is a negative
** integer, then the entry requested is that with the largest numeric id
** value in the dictionary store.
**
** Assuming no error occurs, if the requested entry is not present in the
** dictionary store, then the three output variables are set to -1, 0 and 0,
** respectively. Or, if the entry is present, the first output variable is
** set to the id of the retrieved entry, the second to the size of the 
** dictionary blob in byte and the third to point to a buffer allocated using
** sqlite3_malloc() that contains the dictionary blob. It is the responsibility
** of the caller to eventually free this buffer using sqlite3_free().
**
** If an error occurs, an SQLite error code is returned. Otherwise, SQLITE_OK.
** It is not considered an error if the requested entry does not exist.  
*/
int zipvfs_dictstore_get(
  ZipvfsMethods *pMethods,
  int iId,
  int *piId,                      /* OUT: Id of retrieved entry */
  int *pnDict,                    /* OUT: Size of retrieved blob in bytes */
  unsigned char **ppDict          /* OUT: Pointer to buffer containing blob */
);


/* ENDOFAPI. Do not remove this comment. It is used by the script that
** generates the api.wiki page from the comments in this file. */

/* 
** The following are only available if this module is built with 
** the SQLITE_TEST symbol defined. Usually this is not the case.
**
** ZIPVFS_CTRL_DETECT_CORRUPTION:
**   The argument to this is a pointer to a value of type int. If the value
**   is true, then some expensive integrity-checks are run before and after
**   each write to to the database. This is for use in isolating bugs, not 
**   normal operation. It is very slow.
**
** ZIPVFS_CTRL_STRUCTURE:
**   The argument to this file-control must be a pointer to an instance
**   of struct ZipvfsStructureCb, a structure that encapsulates a callback
**   function. The callback is invoked once for each page and free-slot 
**   in the zipvfs file before the file-control invocation returns. See
**   the comment above the definition of ZipvfsStructureCb for a description
**   of the arguments passed to the callback.
**   
** ZIPVFS_CTRL_WRITE_HOOK:
**   This file-control is used to register a callback that is invoked each
**   time the upper layer writes a page to the zipvfs database file.
**
** ZIPVFS_CTRL_APPEND_FREESLOT:
**   This file-control is used to directly test the zipvfs code that 
**   handles free-slots.
** 
** ZIPVFS_CTRL_REMOVE_FREESLOT:
**   Like APPEND_FREESLOT, this file-control is used to directly test the
**   zipvfs code that handles free-slots.
*/
#define ZIPVFS_CTRL_DETECT_CORRUPTION 230447
#define ZIPVFS_CTRL_STRUCTURE         230448
#define ZIPVFS_CTRL_WRITE_HOOK        230449
#define ZIPVFS_CTRL_APPEND_FREESLOT   230450
#define ZIPVFS_CTRL_REMOVE_FREESLOT   230451

/*
** These are only available if compiled with SQLITE_DEBUG
**
** ZIPVFS_CTRL_TRACE:
**   Set a FILE pointer for tracing.  If the FILE pointer is NULL then
**   tracing is disabled.
*/
#define ZIPVFS_CTRL_TRACE             230452


/*
** Structure used with the ZIPVFS_CTRL_STRUCTURE file-control. The 
** CTRL_STRUCTURE file-control is only available if this module is compiled
** with SQLITE_TEST defined.
**
** The callback function is invoked once for each page and once for each
** free-slot in the zipvfs database file. The first argument is always a
** pointer to the ZipvfsStructureCb object itself. When invoked for pages,
** the remaining five arguments are set as follows:
**
**    arg 2: The page number of the page.
**    arg 3: The offset of the compressed page within the zipvfs file.
**    arg 4: The compressed size of the page, in bytes.
**    arg 5: The number of unused padding bytes that follow the compressed
**           record in the zipvfs file.
**    arg 6: Always NULL.
**
** When the callback is invoked for a free-slot, the last 5 arguments are
** set as follows:
**
**    arg 2: Always 0.
**    arg 3: The offset of the free-slot within the zipvfs file.
**    arg 4: The size of the free-slot, in bytes.
**    arg 5: True if the free-slot is used as a node in the free-slot b-tree,
**           otherwise false.
**    arg 6: If the free-slot is used as a node in the free-slot b-tree, a
**           human readable string describing the contents of the node. 
**           Otherwise, NULL.
*/
typedef struct ZipvfsStructureCb ZipvfsStructureCb;
struct ZipvfsStructureCb {
  void (*x)(ZipvfsStructureCb*,unsigned int,sqlite3_int64,int,int,const char*);
};

/*
** Structure used with the ZIPVFS_CTRL_WRITE_HOOK file-control. The 
** CTRL_WRITE_HOOK file-control is only available if this module is compiled
** with SQLITE_TEST defined.
*/
typedef struct ZipvfsWriteCb ZipvfsWriteCb;
struct ZipvfsWriteCb {
  void (*x)(void *, unsigned int, int);
  void (*xDestruct)(void *pCtx);
  void *pCtx;
};

#ifdef __cplusplus
}  /* End of the 'extern "C"' block */
#endif
#endif
