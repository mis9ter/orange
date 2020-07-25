/*
** 2015-05-02
**
** Copyright (C) 2015 Hipp, Wyrick & Company, Inc. (DBA: SQLite.com)
** All rights reserved.
**
*************************************************************************
** This file contains code to implement several compression algorithms
** that can be used with ZIPVFS.
**
** This file serves as an example of how to use ZIPVFS.  It is not the
** definitive way to use ZIPVFS; it is merely an example.  Actual applications
** are welcomed to copy and modify this file to suite their own 
** particular needs.
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#ifndef SQLITE_OMIT_ZLIB
# include <zlib.h>
#endif
#ifndef _ZIPVFS_H
# include "zipvfs.h"
#endif

/*
** Forward declarations of structures
*/
typedef struct ZipvfsInst ZipvfsInst;
typedef struct ZipvfsAlgorithm ZipvfsAlgorithm;

/*
** Each open connection to a ZIPVFS database has an instance of the following
** structure which holds context information for that connection.  This
** structure is automatically allocated and initialized when the database
** connection is opened and automatically freed when the database connection
** is closed.
**
** The pCrypto field holds context information for the encryption/decryption
** logic.
**
** The pEncode and pDecode fields hold information needed by the compressor
** and decompressor functions.
**
** Callback routines for doing encryption/decryption and compression and
** uncompression are always passed a pointer to the complete ZipvfsInst
** structure.  Those routines should then access the particular fields of
** ZipvfsInst structure that are relevant to them.
**
** The setup and breakdown routines (xComprSetup(), xComprCleanup(),
** xDecmprSetup() and xDecmprCleanup()) are also passed a pointer to the
** complete ZipvfsInst object.  Once again, these routines should setup
** or breakdown only those fields of the ZipvfsInst object that are relevant.
**
** The pCtx pointer is a copy of the original context pointer that was 
** passed in as the 3rd parameter to zipvfs_create_vfs_v3().
** This is currently always a NULL pointer.
*/
struct ZipvfsInst {
  void *pCtx;                     /* Context ptr to zipvfs_create_vfs_v3() */
  struct EncryptionInst *pCrypto; /* Info used by encryption/decryption */
  struct EncoderInst *pEncode;    /* Info used by compression */
  struct DecoderInst *pDecode;    /* Info used by decompression */
  const ZipvfsAlgorithm *pAlg;    /* Corresponding algorithm object */
  int nExtra;                     /* xBound() should return this much extra */
  int iLevel;                     /* Compression level */
};

/*
** An instance of the following structure describes a single ZIPVFS
** compression and encryption algorithm.  There is an array of instances
** of this object further down in this file that contains descriptions of
** all supported algorithms.
**
** zName                This is the name of the algorithm.  When creating
**                      a new database, use the "zv=NAME" query parameter
**                      to select an algorithm, where NAME matches this field.
**                      The name will written into the database header and
**                      so when reopening the database, the same algorithm
**                      will be used.
**
** xBound(X,N)          This function returns the maximum size of the output
**                      buffer for xCompr() assuming that the input is
**                      N bytes in size.  X is a pointer to the ZipvfsInst
**                      object for the open connection.
**
** xComprSetup(X)       This function is called once when the database is
**                      opened.  Its job is to setup the X->pEncode field
**                      appropriately.  Return SQLITE_OK on success or another
**                      error code (ex: SQLITE_NOMEM) upon error.
**
** xCompr(X,O,N,I,M)    This function does compression on input buffer I
**                      of size M bytes, and writes the output into buffer O
**                      and the output buffer size into N.
**
** xComprCleanup(X)     This function is called once when the database is
**                      closing.  Its job is to cleanup the X->pEncode field.
**                      This routine undoes the work of xComprSetup().
**
** xDecmprSetup(X)      This function is called once when the database is
**                      opened.  It should setup the X->pDecode field as
**                      is appropriate.
**
** xDecmpr(X,O,N,I,M)   This routine does decompression.  The input in buffer
**                      I which is M bytes in size is decompressed into an
**                      output buffer O.  The number of bytes of decompressed
**                      content should be written into N.
**
** xDecmprCleanup(X)    This function is called once when the database is
**                      closed in order to cleanup the X->pDecode field.
**                      This undoes the work of xDecmprSetup().
**
** xCryptoSetup(X,F)    This function is invoked once when the database is
**                      first opened, for the purpose of setting up the
**                      X->pCrypto field.  The F argument is the name of the
**                      file that is being opened.  This routine can use
**                      sqlite3_uri_parameter(F, "password") to determine
**                      the value of the password query parameter, if desired.
**                      
** xCryptoCleanup(X)    This routine is called when the database is closing
**                      in order to cleanup the X->pCrypto field.
*/
struct ZipvfsAlgorithm {
  const char zName[12];
  int (*xBound)(void*,int);
  int (*xComprSetup)(ZipvfsInst*);
  int (*xCompr)(void*,char*,int*,const char*,int);
  int (*xComprCleanup)(ZipvfsInst*);
  int (*xDecmprSetup)(ZipvfsInst*);
  int (*xDecmpr)(void*,char*,int*,const char*,int);
  int (*xDecmprCleanup)(ZipvfsInst*);
  int (*xCryptoSetup)(ZipvfsInst*, const char *zFile);
  int (*xCryptoCleanup)(ZipvfsInst*);
};

/************************ AES-128 encryption routines ***********************/
#define AES128_KEY_SZ 16     /* Size of the AES128 key in bytes */
#define AES256_KEY_SZ 32     /* Size of the AES256 key in bytes */
#define AES_KEY_SCHED_SZ 68  /* Space to hold the key schedule */
#define AES_BLOCK_SZ 16      /* Size of each encryption block */  

#ifndef SEEN_AES_TABLES
#define SEEN_AES_TABLES 1

typedef unsigned char u8;
typedef unsigned int u32;

/* Lookup tables for the AES algorithm */
static const u32 Te0[256] = {
    0xc66363a5U, 0xf87c7c84U, 0xee777799U, 0xf67b7b8dU,
    0xfff2f20dU, 0xd66b6bbdU, 0xde6f6fb1U, 0x91c5c554U,
    0x60303050U, 0x02010103U, 0xce6767a9U, 0x562b2b7dU,
    0xe7fefe19U, 0xb5d7d762U, 0x4dababe6U, 0xec76769aU,
    0x8fcaca45U, 0x1f82829dU, 0x89c9c940U, 0xfa7d7d87U,
    0xeffafa15U, 0xb25959ebU, 0x8e4747c9U, 0xfbf0f00bU,
    0x41adadecU, 0xb3d4d467U, 0x5fa2a2fdU, 0x45afafeaU,
    0x239c9cbfU, 0x53a4a4f7U, 0xe4727296U, 0x9bc0c05bU,
    0x75b7b7c2U, 0xe1fdfd1cU, 0x3d9393aeU, 0x4c26266aU,
    0x6c36365aU, 0x7e3f3f41U, 0xf5f7f702U, 0x83cccc4fU,
    0x6834345cU, 0x51a5a5f4U, 0xd1e5e534U, 0xf9f1f108U,
    0xe2717193U, 0xabd8d873U, 0x62313153U, 0x2a15153fU,
    0x0804040cU, 0x95c7c752U, 0x46232365U, 0x9dc3c35eU,
    0x30181828U, 0x379696a1U, 0x0a05050fU, 0x2f9a9ab5U,
    0x0e070709U, 0x24121236U, 0x1b80809bU, 0xdfe2e23dU,
    0xcdebeb26U, 0x4e272769U, 0x7fb2b2cdU, 0xea75759fU,
    0x1209091bU, 0x1d83839eU, 0x582c2c74U, 0x341a1a2eU,
    0x361b1b2dU, 0xdc6e6eb2U, 0xb45a5aeeU, 0x5ba0a0fbU,
    0xa45252f6U, 0x763b3b4dU, 0xb7d6d661U, 0x7db3b3ceU,
    0x5229297bU, 0xdde3e33eU, 0x5e2f2f71U, 0x13848497U,
    0xa65353f5U, 0xb9d1d168U, 0x00000000U, 0xc1eded2cU,
    0x40202060U, 0xe3fcfc1fU, 0x79b1b1c8U, 0xb65b5bedU,
    0xd46a6abeU, 0x8dcbcb46U, 0x67bebed9U, 0x7239394bU,
    0x944a4adeU, 0x984c4cd4U, 0xb05858e8U, 0x85cfcf4aU,
    0xbbd0d06bU, 0xc5efef2aU, 0x4faaaae5U, 0xedfbfb16U,
    0x864343c5U, 0x9a4d4dd7U, 0x66333355U, 0x11858594U,
    0x8a4545cfU, 0xe9f9f910U, 0x04020206U, 0xfe7f7f81U,
    0xa05050f0U, 0x783c3c44U, 0x259f9fbaU, 0x4ba8a8e3U,
    0xa25151f3U, 0x5da3a3feU, 0x804040c0U, 0x058f8f8aU,
    0x3f9292adU, 0x219d9dbcU, 0x70383848U, 0xf1f5f504U,
    0x63bcbcdfU, 0x77b6b6c1U, 0xafdada75U, 0x42212163U,
    0x20101030U, 0xe5ffff1aU, 0xfdf3f30eU, 0xbfd2d26dU,
    0x81cdcd4cU, 0x180c0c14U, 0x26131335U, 0xc3ecec2fU,
    0xbe5f5fe1U, 0x359797a2U, 0x884444ccU, 0x2e171739U,
    0x93c4c457U, 0x55a7a7f2U, 0xfc7e7e82U, 0x7a3d3d47U,
    0xc86464acU, 0xba5d5de7U, 0x3219192bU, 0xe6737395U,
    0xc06060a0U, 0x19818198U, 0x9e4f4fd1U, 0xa3dcdc7fU,
    0x44222266U, 0x542a2a7eU, 0x3b9090abU, 0x0b888883U,
    0x8c4646caU, 0xc7eeee29U, 0x6bb8b8d3U, 0x2814143cU,
    0xa7dede79U, 0xbc5e5ee2U, 0x160b0b1dU, 0xaddbdb76U,
    0xdbe0e03bU, 0x64323256U, 0x743a3a4eU, 0x140a0a1eU,
    0x924949dbU, 0x0c06060aU, 0x4824246cU, 0xb85c5ce4U,
    0x9fc2c25dU, 0xbdd3d36eU, 0x43acacefU, 0xc46262a6U,
    0x399191a8U, 0x319595a4U, 0xd3e4e437U, 0xf279798bU,
    0xd5e7e732U, 0x8bc8c843U, 0x6e373759U, 0xda6d6db7U,
    0x018d8d8cU, 0xb1d5d564U, 0x9c4e4ed2U, 0x49a9a9e0U,
    0xd86c6cb4U, 0xac5656faU, 0xf3f4f407U, 0xcfeaea25U,
    0xca6565afU, 0xf47a7a8eU, 0x47aeaee9U, 0x10080818U,
    0x6fbabad5U, 0xf0787888U, 0x4a25256fU, 0x5c2e2e72U,
    0x381c1c24U, 0x57a6a6f1U, 0x73b4b4c7U, 0x97c6c651U,
    0xcbe8e823U, 0xa1dddd7cU, 0xe874749cU, 0x3e1f1f21U,
    0x964b4bddU, 0x61bdbddcU, 0x0d8b8b86U, 0x0f8a8a85U,
    0xe0707090U, 0x7c3e3e42U, 0x71b5b5c4U, 0xcc6666aaU,
    0x904848d8U, 0x06030305U, 0xf7f6f601U, 0x1c0e0e12U,
    0xc26161a3U, 0x6a35355fU, 0xae5757f9U, 0x69b9b9d0U,
    0x17868691U, 0x99c1c158U, 0x3a1d1d27U, 0x279e9eb9U,
    0xd9e1e138U, 0xebf8f813U, 0x2b9898b3U, 0x22111133U,
    0xd26969bbU, 0xa9d9d970U, 0x078e8e89U, 0x339494a7U,
    0x2d9b9bb6U, 0x3c1e1e22U, 0x15878792U, 0xc9e9e920U,
    0x87cece49U, 0xaa5555ffU, 0x50282878U, 0xa5dfdf7aU,
    0x038c8c8fU, 0x59a1a1f8U, 0x09898980U, 0x1a0d0d17U,
    0x65bfbfdaU, 0xd7e6e631U, 0x844242c6U, 0xd06868b8U,
    0x824141c3U, 0x299999b0U, 0x5a2d2d77U, 0x1e0f0f11U,
    0x7bb0b0cbU, 0xa85454fcU, 0x6dbbbbd6U, 0x2c16163aU,
};
static const u32 Te1[256] = {
    0xa5c66363U, 0x84f87c7cU, 0x99ee7777U, 0x8df67b7bU,
    0x0dfff2f2U, 0xbdd66b6bU, 0xb1de6f6fU, 0x5491c5c5U,
    0x50603030U, 0x03020101U, 0xa9ce6767U, 0x7d562b2bU,
    0x19e7fefeU, 0x62b5d7d7U, 0xe64dababU, 0x9aec7676U,
    0x458fcacaU, 0x9d1f8282U, 0x4089c9c9U, 0x87fa7d7dU,
    0x15effafaU, 0xebb25959U, 0xc98e4747U, 0x0bfbf0f0U,
    0xec41adadU, 0x67b3d4d4U, 0xfd5fa2a2U, 0xea45afafU,
    0xbf239c9cU, 0xf753a4a4U, 0x96e47272U, 0x5b9bc0c0U,
    0xc275b7b7U, 0x1ce1fdfdU, 0xae3d9393U, 0x6a4c2626U,
    0x5a6c3636U, 0x417e3f3fU, 0x02f5f7f7U, 0x4f83ccccU,
    0x5c683434U, 0xf451a5a5U, 0x34d1e5e5U, 0x08f9f1f1U,
    0x93e27171U, 0x73abd8d8U, 0x53623131U, 0x3f2a1515U,
    0x0c080404U, 0x5295c7c7U, 0x65462323U, 0x5e9dc3c3U,
    0x28301818U, 0xa1379696U, 0x0f0a0505U, 0xb52f9a9aU,
    0x090e0707U, 0x36241212U, 0x9b1b8080U, 0x3ddfe2e2U,
    0x26cdebebU, 0x694e2727U, 0xcd7fb2b2U, 0x9fea7575U,
    0x1b120909U, 0x9e1d8383U, 0x74582c2cU, 0x2e341a1aU,
    0x2d361b1bU, 0xb2dc6e6eU, 0xeeb45a5aU, 0xfb5ba0a0U,
    0xf6a45252U, 0x4d763b3bU, 0x61b7d6d6U, 0xce7db3b3U,
    0x7b522929U, 0x3edde3e3U, 0x715e2f2fU, 0x97138484U,
    0xf5a65353U, 0x68b9d1d1U, 0x00000000U, 0x2cc1ededU,
    0x60402020U, 0x1fe3fcfcU, 0xc879b1b1U, 0xedb65b5bU,
    0xbed46a6aU, 0x468dcbcbU, 0xd967bebeU, 0x4b723939U,
    0xde944a4aU, 0xd4984c4cU, 0xe8b05858U, 0x4a85cfcfU,
    0x6bbbd0d0U, 0x2ac5efefU, 0xe54faaaaU, 0x16edfbfbU,
    0xc5864343U, 0xd79a4d4dU, 0x55663333U, 0x94118585U,
    0xcf8a4545U, 0x10e9f9f9U, 0x06040202U, 0x81fe7f7fU,
    0xf0a05050U, 0x44783c3cU, 0xba259f9fU, 0xe34ba8a8U,
    0xf3a25151U, 0xfe5da3a3U, 0xc0804040U, 0x8a058f8fU,
    0xad3f9292U, 0xbc219d9dU, 0x48703838U, 0x04f1f5f5U,
    0xdf63bcbcU, 0xc177b6b6U, 0x75afdadaU, 0x63422121U,
    0x30201010U, 0x1ae5ffffU, 0x0efdf3f3U, 0x6dbfd2d2U,
    0x4c81cdcdU, 0x14180c0cU, 0x35261313U, 0x2fc3ececU,
    0xe1be5f5fU, 0xa2359797U, 0xcc884444U, 0x392e1717U,
    0x5793c4c4U, 0xf255a7a7U, 0x82fc7e7eU, 0x477a3d3dU,
    0xacc86464U, 0xe7ba5d5dU, 0x2b321919U, 0x95e67373U,
    0xa0c06060U, 0x98198181U, 0xd19e4f4fU, 0x7fa3dcdcU,
    0x66442222U, 0x7e542a2aU, 0xab3b9090U, 0x830b8888U,
    0xca8c4646U, 0x29c7eeeeU, 0xd36bb8b8U, 0x3c281414U,
    0x79a7dedeU, 0xe2bc5e5eU, 0x1d160b0bU, 0x76addbdbU,
    0x3bdbe0e0U, 0x56643232U, 0x4e743a3aU, 0x1e140a0aU,
    0xdb924949U, 0x0a0c0606U, 0x6c482424U, 0xe4b85c5cU,
    0x5d9fc2c2U, 0x6ebdd3d3U, 0xef43acacU, 0xa6c46262U,
    0xa8399191U, 0xa4319595U, 0x37d3e4e4U, 0x8bf27979U,
    0x32d5e7e7U, 0x438bc8c8U, 0x596e3737U, 0xb7da6d6dU,
    0x8c018d8dU, 0x64b1d5d5U, 0xd29c4e4eU, 0xe049a9a9U,
    0xb4d86c6cU, 0xfaac5656U, 0x07f3f4f4U, 0x25cfeaeaU,
    0xafca6565U, 0x8ef47a7aU, 0xe947aeaeU, 0x18100808U,
    0xd56fbabaU, 0x88f07878U, 0x6f4a2525U, 0x725c2e2eU,
    0x24381c1cU, 0xf157a6a6U, 0xc773b4b4U, 0x5197c6c6U,
    0x23cbe8e8U, 0x7ca1ddddU, 0x9ce87474U, 0x213e1f1fU,
    0xdd964b4bU, 0xdc61bdbdU, 0x860d8b8bU, 0x850f8a8aU,
    0x90e07070U, 0x427c3e3eU, 0xc471b5b5U, 0xaacc6666U,
    0xd8904848U, 0x05060303U, 0x01f7f6f6U, 0x121c0e0eU,
    0xa3c26161U, 0x5f6a3535U, 0xf9ae5757U, 0xd069b9b9U,
    0x91178686U, 0x5899c1c1U, 0x273a1d1dU, 0xb9279e9eU,
    0x38d9e1e1U, 0x13ebf8f8U, 0xb32b9898U, 0x33221111U,
    0xbbd26969U, 0x70a9d9d9U, 0x89078e8eU, 0xa7339494U,
    0xb62d9b9bU, 0x223c1e1eU, 0x92158787U, 0x20c9e9e9U,
    0x4987ceceU, 0xffaa5555U, 0x78502828U, 0x7aa5dfdfU,
    0x8f038c8cU, 0xf859a1a1U, 0x80098989U, 0x171a0d0dU,
    0xda65bfbfU, 0x31d7e6e6U, 0xc6844242U, 0xb8d06868U,
    0xc3824141U, 0xb0299999U, 0x775a2d2dU, 0x111e0f0fU,
    0xcb7bb0b0U, 0xfca85454U, 0xd66dbbbbU, 0x3a2c1616U,
};
static const u32 Te2[256] = {
    0x63a5c663U, 0x7c84f87cU, 0x7799ee77U, 0x7b8df67bU,
    0xf20dfff2U, 0x6bbdd66bU, 0x6fb1de6fU, 0xc55491c5U,
    0x30506030U, 0x01030201U, 0x67a9ce67U, 0x2b7d562bU,
    0xfe19e7feU, 0xd762b5d7U, 0xabe64dabU, 0x769aec76U,
    0xca458fcaU, 0x829d1f82U, 0xc94089c9U, 0x7d87fa7dU,
    0xfa15effaU, 0x59ebb259U, 0x47c98e47U, 0xf00bfbf0U,
    0xadec41adU, 0xd467b3d4U, 0xa2fd5fa2U, 0xafea45afU,
    0x9cbf239cU, 0xa4f753a4U, 0x7296e472U, 0xc05b9bc0U,
    0xb7c275b7U, 0xfd1ce1fdU, 0x93ae3d93U, 0x266a4c26U,
    0x365a6c36U, 0x3f417e3fU, 0xf702f5f7U, 0xcc4f83ccU,
    0x345c6834U, 0xa5f451a5U, 0xe534d1e5U, 0xf108f9f1U,
    0x7193e271U, 0xd873abd8U, 0x31536231U, 0x153f2a15U,
    0x040c0804U, 0xc75295c7U, 0x23654623U, 0xc35e9dc3U,
    0x18283018U, 0x96a13796U, 0x050f0a05U, 0x9ab52f9aU,
    0x07090e07U, 0x12362412U, 0x809b1b80U, 0xe23ddfe2U,
    0xeb26cdebU, 0x27694e27U, 0xb2cd7fb2U, 0x759fea75U,
    0x091b1209U, 0x839e1d83U, 0x2c74582cU, 0x1a2e341aU,
    0x1b2d361bU, 0x6eb2dc6eU, 0x5aeeb45aU, 0xa0fb5ba0U,
    0x52f6a452U, 0x3b4d763bU, 0xd661b7d6U, 0xb3ce7db3U,
    0x297b5229U, 0xe33edde3U, 0x2f715e2fU, 0x84971384U,
    0x53f5a653U, 0xd168b9d1U, 0x00000000U, 0xed2cc1edU,
    0x20604020U, 0xfc1fe3fcU, 0xb1c879b1U, 0x5bedb65bU,
    0x6abed46aU, 0xcb468dcbU, 0xbed967beU, 0x394b7239U,
    0x4ade944aU, 0x4cd4984cU, 0x58e8b058U, 0xcf4a85cfU,
    0xd06bbbd0U, 0xef2ac5efU, 0xaae54faaU, 0xfb16edfbU,
    0x43c58643U, 0x4dd79a4dU, 0x33556633U, 0x85941185U,
    0x45cf8a45U, 0xf910e9f9U, 0x02060402U, 0x7f81fe7fU,
    0x50f0a050U, 0x3c44783cU, 0x9fba259fU, 0xa8e34ba8U,
    0x51f3a251U, 0xa3fe5da3U, 0x40c08040U, 0x8f8a058fU,
    0x92ad3f92U, 0x9dbc219dU, 0x38487038U, 0xf504f1f5U,
    0xbcdf63bcU, 0xb6c177b6U, 0xda75afdaU, 0x21634221U,
    0x10302010U, 0xff1ae5ffU, 0xf30efdf3U, 0xd26dbfd2U,
    0xcd4c81cdU, 0x0c14180cU, 0x13352613U, 0xec2fc3ecU,
    0x5fe1be5fU, 0x97a23597U, 0x44cc8844U, 0x17392e17U,
    0xc45793c4U, 0xa7f255a7U, 0x7e82fc7eU, 0x3d477a3dU,
    0x64acc864U, 0x5de7ba5dU, 0x192b3219U, 0x7395e673U,
    0x60a0c060U, 0x81981981U, 0x4fd19e4fU, 0xdc7fa3dcU,
    0x22664422U, 0x2a7e542aU, 0x90ab3b90U, 0x88830b88U,
    0x46ca8c46U, 0xee29c7eeU, 0xb8d36bb8U, 0x143c2814U,
    0xde79a7deU, 0x5ee2bc5eU, 0x0b1d160bU, 0xdb76addbU,
    0xe03bdbe0U, 0x32566432U, 0x3a4e743aU, 0x0a1e140aU,
    0x49db9249U, 0x060a0c06U, 0x246c4824U, 0x5ce4b85cU,
    0xc25d9fc2U, 0xd36ebdd3U, 0xacef43acU, 0x62a6c462U,
    0x91a83991U, 0x95a43195U, 0xe437d3e4U, 0x798bf279U,
    0xe732d5e7U, 0xc8438bc8U, 0x37596e37U, 0x6db7da6dU,
    0x8d8c018dU, 0xd564b1d5U, 0x4ed29c4eU, 0xa9e049a9U,
    0x6cb4d86cU, 0x56faac56U, 0xf407f3f4U, 0xea25cfeaU,
    0x65afca65U, 0x7a8ef47aU, 0xaee947aeU, 0x08181008U,
    0xbad56fbaU, 0x7888f078U, 0x256f4a25U, 0x2e725c2eU,
    0x1c24381cU, 0xa6f157a6U, 0xb4c773b4U, 0xc65197c6U,
    0xe823cbe8U, 0xdd7ca1ddU, 0x749ce874U, 0x1f213e1fU,
    0x4bdd964bU, 0xbddc61bdU, 0x8b860d8bU, 0x8a850f8aU,
    0x7090e070U, 0x3e427c3eU, 0xb5c471b5U, 0x66aacc66U,
    0x48d89048U, 0x03050603U, 0xf601f7f6U, 0x0e121c0eU,
    0x61a3c261U, 0x355f6a35U, 0x57f9ae57U, 0xb9d069b9U,
    0x86911786U, 0xc15899c1U, 0x1d273a1dU, 0x9eb9279eU,
    0xe138d9e1U, 0xf813ebf8U, 0x98b32b98U, 0x11332211U,
    0x69bbd269U, 0xd970a9d9U, 0x8e89078eU, 0x94a73394U,
    0x9bb62d9bU, 0x1e223c1eU, 0x87921587U, 0xe920c9e9U,
    0xce4987ceU, 0x55ffaa55U, 0x28785028U, 0xdf7aa5dfU,
    0x8c8f038cU, 0xa1f859a1U, 0x89800989U, 0x0d171a0dU,
    0xbfda65bfU, 0xe631d7e6U, 0x42c68442U, 0x68b8d068U,
    0x41c38241U, 0x99b02999U, 0x2d775a2dU, 0x0f111e0fU,
    0xb0cb7bb0U, 0x54fca854U, 0xbbd66dbbU, 0x163a2c16U,
};
static const u32 Te3[256] = {

    0x6363a5c6U, 0x7c7c84f8U, 0x777799eeU, 0x7b7b8df6U,
    0xf2f20dffU, 0x6b6bbdd6U, 0x6f6fb1deU, 0xc5c55491U,
    0x30305060U, 0x01010302U, 0x6767a9ceU, 0x2b2b7d56U,
    0xfefe19e7U, 0xd7d762b5U, 0xababe64dU, 0x76769aecU,
    0xcaca458fU, 0x82829d1fU, 0xc9c94089U, 0x7d7d87faU,
    0xfafa15efU, 0x5959ebb2U, 0x4747c98eU, 0xf0f00bfbU,
    0xadadec41U, 0xd4d467b3U, 0xa2a2fd5fU, 0xafafea45U,
    0x9c9cbf23U, 0xa4a4f753U, 0x727296e4U, 0xc0c05b9bU,
    0xb7b7c275U, 0xfdfd1ce1U, 0x9393ae3dU, 0x26266a4cU,
    0x36365a6cU, 0x3f3f417eU, 0xf7f702f5U, 0xcccc4f83U,
    0x34345c68U, 0xa5a5f451U, 0xe5e534d1U, 0xf1f108f9U,
    0x717193e2U, 0xd8d873abU, 0x31315362U, 0x15153f2aU,
    0x04040c08U, 0xc7c75295U, 0x23236546U, 0xc3c35e9dU,
    0x18182830U, 0x9696a137U, 0x05050f0aU, 0x9a9ab52fU,
    0x0707090eU, 0x12123624U, 0x80809b1bU, 0xe2e23ddfU,
    0xebeb26cdU, 0x2727694eU, 0xb2b2cd7fU, 0x75759feaU,
    0x09091b12U, 0x83839e1dU, 0x2c2c7458U, 0x1a1a2e34U,
    0x1b1b2d36U, 0x6e6eb2dcU, 0x5a5aeeb4U, 0xa0a0fb5bU,
    0x5252f6a4U, 0x3b3b4d76U, 0xd6d661b7U, 0xb3b3ce7dU,
    0x29297b52U, 0xe3e33eddU, 0x2f2f715eU, 0x84849713U,
    0x5353f5a6U, 0xd1d168b9U, 0x00000000U, 0xeded2cc1U,
    0x20206040U, 0xfcfc1fe3U, 0xb1b1c879U, 0x5b5bedb6U,
    0x6a6abed4U, 0xcbcb468dU, 0xbebed967U, 0x39394b72U,
    0x4a4ade94U, 0x4c4cd498U, 0x5858e8b0U, 0xcfcf4a85U,
    0xd0d06bbbU, 0xefef2ac5U, 0xaaaae54fU, 0xfbfb16edU,
    0x4343c586U, 0x4d4dd79aU, 0x33335566U, 0x85859411U,
    0x4545cf8aU, 0xf9f910e9U, 0x02020604U, 0x7f7f81feU,
    0x5050f0a0U, 0x3c3c4478U, 0x9f9fba25U, 0xa8a8e34bU,
    0x5151f3a2U, 0xa3a3fe5dU, 0x4040c080U, 0x8f8f8a05U,
    0x9292ad3fU, 0x9d9dbc21U, 0x38384870U, 0xf5f504f1U,
    0xbcbcdf63U, 0xb6b6c177U, 0xdada75afU, 0x21216342U,
    0x10103020U, 0xffff1ae5U, 0xf3f30efdU, 0xd2d26dbfU,
    0xcdcd4c81U, 0x0c0c1418U, 0x13133526U, 0xecec2fc3U,
    0x5f5fe1beU, 0x9797a235U, 0x4444cc88U, 0x1717392eU,
    0xc4c45793U, 0xa7a7f255U, 0x7e7e82fcU, 0x3d3d477aU,
    0x6464acc8U, 0x5d5de7baU, 0x19192b32U, 0x737395e6U,
    0x6060a0c0U, 0x81819819U, 0x4f4fd19eU, 0xdcdc7fa3U,
    0x22226644U, 0x2a2a7e54U, 0x9090ab3bU, 0x8888830bU,
    0x4646ca8cU, 0xeeee29c7U, 0xb8b8d36bU, 0x14143c28U,
    0xdede79a7U, 0x5e5ee2bcU, 0x0b0b1d16U, 0xdbdb76adU,
    0xe0e03bdbU, 0x32325664U, 0x3a3a4e74U, 0x0a0a1e14U,
    0x4949db92U, 0x06060a0cU, 0x24246c48U, 0x5c5ce4b8U,
    0xc2c25d9fU, 0xd3d36ebdU, 0xacacef43U, 0x6262a6c4U,
    0x9191a839U, 0x9595a431U, 0xe4e437d3U, 0x79798bf2U,
    0xe7e732d5U, 0xc8c8438bU, 0x3737596eU, 0x6d6db7daU,
    0x8d8d8c01U, 0xd5d564b1U, 0x4e4ed29cU, 0xa9a9e049U,
    0x6c6cb4d8U, 0x5656faacU, 0xf4f407f3U, 0xeaea25cfU,
    0x6565afcaU, 0x7a7a8ef4U, 0xaeaee947U, 0x08081810U,
    0xbabad56fU, 0x787888f0U, 0x25256f4aU, 0x2e2e725cU,
    0x1c1c2438U, 0xa6a6f157U, 0xb4b4c773U, 0xc6c65197U,
    0xe8e823cbU, 0xdddd7ca1U, 0x74749ce8U, 0x1f1f213eU,
    0x4b4bdd96U, 0xbdbddc61U, 0x8b8b860dU, 0x8a8a850fU,
    0x707090e0U, 0x3e3e427cU, 0xb5b5c471U, 0x6666aaccU,
    0x4848d890U, 0x03030506U, 0xf6f601f7U, 0x0e0e121cU,
    0x6161a3c2U, 0x35355f6aU, 0x5757f9aeU, 0xb9b9d069U,
    0x86869117U, 0xc1c15899U, 0x1d1d273aU, 0x9e9eb927U,
    0xe1e138d9U, 0xf8f813ebU, 0x9898b32bU, 0x11113322U,
    0x6969bbd2U, 0xd9d970a9U, 0x8e8e8907U, 0x9494a733U,
    0x9b9bb62dU, 0x1e1e223cU, 0x87879215U, 0xe9e920c9U,
    0xcece4987U, 0x5555ffaaU, 0x28287850U, 0xdfdf7aa5U,
    0x8c8c8f03U, 0xa1a1f859U, 0x89898009U, 0x0d0d171aU,
    0xbfbfda65U, 0xe6e631d7U, 0x4242c684U, 0x6868b8d0U,
    0x4141c382U, 0x9999b029U, 0x2d2d775aU, 0x0f0f111eU,
    0xb0b0cb7bU, 0x5454fca8U, 0xbbbbd66dU, 0x16163a2cU,
};
static const u32 Te4[256] = {
    0x63636363U, 0x7c7c7c7cU, 0x77777777U, 0x7b7b7b7bU,
    0xf2f2f2f2U, 0x6b6b6b6bU, 0x6f6f6f6fU, 0xc5c5c5c5U,
    0x30303030U, 0x01010101U, 0x67676767U, 0x2b2b2b2bU,
    0xfefefefeU, 0xd7d7d7d7U, 0xababababU, 0x76767676U,
    0xcacacacaU, 0x82828282U, 0xc9c9c9c9U, 0x7d7d7d7dU,
    0xfafafafaU, 0x59595959U, 0x47474747U, 0xf0f0f0f0U,
    0xadadadadU, 0xd4d4d4d4U, 0xa2a2a2a2U, 0xafafafafU,
    0x9c9c9c9cU, 0xa4a4a4a4U, 0x72727272U, 0xc0c0c0c0U,
    0xb7b7b7b7U, 0xfdfdfdfdU, 0x93939393U, 0x26262626U,
    0x36363636U, 0x3f3f3f3fU, 0xf7f7f7f7U, 0xccccccccU,
    0x34343434U, 0xa5a5a5a5U, 0xe5e5e5e5U, 0xf1f1f1f1U,
    0x71717171U, 0xd8d8d8d8U, 0x31313131U, 0x15151515U,
    0x04040404U, 0xc7c7c7c7U, 0x23232323U, 0xc3c3c3c3U,
    0x18181818U, 0x96969696U, 0x05050505U, 0x9a9a9a9aU,
    0x07070707U, 0x12121212U, 0x80808080U, 0xe2e2e2e2U,
    0xebebebebU, 0x27272727U, 0xb2b2b2b2U, 0x75757575U,
    0x09090909U, 0x83838383U, 0x2c2c2c2cU, 0x1a1a1a1aU,
    0x1b1b1b1bU, 0x6e6e6e6eU, 0x5a5a5a5aU, 0xa0a0a0a0U,
    0x52525252U, 0x3b3b3b3bU, 0xd6d6d6d6U, 0xb3b3b3b3U,
    0x29292929U, 0xe3e3e3e3U, 0x2f2f2f2fU, 0x84848484U,
    0x53535353U, 0xd1d1d1d1U, 0x00000000U, 0xededededU,
    0x20202020U, 0xfcfcfcfcU, 0xb1b1b1b1U, 0x5b5b5b5bU,
    0x6a6a6a6aU, 0xcbcbcbcbU, 0xbebebebeU, 0x39393939U,
    0x4a4a4a4aU, 0x4c4c4c4cU, 0x58585858U, 0xcfcfcfcfU,
    0xd0d0d0d0U, 0xefefefefU, 0xaaaaaaaaU, 0xfbfbfbfbU,
    0x43434343U, 0x4d4d4d4dU, 0x33333333U, 0x85858585U,
    0x45454545U, 0xf9f9f9f9U, 0x02020202U, 0x7f7f7f7fU,
    0x50505050U, 0x3c3c3c3cU, 0x9f9f9f9fU, 0xa8a8a8a8U,
    0x51515151U, 0xa3a3a3a3U, 0x40404040U, 0x8f8f8f8fU,
    0x92929292U, 0x9d9d9d9dU, 0x38383838U, 0xf5f5f5f5U,
    0xbcbcbcbcU, 0xb6b6b6b6U, 0xdadadadaU, 0x21212121U,
    0x10101010U, 0xffffffffU, 0xf3f3f3f3U, 0xd2d2d2d2U,
    0xcdcdcdcdU, 0x0c0c0c0cU, 0x13131313U, 0xececececU,
    0x5f5f5f5fU, 0x97979797U, 0x44444444U, 0x17171717U,
    0xc4c4c4c4U, 0xa7a7a7a7U, 0x7e7e7e7eU, 0x3d3d3d3dU,
    0x64646464U, 0x5d5d5d5dU, 0x19191919U, 0x73737373U,
    0x60606060U, 0x81818181U, 0x4f4f4f4fU, 0xdcdcdcdcU,
    0x22222222U, 0x2a2a2a2aU, 0x90909090U, 0x88888888U,
    0x46464646U, 0xeeeeeeeeU, 0xb8b8b8b8U, 0x14141414U,
    0xdedededeU, 0x5e5e5e5eU, 0x0b0b0b0bU, 0xdbdbdbdbU,
    0xe0e0e0e0U, 0x32323232U, 0x3a3a3a3aU, 0x0a0a0a0aU,
    0x49494949U, 0x06060606U, 0x24242424U, 0x5c5c5c5cU,
    0xc2c2c2c2U, 0xd3d3d3d3U, 0xacacacacU, 0x62626262U,
    0x91919191U, 0x95959595U, 0xe4e4e4e4U, 0x79797979U,
    0xe7e7e7e7U, 0xc8c8c8c8U, 0x37373737U, 0x6d6d6d6dU,
    0x8d8d8d8dU, 0xd5d5d5d5U, 0x4e4e4e4eU, 0xa9a9a9a9U,
    0x6c6c6c6cU, 0x56565656U, 0xf4f4f4f4U, 0xeaeaeaeaU,
    0x65656565U, 0x7a7a7a7aU, 0xaeaeaeaeU, 0x08080808U,
    0xbabababaU, 0x78787878U, 0x25252525U, 0x2e2e2e2eU,
    0x1c1c1c1cU, 0xa6a6a6a6U, 0xb4b4b4b4U, 0xc6c6c6c6U,
    0xe8e8e8e8U, 0xddddddddU, 0x74747474U, 0x1f1f1f1fU,
    0x4b4b4b4bU, 0xbdbdbdbdU, 0x8b8b8b8bU, 0x8a8a8a8aU,
    0x70707070U, 0x3e3e3e3eU, 0xb5b5b5b5U, 0x66666666U,
    0x48484848U, 0x03030303U, 0xf6f6f6f6U, 0x0e0e0e0eU,
    0x61616161U, 0x35353535U, 0x57575757U, 0xb9b9b9b9U,
    0x86868686U, 0xc1c1c1c1U, 0x1d1d1d1dU, 0x9e9e9e9eU,
    0xe1e1e1e1U, 0xf8f8f8f8U, 0x98989898U, 0x11111111U,
    0x69696969U, 0xd9d9d9d9U, 0x8e8e8e8eU, 0x94949494U,
    0x9b9b9b9bU, 0x1e1e1e1eU, 0x87878787U, 0xe9e9e9e9U,
    0xcecececeU, 0x55555555U, 0x28282828U, 0xdfdfdfdfU,
    0x8c8c8c8cU, 0xa1a1a1a1U, 0x89898989U, 0x0d0d0d0dU,
    0xbfbfbfbfU, 0xe6e6e6e6U, 0x42424242U, 0x68686868U,
    0x41414141U, 0x99999999U, 0x2d2d2d2dU, 0x0f0f0f0fU,
    0xb0b0b0b0U, 0x54545454U, 0xbbbbbbbbU, 0x16161616U,
};
static const u32 rcon[] = {
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000,
    0x1B000000, 0x36000000,
    /* for 128-bit blocks, Rijndael never uses more than 10 rcon values */
};

/*
** Macros for reading and writing 32-bit values in a byte-order
** independent way.  Used by AES
*/
#if SQLITE_BYTEORDER==4321
#define GETU32(p)       (*(u32*)(p))
#define PUTU32(ct, st)  { *((u32*)(ct)) = (st); }
#elif SQLITE_BYTEORDER==1234 && !defined(SQLITE_DISABLE_INTRINSIC) \
    && defined(__GNUC__) && GCC_VERSION>=4003000
#define GETU32(p)       __builtin_bswap32(*(u32*)(p))
#define PUTU32(ct, st)  { *((u32*)(ct)) = __builtin_bswap32(st); }
#elif SQLITE_BYTEORDER==1234 && !defined(SQLITE_DISABLE_INTRINSIC) \
    && defined(_MSC_VER) && _MSC_VER>=1300
#define GETU32(p)       _byteswap_ulong(*(u32*)(p))
#define PUTU32(ct, st)  { *((u32*)(ct)) = _byteswap_ulong(st); }
#elif defined(_MSC_VER) && !defined(_WIN32_WCE)
#define CXSWAP(x) (_lrotl(x, 8) & 0x00ff00ff | _lrotr(x, 8) & 0xff00ff00)
#define GETU32(p) CXSWAP(*((u32 *)(p)))
#define PUTU32(ct, st) { *((u32 *)(ct)) = CXSWAP((st)); }
#else
#define GETU32(pt) (((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] <<  8) ^ ((u32)(pt)[3]))
#define PUTU32(ct, st) { (ct)[0] = (u8)((st)>>24); (ct)[1] = (u8)((st)>>16); (ct)[2] = (u8)((st)>> 8); (ct)[3] = (u8)(st); }
#endif

#endif /* SEEN_AES_TABLES */

/* Only include the AES256 implementation once */
#ifndef SEEN_AES256_CORE
#define SEEN_AES256_CORE 1

/*
** Set up the AES key schedule given a 256-bit (32-byte) key in cipherKey[]
*/
static void rijndaelKeySetupEnc256(u32 *rk, const u8 *cipherKey){
  int i = 0;
  u32 temp;

  rk[0] = GETU32(cipherKey     );
  rk[1] = GETU32(cipherKey +  4);
  rk[2] = GETU32(cipherKey +  8);
  rk[3] = GETU32(cipherKey + 12);
  rk[4] = GETU32(cipherKey + 16);
  rk[5] = GETU32(cipherKey + 20);
  rk[6] = GETU32(cipherKey + 24);
  rk[7] = GETU32(cipherKey + 28);
  for (;;) {
    temp = rk[ 7];
    rk[ 8] = rk[ 0] ^
      (Te4[(temp >> 16) & 0xff] & 0xff000000) ^
      (Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
      (Te4[(temp      ) & 0xff] & 0x0000ff00) ^
      (Te4[(temp >> 24)       ] & 0x000000ff) ^
      rcon[i];
    rk[ 9] = rk[ 1] ^ rk[ 8];
    rk[10] = rk[ 2] ^ rk[ 9];
    rk[11] = rk[ 3] ^ rk[10];
    if (++i == 7) {
      return;
    }
    temp = rk[11];
    rk[12] = rk[ 4] ^
      (Te4[(temp >> 24)       ] & 0xff000000) ^
      (Te4[(temp >> 16) & 0xff] & 0x00ff0000) ^
      (Te4[(temp >>  8) & 0xff] & 0x0000ff00) ^
      (Te4[(temp      ) & 0xff] & 0x000000ff);
    rk[13] = rk[ 5] ^ rk[12];
    rk[14] = rk[ 6] ^ rk[13];
    rk[15] = rk[ 7] ^ rk[14];
    rk += 8;
  }
}

/*
** Encrypt a single block pt[].  Store the output in ct[].
** 10 rounds.  Block size 128 bits.
*/
static void rijndaelEncrypt256(const u32 *rk, const u8 pt[16], u8 ct[16]) {
  u32 s0, s1, s2, s3, t0, t1, t2, t3;

  /*
   * map byte array block to cipher state
   * and add initial round key:
   */
  s0 = GETU32(pt     ) ^ rk[0];
  s1 = GETU32(pt +  4) ^ rk[1];
  s2 = GETU32(pt +  8) ^ rk[2];
  s3 = GETU32(pt + 12) ^ rk[3];
  /* round 1: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[ 4];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[ 5];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[ 6];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[ 7];
  /* round 2: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[ 8];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[ 9];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[10];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[11];
  /* round 3: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[12];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[13];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[14];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[15];
  /* round 4: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[16];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[17];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[18];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[19];
  /* round 5: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[20];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[21];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[22];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[23];
  /* round 6: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[24];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[25];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[26];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[27];
  /* round 7: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[28];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[29];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[30];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[31];
  /* round 8: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[32];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[33];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[34];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[35];
  /* round 9: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[36];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[37];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[38];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[39];
  /* round 10: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[40];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[41];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[42];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[43];
  /* round 11: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[44];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[45];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[46];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[47];
  /* round 12: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[48];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[49];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[50];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[51];
  /* round 13: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[52];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[53];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[54];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[55];
  rk += 56;
  /*
   * apply last round and
   * map cipher state to byte array block:
   */
  s0 =
    (Te4[(t0 >> 24)       ] & 0xff000000) ^
    (Te4[(t1 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t2 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t3      ) & 0xff] & 0x000000ff) ^
    rk[0];
  PUTU32(ct     , s0);
  s1 =
    (Te4[(t1 >> 24)       ] & 0xff000000) ^
    (Te4[(t2 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t3 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t0      ) & 0xff] & 0x000000ff) ^
    rk[1];
  PUTU32(ct +  4, s1);
  s2 =
    (Te4[(t2 >> 24)       ] & 0xff000000) ^
    (Te4[(t3 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t0 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t1      ) & 0xff] & 0x000000ff) ^
    rk[2];
  PUTU32(ct +  8, s2);
  s3 =
    (Te4[(t3 >> 24)       ] & 0xff000000) ^
    (Te4[(t0 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t1 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t2      ) & 0xff] & 0x000000ff) ^
    rk[3];
  PUTU32(ct + 12, s3);
}
#endif /* SEEN_AES256_CORE */


/* Only include the AES128 implementation once */
#ifndef SEEN_AES128_CORE
#define SEEN_AES128_CORE 1

/*
** Set up the AES key schedule given a 128-bit (16-byte) key in cipherKey[]
*/
static int rijndaelKeySetupEnc128(u32 *rk, const u8 *cipherKey){
  int i = 0;
  u32 temp;

  rk[0] = GETU32(cipherKey     );
  rk[1] = GETU32(cipherKey +  4);
  rk[2] = GETU32(cipherKey +  8);
  rk[3] = GETU32(cipherKey + 12);
  for (;;) {
    temp  = rk[3];
    rk[4] = rk[0] ^
      (Te4[(temp >> 16) & 0xff] & 0xff000000) ^
      (Te4[(temp >>  8) & 0xff] & 0x00ff0000) ^
      (Te4[(temp      ) & 0xff] & 0x0000ff00) ^
      (Te4[(temp >> 24)       ] & 0x000000ff) ^
      rcon[i];
    rk[5] = rk[1] ^ rk[4];
    rk[6] = rk[2] ^ rk[5];
    rk[7] = rk[3] ^ rk[6];
    if (++i == 10) {
      return 10;
    }
    rk += 4;
  }
}

/*
** Encrypt a single block pt[].  Store the output in ct[].
** 10 rounds.  Block size 128 bits.
*/
static void rijndaelEncrypt128(const u32 *rk, const u8 pt[16], u8 ct[16]) {
  u32 s0, s1, s2, s3, t0, t1, t2, t3;

  /*
   * map byte array block to cipher state
   * and add initial round key:
   */
  s0 = GETU32(pt     ) ^ rk[0];
  s1 = GETU32(pt +  4) ^ rk[1];
  s2 = GETU32(pt +  8) ^ rk[2];
  s3 = GETU32(pt + 12) ^ rk[3];
  /* round 1: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[ 4];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[ 5];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[ 6];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[ 7];
  /* round 2: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[ 8];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[ 9];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[10];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[11];
  /* round 3: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[12];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[13];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[14];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[15];
  /* round 4: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[16];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[17];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[18];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[19];
  /* round 5: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[20];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[21];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[22];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[23];
  /* round 6: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[24];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[25];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[26];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[27];
  /* round 7: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[28];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[29];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[30];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[31];
  /* round 8: */
  s0 = Te0[t0>>24]^Te1[(t1>>16)&0xff]^Te2[(t2>> 8)&0xff]^Te3[t3&0xff]^rk[32];
  s1 = Te0[t1>>24]^Te1[(t2>>16)&0xff]^Te2[(t3>> 8)&0xff]^Te3[t0&0xff]^rk[33];
  s2 = Te0[t2>>24]^Te1[(t3>>16)&0xff]^Te2[(t0>> 8)&0xff]^Te3[t1&0xff]^rk[34];
  s3 = Te0[t3>>24]^Te1[(t0>>16)&0xff]^Te2[(t1>> 8)&0xff]^Te3[t2&0xff]^rk[35];
  /* round 9: */
  t0 = Te0[s0>>24]^Te1[(s1>>16)&0xff]^Te2[(s2>> 8)&0xff]^Te3[s3&0xff]^rk[36];
  t1 = Te0[s1>>24]^Te1[(s2>>16)&0xff]^Te2[(s3>> 8)&0xff]^Te3[s0&0xff]^rk[37];
  t2 = Te0[s2>>24]^Te1[(s3>>16)&0xff]^Te2[(s0>> 8)&0xff]^Te3[s1&0xff]^rk[38];
  t3 = Te0[s3>>24]^Te1[(s0>>16)&0xff]^Te2[(s1>> 8)&0xff]^Te3[s2&0xff]^rk[39];
  rk += 40;
  /*
   * apply last round and
   * map cipher state to byte array block:
   */
  s0 =
    (Te4[(t0 >> 24)       ] & 0xff000000) ^
    (Te4[(t1 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t2 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t3      ) & 0xff] & 0x000000ff) ^
    rk[0];
  PUTU32(ct     , s0);
  s1 =
    (Te4[(t1 >> 24)       ] & 0xff000000) ^
    (Te4[(t2 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t3 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t0      ) & 0xff] & 0x000000ff) ^
    rk[1];
  PUTU32(ct +  4, s1);
  s2 =
    (Te4[(t2 >> 24)       ] & 0xff000000) ^
    (Te4[(t3 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t0 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t1      ) & 0xff] & 0x000000ff) ^
    rk[2];
  PUTU32(ct +  8, s2);
  s3 =
    (Te4[(t3 >> 24)       ] & 0xff000000) ^
    (Te4[(t0 >> 16) & 0xff] & 0x00ff0000) ^
    (Te4[(t1 >>  8) & 0xff] & 0x0000ff00) ^
    (Te4[(t2      ) & 0xff] & 0x000000ff) ^
    rk[3];
  PUTU32(ct + 12, s3);
}
#endif /* SEEN_AES128_CORE */
/********************* End Of AES-128 encryption routines ********************/


/*****************************************************************************
** If the ZIPVFS module is started using a URI filename that contains
** a "password=" query parameter, then that password is used with 
** AES-128 to encrypt the content.  If the URI filename contains
** a "password256=" query parameter, then use AES-256.
**
** The aesEncryptionSetup() and aesEncryptionCleanup() routines are setup
** and breakdown routines for the encryption logic.
*/

/*
** Context for the AES encryption 
*/
typedef struct AesEncryptInst AesEncryptInst;
struct AesEncryptInst {
  int nTmp;                           /* Size of aTmp[] */
  char *aTmp;                         /* Temporary space to decrypt into */
  void (*xEncrypt)(AesEncryptInst*,char*,int*);              /* Encryption */
  void (*xDecrypt)(AesEncryptInst*,char*,const char*,int*);  /* Decryption */
  u32 keySchedule[AES_KEY_SCHED_SZ];  /* Rijndael key schedule */
};

/* This routine is called by ZIPVFS when a database connection is shutting
** down in order to deallocate resources previously allocated by
** aesCompressOpen().
*/
static int aesEncryptionCleanup(ZipvfsInst *p){
  if( p->pCrypto ){
    AesEncryptInst *pInst = (AesEncryptInst*)p->pCrypto;
    sqlite3_free(pInst->aTmp);
    sqlite3_free(pInst);
  }
  p->pCrypto = 0;
  return SQLITE_OK;
}

/* 
** Encryption using 128-bit rijndael.
**
** Output Feedback (OFB) is used.  A 16-byte initial value (IV) is obtained
** from sqlite3_randomness() and is stored at the end of the page.  Hence,
** the size of the page grows by 16 bytes as it is encrypted.
*/
static void aes128Encryption(
  AesEncryptInst *pInst,  /* ZIPVFS algorithm context */
  char *zOut,             /* Write the results here */
  int *pnIn               /* Number of bytes to encrypt */
){
  int i, j, n;
  unsigned char iv[AES128_KEY_SZ];
  sqlite3_randomness(AES128_KEY_SZ, iv);
  memcpy(zOut + *pnIn, iv, AES128_KEY_SZ);
  n = *pnIn;
  *pnIn += AES128_KEY_SZ;
  for(i=0, j=AES128_KEY_SZ; i<n; i++, j++){
    if( j>=AES128_KEY_SZ ){
      rijndaelEncrypt128(pInst->keySchedule, iv, iv);
      j = 0;
    }
    zOut[i] ^= iv[j];
  }
}

/* 
** Encryption using 256-bit rijndael.
**
** Output Feedback (OFB) is used.  A 32-byte initial value (IV) is obtained
** from sqlite3_randomness() and is stored at the end of the page.  Hence,
** the size of the page grows by 32 bytes as it is encrypted.
*/

static void aes256Encryption(
  AesEncryptInst *pInst,  /* ZIPVFS algorithm context */
  char *zOut,             /* Write the results here */
  int *pnIn               /* Number of bytes to encrypt */
){
  int i, j, n;
  unsigned char iv[AES256_KEY_SZ];
  sqlite3_randomness(AES256_KEY_SZ, iv);
  memcpy(zOut + *pnIn, iv, AES256_KEY_SZ);
  n = *pnIn;
  *pnIn += AES256_KEY_SZ;
  for(i=0, j=AES256_KEY_SZ; i<n; i++, j++){
    if( j>=AES256_KEY_SZ ){
      rijndaelEncrypt256(pInst->keySchedule, iv, iv);
      j = 0;
    }
    zOut[i]  ^= iv[j];
  }
}

/*
** This is a wrapper routine around xDecrypt().  It allocates space to do 
** the decryption so that the input, aIn[], is not altered.   A pointer to
** the decrypted text is returned.  NULL is returned if there is a memory
** allocation error.
*/
static void zipvfsEncryptWrapper(
  ZipvfsInst *p,       /* ZIPVFS algorithm context */
  char *zOut,          /* Read cleartext and overwrite with cyphertext */
  int *pnIn            /* Byte of cleartext in.  Bytes cyphertext out */
){
  if( p->pCrypto ){
    AesEncryptInst *pInst = (AesEncryptInst*)p->pCrypto;
    pInst->xEncrypt(pInst, zOut, pnIn);
  }   
}

/*
** Decryption using 128-bit rijndael.
**
** Output Feedback (OFB) is used.  The initial value (IV) is the final
** 16 bytes in the record.  The record size is reduced by 16 byte by
** decryption, since the IV is removed.
*/
static void aes128Decryption(
  AesEncryptInst *pInst,  /* Information about how to encrypt */
  char *aOut,             /* Store results here */
  const char *aIn,        /* Buffer to be decrypted */
  int *pnIn               /* Size of the input buffer */
){
  int i, j, n;
  unsigned char iv[AES128_KEY_SZ];
  *pnIn -= AES128_KEY_SZ;
  n = *pnIn;
  assert( n<=pInst->nTmp );
  aOut = pInst->aTmp;
  memcpy(iv, aIn+n, AES128_KEY_SZ);
  for(i=0, j=AES128_KEY_SZ; i<n; i++, j++){
    if( j>=AES128_KEY_SZ ){
      rijndaelEncrypt128(pInst->keySchedule, iv, iv);
      j = 0;
    }
    aOut[i] = aIn[i] ^ iv[j];
  }
}

/*
** Decryption using 256-bit rijndael.
**
** Output Feedback (OFB) is used.  The initial value (IV) is the final
** 16 bytes in the record.  The record size is reduced by 16 byte by
** decryption, since the IV is removed.
*/
static void aes256Decryption(
  AesEncryptInst *pInst,  /* Information about how to encrypt */
  char *aOut,             /* Store results here */
  const char *aIn,        /* Buffer to be decrypted */
  int *pnIn               /* Size of the input buffer */
){
  int i, j, n;
  unsigned char iv[AES256_KEY_SZ];
  *pnIn -= AES256_KEY_SZ;
  n = *pnIn;
  assert( n<=pInst->nTmp );
  aOut = pInst->aTmp;
  memcpy(iv, aIn+n, AES256_KEY_SZ);
  for(i=0, j=AES256_KEY_SZ; i<n; i++, j++){
    if( j>=AES256_KEY_SZ ){
      rijndaelEncrypt256(pInst->keySchedule, iv, iv);
      j = 0;
    }
    aOut[i] = aIn[i] ^ iv[j];
  }
}

/*
** This is a wrapper routine around xDecrypt().  It allocates space to do 
** the decryption so that the input, aIn[], is not altered.   A pointer to
** the decrypted text is returned.  NULL is returned if there is a memory
** allocation error.
*/
const char *zipvfsDecryptWrapper(
  ZipvfsInst *p,          /* The open ZIPVFS connection */
  const char *aIn,        /* Buffer to be decrypted */
  int *pnIn               /* Size of the input buffer */
){
  if( p->pCrypto ){
    AesEncryptInst *pInst = (AesEncryptInst*)p->pCrypto;
    int n = *pnIn;
    if( pInst->nTmp<n ){
      int nNew = n + (n/10);
      char *aNew = sqlite3_realloc(pInst->aTmp, nNew);
      if( aNew==0 ) return 0;
      pInst->aTmp = aNew;
      pInst->nTmp = nNew;
    }
    pInst->xDecrypt(pInst, pInst->aTmp, aIn, pnIn);
    aIn = pInst->aTmp;
  }   
  return aIn;
}

/*
** The aesEncryptionSetup() routine determines whether or not encryption
** should be used for the database file.   The aesEncryptionSetup() routine
** is called directly from ZIPVFS if ZIPVFS was registered using
** zipvfs_create_vfs_v2().  If ZIPVFS was registered using the newer
** zipvfs_create_vfs_v3() interface then ZIPVFS will never invoke this
** routine itself; the xAutoDetect callback supplied by the application
** should invoke this routine instead.  
*/
static int aesEncryptionSetup(
  ZipvfsInst *p,           /* The ZipvfsInst object for the chosen algorithm */
  const char *zFilename    /* Name of the file being opened */
){
  const char *zPasswd;

  if( (zPasswd = sqlite3_uri_parameter(zFilename, "password"))!=0 ){
    /* If the database connection is being opened with a URI filename
    ** and there is a password= query parameter on the URI, then set
    ** the local context pointer to a suitable EncryptionInst object.
    */
    AesEncryptInst *pInst;
    int nKey = (int)strlen(zPasswd) + 1;
    int i, n;
    unsigned char aKey[AES128_KEY_SZ];

    pInst = (struct AesEncryptInst*)sqlite3_malloc( sizeof(*pInst) );
    if( pInst==0 ) return SQLITE_NOMEM;
    n = AES128_KEY_SZ;
    if( nKey>n ) n = nKey;
    for(i=0; i<n; i++) aKey[i%AES128_KEY_SZ] = zPasswd[i%nKey];
    rijndaelKeySetupEnc128(pInst->keySchedule, aKey);
    pInst->nTmp = 0;
    pInst->aTmp = 0;
    pInst->xDecrypt = aes128Decryption;
    pInst->xEncrypt = aes128Encryption;
    p->pCrypto = (struct EncryptionInst*)pInst;
    p->nExtra = AES128_KEY_SZ;
  }else if( (zPasswd = sqlite3_uri_parameter(zFilename, "password256"))!=0 ){
    /* If the database connection is being opened with a URI filename
    ** and there is a password= query parameter on the URI, then set
    ** the local context pointer to a suitable EncryptionInst object.
    */
    AesEncryptInst *pInst;
    int nKey = (int)strlen(zPasswd) + 1;
    int i, n;
    unsigned char aKey[AES256_KEY_SZ];

    pInst = (struct AesEncryptInst*)sqlite3_malloc( sizeof(*pInst) );
    if( pInst==0 ) return SQLITE_NOMEM;
    n = AES256_KEY_SZ;
    if( nKey>n ) n = nKey;
    for(i=0; i<n; i++) aKey[i%AES256_KEY_SZ] = zPasswd[i%nKey];
    rijndaelKeySetupEnc256(pInst->keySchedule, aKey);
    pInst->nTmp = 0;
    pInst->aTmp = 0;
    pInst->xDecrypt = aes256Decryption;
    pInst->xEncrypt = aes256Encryption;
    p->pCrypto = (struct EncryptionInst*)pInst;
    p->nExtra = AES256_KEY_SZ;
  }else{
    /* If not using URIs or if there is no password, then the local
    ** context pointer is NULL.
    */
    p->pCrypto = 0;
    p->nExtra = 0;
  }
  return SQLITE_OK;
}
/* End encryption logic
******************************************************************************/

/******************************************************************************
** RLE (run-length encoding) compression routines for use with ZIPVFS
**
** This is a very simple compression algorithm in which between 1
** and 256 0x00 bytes are represented as two bytes: 0x00, N-1
** where N is the number of consecutive 0x00 bytes.
**
** Maximum compression growth is for a buffer with alternating 0x00
** and non-0x00 bytes; maximum size is ((n+1)/2)*2 + n/2 if n is the
** initial size.
*/
static int rleBound(void *pLocalCtx, int n){
  return ((n+1)/2)*2 + n/2 + ((ZipvfsInst*)pLocalCtx)->nExtra;
}
static int rleCompress(
  void *pLocalCtx,
  char *aOut, int *pnOut,
  const char *aIn,  int nIn
){
  int i, j, k;
  ZipvfsInst *p = (ZipvfsInst*)pLocalCtx;
  for(i=j=0; i<nIn; i++){
    if( (aOut[j++] = aIn[i])==0 ){
      for(k=0; k<255 && k+i+1<nIn && aIn[k+i+1]==0; k++){}
      aOut[j++] = (char)k;
      i += k;
    }
  }
  zipvfsEncryptWrapper(p, aOut, &j);
  *pnOut = j;
  return SQLITE_OK;
}
static int rleUncompress(
  void *pLocalCtx,
  char *aOut, int *pnOut,
  const char *aIn,  int nIn
){
  int i, j, k;
  ZipvfsInst *p = (ZipvfsInst*)pLocalCtx;
  int nOut = *pnOut;

  aIn = zipvfsDecryptWrapper(p, aIn, &nIn);
  if( aIn==0 ) return SQLITE_NOMEM;
  for(i=j=0; i<nIn && j<nOut; i++){
    if( (aOut[j++] = aIn[i])==0 ){
      k = ((unsigned char*)aIn)[++i];
      while( (k--)>0 && j<nOut ){ aOut[j++] = 0; }
    }
  }
  *pnOut = j;
  return SQLITE_OK;
}

/* The RLE algorithm does not require setup or cleanup routines.
** The following Setup() and Cleanup() routines are here purely
** to exercise the Setup() and Cleanup() logic and to verify that the
** Setup() and Cleanup() routines are called at the appropriate times.
*/
static int rleCompressSetup(ZipvfsInst *p){
  assert( p->pEncode==0 );
  p->pEncode = (struct EncoderInst*)sqlite3_malloc( 11 );
  if( p->pEncode==0 ) return SQLITE_NOMEM;
  memcpy(p->pEncode, "rle-encode", 11);
  return SQLITE_OK;
}
static int rleCompressCleanup(ZipvfsInst *p){
  /* p->pEncode can be NULL if rleCompressSetup() hit an OOM error */
  assert( p->pEncode==0 || memcmp(p->pEncode, "rle-encode", 11)==0 );
  sqlite3_free(p->pEncode);
  p->pEncode = 0;
  return SQLITE_OK;
}
static int rleUncompressSetup(ZipvfsInst *p){
  assert( p->pDecode==0 );
  p->pDecode = (struct DecoderInst*)sqlite3_malloc( 11 );
  if( p->pDecode==0 ) return SQLITE_NOMEM;
  memcpy(p->pDecode, "rle-decode", 11);
  return SQLITE_OK;
}
static int rleUncompressCleanup(ZipvfsInst *p){
  /* p->pDecode can be NULL if rleCompressSetup() hit an OOM error */
  assert( p->pDecode==0 || memcmp(p->pDecode, "rle-decode", 11)==0 );
  sqlite3_free(p->pDecode);
  p->pDecode = 0;
  return SQLITE_OK;
}

/* End of the "rle" compression method.
*****************************************************************************/

/*****************************************************************************
** ZLIB compression for ZipVFS.
**
** These routines implement compression using the external zLib library.
*/
#ifndef SQLITE_OMIT_ZLIB
static int zlibBound(void *pCtx, int nByte){
  return nByte + (nByte >> 12) + (nByte >> 14) + 11 +
             ((ZipvfsInst*)pCtx)->nExtra;
}
static int zlibCompress(
  void *pLocalCtx, 
  char *aDest, int *pnDest, 
  const char *aSrc, int nSrc
){
  uLongf n = *pnDest;             /* In/out buffer size for compress() */
  int rc;                         /* compress() return code */
  ZipvfsInst *p = (ZipvfsInst*)pLocalCtx;
  int iLevel = p->iLevel;
 
  if( iLevel<0 || iLevel>9 ) iLevel = Z_DEFAULT_COMPRESSION;
  rc = compress2((unsigned char*)aDest, &n, (unsigned char*)aSrc, nSrc, iLevel);
  if( p->pCrypto ){
    int nx = n;
    zipvfsEncryptWrapper(p, aDest, &nx);
    n = nx;
  }
  *pnDest = n;
  return (rc==Z_OK ? SQLITE_OK : SQLITE_ERROR);
}
static int zlibUncompress(
  void *pLocalCtx, 
  char *aDest, int *pnDest, 
  const char *aSrc, int nSrc
){
  uLongf n = *pnDest;             /* In/out buffer size for uncompress() */
  int rc;                         /* uncompress() return code */
  ZipvfsInst *p = (ZipvfsInst*)pLocalCtx;
 
  aSrc = zipvfsDecryptWrapper(p, aSrc, &nSrc);
  if( aSrc==0 ) return SQLITE_NOMEM;
  rc = uncompress((unsigned char*)aDest, &n, (unsigned char*)aSrc, nSrc);
  *pnDest = n;
  return (rc==Z_OK ? SQLITE_OK : SQLITE_ERROR);
}
#endif
/* End zlib compression
******************************************************************************/

/******************************************************************************
** Blank-space removal compression routines for use with ZIPVFS
**
** Many SQLite database pages contain a large span of zero bytes.  This
** compression algorithm attempts to reduce the page size by simply not
** storing the single largest span of zeros within each page.
**
** The compressed format consists of a single 2-byte big-endian number X
** followed by N bytes of content.  To decompress, copy the first X
** bytes of content, followed by Pagesize-N zeros, followed by the final
** N-X bytes of content.
*/
static int bsrBound(void *pCtx, int n){
  return n+2+((ZipvfsInst*)pCtx)->nExtra;
}
static int bsrCompress(
  void *pLocalCtx,
  char *aOut, int *pnOut,
  const char *aIn,  int nIn
){
  const char *p = aIn;          /* Loop pointer */
  const char *pEnd = &aIn[nIn]; /* Ptr to end of data */
  const char *pBestStart = p;   /* Ptr to first byte of longest run of zeros */
  const char *pN = pEnd;  /* End of data to search (reduces for longer runs) */
  int X;                  /* Length of the current run of zeros */
  int bestLen = 0;        /* Length of the longest run of zeros */
  ZipvfsInst *pInst = (ZipvfsInst*)pLocalCtx;

  /* Find the longest run of zeros */
  while( p<pN ){
    if( *p==0 ){
      const char *pS = p;  /* save start */
      p++;
      while( p<pEnd && *p==0 ) p++;
      X = p - pS;
      if( X>bestLen ){
        bestLen = X;
        pBestStart = pS;
        pN = &aIn[nIn - bestLen]; /* reduce search space based on longest run */
      }
    }
    p++;
  }

  /* Do the compression */
  X = pBestStart-aIn;          /* length of first data block */
  aOut[0] = (X>>8)&0xff;
  aOut[1] = X & 0xff;
  memcpy(&aOut[2], aIn, X);    /* copy first data block */
  memcpy(&aOut[2+X], &pBestStart[bestLen], nIn-X-bestLen);
  *pnOut = 2+nIn-bestLen;
  zipvfsEncryptWrapper(pInst, aOut, pnOut);
  return SQLITE_OK;
}
static int bsrUncompress(
  void *pLocalCtx,
  char *aOut, int *pnOut,
  const char *aIn,  int nIn
){
  int X;                     /* Initial number of bytes to copy */
  int szPage;                /* Size of a page */
  int nTail;                 /* Byte of content to write to tail of page */
  const unsigned char *a;    /* Input unsigned */
  ZipvfsInst *p = (ZipvfsInst*)pLocalCtx;

  aIn = zipvfsDecryptWrapper(p, aIn, &nIn);
  if( aIn==0 ) return SQLITE_NOMEM;
  a = (unsigned char*)aIn;
  X = (a[0]<<8) + a[1];
  nIn -= 2;
  nTail = nIn - X;
  szPage = *pnOut;
  if( X>0 ){
    memcpy(aOut, &aIn[2], X);
  }
  memset(&aOut[X], 0, szPage-nIn);
  memcpy(&aOut[szPage-nTail], &aIn[2+X], nTail);
  return SQLITE_OK;
}
/* End BSR compression routines
******************************************************************************/

/******************************************************************************
** Longest-run removal compression routines for use with ZIPVFS
**
** This compression algorithm is very similar to the BSR algorithm
** but looks for the longest run on any single character in a page.
** It has the same goal as the BSR algorithm of striving for speed
** and simplicity.
**
** The compressed format consists of a single 2-byte big-endian number X,
** followed by a single byte containing the character representing the
** longest run, followed by N bytes of content.  To decompress, copy the 
** first X bytes of content, followed by Pagesize-N of the LRR character, 
** followed by the final N-X bytes of content.
*/
static int lrrBound(void *pCtx, int n){
  return n+3+((ZipvfsInst*)pCtx)->nExtra;
}
static int lrrCompress(
  void *pLocalCtx,
  char *aOut, int *pnOut,
  const char *aIn,  int nIn
){
  const char *p = aIn;           /* Loop pointer */
  const char *pEnd = &aIn[nIn];  /* Pointer to end of data */
  const char *pS;                /* Pointer to start of current run */
  int X;                         /* Length of the current run of char */
  const char *pBestStart = p;    /* Ptr to first byte of longest run of char */
  int bestLen = nIn ? 1 : 0;     /* Length of the longest run of char */
  const char *pN;      /* Ptr to end of data (reduces as we find longer runs) */
  ZipvfsInst *pInst = (ZipvfsInst*)pLocalCtx;

  pN = &aIn[nIn - bestLen];
  if(nIn>0){
    while( p<pN ){
      pS = p;  /* save start */
      p++;
      if( *p==*pS ){
        p++;
        while( p<pEnd && *p==*pS ) p++;
        X = p - pS;
        if( X>bestLen ){
          bestLen = X;
          pBestStart = pS;
          pN = &aIn[nIn - bestLen]; /* reduce search based on longest run */
        }
      }
    }
  }

  /* Do the compression */
  X = pBestStart-aIn;          /* length of first data block */
  aOut[0] = (X>>8)&0xff;
  aOut[1] = X & 0xff;
  aOut[2] = nIn ? *pBestStart : 0;
  memcpy(&aOut[3], aIn, X);    /* copy first data block */
  memcpy(&aOut[3+X], &pBestStart[bestLen], nIn-X-bestLen);
  *pnOut = 3+nIn-bestLen;
  zipvfsEncryptWrapper(pInst, aOut, pnOut);
  return SQLITE_OK;
}
static int lrrUncompress(
  void *pLocalCtx,
  char *aOut, int *pnOut,
  const char *aIn,  int nIn
){
  int X;                     /* Initial number of bytes to copy */
  int szPage;                /* Size of a page */
  int nTail;                 /* Byte of content to write to tail of page */
  const unsigned char *a;    /* Input unsigned */
  ZipvfsInst *p = (ZipvfsInst*)pLocalCtx;

  aIn = zipvfsDecryptWrapper(p, aIn, &nIn);
  if( aIn==0 ) return SQLITE_NOMEM;
  a = (unsigned char*)aIn;
  X = (a[0]<<8) + a[1];
  nIn -= 3;
  nTail = nIn - X;
  szPage = *pnOut;
  if( X>0 ){
    memcpy(aOut, &aIn[3], X);
  }
  memset(&aOut[X], aIn[2], szPage-nIn);
  memcpy(&aOut[szPage-nTail], &aIn[3+X], nTail);
  return SQLITE_OK;
}
/* End the LRR compression routines.
******************************************************************************/
/*
    REDEYE
*/
#include "Lz4Lib/lz4.h"
static	int		REDEYE_Bound(void* pCtx, int nByte)
{
    return nByte ? LZ4_compressBound(nByte) : 0;
}
static	int		REDEYE_Compress(void* pCtx, char* pDest, int* pnDest, const char* pSrc, int nSrc)
{
    int			nRet;
    //int LZ4_compress_default(const char* source, char* dest, int sourceSize, int maxDestSize);
    nRet = LZ4_compress_fast(pSrc, pDest, nSrc, *pnDest, 1);
    if (nRet > 0)
    {
        *pnDest = nRet;
        return SQLITE_OK;
    }
    return SQLITE_ERROR;
}
static	int		REDEYE_Uncompress(void* pCtx, char* pDest, int* pnDest, const char* pSrc, int nSrc)
{
    int			nRet;
    //	int LZ4_decompress_safe (const char* source, char* dest, int compressedSize, int maxDecompressedSize);
    nRet = LZ4_decompress_safe(pSrc, pDest, nSrc, *pnDest);
    if (nRet > 0)
    {
        *pnDest = nRet;
        return SQLITE_OK;
    }
    return SQLITE_ERROR;
}

/*
** The following is the array of available compression and encryption 
** algorithms.  To add new compression or encryption algorithms, make
** new entries to this array.
*/
static const ZipvfsAlgorithm aZipvfs[] = {

  /* Run-length encoding */ {
  /* zName          */  "rle",
  /* xBound         */  rleBound,
  /* xComprSetup    */  rleCompressSetup,
  /* xCompr         */  rleCompress,
  /* xComprCleanup  */  rleCompressCleanup,
  /* xDecmprSetup   */  rleUncompressSetup,
  /* xDecmpr        */  rleUncompress,
  /* xDecmprCleanup */  rleUncompressCleanup,
  /* xCryptoSetup   */  aesEncryptionSetup,
  /* xCryptoCleanup */  aesEncryptionCleanup
  },

  /* Blank-space removal */ {
  /* zName          */  "bsr",
  /* xBound         */  bsrBound,
  /* xComprSetup    */  0,
  /* xCompr         */  bsrCompress,
  /* xComprCleanup  */  0,
  /* xDecmprSetup   */  0,
  /* xDecmpr        */  bsrUncompress,
  /* xDecmprCleanup */  0,
  /* xCryptoSetup   */  aesEncryptionSetup,
  /* xCryptoCleanup */  aesEncryptionCleanup
  },


  /* Longest-Run Removal */ {
  /* zName          */  "lrr",
  /* xBound         */  lrrBound,
  /* xComprSetup    */  0,
  /* xCompr         */  lrrCompress,
  /* xComprCleanup  */  0,
  /* xDecmprSetup   */  0,
  /* xDecmpr        */  lrrUncompress,
  /* xDecmprCleanup */  0,
  /* xCryptoSetup   */  aesEncryptionSetup,
  /* xCryptoCleanup */  aesEncryptionCleanup
  },

    /* Longest-Run Removal */ {
    /* zName          */  "redeye",
    /* xBound         */  REDEYE_Bound,
    /* xComprSetup    */  0,
    /* xCompr         */  REDEYE_Compress,
    /* xComprCleanup  */  0,
    /* xDecmprSetup   */  0,
    /* xDecmpr        */  REDEYE_Uncompress,
    /* xDecmprCleanup */  0,
    /* xCryptoSetup   */  aesEncryptionSetup,
    /* xCryptoCleanup */  aesEncryptionCleanup
},
#ifndef SQLITE_OMIT_ZLIB
  /* ZLib */ {
  /* zName          */  "zlib",
  /* xBound         */  zlibBound,
  /* xComprSetup    */  0,
  /* xCompr         */  zlibCompress,
  /* xComprCleanup  */  0,
  /* xDecmprSetup   */  0,
  /* xDecmpr        */  zlibUncompress,
  /* xDecmprCleanup */  0,
  /* xCryptoSetup   */  aesEncryptionSetup,
  /* xCryptoCleanup */  aesEncryptionCleanup
  },
#endif /* SQLITE_OMIT_ZLIB */

};

/*
** This routine is called when a ZIPVFS database connection is shutting
** down.  Invoke all of the cleanup procedures in the ZipvfsAlgorithm
** object.
*/
static int zipvfsCompressClose(void *pLocalCtx){
  ZipvfsInst *p = (ZipvfsInst*)pLocalCtx;
  const ZipvfsAlgorithm *pAlg = p->pAlg;
  if( pAlg->xComprCleanup ){
    (void)pAlg->xComprCleanup(p);
  }
  if( pAlg->xDecmprCleanup ){
    (void)pAlg->xDecmprCleanup(p);
  }
  if( pAlg->xCryptoCleanup ){
    (void)pAlg->xCryptoCleanup(p);
  }
  sqlite3_free(p);
  return SQLITE_OK;
}

/*****************************************************************************/
/* Start of SQL functions for zlib.                                          */
/*****************************************************************************/

#if !defined(SQLITE_OMIT_ZLIB) && defined(SQLITE_ENABLE_ZLIB_FUNCS)
/*
** Implementation of the "compress(X)" SQL function.  The input X is
** compressed using zLib and the output is returned.
*/
static void zipvfs_sqlcmd_compress(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const unsigned char *pIn;
  unsigned char *pOut;
  unsigned int nIn;
  unsigned long int nOut;

  (void)argc;  /* unused parameter */
  pIn = sqlite3_value_blob(argv[0]);
  nIn = sqlite3_value_bytes(argv[0]);
  nOut = 13 + nIn + (nIn+999)/1000;
  pOut = sqlite3_malloc( nOut+4 );
  pOut[0] = nIn>>24 & 0xff;
  pOut[1] = nIn>>16 & 0xff;
  pOut[2] = nIn>>8 & 0xff;
  pOut[3] = nIn & 0xff;
  compress(&pOut[4], &nOut, pIn, nIn);
  sqlite3_result_blob(context, pOut, nOut+4, sqlite3_free);
}

/*
** Implementation of the "decompress(X)" SQL function.  The argument X
** is a blob which was obtained from compress(Y).  The output will be
** the value Y.
*/
static void zipvfs_sqlcmd_decompress(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const unsigned char *pIn;
  unsigned char *pOut;
  unsigned int nIn;
  unsigned long int nOut;
  int rc;

  (void)argc;  /* unused parameter */
  pIn = sqlite3_value_blob(argv[0]);
  nIn = sqlite3_value_bytes(argv[0]);
  nOut = (pIn[0]<<24) + (pIn[1]<<16) + (pIn[2]<<8) + pIn[3];
  pOut = sqlite3_malloc( nOut+1 );
  rc = uncompress(pOut, &nOut, &pIn[4], nIn-4);
  if( rc==Z_OK ){
    sqlite3_result_blob(context, pOut, nOut, sqlite3_free);
  }else{
    sqlite3_result_error(context, "input is not zlib compressed", -1);
  }
}
#endif /* !defined(SQLITE_OMIT_ZLIB) && defined(SQLITE_ENABLE_ZLIB_FUNCS) */

/*
** This is the "automatic extension" initializer that runs right after
** the connection to the repository database is opened.  Set up the
** database connection to be more useful to the human operator.
*/
int zipvfs_sqlcmd_autoinit(
  sqlite3 *db,
  const char **pzErrMsg,
  const void *notUsed
){
  (void)notUsed;  /* unused parameter */
  (void)pzErrMsg; /* unused parameter */
#if !defined(SQLITE_OMIT_ZLIB) && defined(SQLITE_ENABLE_ZLIB_FUNCS)
  sqlite3_create_function(db, "compress", 1, SQLITE_ANY, 0,
                          zipvfs_sqlcmd_compress, 0, 0);
  sqlite3_create_function(db, "decompress", 1, SQLITE_ANY, 0,
                          zipvfs_sqlcmd_decompress, 0, 0);
#else
  (void)db; /* unused parameter */
#endif /* !defined(SQLITE_OMIT_ZLIB) && defined(SQLITE_ENABLE_ZLIB_FUNCS) */
#ifdef SQLITE_ENABLE_ZIPVFS_VTAB
  {
    int sqlite3_zipvfsvtab_register(sqlite3 *db);
    sqlite3_zipvfsvtab_register(db);
  }
#endif
  return SQLITE_OK;
}

/*****************************************************************************/
/* Start of shell integration code using zipvfs_create_vfs_v2().             */
/*****************************************************************************/

/*
** Activate a new ZIPVFS connection.
*/
static int zipvfsLegacyOpen(
  void *pCtxIn,
  const char *zDatabaseURI,
  void **ppCtxOut
){
  ZipvfsInst *pInst = sqlite3_malloc( sizeof(*pInst) );
  ZipvfsAlgorithm *pAlg = (ZipvfsAlgorithm*)pCtxIn;
  int rc = SQLITE_OK;
  if( pInst==0 ) return SQLITE_NOMEM;
  memset(pInst, 0, sizeof(*pInst));
  pInst->pAlg = pAlg;
  *ppCtxOut = pInst;
  rc = aesEncryptionSetup(pInst, zDatabaseURI);
  if( rc!=SQLITE_OK ){
    zipvfsCompressClose(pInst);
  }
  if( rc==SQLITE_OK && pAlg->xComprSetup ){
    rc = pAlg->xComprSetup(pInst);
  }
  if( rc==SQLITE_OK && pAlg->xDecmprSetup ){
    rc = pAlg->xDecmprSetup(pInst);
  }
  return rc;
}

/*
** Only initialize ZIPVFS once.  Set this variable to true after the
** first initialization to prevent a second.
*/
static int _zipvfsIsInit = 0;

inline  void    SetInitFlag(const char * pCause, int nFlag) {
    printf("%s %d", pCause, nFlag);
    _zipvfsIsInit   = nFlag;
}
inline  int     GetInitFlag() {
    return _zipvfsIsInit;
}


/*
** Register all ZipVFS algorithms, if we have not done so already.
*/
#ifdef _WIN32
__declspec(dllexport)
#endif
void zipvfsInit_v2(void){
  unsigned int i;
  if( GetInitFlag() ) return;
  SetInitFlag(__FUNCTION__, 1);
  for(i=0; i<sizeof(aZipvfs)/sizeof(aZipvfs[0]); i++){
    zipvfs_create_vfs_v2(
        aZipvfs[i].zName, 0, (void*)&aZipvfs[0],
        aZipvfs[i].xBound,
        aZipvfs[i].xCompr,
        aZipvfs[i].xDecmpr,
        zipvfsLegacyOpen,
        zipvfsCompressClose
    );
  }
}

/*****************************************************************************/
/* Start of shell integration code using zipvfs_create_vfs_v3().             */
/*****************************************************************************/

/*
** The following routine is the crux of this whole module.
**
** This is the xAutoDetect callback for ZipVFS.  The job of this function
** is to figure out which compression algorithm to use for a database
** and fill out the pMethods object with pointers to the appropriate
** compression and decompression routines.
**
** ZIPVFS calls this routine exactly once for each database file that is
** opened.  The zHeader parameter is a copy of the text found in bytes
** 3 through 16 of the database header.  Or, if a new database is being
** created, zHeader is a NULL pointer.  zFile is the name of the database
** file.  If the database was opened as URI, then sqlite3_uri_parameter()
** can be used with the zFile parameter to extract query parameters from
** the URI.
*/
static int zipvfsAutoDetect(
  void *pCtx,              /* Copy of pCtx from zipvfs_create_vfs_v3() */
  const char *zFile,       /* Name of file being opened */
  const char *zHeader,     /* Algorithm name in the database header */
  ZipvfsMethods *pMethods  /* OUT: Write new pCtx and function pointers here */
){
  /* If zHeader==0 that means we have a new database file.
  ** Look to the zv query parameter (if there is one) as a
  ** substitute for the database header.
  */
  if( zHeader==0 ){
    const char *zZv = sqlite3_uri_parameter(zFile, "zv");
    if( zZv ) zHeader = zZv;
  }

  /* Look for a compression algorithm that matches zHeader.
  */
  if( zHeader ){
    int i;
    for(i=0; i<(int)(sizeof(aZipvfs)/sizeof(aZipvfs[0])); i++){
      if( strcmp(aZipvfs[i].zName, zHeader)==0 ){
        ZipvfsInst *pInst = sqlite3_malloc( sizeof(*pInst) );
        int rc = SQLITE_OK;
        if( pInst==0 ) return SQLITE_NOMEM;
        memset(pInst, 0, sizeof(*pInst));
        pInst->pCtx = pCtx;
        pInst->pAlg = &aZipvfs[i];
        pInst->iLevel = (int)sqlite3_uri_int64(zFile, "level", -1);
        pMethods->zHdr = aZipvfs[i].zName;
        pMethods->xCompressBound = aZipvfs[i].xBound;
        pMethods->xCompress = aZipvfs[i].xCompr;
        pMethods->xUncompress = aZipvfs[i].xDecmpr;
        pMethods->xCompressClose = zipvfsCompressClose;
        pMethods->pCtx = pInst;
        if( aZipvfs[i].xCryptoSetup ){
          rc = aZipvfs[i].xCryptoSetup(pInst, zFile);
        }
        if( rc==SQLITE_OK && aZipvfs[i].xComprSetup ){
          rc = aZipvfs[i].xComprSetup(pInst);
        }
        if( rc==SQLITE_OK && aZipvfs[i].xDecmprSetup ){
          rc = aZipvfs[i].xDecmprSetup(pInst);
        }
        if( rc!=SQLITE_OK ){
          zipvfsCompressClose(pInst);
          memset(pMethods, 0, sizeof(*pMethods));
        }
        return rc;
      }
    }
  }

  /* If no compression algorithm is found whose name matches zHeader,
  ** then assume no compression is desired.  Clearing the pMethod object
  ** causes ZIPVFS to be a no-op so that it reads and writes ordinary
  ** uncompressed SQLite database files.
  */
  memset(pMethods, 0, sizeof(*pMethods));
  return SQLITE_OK;
}

/*
** Register the ZIPVFS extension if it has not been registered already.
*/
#ifdef _WIN32
__declspec(dllexport)
#endif
void zipvfsInit_v3(int regDflt){
  if( !zipvfsIsInit ){
    zipvfsIsInit = 1;
    zipvfs_create_vfs_v3("zipvfs", 0, 0, zipvfsAutoDetect);
    if( regDflt ){
      sqlite3_vfs_register(sqlite3_vfs_find("zipvfs"),1);
    }
  }
}

/*
** Deregister the ZIPVFS extension if it has already been registered.
*/
#ifdef _WIN32
__declspec(dllexport)
#endif
void zipvfsShutdown_v3(void){
  if( zipvfsIsInit ){
    zipvfsIsInit = 0;
    zipvfs_destroy_vfs("zipvfs");
  }
}

/*
** A wrapper around zipvfsInit_v3() that can be used with the 
** SQLITE_EXTRA_INIT macro. This wrapper installs zipvfs as the
** default VFS.
*/
int zipvfsAutoInit_v3(const char *zUnused){
  (void)zUnused;  /* unused parameter */
  zipvfsInit_v3(1);
  return SQLITE_OK;
}

/*****************************************************************************/
/* End of shell integration code.                                            */
/*****************************************************************************/
