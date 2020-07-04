/*
** This file implements a simple command-line utility that will defragment
** and compact a ZIPVFS database file.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sqlite3.h"

/*
** Needed datatypes
*/
typedef unsigned char u8;
typedef sqlite3_int64 i64;
typedef unsigned int u32;

#define SQLITE_PENDING_BYTE      (1024 << 20)
#define SQLITE_PENDING_BYTE_SIZE (64   << 10)

/* This is copied from zipvfs.c. It is important that it is the same. */
#define ZIPVFS_MIN_PAYLOAD            (6 + 5 + 2*13)

#define MIN(x,y) ((x)<(y) ? (x) : (y))

/*
** A structure that contains the same data as the zipvfs file header. 
** Serialization and deserialization is done using the zipvfsReadHeader() 
** and zipvfsWriteHeader() functions.
*/
typedef struct ZipvfsHdr ZipvfsHdr;
struct ZipvfsHdr {
  i64 iFreeSlot;                  /* Byte offset of first free-slot in file */
  i64 iDataStart;                 /* Byte offset of first byte in data-area */
  i64 iDataEnd;                   /* Offset of first byte following data-area */
  i64 iGapStart;                  /* Offset of first byte in gap region */
  i64 iGapEnd;                    /* Offset of first byte after gap region */
  i64 iSize;                      /* Size of user database in bytes */
  int pgsz;                       /* User db page size */
  i64 nFreeSlot;                  /* Number of free-slots in file */
  i64 nFreeByte;                  /* Total size of all free-slots */
  i64 nFreeFragment;              /* Total size of all padding in used slots */
  int iVersion;                   /* Zipvfs file version */
};

static i64 zipvfsGetU32(u8 *aBuf){
  return (((i64)aBuf[0]) << 24)
       + (((i64)aBuf[1]) << 16)
       + (((i64)aBuf[2]) <<  8)
       + (((i64)aBuf[3]));
}
static i64 zipvfsGetU40(u8 *aBuf){
  return (((i64)aBuf[0]) << 32)
       + (((i64)aBuf[1]) << 24)
       + (((i64)aBuf[2]) << 16)
       + (((i64)aBuf[3]) <<  8)
       + (((i64)aBuf[4]));
}
static i64 zipvfsGetU64(u8 *aBuf){
  return (((i64)aBuf[0]) << 56)
       + (((i64)aBuf[1]) << 48)
       + (((i64)aBuf[2]) << 40)
       + (((i64)aBuf[3]) << 32)
       + (((i64)aBuf[4]) << 24)
       + (((i64)aBuf[5]) << 16)
       + (((i64)aBuf[6]) <<  8)
       + (((i64)aBuf[7]) <<  0);
}

/*
** Seek to offset iOffset of the file specified by the first argument. Then
** read nAmt bytes of data into buffer aBuf. Return 0 if successful, or 1 if
** an error occurs.
**
** This function accounts for the 64KB gap that appears at the 1GB boundary
** of zipvfs files. It should be used for all reads from zipvfs files.
*/
static int seekAndRead(
  sqlite3_file *in,               /* Read from this file */
  u8 *aBuf,                       /* Read data into this buffer */
  i64 iOffset,                    /* Offset within file to read from */
  int nAmt                        /* Bytes of data to read */
){
  if( iOffset<SQLITE_PENDING_BYTE ){
    int rc;
    int nRead = MIN(SQLITE_PENDING_BYTE-iOffset, nAmt);
    rc = in->pMethods->xRead(in, aBuf, nRead, iOffset);
    assert( rc==SQLITE_OK );
    if( rc!=SQLITE_OK ) return 1;
  }
  if( (iOffset+nAmt)>SQLITE_PENDING_BYTE ){
    int rc;
    int nRead = MIN(iOffset+nAmt-SQLITE_PENDING_BYTE, nAmt);
    rc = in->pMethods->xRead(in, &aBuf[nAmt-nRead], nRead, 
        iOffset + SQLITE_PENDING_BYTE_SIZE + nAmt - nRead
    );
    assert( rc==SQLITE_OK );
    if( rc!=SQLITE_OK ) return 1;
  }
  return 0;
}

/*
** Seek to offset iOffset of the file specified by the first argument. Then
** write nAmt bytes of data from buffer aBuf. Return 0 if successful, or 1 if
** an error occurs.
**
** This function accounts for the 64KB gap that appears at the 1GB boundary
** of zipvfs files. It should be used for all writes to zipvfs files.
*/
static int seekAndWrite(
  sqlite3_file *out,              /* Write to this file */
  u8 *aBuf,                       /* Write data from this buffer */
  i64 iOffset,                    /* Offset within file to write to */
  int nAmt                        /* Bytes of data to write */
){
  if( iOffset<SQLITE_PENDING_BYTE ){
    int rc;
    int nWrite = MIN(SQLITE_PENDING_BYTE-iOffset, nAmt);
    rc = out->pMethods->xWrite(out, aBuf, nWrite, iOffset);
    assert( rc==SQLITE_OK );
    if( rc!=SQLITE_OK ) return 1;
  }
  if( (iOffset+nAmt)>SQLITE_PENDING_BYTE ){
    int rc;
    int nWrite = MIN(iOffset+nAmt-SQLITE_PENDING_BYTE, nAmt);
    rc = out->pMethods->xWrite(out, &aBuf[nAmt-nWrite], nWrite, 
        iOffset + SQLITE_PENDING_BYTE_SIZE + nAmt - nWrite
    );
    assert( rc==SQLITE_OK );
    if( rc!=SQLITE_OK ) return 1;
  }
  return 0;
}

static sqlite3_file *openFile(const char *zFilename){
  sqlite3_file *pRet;
  char *zFull;
  int rc;
  int dummy;
  sqlite3_vfs *pVfs = sqlite3_vfs_find(0);

  pRet = malloc(pVfs->szOsFile + pVfs->mxPathname + 1);
  memset(pRet, 0, pVfs->szOsFile + pVfs->mxPathname + 1);
  zFull = &((char *)pRet)[pVfs->szOsFile];

  pVfs->xFullPathname(pVfs, zFilename, pVfs->mxPathname+1, zFull);
  rc = pVfs->xOpen(pVfs, zFull, pRet, 
      SQLITE_OPEN_MAIN_DB | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
      &dummy
  );
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "failed to open file: %s\n", zFilename);
    exit(1);
  }
  return pRet;
}

/*
** Read the zipvfs file header from disk and populate pZip->hdr with
** the values read. This function is a no-op if *pRc is not SQLITE_OK
** when it is called. It sets *pRc to an error code before returning if
** an error occurs.
**
** Normally, this function returns 0. Except, if this is the first time
** the header has been read since the file was opened and the header
** appears to indicate that this is an uncompressed SQLite database, and
** not a Zipvfs db, then return 1.
*/
static int zipvfsReadHeader(sqlite3_file *in, ZipvfsHdr *pHdr){
  u8 aHdr[100];

  if( seekAndRead(in, aHdr, 100, sizeof(aHdr)) ){
    return 1;
  }
  pHdr->iFreeSlot = zipvfsGetU64(&aHdr[0]);
  pHdr->iDataStart = zipvfsGetU64(&aHdr[8]);
  pHdr->iDataEnd = zipvfsGetU64(&aHdr[16]);
  pHdr->iGapStart = zipvfsGetU64(&aHdr[24]);
  pHdr->iGapEnd = zipvfsGetU64(&aHdr[32]);
  pHdr->iSize = zipvfsGetU64(&aHdr[40]);
  pHdr->nFreeSlot = zipvfsGetU64(&aHdr[48]);
  pHdr->nFreeByte = zipvfsGetU64(&aHdr[56]);
  pHdr->nFreeFragment = zipvfsGetU64(&aHdr[64]);
  pHdr->pgsz = (int)zipvfsGetU32(&aHdr[72]);
  pHdr->iVersion = (int)zipvfsGetU32(&aHdr[76]);
  return 0;
}

static void zipvfsPutU32(u8 *aBuf, u32 iVal){
  aBuf[0] = (iVal >> 24) & 0xFF;
  aBuf[1] = (iVal >> 16) & 0xFF;
  aBuf[2] = (iVal >>  8) & 0xFF;
  aBuf[3] = (iVal >>  0) & 0xFF;
}
static void zipvfsPutU40(u8 *aBuf, i64 iVal){
  aBuf[0] = (iVal >> 32) & 0xFF;
  aBuf[1] = (iVal >> 24) & 0xFF;
  aBuf[2] = (iVal >> 16) & 0xFF;
  aBuf[3] = (iVal >>  8) & 0xFF;
  aBuf[4] = (iVal >>  0) & 0xFF;
}
static void zipvfsPutU64(u8 *aBuf, i64 iVal){
  aBuf[0] = (iVal >> 56) & 0xFF;
  aBuf[1] = (iVal >> 48) & 0xFF;
  aBuf[2] = (iVal >> 40) & 0xFF;
  aBuf[3] = (iVal >> 32) & 0xFF;
  aBuf[4] = (iVal >> 24) & 0xFF;
  aBuf[5] = (iVal >> 16) & 0xFF;
  aBuf[6] = (iVal >>  8) & 0xFF;
  aBuf[7] = (iVal >>  0) & 0xFF;
}


/*
** Update the zipvfs file header with the values now contained in pHdr
*/
static int zipvfsWriteHeader(sqlite3_file *out, ZipvfsHdr *pHdr){
  u8 aHdr[100];
  memset(aHdr, 0, sizeof(aHdr));
  zipvfsPutU64(&aHdr[0], pHdr->iFreeSlot);
  zipvfsPutU64(&aHdr[8], pHdr->iDataStart);
  zipvfsPutU64(&aHdr[16], pHdr->iDataEnd);
  zipvfsPutU64(&aHdr[24], pHdr->iGapStart);
  zipvfsPutU64(&aHdr[32], pHdr->iGapEnd);
  zipvfsPutU64(&aHdr[40], pHdr->iSize);
  zipvfsPutU64(&aHdr[48], pHdr->nFreeSlot);
  zipvfsPutU64(&aHdr[56], pHdr->nFreeByte);
  zipvfsPutU64(&aHdr[64], pHdr->nFreeFragment);
  zipvfsPutU32(&aHdr[72], (u32)pHdr->pgsz);
  zipvfsPutU32(&aHdr[76], (u32)pHdr->iVersion);
  return seekAndWrite(out, aHdr, 100, 100);
}


static void usage(const char *argv0){
  fprintf(stderr, "Usage: %s SOURCE DESTINATION\n", argv0);
  exit(1);
}

int main(int argc, char **argv){
  const char *zInFile;
  const char *zOutFile;
  sqlite3_file *in;
  sqlite3_file *out;
  ZipvfsHdr hdr;                  /* The decoded zipvfs file header */
  u8 *aBuf;                       /* General use buffer */
  i64 iOut;                       /* Output file offset */
  int iPg;                        /* Used to loop through all input pages */
  u8 aEntry[8];                   /* Buffer for a lookup table entry */
  
  if( argc!=3 ) usage(argv[0]);
  zInFile = argv[1];
  zOutFile = argv[2];
  in = openFile(zInFile);
  out = openFile(zOutFile);

  if( zipvfsReadHeader(in, &hdr) ){
    fprintf(stderr, "%s: not a valid ZIPVFS database: \"%s\"\n", 
                    argv[0], zInFile);
    exit(1);
  }
  if( hdr.iVersion==0 && hdr.iDataEnd>SQLITE_PENDING_BYTE ){
    fprintf(stderr, "%s: cannot open \"%s\" as it is too large and too old\n",
        argv[0], zInFile
    );
    exit(1);
  }
  aBuf = malloc( 131073 );
  if( aBuf==0 ){
    fprintf(stderr, "out of memory\n");
    exit(1);
  }

  /* Copy the 200 byte header from the input directly to the output file.
  ** The second half of this will be overwritten with a new zipvfs header
  ** later on. So this is really just to ensure that the first 100 bytes
  ** of the output file are correctly populated. 
  */
  memset(aBuf, 200, 0);
  seekAndRead(in, aBuf, 0, 200);
  seekAndWrite(out, aBuf, 0, 200);

  /* Loop through all pages stored in the zipvfs file, in order of page
  ** number. Copy them into the output file in order, so as to create a
  ** defragmented file. 
  */
  iOut = hdr.iDataStart;
  for(iPg=1; iPg<=(hdr.iSize/hdr.pgsz); iPg++){
    i64 iOff;                     /* Offset in file of compressed page */
    int nByte;                    /* Size of compressed page in bytes */
    int nPadding = 0;             /* Bytes of padding added to output */

    /* Read and decode the lookup table entry for this page. The lookup
    ** table entry format is:
    **
    **   * Offset of compressed page image in file.         5 bytes (40 bits). 
    **   * Size of compressed page image in file, less 1.   17 bits.
    **   * Number of unused bytes (if any) in slot.         7 bits.
    */
    if( seekAndRead(in, aEntry, 200 + (iPg-1)*8, 8) ) goto io_error;
    iOff = zipvfsGetU40(aEntry);
    nByte = (aEntry[5]<<9) + (aEntry[6]<<1) + (aEntry[7]>>7);

    /* Read the compressed page record and its 6 byte header. */
    if( seekAndRead(in, aBuf, iOff, nByte+6) ) goto io_error;

    /* Normally, the compressed record will be written to the output file
    ** with zero bytes of padding. However, if the compressed buffer is 
    ** smaller than the minimum allowed slot size, a small number of padding
    ** bytes are added. */
    if( nByte<ZIPVFS_MIN_PAYLOAD ) nPadding = ZIPVFS_MIN_PAYLOAD - nByte;

    /* Write the lookup table entry to the output file */
    zipvfsPutU40(aEntry, iOut);
    aEntry[5] = (nByte>>9) & 0xFF;
    aEntry[6] = (nByte>>1) & 0xFF;
    aEntry[7] = ((nByte<<7) + nPadding) & 0xFF;
    if( seekAndWrite(out, aEntry, 200 + (iPg-1)*8, 8) ) goto io_error;

    /* Write the record to the output file. */
    aBuf[0] = (iPg >> 23) & 0xFF;
    aBuf[1] = (iPg >> 15) & 0xFF;
    aBuf[2] = (iPg >>  7) & 0xFF;
    aBuf[3] = ((iPg << 1) & 0xFE) + (((nByte + nPadding) >> 16) & 0x01);
    aBuf[4] = ((nByte + nPadding) >> 8) & 0xFF;
    aBuf[5] = ((nByte + nPadding) >> 0) & 0xFF;
    if( seekAndWrite(out, aBuf, iOut, 6+nByte+nPadding) ) goto io_error;
    iOut += 6 + nByte + nPadding;
  }

  /* Write a new zipvfs header to the 2nd 100 bytes of the output file. */
  hdr.iDataEnd = iOut;
  hdr.iFreeSlot = 0;
  hdr.iGapStart = 0;
  hdr.iGapEnd = 0;
  hdr.nFreeSlot = 0;
  hdr.nFreeByte = 0;
  hdr.nFreeFragment = 0;
  hdr.iVersion = 1;
  zipvfsWriteHeader(out, &hdr);

  /* Close the input and output and exit. */
  in->pMethods->xClose(in);
  out->pMethods->xClose(out);
  free(in);
  free(out);
  return 0;
 io_error:
  fprintf(stderr, "IO error\n");
  return 1;
}
