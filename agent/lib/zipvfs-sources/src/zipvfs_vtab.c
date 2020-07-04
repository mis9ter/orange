/*
** 2016-06-12
**
** This file contains the implementation of an SQLite loadable extension 
** that supplies an eponymous virtual table named "zipvfs". Which may be 
** used to find the compressed size and offset of database pages.
**
** The virtual table contains one row for each page in the zipvfs 
** database. Each row contains three columns:
**
**   pgno:
**     The page number of the page (>= 1).
**
**   offset:
**     Byte offset of compressed page within zipvfs file.
**
**   size:
**     Size in bytes of compressed page within zipvfs file.
**
**------------------------------------------------------------------------
** Eponymous virtual table "zipvfs_freelist"
**
**   offset:
**     Byte offset of free space within file.
**
**   size:
**     Size of free space within file.
**
**   parent:
**     Offset of parent block in free-list btree. Or NULL for the root node.
**
**   parent_idx:
**     Index of pointer within parent block.
*/
#ifndef SQLITE_CORE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <string.h>
#ifndef _ZIPVFS_H
# include "zipvfs.h"
#endif
#endif

typedef struct ZipvfsCursor ZipvfsCursor;
typedef struct ZipvfsStatCursor ZipvfsStatCursor;
typedef struct ZipvfsFlCursor ZipvfsFlCursor;
typedef struct ZipvfsTable ZipvfsTable;

struct ZipvfsTable {
  sqlite3_vtab base;              /* Base class */
  sqlite3 *db;
};

struct ZipvfsCursor {
  sqlite3_vtab_cursor base;       /* Base class */
  int bEof;                       /* True if cursor is at EOF */
  sqlite3_int64 iLast;            /* Last pgno to return */
  sqlite3_int64 pgno;             /* Value of "pgno" column */
  sqlite3_int64 iOffset;          /* Value of "offset" column */
  sqlite3_int64 nSize;            /* Value of "size" column */
  char *zDatabase;                /* Database to query - e.g. "main" */
};

struct ZipvfsStatCursor {
  sqlite3_vtab_cursor base;       /* Base class */
  int iRowid;                     /* Rowid value to return next */
  ZipvfsStat sStat;                /* Stat values to return */
};

struct ZipvfsFlCursor {
  sqlite3_vtab_cursor base;
  int iRow;
  sqlite3_int64 *pA;
};

#define ZIPVFS_COLUMN_PGNO    0
#define ZIPVFS_COLUMN_OFFSET  1
#define ZIPVFS_COLUMN_SIZE    2
#define ZIPVFS_COLUMN_SCHEMA  3

#define ZIPVFS_VTAB_SCHEMA "CREATE TABLE x(pgno, offset, size, schema HIDDEN)"
#define ZIPVFS_STAT_SCHEMA "CREATE TABLE x(field, value, schema HIDDEN)"
#define ZIPVFS_FL_SCHEMA   "CREATE TABLE x(offset, size, parent, parent_idx)"

/*
** Create a new ZipvfsTable object.
**
**   argv[0]   -> module name  ("zipvfs")
**   argv[1]   -> database name
**   argv[2]   -> table name
*/
static int zipvfsVtabConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  ZipvfsTable *pNew = 0;
  int rc;

  rc = sqlite3_declare_vtab(db, pAux ? ZIPVFS_STAT_SCHEMA : ZIPVFS_VTAB_SCHEMA);
  if( rc==SQLITE_OK ){
    pNew = sqlite3_malloc( sizeof(*pNew) );
    if( pNew==0 ){
      rc = SQLITE_NOMEM;
    }else{
      pNew->db = db;
    }
  }

  *ppVtab = (sqlite3_vtab*)pNew;
  return rc;
}

/*
** This method is the destructor for ZipvfsTable objects.
*/
static int zipvfsVtabDisconnect(sqlite3_vtab *pVtab){
  sqlite3_free(pVtab);
  return SQLITE_OK;
}

/*
** Create a new ZipvfsTable object.
**
**   argv[0]   -> module name  ("zipvfs")
**   argv[1]   -> database name
**   argv[2]   -> table name
*/
static int zipvfsFlConnect(
  sqlite3 *db,
  void *pAux,
  int argc, const char *const*argv,
  sqlite3_vtab **ppVtab,
  char **pzErr
){
  ZipvfsTable *pNew = 0;
  int rc;

  rc = sqlite3_declare_vtab(db, ZIPVFS_FL_SCHEMA);
  if( rc==SQLITE_OK ){
    pNew = sqlite3_malloc( sizeof(*pNew) );
    if( pNew==0 ){
      rc = SQLITE_NOMEM;
    }else{
      pNew->db = db;
    }
  }

  *ppVtab = (sqlite3_vtab*)pNew;
  return rc;
}

/*
** Constructor for a new ZipvfsCursor object.
*/
static int zipvfsVtabOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor){
  ZipvfsCursor *pCsr;
  pCsr = sqlite3_malloc( sizeof(*pCsr) );
  if( pCsr==0 ) return SQLITE_NOMEM;
  memset(pCsr, 0, sizeof(*pCsr));
  *ppCursor = &pCsr->base;
  return SQLITE_OK;
}

/*
** Constructor for a new ZipvfsStatCursor object.
*/
static int zipvfsStatOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor){
  ZipvfsStatCursor *pCsr;
  pCsr = sqlite3_malloc( sizeof(*pCsr) );
  if( pCsr==0 ) return SQLITE_NOMEM;
  memset(pCsr, 0, sizeof(*pCsr));
  *ppCursor = &pCsr->base;
  return SQLITE_OK;
}

/*
** Constructor for a new ZipvfsFlCursor object.
*/
static int zipvfsFlOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor){
  ZipvfsFlCursor *pCsr;
  pCsr = sqlite3_malloc( sizeof(*pCsr) );
  if( pCsr==0 ) return SQLITE_NOMEM;
  memset(pCsr, 0, sizeof(*pCsr));
  *ppCursor = &pCsr->base;
  return SQLITE_OK;
}

/*
** Destructor for a ZipvfsCursor.
*/
static int zipvfsVtabClose(sqlite3_vtab_cursor *pCur){
  ZipvfsCursor *pCsr = (ZipvfsCursor*)pCur;
  sqlite3_free(pCsr->zDatabase);
  sqlite3_free(pCur);
  return SQLITE_OK;
}

/*
** Destructor for a ZipvfsStatCursor.
*/
static int zipvfsStatClose(sqlite3_vtab_cursor *pCur){
  sqlite3_free(pCur);
  return SQLITE_OK;
}

/*
** Destructor for a ZipvfsFlCursor.
*/
static int zipvfsFlClose(sqlite3_vtab_cursor *pCur){
  ZipvfsFlCursor *p = (ZipvfsFlCursor*)pCur;
  sqlite3_free(p->pA);
  sqlite3_free(p);
  return SQLITE_OK;
}

static void zipvfsSetNotFoundError(ZipvfsTable *pTab, int rc){
  if( rc==SQLITE_NOTFOUND ){
    assert( pTab->base.zErrMsg==0 );
    pTab->base.zErrMsg = sqlite3_mprintf("not a zipvfs database");
  }
}

/*
** Advance a ZipvfsCursor to its next row of output.
*/
static int zipvfsVtabNext(sqlite3_vtab_cursor *cur){
  ZipvfsTable *pTab = (ZipvfsTable*)cur->pVtab;
  ZipvfsCursor *pCsr = (ZipvfsCursor*)cur;
  int rc = SQLITE_OK;

  pCsr->pgno++;
  if( pCsr->pgno>pCsr->iLast ){
    pCsr->bEof = 1;
  }else{
    sqlite3_int64 aVal[2];
    aVal[0] = pCsr->pgno;
    rc = sqlite3_file_control(pTab->db, 
        pCsr->zDatabase, ZIPVFS_CTRL_OFFSET_AND_SIZE, aVal
    );
    zipvfsSetNotFoundError(pTab, rc);
    pCsr->iOffset = aVal[0];
    pCsr->nSize = aVal[1];
  }

  return rc;
}

/*
** Advance a ZipvfsStatCursor to its next row of output.
*/
static int zipvfsStatNext(sqlite3_vtab_cursor *cur){
  ZipvfsStatCursor *pCsr = (ZipvfsStatCursor*)cur;
  pCsr->iRowid++;
  return SQLITE_OK;
}

/*
** Advance a ZipvfsFlCursor to its next row of output.
*/
static int zipvfsFlNext(sqlite3_vtab_cursor *cur){
  ZipvfsFlCursor *pCsr = (ZipvfsFlCursor*)cur;
  pCsr->iRow++;
  return SQLITE_OK;
}

/*
** Return values of columns for the row at which the series_cursor
** is currently pointing.
*/
static int zipvfsVtabColumn(
  sqlite3_vtab_cursor *cur,   /* The cursor */
  sqlite3_context *ctx,       /* First argument to sqlite3_result_...() */
  int iCol                    /* Which column to return */
){
  ZipvfsCursor *pCsr = (ZipvfsCursor*)cur;
  switch( iCol ){
    case ZIPVFS_COLUMN_PGNO:
      sqlite3_result_int64(ctx, pCsr->pgno);
      break;
    case ZIPVFS_COLUMN_OFFSET:
      sqlite3_result_int64(ctx, pCsr->iOffset);
      break;
    case ZIPVFS_COLUMN_SIZE:
      sqlite3_result_int64(ctx, pCsr->nSize);
      break;
  }
  return SQLITE_OK;
}

/*
** Return values of columns for the row at which the ZipvfsFlCursor
** is currently pointing.
*/
static int zipvfsFlColumn(
  sqlite3_vtab_cursor *cur,   /* The cursor */
  sqlite3_context *ctx,       /* First argument to sqlite3_result_...() */
  int iCol                    /* Which column to return */
){
  ZipvfsFlCursor *pCsr = (ZipvfsFlCursor*)cur;
  assert( iCol<4 );
  sqlite3_result_int64(ctx, pCsr->pA[1 + pCsr->iRow*4 + iCol]);
  return SQLITE_OK;
}

/*
** Return the rowid for the current row. The rowid is the same as 
** the value of the pgno column.
*/
static int zipvfsVtabRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  ZipvfsCursor *pCsr = (ZipvfsCursor*)cur;
  *pRowid = pCsr->pgno;
  return SQLITE_OK;
}

/*
** Return the rowid for the current row. 
*/
static int zipvfsFlRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  ZipvfsFlCursor *pCsr = (ZipvfsFlCursor*)cur;
  *pRowid = (i64)(pCsr->iRow);
  return SQLITE_OK;
}

/*
** Return true if the cursor is at EOF.
*/
static int zipvfsVtabEof(sqlite3_vtab_cursor *cur){
  ZipvfsCursor *pCsr = (ZipvfsCursor*)cur;
  return pCsr->bEof;
}

/*
** Return true if the cursor is at EOF.
*/
static int zipvfsFlEof(sqlite3_vtab_cursor *cur){
  ZipvfsFlCursor *pCsr = (ZipvfsFlCursor*)cur;
  return pCsr->pA==0 || pCsr->iRow>=pCsr->pA[0];
}

/*
** Find the size of the database in pages. Write this value to *pnPg
** and return SQLITE_OK if successful. Otherwise, return an SQLite
** error code. If an error code is returned, the final value of
** *pnPg is undefined.
*/
static int zipvfsGetDbSize(
  sqlite3 *db, 
  const char *zDatabase, 
  sqlite3_int64 *pnPg
){
  int rc;
  char *zPragma = 0;
  sqlite3_int64 nPg = -1;

  zPragma = sqlite3_mprintf("PRAGMA '%q'.page_count", zDatabase);
  if( zPragma==0 ){
    rc = SQLITE_NOMEM;
  }else{
    sqlite3_stmt *pPragma = 0;
    rc = sqlite3_prepare(db, zPragma, -1, &pPragma, 0);
    if( rc==SQLITE_OK ){
      if( SQLITE_ROW==sqlite3_step(pPragma) ){
        nPg = sqlite3_column_int64(pPragma, 0);
      }
      rc = sqlite3_finalize(pPragma);
      if( rc==SQLITE_OK && nPg<0 ){
        rc = SQLITE_ERROR;
      }
    }
  }

  sqlite3_free(zPragma);
  *pnPg = nPg;
  return rc;
}

/*
** xFilter callback for "zipvfs" virtual table.
*/
static int zipvfsVtabFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  ZipvfsTable *pTab = (ZipvfsTable*)(pVtabCursor->pVtab);
  ZipvfsCursor *pCsr = (ZipvfsCursor*)pVtabCursor;
  int rc;
  const char *zDatabase = "main";
  sqlite3_int64 iFirst = 1;
  sqlite3_int64 iLast = 0x7FFFFFFF;
  int iArg = 0;

  if( idxNum & 0x0001 ){
    zDatabase = (const char*)sqlite3_value_text(argv[iArg++]);
  }
  sqlite3_free(pCsr->zDatabase);
  pCsr->zDatabase = sqlite3_mprintf("%s", zDatabase);
  if( pCsr->zDatabase==0 ) return SQLITE_NOMEM;

  rc = zipvfsGetDbSize(pTab->db, zDatabase, &pCsr->iLast);
  if( rc!=SQLITE_OK ) return rc;

  if( idxNum & 0x0002 ){
    iFirst = iLast = sqlite3_value_int64(argv[iArg++]);
  }else{
    if( idxNum & 0x0004 ){
      iLast = sqlite3_value_int64(argv[iArg++]);
    }
    if( idxNum & 0x0008 ){
      iFirst = sqlite3_value_int64(argv[iArg++]);
    }
  }

  if( iFirst>=1 ){
    pCsr->pgno = iFirst-1;
  }else{
    pCsr->pgno = 0;
  }

  if( iLast<pCsr->iLast ){
    pCsr->iLast = iLast;
  }

  pCsr->bEof = 0;
  return zipvfsVtabNext(pVtabCursor);
}

/*
** xFilter callback for "zipvfs" virtual table.
*/
static int zipvfsStatFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  sqlite3 *db = ((ZipvfsTable*)(pVtabCursor->pVtab))->db;
  ZipvfsStatCursor *pCsr = (ZipvfsStatCursor*)pVtabCursor;
  const char *zDb = "main";
  int rc;

  if( argc>0 ){
    zDb = (const char*)sqlite3_value_text(argv[0]);
  }
  rc = sqlite3_file_control(db, zDb, ZIPVFS_CTRL_STAT, (void*)&pCsr->sStat);
  zipvfsSetNotFoundError((ZipvfsTable*)(pVtabCursor->pVtab), rc);
  if( rc!=SQLITE_OK ){
    return rc;
  }

  pCsr->iRowid = 0;
  return zipvfsStatNext(pVtabCursor);
}

/*
** xFilter callback for "zipvfs_freelist" virtual table.
*/
static int zipvfsFlFilter(
  sqlite3_vtab_cursor *pVtabCursor, 
  int idxNum, const char *idxStr,
  int argc, sqlite3_value **argv
){
  sqlite3 *db = ((ZipvfsTable*)(pVtabCursor->pVtab))->db;
  const char *zDb = "main";
  ZipvfsFlCursor *pCsr = (ZipvfsFlCursor*)pVtabCursor;
  int rc;

  pCsr->iRow = 0;
  sqlite3_free(pCsr->pA);
  pCsr->pA = 0;
  rc = sqlite3_file_control(db, zDb, ZIPVFS_CTRL_FREELIST, (void*)(&pCsr->pA));
  zipvfsSetNotFoundError((ZipvfsTable*)(pVtabCursor->pVtab), rc);
  return rc;
}

/*
** SQLite will invoke this method one or more times while planning a query
** that uses the generate_series virtual table.  This routine needs to create
** a query plan for each invocation and compute an estimated cost for that
** plan.
**
** In this implementation idxNum is used to represent the
** query plan.  idxStr is unused.
**
** The query plan is represented by bits in idxNum:
**
**  (1)  schema == $value -- constraint exists
**  (2)  pgno == $value   -- constraint exists
**  (4)  pgno <= $value   -- constraint exists
**  (8)  pgno >= $value   -- constraint exists
*/
static int zipvfsVtabBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  int i;                          /* Loop counter */
  int idxNum = 0;                 /* The query plan bitmask */
  int aCons[] = {-1, -1, -1, -1}; /* {eq, le, ge, schema==} */
  int nArg = 0;

                  /* 0x00    0x01  0x02  0x03  0x04  0x05  0x06  0x07 */
  int aEstRows[] = { 100000, 1,    5000, 1,    5000, 1,    1000, 1 };

  for(i=0; i<pIdxInfo->nConstraint; i++){ 
    const struct sqlite3_index_constraint *pCons = &pIdxInfo->aConstraint[i];
    if( pCons->usable ){
      if( pCons->iColumn==ZIPVFS_COLUMN_PGNO ){
        switch( pCons->op ){
          case SQLITE_INDEX_CONSTRAINT_EQ:
            aCons[1] = i;
            break;

          case SQLITE_INDEX_CONSTRAINT_LE:
          case SQLITE_INDEX_CONSTRAINT_LT:
            aCons[2] = i;
            break;

          case SQLITE_INDEX_CONSTRAINT_GE:
          case SQLITE_INDEX_CONSTRAINT_GT:
            aCons[3] = i;
            break;
        }
      }else if( pCons->iColumn==ZIPVFS_COLUMN_SCHEMA 
             && pCons->op==SQLITE_INDEX_CONSTRAINT_EQ 
             && aCons[0]<0
      ){
        aCons[0] = i;
        pIdxInfo->aConstraintUsage[i].omit = 1;
      }
    }
  }

  for(i=0; i<sizeof(aCons)/sizeof(aCons[0]); i++){
    int iCons = aCons[i];
    if( iCons>=0 ){
      idxNum |= (1 << i);
      pIdxInfo->aConstraintUsage[iCons].argvIndex = ++nArg;
    }
  }

  pIdxInfo->estimatedRows = aEstRows[idxNum << 1];
  pIdxInfo->estimatedCost = 100 * pIdxInfo->estimatedRows;
  pIdxInfo->idxNum = idxNum;
  return SQLITE_OK;
}

static int zipvfsStatBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  int i;
  for(i=0; i<pIdxInfo->nConstraint; i++){ 
    const struct sqlite3_index_constraint *pCons = &pIdxInfo->aConstraint[i];
    if( pCons->usable 
     && pCons->op==SQLITE_INDEX_CONSTRAINT_EQ 
     && pCons->iColumn==2 
    ){
      pIdxInfo->aConstraintUsage[i].argvIndex = 1;
      pIdxInfo->aConstraintUsage[i].omit = 1;
      break;
    }
  }

  pIdxInfo->estimatedRows = 6;
  pIdxInfo->estimatedCost = 100;
  return SQLITE_OK;
}

/*
** Return true if the cursor is at EOF.
*/
static int zipvfsStatEof(sqlite3_vtab_cursor *cur){
  ZipvfsStatCursor *pCsr = (ZipvfsStatCursor*)cur;
  return (pCsr->iRowid>=7);
}

/*
** Return values of columns for the row at which the series_cursor
** is currently pointing.
*/
static int zipvfsStatColumn(
  sqlite3_vtab_cursor *cur,   /* The cursor */
  sqlite3_context *ctx,       /* First argument to sqlite3_result_...() */
  int iCol                    /* Which column to return */
){
  ZipvfsStatCursor *pCsr = (ZipvfsStatCursor*)cur;
  switch( iCol ){
    case 0: {
      const char *zVal = 0;
      switch( pCsr->iRowid ){
        case 1: zVal = "free-slots"; break;
        case 2: zVal = "file-bytes"; break;
        case 3: zVal = "content-bytes"; break;
        case 4: zVal = "free-bytes"; break;
        case 5: zVal = "frag-bytes"; break;
        case 6: zVal = "gap-bytes"; break;
      }
      sqlite3_result_text(ctx, zVal, -1, SQLITE_STATIC);
      break;
    }

    case 1: {
      sqlite3_int64 iVal = 0;
      switch( pCsr->iRowid ){
        case 1: iVal = (sqlite3_int64)pCsr->sStat.nFreeSlot; break;
        case 2: iVal = pCsr->sStat.nFileByte; break;
        case 3: iVal = pCsr->sStat.nContentByte; break;
        case 4: iVal = pCsr->sStat.nFreeByte; break;
        case 5: iVal = pCsr->sStat.nFragByte; break;
        case 6: iVal = pCsr->sStat.nGapByte; break;
      }
      sqlite3_result_int64(ctx, iVal);
      break;
    }
  }
  return SQLITE_OK;
}

/*
** Return the rowid for the current row.
*/
static int zipvfsStatRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid){
  ZipvfsStatCursor *pCsr = (ZipvfsStatCursor*)cur;
  *pRowid = (sqlite3_int64)pCsr->iRowid;
  return SQLITE_OK;
}

static int zipvfsFlBestIndex(
  sqlite3_vtab *tab,
  sqlite3_index_info *pIdxInfo
){
  pIdxInfo->estimatedRows = 1000000;
  pIdxInfo->estimatedCost = 1000000;
  return SQLITE_OK;
}

/*
** Register the virtual table module with database handle db.
*/
static int zipvfsRegisterModule(sqlite3 *db){
  static sqlite3_module zipvfsModule = {
    0,                         /* iVersion */
    0,                         /* xCreate */
    zipvfsVtabConnect,         /* xConnect */
    zipvfsVtabBestIndex,       /* xBestIndex */
    zipvfsVtabDisconnect,      /* xDisconnect */
    0,                         /* xDestroy */
    zipvfsVtabOpen,            /* xOpen - open a cursor */
    zipvfsVtabClose,           /* xClose - close a cursor */
    zipvfsVtabFilter,          /* xFilter - configure scan constraints */
    zipvfsVtabNext,            /* xNext - advance a cursor */
    zipvfsVtabEof,             /* xEof - check for end of scan */
    zipvfsVtabColumn,          /* xColumn - read data */
    zipvfsVtabRowid,           /* xRowid - read data */
    0,                         /* xUpdate */
    0,                         /* xBegin */
    0,                         /* xSync */
    0,                         /* xCommit */
    0,                         /* xRollback */
    0,                         /* xFindMethod */
    0,                         /* xRename */
  };

  static sqlite3_module zipvfsStatModule = {
    0,                         /* iVersion */
    0,                         /* xCreate */
    zipvfsVtabConnect,         /* xConnect */
    zipvfsStatBestIndex,       /* xBestIndex */
    zipvfsVtabDisconnect,      /* xDisconnect */
    0,                         /* xDestroy */
    zipvfsStatOpen,            /* xOpen - open a cursor */
    zipvfsStatClose,           /* xClose - close a cursor */
    zipvfsStatFilter,          /* xFilter - configure scan constraints */
    zipvfsStatNext,            /* xNext - advance a cursor */
    zipvfsStatEof,             /* xEof - check for end of scan */
    zipvfsStatColumn,          /* xColumn - read data */
    zipvfsStatRowid,           /* xRowid - read data */
    0,                         /* xUpdate */
    0,                         /* xBegin */
    0,                         /* xSync */
    0,                         /* xCommit */
    0,                         /* xRollback */
    0,                         /* xFindMethod */
    0,                         /* xRename */
  };

  static sqlite3_module zipvfsFlModule = {
    0,                         /* iVersion */
    0,                         /* xCreate */
    zipvfsFlConnect,           /* xConnect */
    zipvfsFlBestIndex,         /* xBestIndex */
    zipvfsVtabDisconnect,      /* xDisconnect */
    0,                         /* xDestroy */
    zipvfsFlOpen,              /* xOpen - open a cursor */
    zipvfsFlClose,             /* xClose - close a cursor */
    zipvfsFlFilter,            /* xFilter - configure scan constraints */
    zipvfsFlNext,              /* xNext - advance a cursor */
    zipvfsFlEof,               /* xEof - check for end of scan */
    zipvfsFlColumn,            /* xColumn - read data */
    zipvfsFlRowid,             /* xRowid - read data */
    0,                         /* xUpdate */
    0,                         /* xBegin */
    0,                         /* xSync */
    0,                         /* xCommit */
    0,                         /* xRollback */
    0,                         /* xFindMethod */
    0,                         /* xRename */
  };

  int rc = sqlite3_create_module(db, "zipvfs", &zipvfsModule, 0);
  if( rc==SQLITE_OK ){
    rc = sqlite3_create_module(db, "zipvfs_stat", &zipvfsStatModule, (void*)1);
  }
  if( rc==SQLITE_OK ){
    rc = sqlite3_create_module(db, "zipvfs_freelist", &zipvfsFlModule, 0);
  }
  return rc;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_zipvfsvtab_init(
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
#ifndef SQLITE_CORE
  SQLITE_EXTENSION_INIT2(pApi);
#endif
  return zipvfsRegisterModule(db);
}

#ifdef SQLITE_CORE
/*
** Entry point used when statically linked.
*/
int sqlite3_zipvfsvtab_register(sqlite3 *db){
  return zipvfsRegisterModule(db);
}
#endif
