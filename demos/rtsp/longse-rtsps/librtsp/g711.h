#ifndef __G711_H__
#define __G711_H__

#ifdef __cplusplus
#   define EXTERN extern "C"
#else
#   define EXTERN extern
#endif


EXTERN unsigned charlinear2alaw(int  pcm_val); /* 2's complement (16-bit range) */
EXTERN int alaw2linear(unsigned char a_val);
EXTERN unsigned charlinear2ulaw(int  pcm_val); /* 2's complement (16-bit range) */
EXTERN int ulaw2linear(unsigned char u_val);
EXTERN unsigned charalaw2ulaw(unsigned char aval);
EXTERN unsigned charulaw2alaw(unsigned char uval);

EXTERN void ulaw2linear_g(unsigned char *inbuf, short *outbuf, int count);














#endif
