
#include "tcl.h"
#include "sqlite3.h"
#include "zipvfs.h"

/* Pull in the various compression and encryption functions from algorithms.c */
#include "algorithms.c"

/* Include the code for the zipvfs_vtab extension */
#include "zipvfs_vtab.c"

/*************************************************************************
** Beginning of zlib hooks.
*/
#include "zlib.h"
#include "stdio.h"

/*
** Context object required by tclzlibCompress() and tclzlibUncompress() if
** encryption is enabled.
*/
typedef struct ZlibCtx ZlibCtx;
struct ZlibCtx {
  char *zPasswd;                  /* nul-terminated encryption key */
  int nBuf;                       /* Number of bytes allocated at aBuf[] */
  char *aBuf;                     /* Temporary buffer used for decryption */
};

/*
** Zlib xCompressBound callback.
*/
static int tclzlibBound(void *pCtx, int nByte){
  return compressBound(nByte);
}

/*
** Do encryption/decryption if pLocalCtx is not null.  The pLocalCtx is
** a string which is the encryption key.
**
** This encryption is very weak and is trivially broken by a knowledgable
** attacker.  It is intended for demonstration use only.  Applications
** that care able protecting data should substitute a real encryption
** algorithm such as AES.
*/
static void zlibCrypt(const char *zKey, char *zBuf, int nBuf){
  int nKey = strlen(zKey);
  unsigned int x = nBuf;
  int i;

  for(i=0; i<nBuf; i++){
    x = x*1103515245 + 12345;
    zBuf[i] ^= (x&0xff) ^ i ^ zKey[i%nKey];
  }
}

/*
** Zlib xCompress callback.
*/
static int tclzlibCompress(
  void *pLocalCtx, 
  char *aDest, 
  int *pnDest, 
  const char *aSrc, 
  int nSrc
){
  uLongf n = *pnDest;             /* In/out buffer size for compress() */
  int rc;                         /* compress() return code */
 
  rc = compress((Bytef*)aDest, &n, (Bytef*)aSrc, nSrc);
  if( pLocalCtx ){
    ZlibCtx *pCtx = (ZlibCtx *)pLocalCtx;
    zlibCrypt(pCtx->zPasswd, aDest, n);
  }
  *pnDest = n;
  return (rc==Z_OK ? SQLITE_OK : SQLITE_ERROR);
}

/*
** Zlib xUncompress callback.
*/
static int tclzlibUncompress(
  void *pLocalCtx, 
  char *aDest, 
  int *pnDest, 
  const char *aSrc, 
  int nSrc
){
  uLongf n = *pnDest;             /* In/out buffer size for uncompress() */
  const char *aCompressed;        /* Pointer to unencrypted data */
  int rc;                         /* uncompress() return code */

  if( pLocalCtx==0 ){
    aCompressed = aSrc;
  }else{
    ZlibCtx *pCtx = (ZlibCtx *)pLocalCtx;
    if( pCtx->nBuf<nSrc ){
      char *aNew = (char *)sqlite3_malloc(nSrc);
      if( !aNew ) return SQLITE_NOMEM;
      sqlite3_free(pCtx->aBuf);
      pCtx->aBuf = aNew;
      pCtx->nBuf = nSrc;
    }
    memcpy(pCtx->aBuf, aSrc, nSrc);
    zlibCrypt(pCtx->zPasswd, pCtx->aBuf, nSrc);
    aCompressed = pCtx->aBuf;
  }

  rc = uncompress((Bytef*)aDest, &n, (Bytef*)aCompressed, nSrc);
  *pnDest = n;
  return (rc==Z_OK ? SQLITE_OK : SQLITE_ERROR);
}

/*
** Set up a new zlib connection.  If the "pw=PASSWORD" parameter is
** specified on the filename, then make a copy of PASSWORD and set
** the local context to point to that copy.  Otherwise, make the
** local context a NULL pointer.
*/
static int zlibOpen(void *pCtx, const char *zFile, void **ppLocalCtx){
  const char *zPw;

  zPw = sqlite3_uri_parameter(zFile, "pw");
  if( zPw && zPw[0] ){
    int nPw = strlen(zPw);
    ZlibCtx *pCtx = (ZlibCtx *)sqlite3_malloc(sizeof(ZlibCtx) + nPw+1);
    if( pCtx==0 ) return SQLITE_NOMEM;
    memset(pCtx, 0, sizeof(ZlibCtx));
    pCtx->zPasswd = (char *)&pCtx[1];
    memcpy(pCtx->zPasswd, zPw, nPw+1);
    *ppLocalCtx = (void *)pCtx;
  }else{
    *ppLocalCtx = 0;
  }
  return SQLITE_OK;
}

/*
** Clean up a zlib connection.
*/
static int zlibClose(void *pLocalCtx){
  if( pLocalCtx ){
    ZlibCtx *pCtx = (ZlibCtx *)pLocalCtx;
    sqlite3_free(pCtx->aBuf);
    sqlite3_free(pCtx);
  }
  return SQLITE_OK;
}

/*
** End of zlib hooks.
**************************************************************************/

static int tclrleBound(void *pCtx, int nByte){
  return nByte*2;
}

/*
** RLE xCompress callback.
*/
static int tclrleCompress(
  void *pCtx, 
  char *aDest, 
  int *pnDest, 
  const char *aSrc, 
  int nSrc
){
  int iOut = 0;
  int iIn = 0;
  while( iIn<nSrc ){
    char c = aSrc[iIn++];
    aDest[iOut++] = c;
    if( c==0 ){
      int n = 1;
      while( iIn<nSrc && n<100 && aSrc[iIn]==0 ){
        iIn++;
        n++;
      }
      aDest[iOut++] = (char)n;
    }
  }
  *pnDest = iOut;
  return 0;
}

/*
** RLE xUncompress callback.
*/
static int tclrleUncompress(
  void *pCtx, 
  char *aDest, 
  int *pnDest, 
  const char *aSrc, 
  int nSrc
){
  int iOut = 0;
  int iIn = 0;
  while( iIn<nSrc ){
    char c = aSrc[iIn++];
    if( c==0 ){
      int n = aSrc[iIn++];
      memset(&aDest[iOut], 0, n);
      iOut += n;
    }else{
      aDest[iOut++] = c;
    }
  }
  *pnDest = iOut;
  return 0;
}
/*
** End of RLE hooks.
**************************************************************************/

/*************************************************************************
** Beginning of noop hooks.
*/

#define PTRTOINT(x) ((u8 *)(x) - (u8 *)0)

/*
** NOOP xBound callback.
*/
static int noopBound(void *pCtx, int nByte){ 
  return nByte + PTRTOINT(pCtx);
}

/*
** NOOP xCompress callback.
*/
static int noopCompress(
  void *pCtx, 
  char *aDest, 
  int *pnDest, 
  const char *aSrc, 
  int nSrc
){
  memcpy(&aDest[PTRTOINT(pCtx)], aSrc, nSrc);
  *pnDest = PTRTOINT(pCtx) + nSrc;
  return 0;
}

/*
** NOOP xUncompress callback.
*/
static int noopUncompress(
  void *pCtx, 
  char *aDest, 
  int *pnDest, 
  const char *aSrc, 
  int nSrc
){
  memcpy(aDest, &aSrc[PTRTOINT(pCtx)], nSrc - PTRTOINT(pCtx));
  *pnDest = nSrc - PTRTOINT(pCtx);
  return 0;
}
/*
** End of no-op hooks.
**************************************************************************/

typedef struct WriteCb WriteCb;
struct WriteCb {
  Tcl_Interp *interp;
  Tcl_Obj *pCmd;
};
static void zip_write_callback(void *pCtx, unsigned int iPg, int nByte){
  WriteCb *p = (WriteCb *)pCtx;
  Tcl_Obj *pEval;

  pEval = Tcl_DuplicateObj(p->pCmd);
  Tcl_IncrRefCount(pEval);
  Tcl_ListObjAppendElement(p->interp, pEval, Tcl_NewIntObj(iPg));
  Tcl_ListObjAppendElement(p->interp, pEval, Tcl_NewIntObj(nByte));
  Tcl_EvalObjEx(p->interp, pEval, TCL_EVAL_GLOBAL|TCL_EVAL_DIRECT);
  Tcl_DecrRefCount(pEval);
}
static void zip_write_destroy(void *p){
  Tcl_DecrRefCount(((WriteCb *)p)->pCmd);
  sqlite3_free(p);
}

static int vfs_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[],
  char const *zName,
  void *pCtx,
  int (*xBound)(void *, int nSrc),
  int (*xCompress)(void*, char *aDest, int *pnDest, const char *aSrc, int nSrc),
  int (*xUncompress)(void*, char *aDest, int *pnDest,const char *aSrc,int nSrc),
  int (*xOpen)(void*, const char *, void**),
  int (*xClose)(void*)
){
  int bDefault = 0;               /* True to make "zip" the default VFS */
  int rc = SQLITE_OK;             /* zipvfs_create_vfs() return code */
  char *zParent;

  if( objc!=3 && objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "PARENT DEFAULT ?NAME?");
    return TCL_ERROR;
  }
  if( Tcl_GetBooleanFromObj(interp, objv[2], &bDefault) ){
    return TCL_ERROR;
  }
  zParent = Tcl_GetString(objv[1]);
  if( !*zParent ) zParent = 0;
  if( objc==4 ){
    zName = Tcl_GetString(objv[3]);
  }

  if( sqlite3_vfs_find(zName)==0 ){
    if( xOpen || xClose ){
      rc = zipvfs_create_vfs_v2(zName, zParent, pCtx,
                                xBound, xCompress, xUncompress,
                                xOpen, xClose);
    }else{
      rc = zipvfs_create_vfs(zName, zParent, pCtx,
                             xBound, xCompress, xUncompress);
    }
    assert( rc==SQLITE_OK || rc==SQLITE_NOMEM || rc==SQLITE_ERROR );
  }

  if( rc==SQLITE_OK ){
    sqlite3_vfs_register(sqlite3_vfs_find(zName), bDefault);
    Tcl_ResetResult(interp);
  }else{
    Tcl_SetResult(interp, (char *)zipvfs_errmsg(rc), TCL_STATIC);
    return TCL_ERROR;
  }

  return TCL_OK;
}

static int vfs_unregister_cmd(
  ClientData cd,
  Tcl_Interp *interp, 
  int objc, 
  Tcl_Obj *CONST objv[]
){
  if( objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }
  zipvfs_destroy_vfs((char *)cd);
  return TCL_OK;
}

/*
** Tclcmd: generic_unregister NAME
*/
static int generic_unregister_cmd(
  ClientData cd,
  Tcl_Interp *interp, 
  int objc, 
  Tcl_Obj *CONST objv[]
){
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }
  zipvfs_destroy_vfs(Tcl_GetString(objv[1]));
  return TCL_OK;
}

/*
** Tclcmd: zip_register PARENT DEFAULT ?NAME?
*/
static int zip_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  return vfs_register_cmd(cd, interp, objc, objv, 
      "zip", 0, tclzlibBound, tclzlibCompress, tclzlibUncompress, 
      zlibOpen, zlibClose
  );
}

/*
** Tclcmd: noop_register PARENT DEFAULT
*/
static int noop_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  return vfs_register_cmd(cd, interp, objc, objv, 
      "noop", 0, noopBound, noopCompress, noopUncompress, 0, 0
  );
}

/*
** Tclcmd: padded_register PARENT DEFAULT
*/
static int padded_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  return vfs_register_cmd(cd, interp, objc, objv, 
      "padded", (void*)70000, noopBound, noopCompress, noopUncompress, 0, 0
  );
}

/*
** Tclcmd: rle_register PARENT DEFAULT
*/
static int rle_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  return vfs_register_cmd(cd, interp, objc, objv, 
      "rle", 0, tclrleBound, tclrleCompress, tclrleUncompress, 0, 0
  );
}

typedef struct DictCompression DictCompression;
struct DictCompression {
  ZipvfsMethods *pMethods;
  int iId;                        /* Current dictionary id */
  int bWriter;                    /* True to write with current dictionary */
  u8 *aDict;                      /* Pointer to buffer containing dictionary */
  int nDict;                      /* Size of aDict[] in bytes */
};

int dictBound(void *pCtx, int nSrc){
  return nSrc+1;
}

int dictCompress(
  void *pCtx, 
  char *aDest, int *pnDest, 
  const char *aSrc, int nSrc
){
  int i;
  DictCompression *p = (DictCompression*)pCtx;
  if( p->bWriter==0 ){
    int rc;
    sqlite3_free(p->aDict);
    p->aDict = 0;
    p->iId = -1;
    p->bWriter = 0;
    rc = zipvfs_dictstore_get(p->pMethods, -1, &p->iId, &p->nDict, &p->aDict);
    if( rc!=SQLITE_OK ) return rc;
    if( p->aDict==0 ) return SQLITE_ERROR;
    p->bWriter = 1;
  }

  aDest[0] = (char)p->iId;
  for(i=0; i<nSrc; i++){
    aDest[i+1] = aSrc[i] ^ p->aDict[i % p->nDict];
  }
  *pnDest = nSrc+1;

  return SQLITE_OK;
}

int dictUncompress(
  void *pCtx, 
  char *aDest, int *pnDest,
  const char *aSrc, int nSrc
){
  int i;
  DictCompression *p = (DictCompression*)pCtx;
  int iId = (int)aSrc[0];
  if( p->iId!=iId ){
    int rc;
    sqlite3_free(p->aDict);
    p->aDict = 0;
    p->iId = -1;
    p->bWriter = 0;
    rc = zipvfs_dictstore_get(p->pMethods, iId, &p->iId, &p->nDict, &p->aDict);
    if( rc!=SQLITE_OK ) return rc;
    if( p->aDict==0 ) return SQLITE_ERROR;
  }

  for(i=0; i<nSrc-1; i++){
    aDest[i] = aSrc[i+1] ^ p->aDict[i % p->nDict];
  }
  *pnDest = nSrc-1;

  return SQLITE_OK;
}

int dictCompressClose(void *pCtx){
  DictCompression *p = (DictCompression*)pCtx;
  sqlite3_free(p->aDict);
  sqlite3_free(p);
  return SQLITE_OK;
}

/*
** xAutoDetect callback for VFS created with [v3_register].
*/
static int testAutoDetect(
  void *pCtx, 
  const char *zFile, 
  const char *zHdr, 
  ZipvfsMethods *pOut
){
  assert( pOut->pCtx==0 && pOut->xUncompress==0 && pOut->xCompressClose==0 );
  assert( pOut->xCompress==0 && pOut->zHdr==0 && pOut->xCompressBound==0 );

  if( zHdr==0 ){
    zHdr = sqlite3_uri_parameter(zFile, "zv");
  }
  if( zHdr==0 || 0==strcmp(zHdr, "zip") ){
    pOut->zHdr = "zip";
    pOut->xCompressBound = tclzlibBound;
    pOut->xCompress = tclzlibCompress;
    pOut->xUncompress = tclzlibUncompress;
    pOut->xCompressClose = zlibClose;
    zlibOpen(0, zFile, &pOut->pCtx);
  }else if( 0==strcmp(zHdr, "zlib") ){
    pOut->zHdr = "zlib";
    pOut->xCompressBound = tclzlibBound;
    pOut->xCompress = tclzlibCompress;
    pOut->xUncompress = tclzlibUncompress;
    pOut->xCompressClose = zlibClose;
    zlibOpen(0, zFile, &pOut->pCtx);
  }else if( 0==strcmp(zHdr, "rle") ){
    pOut->zHdr = "rle";
    pOut->xCompressBound = tclrleBound;
    pOut->xCompress = tclrleCompress;
    pOut->xUncompress = tclrleUncompress;
    zlibOpen(0, zFile, &pOut->pCtx);
  }else if( 0==strcmp(zHdr, "noop") ){
    pOut->zHdr = "noop";
    pOut->xCompressBound = noopBound;
    pOut->xCompress = noopCompress;
    pOut->xUncompress = noopUncompress;
    pOut->zAuxHdr = "auxheaderfornoop";
    zlibOpen(0, zFile, &pOut->pCtx);
  }else if( 0==strcmp(zHdr, "noop2") ){
    pOut->zHdr = "compressedbynoop";
    pOut->xCompressBound = noopBound;
    pOut->xCompress = noopCompress;
    pOut->xUncompress = noopUncompress;
    pOut->zAuxHdr = "auxheaderfornoop";
    zlibOpen(0, zFile, &pOut->pCtx);
  }else if( 0==strcmp(zHdr, "dict") ){
    DictCompression *p = sqlite3_malloc(sizeof(DictCompression));
    if( p==0 ){
      return SQLITE_NOMEM;
    }
    memset(p, 0, sizeof(DictCompression));
    p->pMethods = pOut;
    pOut->zHdr = "dict";
    pOut->xCompressBound = dictBound;
    pOut->xCompress = dictCompress;
    pOut->xUncompress = dictUncompress;
    pOut->xCompressClose = dictCompressClose;
    pOut->pCtx = (void*)p;
  }else if( 0==strcmp(zHdr, "pass") ){
    /* do nothing */
  }else{
    return SQLITE_NOTFOUND;
  }

  return SQLITE_OK;
}

/*
** Tclcmd: v3_register NAME PARENT DEFAULT
*/
static int v3_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zName;
  const char *zParent;
  int bDefault;
  int rc;

  if( objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "NAME PARENT DEFAULT");
    return TCL_ERROR;
  }

  zName = Tcl_GetString(objv[1]);
  zParent = Tcl_GetString(objv[2]);
  if( !*zParent ) zParent = 0;
  if( Tcl_GetBooleanFromObj(interp, objv[3], &bDefault) ) return TCL_ERROR;

  rc = zipvfs_create_vfs_v3(zName, zParent, 0, testAutoDetect);

  if( rc==SQLITE_OK ){
    sqlite3_vfs_register(sqlite3_vfs_find(zName), bDefault);
    Tcl_ResetResult(interp);
  }else{
    Tcl_SetResult(interp, (char *)zipvfs_errmsg(rc), TCL_STATIC);
    return TCL_ERROR;
  }

  return TCL_OK;
}

/*
** Tclcmd: zipvfs_register DEFAULT
*/
static int zipvfs_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int bDefault;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DEFAULT");
    return TCL_ERROR;
  }
  if( Tcl_GetBooleanFromObj(interp, objv[1], &bDefault) ) return TCL_ERROR;

  zipvfsInit_v3(bDefault);
  Tcl_ResetResult(interp);
  return TCL_OK;
}

/*
** Tclcmd: zipvfs_unregister
*/
static int zipvfs_unregister_cmd(
  ClientData cd,
  Tcl_Interp *interp, 
  int objc, 
  Tcl_Obj *CONST objv[]
){
  if( objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }
  zipvfsShutdown_v3();
  return TCL_OK;
}

struct ZipvfsStructureTclCb {
  ZipvfsStructureCb base;
  Tcl_Interp *interp;
  Tcl_Obj *script;
};
static void xStructure(
  ZipvfsStructureCb *pArg,        /* Context object */
  unsigned int iPg,               /* Page number (or 0 for a free slot) */
  sqlite3_int64 iOff,             /* Offset in file */
  int nByte,                      /* Size of slot payload */
  int nPadding,                   /* Bytes of padding (0 for a free slot) */
  const char *zDesc               /* Description of free-list btree node */
){
  Tcl_Obj *pExec;                 /* Complete tcl command to evaluating */
  int rc;                         /* Result of evaluating pExec */
  struct ZipvfsStructureTclCb *p; /* Context object */

  p = (struct ZipvfsStructureTclCb *)pArg;
  pExec = Tcl_DuplicateObj(p->script);
  Tcl_IncrRefCount(pExec);
  Tcl_ListObjAppendElement(p->interp, pExec, Tcl_NewWideIntObj(iPg));
  Tcl_ListObjAppendElement(p->interp, pExec, Tcl_NewWideIntObj(iOff));
  Tcl_ListObjAppendElement(p->interp, pExec, Tcl_NewWideIntObj(nByte));
  Tcl_ListObjAppendElement(p->interp, pExec, Tcl_NewWideIntObj(nPadding));

  if( zDesc ){
    Tcl_ListObjAppendElement(p->interp, pExec, Tcl_NewStringObj(zDesc, -1));
  }else{
    Tcl_ListObjAppendElement(p->interp, pExec, Tcl_NewObj());
  }
  rc = Tcl_EvalObjEx(p->interp, pExec, TCL_EVAL_GLOBAL|TCL_EVAL_DIRECT);
  if( rc!=TCL_OK ) Tcl_BackgroundError(p->interp);
  Tcl_DecrRefCount(pExec);
}

/*
** Tclcmd: zip_unzip BLOB
*/
static int zip_unzip_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  u8 *aSrc;
  int nSrc;
  u8 *aDest;
  uLongf nDest;
  int rc;

  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "BLOB");
    return TCL_ERROR;
  }
  aSrc = Tcl_GetByteArrayFromObj(objv[1], &nSrc);

  nDest = 64*1024;
  aDest = (u8 *)ckalloc(nDest);
  rc = uncompress((Bytef*)aDest, &nDest, (Bytef*)aSrc, nSrc);
  if( rc!=Z_OK ){
    const char *zErr;
    switch( rc ){
      case Z_MEM_ERROR: zErr = "Z_MEM_ERROR"; break;
      case Z_BUF_ERROR: zErr = "Z_BUF_ERROR"; break;
      case Z_DATA_ERROR: zErr = "Z_DATA_ERROR"; break;
      default: zErr = "unknown error" ; break;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(zErr, -1));
    rc = TCL_ERROR;
  }else{
    Tcl_SetObjResult(interp, Tcl_NewByteArrayObj(aDest, nDest));
    rc = TCL_OK;
  }
  ckfree((char *)aDest);

  return rc;
}

/*
** The object passed as the second argument is assumed to contain the name
** of an SQLite database handle command. If successful, this function extracts
** the C API database handle (type sqlite3*) and writes it to *pDb before
** returning TCL_OK.
*/
static int getDatabaseHandle(Tcl_Interp *interp, Tcl_Obj *pObj, sqlite3 **pDb){
  Tcl_CmdInfo cmdInfo;            /* Command info structure for HANDLE */
  if( 0==Tcl_GetCommandInfo(interp, Tcl_GetString(pObj), &cmdInfo) ){
    Tcl_AppendResult(interp, "expected database handle, got \"", 0);
    Tcl_AppendResult(interp, Tcl_GetString(pObj), "\"", 0);
    return TCL_ERROR;
  }
  *pDb = *(sqlite3 **)cmdInfo.objClientData;
  return TCL_OK;
}

/*
** Tclcmd: zip_control HANDLE DBNAME SUB-COMMAND ?VALUE?
*/
static int zip_control_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int rc;                         /* Return code from file_control() */
  int idx;                        /* Index in aSub[] */
  sqlite3 *db;                    /* Underlying db handle for HANDLE */
  int iValue = 0;
  sqlite3_int64 iOut;
  int iOut32;
  sqlite3_int64 aOut64[2];
  void *pArg = 0;
  void **apArg[2] = {0, 0};

  struct SubCommand {
    const char *zName;
    int op;
    int argtype;
  } aSub[] = {
    { "compact",           ZIPVFS_CTRL_COMPACT,           5 },
    { "locking_mode",      ZIPVFS_CTRL_LOCKING_MODE,      6 },
    { "integrity_check",   ZIPVFS_CTRL_INTEGRITY_CHECK,   0 },
    { "detect_corruption", ZIPVFS_CTRL_DETECT_CORRUPTION, 1 },
    { "max_free",          ZIPVFS_CTRL_MAXFREE,           1 },
    { "append_freeslot",   ZIPVFS_CTRL_APPEND_FREESLOT,   1 },
    { "remove_freeslot",   ZIPVFS_CTRL_REMOVE_FREESLOT,   1 },
    { "max_frag",          ZIPVFS_CTRL_MAXFRAG,           1 },
    { "cache_size",        ZIPVFS_CTRL_CACHESIZE,         1 },
    { "stat",              ZIPVFS_CTRL_STAT,              2 },
    { "structure",         ZIPVFS_CTRL_STRUCTURE,         3 },
    { "write_hook",        ZIPVFS_CTRL_WRITE_HOOK,        4 },
    { "offset_and_size",   ZIPVFS_CTRL_OFFSET_AND_SIZE,   7 },
    { "replace",           ZIPVFS_CTRL_REPLACE,          12 },
    { "replace_init",      ZIPVFS_CTRL_REPLACE_INIT,     12 },
    { "replace_step",      ZIPVFS_CTRL_REPLACE_STEP,      1 },
    { "replace_finish",    ZIPVFS_CTRL_REPLACE_FINISH,    1 },
    { "replace_ndone",     ZIPVFS_CTRL_REPLACE_NDONE,    13 },
    { "replace_ntotal",    ZIPVFS_CTRL_REPLACE_NTOTAL,   13 },

    { "cache_used",        ZIPVFS_CTRL_CACHE_USED,        8 },
    { "cache_hit",         ZIPVFS_CTRL_CACHE_HIT,         9 },
    { "cache_write",       ZIPVFS_CTRL_CACHE_WRITE,       9 },
    { "cache_miss",        ZIPVFS_CTRL_CACHE_MISS,        9 },
    { "direct_read",       ZIPVFS_CTRL_DIRECT_READ,       9 },
    { "direct_bytes",      ZIPVFS_CTRL_DIRECT_BYTES,      9 },
    { "busy_handler",      SQLITE_FCNTL_BUSYHANDLER,     10 },
    { "overwrite",         SQLITE_FCNTL_OVERWRITE,        5 },
    { "sync",              SQLITE_FCNTL_SYNC,             0 },
    { "trace",             ZIPVFS_CTRL_TRACE,            11 },
    { 0, 0, 0 }
  };

  struct ZipvfsStructureTclCb tclcb;
  ZipvfsWriteCb writecb;
  ZipvfsStat stat;

  if( objc!=4 && objc!=5 ){
    Tcl_WrongNumArgs(interp, 1, objv, "HANDLE DBNAME SUB-COMMAND ?INT-VALUE?");
    return TCL_ERROR;
  }

  if( getDatabaseHandle(interp, objv[1], &db) ) return TCL_ERROR;

  rc = Tcl_GetIndexFromObjStruct(
      interp, objv[3], aSub, sizeof(aSub[0]), "sub-command", 0, &idx
  );
  if( rc!=TCL_OK ) return rc;

  switch( aSub[idx].argtype ){
    case 0: 
      if( objc!=4 ){ Tcl_WrongNumArgs(interp, 4, objv, ""); return TCL_ERROR; }
      pArg = 0;
      break;

    case 1:
      if( objc!=5 ){
        Tcl_WrongNumArgs(interp, 4, objv, "INT-VALUE");
        return TCL_ERROR;
      }
      if( Tcl_GetIntFromObj(interp, objv[4], &iValue) ) return TCL_ERROR;
      pArg = (void *)&iValue;
      break;

    case 2:
      if( objc!=4 ){ 
        Tcl_WrongNumArgs(interp, 4, objv, ""); 
        return TCL_ERROR; 
      }
      pArg = (void *)&stat;
      break;

    case 5:
      if( objc==5 ){ 
        if( Tcl_GetWideIntFromObj(interp, objv[4], &iOut) ) return TCL_ERROR;
        pArg = (void *)&iOut;
      }else if ( objc!=4 ){ 
        Tcl_WrongNumArgs(interp, 4, objv, "?INT-VALUE?"); 
        return TCL_ERROR; 
      }
      break;

    case 6:
      if( objc==5 ){ 
        if( Tcl_GetIntFromObj(interp, objv[4], &iOut32) ) return TCL_ERROR;
        pArg = (void *)&iOut32;
      }else if ( objc!=4 ){ 
        Tcl_WrongNumArgs(interp, 4, objv, "?INT-VALUE?"); 
        return TCL_ERROR; 
      }
      break;

    case 3:
      if( objc!=5 ){
        Tcl_WrongNumArgs(interp, 4, objv, "SCRIPT");
        return TCL_ERROR;
      }
      pArg = (void *)&tclcb;
      tclcb.base.x = xStructure;
      tclcb.interp = interp;
      tclcb.script = objv[4];
      break;

    case 4: {
      WriteCb *p;
      if( objc!=5 ){
        Tcl_WrongNumArgs(interp, 4, objv, "SCRIPT");
        return TCL_ERROR;
      }
      if( Tcl_GetCharLength(objv[4])==0 ){
        writecb.x = 0;
        writecb.xDestruct = 0;
        writecb.pCtx = 0;
      }else{
        p = sqlite3_malloc(sizeof(WriteCb));
        if( !p ) return SQLITE_NOMEM;
        p->interp = interp;
        p->pCmd = Tcl_DuplicateObj(objv[4]);
        Tcl_IncrRefCount(p->pCmd);
        writecb.x = zip_write_callback;
        writecb.xDestruct = zip_write_destroy;
        writecb.pCtx = (void *)p;
      }
      pArg = (void *)&writecb;
      break;
    }

    case 7:
      if( objc!=5 ){ 
        Tcl_WrongNumArgs(interp, 4, objv, "PGNO"); 
        return TCL_ERROR; 
      }
      if( Tcl_GetWideIntFromObj(interp, objv[4], &aOut64[0]) ) return TCL_ERROR;
      pArg = (void*)aOut64;
      break;

    case 8:
      if( objc!=4 ){ 
        Tcl_WrongNumArgs(interp, 4, objv, ""); 
        return TCL_ERROR; 
      }
      pArg = (void*)&iOut;
      break;

    case 9: {
      int b;
      if( objc!=5 ){ 
        Tcl_WrongNumArgs(interp, 4, objv, "RESET"); 
        return TCL_ERROR; 
      }
      if( Tcl_GetBooleanFromObj(interp, objv[4], &b) ) return TCL_ERROR;
      iOut = b;
      pArg = (void*)&iOut;
      break;
    }

    case 10: {
      if( objc!=4 ){ 
        Tcl_WrongNumArgs(interp, 4, objv, ""); 
        return TCL_ERROR; 
      }
      pArg = (void*)apArg;
      break;
    }

    case 11: {
      pArg = stderr;
      break;
    }

    case 12: {
      sqlite3 *dbFrom = 0;
      if( objc!=5 ){
        Tcl_WrongNumArgs(interp, 4, objv, "DB"); 
        return TCL_ERROR; 
      }
      if( getDatabaseHandle(interp, objv[4], &dbFrom) ) return TCL_ERROR;
      pArg = (void*)dbFrom;
      break;
    }

    case 13: {
      if( objc!=4 ){
        Tcl_WrongNumArgs(interp, 4, objv, ""); 
        return TCL_ERROR; 
      }
      pArg = (void*)&iOut32;
      break;
    }
  }

  rc = sqlite3_file_control(db, Tcl_GetString(objv[2]), aSub[idx].op, pArg);
  if( rc==SQLITE_OK ){
    if( pArg==(void *)&iOut32 ){
      Tcl_SetObjResult(interp, Tcl_NewIntObj(iOut32));
    }else if( pArg==(void *)&iOut ){
      Tcl_SetObjResult(interp, Tcl_NewWideIntObj(iOut));
    }else if( pArg==(void *)&stat ){
      Tcl_Obj *p = Tcl_NewObj();
      Tcl_ListObjAppendElement(interp, p, Tcl_NewStringObj("nFreeSlot", -1));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewIntObj(stat.nFreeSlot));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewStringObj("nFileByte", -1));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewWideIntObj(stat.nFileByte));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewStringObj("nFreeByte", -1));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewWideIntObj(stat.nFreeByte));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewStringObj("nFragByte", -1));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewWideIntObj(stat.nFragByte));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewStringObj("nGapByte", -1));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewWideIntObj(stat.nGapByte));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewStringObj("nContentByte",-1));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewWideIntObj(stat.nContentByte));
      Tcl_SetObjResult(interp, p);
    }else if( pArg==(void*)aOut64 ){
      Tcl_Obj *p = Tcl_NewObj();
      Tcl_ListObjAppendElement(interp, p, Tcl_NewWideIntObj(aOut64[0]));
      Tcl_ListObjAppendElement(interp, p, Tcl_NewWideIntObj(aOut64[1]));
      Tcl_SetObjResult(interp, p);
    }else{
      Tcl_ResetResult(interp);
    }
  }else if( rc==SQLITE_DONE ){
    Tcl_SetResult(interp, "SQLITE_DONE", TCL_VOLATILE);
  }else{
    Tcl_SetResult(interp, (char *)zipvfs_errmsg(rc), TCL_VOLATILE);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
** Tclcmd: zipvfs_vtab_register DB
*/
static int zipvfs_vtab_register_cmd(
  ClientData cd,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  sqlite3 *db;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DB");
    return TCL_ERROR;
  }
  if( getDatabaseHandle(interp, objv[1], &db) ) return TCL_ERROR;

  zipvfsRegisterModule(db);
  sqlite3_zipvfs_dictstore_register(db);
  Tcl_ResetResult(interp);
  return TCL_OK;
}


int Zipvfs_Init(Tcl_Interp *interp){
  Tcl_CreateObjCommand(interp, "zip_register", zip_register_cmd, 0, 0);
  Tcl_CreateObjCommand(interp, "rle_register", rle_register_cmd, 0, 0);
  Tcl_CreateObjCommand(interp, "noop_register", noop_register_cmd, 0, 0);
  Tcl_CreateObjCommand(interp, "padded_register", padded_register_cmd, 0, 0);
  Tcl_CreateObjCommand(interp, "v3_register", v3_register_cmd, 0, 0);

  /* Register the algorithms.c VFS */
  Tcl_CreateObjCommand(interp, "zipvfs_register", zipvfs_register_cmd, 0, 0);
  Tcl_CreateObjCommand(interp, "zipvfs_unregister", zipvfs_unregister_cmd, 0,0);

  /* Register the virtual table from zipvfs_vtab.c */
  Tcl_CreateObjCommand(
      interp, "zipvfs_vtab_register", zipvfs_vtab_register_cmd, 0, 0
  );

  Tcl_CreateObjCommand(interp, 
      "noop_unregister", vfs_unregister_cmd, (ClientData)"noop", 0);
  Tcl_CreateObjCommand(interp, 
      "rle_unregister", vfs_unregister_cmd, (ClientData)"rle", 0);
  Tcl_CreateObjCommand(interp, 
      "zip_unregister", vfs_unregister_cmd, (ClientData)"zip", 0);
  Tcl_CreateObjCommand(interp, 
      "padded_unregister", vfs_unregister_cmd, (ClientData)"padded", 0);

  Tcl_CreateObjCommand(interp, 
      "generic_unregister", generic_unregister_cmd, 0, 0);

  Tcl_CreateObjCommand(interp, "zip_control", zip_control_cmd, 0, 0);
  Tcl_CreateObjCommand(interp, "zip_unzip", zip_unzip_cmd, 0, 0);
  return TCL_OK;
}
