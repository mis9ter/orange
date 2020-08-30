#pragma once
typedef unsigned char md5_byte_t; /* 8-bit byte */
typedef unsigned int md5_word_t; /* 32-bit word */

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s {
    md5_word_t count[2];    /* message length in bits, lsw first */
    md5_word_t abcd[4];     /* digest buffer */
    md5_byte_t buf[64];     /* accumulate block */
} md5_state_t;


static void
md5_process(md5_state_t* pms, const md5_byte_t* data /*[64]*/);

void
md5_init(md5_state_t* pms);
void
md5_append(md5_state_t* pms, const md5_byte_t* data, int nbytes);

void
md5_finish(md5_state_t* pms, md5_byte_t digest[16]);



char*                  /* O - MD5 sum in hex */
httpMD5String(const md5_byte_t* sum,    /* I - MD5 sum data */
    char             md5[33])   /* O - MD5 sum in hex */;