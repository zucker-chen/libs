/****************************************************************************
 *
 * FILENAME:        $RCSfile: MD5Auth.h,v $
 *
 * LAST REVISION:   $Revision: 1.2 $
 * LAST MODIFIED:   $Date: 2006/06/10 06:06:04 $
 *
 * DESCRIPTION:     The header file for MD5Auth class
 *
 * vi: set ts=4:
 *
 * Copyright (c) 2006 by Grandstream Networks, Inc.
 * All rights reserved.
 *
 * This material is proprietary to Grandstream Networks, Inc. and,
 * in addition to the above mentioned Copyright, may be
 * subject to protection under other intellectual property
 * regimes, including patents, trade secrets, designs and/or
 * trademarks.
 *
 * Any use of this material for any purpose, except with an
 * express license from Grandstream Networks, Inc. is strictly
 * prohibited.
 *
 ***************************************************************************/

#ifndef __MD5_H__
#define __MD5_H__

/***********************************************/
/*               MD5 Base Define               */
/***********************************************/
/* modified for oSIP: GCC supports this feature */
#define PROTOTYPES 1

#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif


/**
 * Structure for holding MD5 context.
 * @var MD5_CTX
 */
typedef struct
{
    UINT4 state[4];             /* state (ABCD) */
    UINT4 count[2];             /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];   /* input buffer */
}MD5_CTX;

void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST ((MD5_CTX *, unsigned char *, unsigned int));
void MD5Final PROTO_LIST ((unsigned char[16], MD5_CTX *));

/***********************************************/
/*               MD5 Application               */
/***********************************************/
#define HASHLEN         16
#define HASHHEXLEN      32

typedef char HASH[HASHLEN];
typedef char HASHHEX[HASHHEXLEN + 1];

/* md5 auth algo type */
typedef enum
{
    EN_MD5AUTH_ALGO_MD5,
    EN_MD5AUTH_ALGO_SESS,
    EN_MD5AUTH_ALGO_INVALID
}EN_MD5AUTH_ALGO;

void MD5Auth_CvtHex( HASH bin, HASHHEX hex );
void MD5Auth_DigestCalcHA1(  const char* authId, const char* password,
                             unsigned char alg, const char* realm, const char* nonce,
                             const char* cnonce, HASHHEX sessionKey );
void MD5Auth_DigestCalcResponse( HASHHEX ha1, const char* nonce, const char* nc,
                                 const char* cnonce, const char* qop, const char* method,
                                 const char* uri, HASHHEX entity, HASHHEX response );
int MD5Auth_BuildAuthResonseWithMD5( char* resp, unsigned short maxlen,
                                     unsigned char alg, const char* auth, const char* realm,
                                     const char* pass, const char* nonce, const char* nc,
                                     const char* cnonce, const char* qop,
                                     const char* method,const char*  uri);

#endif

