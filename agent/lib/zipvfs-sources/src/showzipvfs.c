/*
** This file implements a simple command-line utility that will decode
** and display the meta-data content of a ZIPVFS database file.  Page
** content is not displayed because this utility does not have the
** appropriate decryption and decompression routines.
**
** Compile as follows:
**
**     gcc -o showzipvfs showzipvfs.c
**
** Usage:
**
**     showzipvfs FILENAME [options]
**
** The FILENAME file is read and decoded and output appears on stdout.
** Options may be one of:  header pagemap freelist all
** If no [options] are specified then "all" is assumed.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "sqlite3.h"  /* only for the definition of sqlite3_int64 */

/*
** Needed datatypes
*/
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned short u16;
typedef sqlite3_int64 i64;

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


static u16 zipvfsGetU16(u8 *aBuf){
  return ((u16)aBuf[0] << 8)
       + (u16)aBuf[1];
}
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
static int zipvfsReadHeader(FILE *in, ZipvfsHdr *pHdr, u8 *aFirstHdr){
  u8 aHdr[100];
  size_t n;

  fseek(in, 0, SEEK_SET);
  n = fread(aFirstHdr, 1, 100, in);
  if( n<100 ) return 1;
  fseek(in, 100, SEEK_SET);
  n = fread(aHdr, 1, sizeof(aHdr), in);
  if( n<sizeof(aHdr) ) return 1;
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


static void usage(const char *argv0){
  fprintf(stderr, "Usage: %s FILENAME pagemap     -- page locates\n"
                  "       %s FILENAME header      -- header\n"
                  "       %s FILENAME freelist    -- freelist in table format\n"
                  "       %s FILENAME all         -- all of the above\n"
                  "       %s FILENAME tree        -- freelist in tree format\n",
                  argv0, argv0, argv0, argv0, argv0);
  exit(1);
}

/* Entry used to keep track of space used in the file */
typedef struct Slot {
  i64 addr;      /* Address of the entry */
  int pgno;      /* Page number for the entry.  -1 for a free block */
  int sz;        /* Size of the entry */
} Slot;

/* A heap of slots
*/
static u32 nSlot = 0;
static u32 nSlotAlloc = 0;
static Slot *aSlot = 0;

/* Insert an entry into the slot heap
*/
static void slotInsert(i64 addr, int sz, int pgno){
  u32 j, i;
  if( nSlot+1>=nSlotAlloc ){
    nSlotAlloc = nSlot*2 + 100;
    aSlot = realloc(aSlot, nSlotAlloc*sizeof(aSlot[0]) );
    if( aSlot==0 ){
      fprintf(stderr, "could not allocate %d bytes for the slot heap\n",
              (int)(nSlotAlloc*sizeof(aSlot[0])));
    }
  }
  i = ++nSlot;
  aSlot[i].addr = addr;
  aSlot[i].sz = sz;
  aSlot[i].pgno = pgno;
  while( (j = i/2)>0 && aSlot[j].addr>aSlot[i].addr ){
    Slot x = aSlot[j];
    aSlot[j] = aSlot[i];
    aSlot[i] = x;
    i = j;
  }
}

/* Extract the next entry off of the heap.  Entries are extracted
** in addr order.  Return 1 on success and 0 if the heap is empty.
*/
static int slotPull(i64 *pAddr, int *pSz, int *pPgno){
  u32 j, i;
  Slot x;
  if( nSlot==0 ) return 0;
  *pAddr = aSlot[1].addr;
  *pSz = aSlot[1].sz;
  *pPgno = aSlot[1].pgno;
  aSlot[1] = aSlot[nSlot];
  aSlot[nSlot].addr = ((i64)0x7fffffff)<<32 | (u32)0xffffffff;
  nSlot--;
  i = 1;
  while( (j = i*2)<=nSlot ){
    if( aSlot[j].addr>aSlot[j+1].addr ) j++;
    if( aSlot[i].addr<aSlot[j].addr ) break;
    x = aSlot[i];
    aSlot[i] = aSlot[j];
    aSlot[j] = x;
    i = j;
  }
  return 1;  
}

#define ZIPVFS_SLOT_HDRSIZE           6
#define ZIPVFS_MIN_PAYLOAD            (6 + 5 + 2*13)

static void zipvfsFlNodeReadHeader(
  u8 *aHdr,                       /* Pointer to buffer containing node header */
  int *piHeight,                  /* OUT: Height of node in b-tree */
  int *pnEntry                    /* OUT: Number of entries on b-tree node */
){
  if( piHeight ) *piHeight = (int)zipvfsGetU16(&aHdr[0]);
  if( pnEntry ) *pnEntry = (int)zipvfsGetU16(&aHdr[2]);
}
static int zipvfsFlNodeOffset(int iHeight, int iEntry){
  return 4 + ((iHeight>1)?5:0) + iEntry*((iHeight>1)?13:8);
}
static void zipvfsFlDecodeKey(
  i64 iKey,                       /* 7 byte key value to split */
  int *pnPayload,                 /* OUT: Payload size key component */
  i64 *piOffset,                  /* OUT: Offset key component */
  int *pbUsed                     /* OUT: True if slot is part of b-tree */
){
  static const i64 offset_mask = ((((i64)0xFF) << 32) + (i64)0xFFFFFFFF);

  *piOffset = (iKey >> 1) & offset_mask;
  *pnPayload = (iKey >> 41);
  if( pbUsed ) *pbUsed = (iKey & 0x01);
}

/*
** Read a 6-byte slot-header from offset iOffset of the zipvfs file. A slot
** header consists of two integer fields:
**
**   31 bits: Page number of current content.
**   17 bits: Slot payload size.
*/
static void zipvfsReadSlotHdr(
  FILE *pFile,                    /* File to read from */
  i64 iOffset,                    /* Offset to read slot header from */
  u32 *piPg,                      /* OUT: Page number from slot header */
  int *pnPayload,                 /* OUT: Page size from slot header */
  int *pRc                        /* IN/OUT: Error code */
){
  u8 aHdr[6] = {0,0,0,0,0,0};

  fseek(pFile, iOffset, SEEK_SET);
  fread(aHdr, 6, 1, pFile);
  
  if( *pRc==SQLITE_OK ){
    int nPayload;
    if( piPg ){
      *piPg = (((u32)aHdr[0] << 23) & 0x7F800000) +
              (((u32)aHdr[1] << 15) & 0x007F8000) +
              (((u32)aHdr[2] <<  7) & 0x00007F80) +
              (((u32)aHdr[3] >>  1) & 0x0000007F);
    }
  
    nPayload = (((int)aHdr[3] << 16) & 0x00010000) +
               (((int)aHdr[4] <<  8) & 0x0000FF00) +
               (((int)aHdr[5] <<  0) & 0x000000FF);
  
    if( nPayload<ZIPVFS_MIN_PAYLOAD ) *pRc = -1;
    *pnPayload = nPayload;
  }
}

/*
** Load the contents of a b-tree node into a malloced buffer. Decode header
** fields at the same time.
*/
static void zipvfsFlLoadSlot(
  FILE *pFile,                    /* File to read from */
  i64 iOff,                       /* Offset of node to load */
  int *pnPayload,                 /* OUT: Node payload size */
  int *piHeight,                  /* OUT: Height field of node header */
  int *pnEntry,                   /* OUT: Number of entries stored on node */
  u8 **paPayload,                 /* OUT: Malloced buffer containing payload */
  int *pRc                        /* IN/OUT: Error code */
){
  int nPayload = 0;               /* Node payload size */
  u8 *aPayload;

  assert( paPayload );
  zipvfsReadSlotHdr(pFile, iOff, 0, &nPayload, pRc); 
  aPayload = malloc(nPayload);

  fseek(pFile, iOff+ZIPVFS_SLOT_HDRSIZE, SEEK_SET);
  fread(aPayload, nPayload, 1, pFile);
  
  *pnPayload = nPayload;
  *paPayload = aPayload;

  if( *pRc==SQLITE_OK ){
    int nEntry;                   /* Number of entries on node */
    int iHeight;                  /* Height of slot in tree */
    assert( nPayload>=ZIPVFS_MIN_PAYLOAD );
    zipvfsFlNodeReadHeader(aPayload, &iHeight, &nEntry);
    if( nEntry<1 || iHeight<1 
     || zipvfsFlNodeOffset(iHeight, nEntry)>nPayload 
    ){
      *pRc = -2;
    }
    if( pnEntry ) *pnEntry = nEntry;
    if( piHeight ) *piHeight = iHeight;
  }
}

static void dump_freelist_node(FILE *pFile, i64 iOff, int *pRc, int showTree){
  if( iOff ){
    u8 *aPayload = 0;
    int nPayload = 0;
    int nEntry = 0;
    int iHeight = 0;

    zipvfsFlLoadSlot(pFile, iOff, &nPayload, &iHeight, &nEntry, &aPayload, pRc);

    if( *pRc==0 ){
      int i;
      if( showTree>0 ){
        printf("%8d:%*s # height=%d nEntry=%d\n", (int)iOff, showTree-2, "",
               iHeight, nEntry);
      }
      iOff += 10;  /* Skip the slot header, height, and number of entries */
      if( iHeight>1 ) iOff += 6;  /* Skip the right child pointer too */
      for(i=0; i<nEntry; i++){
        i64 x;                      /* Slot offset */
        int nByte;                  /* Slot payload size */
        int bUsed;                  /* True if slot is part of b-tree */

        u8 *a = &aPayload[zipvfsFlNodeOffset(iHeight, i)];
        if( iHeight>1 ){
          dump_freelist_node(pFile, zipvfsGetU40(&a[8]), pRc,
                             showTree>0 ? showTree + 2 : showTree);
        }
        zipvfsFlDecodeKey(zipvfsGetU64(a), &nByte, &x, &bUsed);
        if( showTree>0 ){
          printf("%8d:%*s ", (int)iOff, showTree-2, "");
        }
        slotInsert(x, nByte+6, 0);
        if( showTree>=0 ){
          printf("free-slot    ofst %-14lld sz %-7d used %-3d\n",
                 x, nByte, bUsed);
        }
        iOff += 8;
        if( iHeight>1 ) iOff += 5;
      }
      if( iHeight>1 ){
        dump_freelist_node(pFile, zipvfsGetU40(&aPayload[4]), pRc,
                           showTree>0 ? showTree + 2 : showTree);
      }
    }

    free(aPayload);
  }
}

/* Flags for artifacts to show */
#define SHOW_HEADER    0x001
#define SHOW_PAGEMAP   0x002
#define SHOW_FREELIST  0x004
#define SHOW_TREE      0x008
#define SHOW_ERRORS    0x010
#define SHOW_ALL       0x01f

int main(int argc, char **argv){
  const char *zFilename;
  FILE *in;
  ZipvfsHdr hdr;
  int i;
  int nErr = 0;
  int eToShow = SHOW_ALL;
  u8 aFirstHdr[100];

  if( argc<2 ) usage(argv[0]);
  zFilename = argv[1];
  if( argc>2 ){
    eToShow = 0;
    for(i=2; i<argc; i++){
      const char *zCmd = argv[i];
      if( strcmp(zCmd,"all")==0 ){
        eToShow |= SHOW_ALL;
      }else if( strcmp(zCmd,"header")==0 ){
        eToShow |= SHOW_HEADER;
      }else if( strcmp(zCmd,"pagemap")==0 ){
        eToShow |= SHOW_PAGEMAP;
      }else if( strcmp(zCmd,"freelist")==0 ){
        eToShow |= SHOW_FREELIST;
      }else if( strcmp(zCmd,"tree")==0 ){
        eToShow |= SHOW_TREE;
      }else if( strcmp(zCmd,"errors")==0 ){
        eToShow |= SHOW_ERRORS;
      }else{
        fprintf(stderr,"Unrecognized command: \"%s\"\n"
                       "Type \"%s\" with no arguments for help\n",
           zCmd, argv[0]);
        exit(1);
      }
    }
  }
  in = fopen(zFilename, "rb");
  if( in==0 ){
    fprintf(stderr, "%s: cannot open \"%s\"\n", argv[0], zFilename);
    exit(1);
  }
  if( zipvfsReadHeader(in, &hdr, aFirstHdr) ){
    fprintf(stderr, "%s: not a valid ZIPVFS database: \"%s\"\n", 
                    argv[0], zFilename);
    exit(1);
  }
  if( eToShow & SHOW_HEADER ){
    const char zFormat[]  = "%-15s %14lld\n";
    const char zFormat2[] = "%-15s %14d\n";
    const char zRegion[]  = "%-15s %14lld..%-14lld  %10lld bytes\n";
    printf("%-15s", "header (hex)");
    for(i=0; i<16; i++) printf(" %02x", aFirstHdr[i]);
    printf("\n%-15s", "header (text)");
    for(i=0; i<16; i++) printf("  %c", isprint(aFirstHdr[i])?aFirstHdr[i]:' ');
    printf("\n");
    printf(zFormat2, "page-size", zipvfsGetU16(aFirstHdr+16));
    printf(zFormat2, "write-format", aFirstHdr[18]);
    printf(zFormat2, "read-format", aFirstHdr[19]);
    printf(zFormat2, "reserved-bytes", aFirstHdr[20]);
    printf(zFormat2, "max-fraction", aFirstHdr[21]);
    printf(zFormat2, "min-fraction", aFirstHdr[22]);
    printf(zFormat2, "leaf-fraction", aFirstHdr[23]);
    printf(zFormat, "change-counter", zipvfsGetU32(aFirstHdr+24));
    printf(zFormat, "page-count", zipvfsGetU32(aFirstHdr+28));
    printf(zFormat, "freelist", zipvfsGetU32(aFirstHdr+32));
    printf(zFormat, "freepage-count", zipvfsGetU32(aFirstHdr+36));
    printf(zFormat, "schema-cookie", zipvfsGetU32(aFirstHdr+40));
    printf(zFormat, "schema-format", zipvfsGetU32(aFirstHdr+44));
    printf(zFormat, "default-cache", zipvfsGetU32(aFirstHdr+48));
    printf(zFormat, "auto-vacuum", zipvfsGetU32(aFirstHdr+52));
    printf(zFormat, "encoding", zipvfsGetU32(aFirstHdr+56));
    printf(zFormat, "user-version", zipvfsGetU32(aFirstHdr+60));
    printf(zFormat, "incr-vacuum", zipvfsGetU32(aFirstHdr+64));
    printf(zFormat, "application-id", zipvfsGetU32(aFirstHdr+68));
    printf(zFormat, "valid-for", zipvfsGetU32(aFirstHdr+92));
    printf(zFormat, "sqlite-version", zipvfsGetU32(aFirstHdr+96));
    printf(zFormat, "iFreeSlot", hdr.iFreeSlot);
    printf(zFormat, "iDataStart", hdr.iDataStart);
    printf(zFormat, "iDataEnd", hdr.iDataEnd);
    printf(zFormat, "iGapStart", hdr.iGapStart);
    printf(zFormat, "iGapEnd", hdr.iGapEnd);
    printf(zFormat, "iSize", hdr.iSize);
    printf(zFormat, "pgsz", (i64)hdr.pgsz);
    printf(zFormat, "nFreeSlot", hdr.nFreeSlot);
    printf(zFormat, "nFreeByte", hdr.nFreeByte);
    printf(zFormat, "nFreeFragment", hdr.nFreeFragment);
    printf(zFormat, "iVersion", (i64)hdr.iVersion);
    printf(zRegion, "header-space", (i64)0, (i64)199, (i64)200);
    printf(zRegion, "page-map", (i64)200, hdr.iDataStart-1, hdr.iDataStart-200);
    printf(zFormat, "page-map-slots", (hdr.iDataStart-200)/8);
    if( hdr.iGapStart<hdr.iGapEnd ){
      printf(zRegion, "gap", hdr.iGapStart, hdr.iGapEnd-1, 
                      hdr.iGapEnd - hdr.iGapStart);
    }
    printf(zRegion, "data-area", hdr.iDataStart, hdr.iDataEnd-1,
                      hdr.iDataEnd - hdr.iDataStart);
  }
  if( eToShow & (SHOW_PAGEMAP|SHOW_ERRORS) ){
    int off = 200;
    int i = 0;
    i64 prevEnd = hdr.iDataStart;
    while( off<hdr.iDataStart ){
      u8 aEntry[8];
      i64 dbOffset;
      int szEntry;
      int szUnused;

      memset(aEntry, 0, sizeof(aEntry)); 
      fseek(in, off, SEEK_SET);
      fread(aEntry, 8, 1, in);
      dbOffset = zipvfsGetU40(aEntry);
      szEntry = (aEntry[5]<<9) + (aEntry[6]<<1) + (aEntry[7]>>7);
      szUnused = aEntry[7]&0x7f;
      if( dbOffset>0 ){
        if( eToShow & SHOW_ERRORS ){
          slotInsert(dbOffset, szEntry+szUnused+6, i+1);
        }
        if( eToShow & SHOW_PAGEMAP ){
          printf("page %-7d ofst %-14lld sz %-5d unused %-3d  %s\n",
              i+1, dbOffset, szEntry,
              szUnused, prevEnd!=dbOffset ? "out-of-order" : "");
        }
        prevEnd = dbOffset + szEntry + szUnused + 6;
      }
      i++;
      off += 8;
    }
  }
  if( eToShow & (SHOW_FREELIST|SHOW_TREE|SHOW_ERRORS) ){
    int rc = 0;
    int showTree;
    if( eToShow & SHOW_TREE ){
      showTree = 2;
    }else if( eToShow & SHOW_FREELIST ){
      showTree = 0;
    }else{
      showTree = -1;
    }
    dump_freelist_node(in, hdr.iFreeSlot, &rc, showTree);
    if( rc ){
      printf("error: %d\n", rc);
      return rc;
    }
  }
  fclose(in);
  if( eToShow & SHOW_ERRORS ){
    i64 addr, prevAddr;
    int sz, pgno, prevSz, prevPgno;
    slotPull(&prevAddr, &prevSz, &prevPgno);
    while( slotPull(&addr, &sz, &pgno) ){
      if( prevAddr+prevSz>addr ){
        nErr++;
        printf("OVERLAP ");
        if( prevPgno>0 ){
          printf("page %d (%lld..%lld) ", prevPgno,
                 prevAddr, prevAddr+prevSz-1);
        }else{
          printf("freelist entry (%lld..%lld) ",
                 prevAddr, prevAddr+prevSz-1);
        }
        if( pgno>0 ){
          printf("page %d (%lld..%lld)\n", pgno,
                 addr, addr+sz-1);
        }else{
          printf("freelist entry (%lld..%lld)\n",
                 addr, addr+sz-1);
        }
      }
      if( prevAddr+prevSz<addr ){
        printf("Unused space: %lld bytes at %lld\n",
               addr - (prevAddr+prevSz), prevAddr+prevSz);
      }
      prevAddr = addr;
      prevSz = sz;
      prevPgno = pgno;
    }
  }
  free(aSlot);
  return nErr>0;
}
