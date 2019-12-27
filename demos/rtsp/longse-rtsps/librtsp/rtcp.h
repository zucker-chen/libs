
#include "rs_type.h"


		/*Current	protocol   version.   */  
#define   RTP_VERSION		  2  
#define   MIN_SEQUENTIAL	 1	
#define   RTP_SEQ_MOD	    (1<<16)  
#define   RTP_MAX_SDES	 255			 /*   maximum	text   length	for   SDES	 */  
#define   MID_BUFFER_NUM	   2  
#define   MAX_DROPOUT		25	
 
typedef   enum	 {	
		RTCP_SR 	  =   200,	
		RTCP_RR 	  =   201,	
		RTCP_SDES	=	202,  
		RTCP_BYE	 =	 203,  
		RTCP_APP	 =	 204  
}	rtcp_type_t;  
 
typedef   enum	 {	
	  RTCP_SDES_END 	  =   0,  
	  RTCP_SDES_CNAME	=	1,	
	  RTCP_SDES_NAME	 =	 2,  
	  RTCP_SDES_EMAIL	=	3,	
	  RTCP_SDES_PHONE	=	4,	
	  RTCP_SDES_LOC 	  =   5,  
	  RTCP_SDES_TOOL	 =	 6,  
	  RTCP_SDES_NOTE	 =	 7,  
	  RTCP_SDES_PRIV	 =	 8	
}	rtcp_sdes_type_t;  
 

 
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif
/*	
  *   RTCP	 common   header   word  
  */  
typedef   struct   {  

#if (BYTE_ORDER == LITTLE_ENDIAN)
	u8	 count:5;			/*	 varies   by   packet	type   */  
	u8    p:1;					/*	 padding   flag   */  
	u8	 version:2; 	  /*   protocol   version	*/		
	u8	 pt:8;				   /*	RTCP   packet	type   */  	
#elif (BYTE_ORDER == BIG_ENDIAN)
	u8	 version:2; 	  /*   protocol   version	*/	
	u8	 p:1;					/*	 padding   flag   */  
	u8	 count:5;			/*	 varies   by   packet	type   */  

	u8	 pt:8;				   /*	RTCP   packet	type   */  
#else
    #error YOU MUST DEFINE BYTE_ORDER == LITTLE_ENDIAN OR BIG_ENDIAN !
#endif	  
	  u16 		length; 					  /*   pkt	 len   in	words,	 w/o   this   word	 */  
}	rtcp_common_t;	
 
/*	
  *   Big-endian   mask   for	version,   padding	 bit   and	 packet   type	 pair  
  */  
#define   RTCP_VALID_MASK	(0xc000   |   0x2000   |   0xfe)  
#define   RTCP_VALID_VALUE	 ((RTP_VERSION	 <<   14)	|	RTCP_SR)  
 
/*	
  *   Reception   report   block  
  */  
typedef   struct   {  
	  u32	ssrc;							/*	 data	source	 being	 reported	*/	
	  unsigned	 int   fraction:8;	   /*	fraction   lost   since   last	 SR/RR	 */  
	  int	lost:24;							 /*   cumul.   no.	 pkts	lost   (signed!)   */  
	  u32	last_seq;					/*	 extended	last   seq.   no.	received   */  
	  u32	jitter; 					  /*   interarrival   jitter   */  
	  u32	lsr;							 /*   last	 SR   packet   from   this	 source   */  
	  u32	dlsr;							/*	 delay	 since	 last	SR	 packet   */  
}	rtcp_rr_t;	
 
/*	
  *   SDES	 item  
  */  
typedef   struct   {  
	  u8   type;							 /*   type	 of   item	 (rtcp_sdes_type_t)   */  
	  u8   length;						   /*	length	 of   item	 (in   octets)	 */  
	  char	 data[1];							/*	 text,	 not   null-terminated	 */  
}	rtcp_sdes_item_t;  
 
/*	
  *   One	RTCP   packet  
  */  
typedef   struct   {  
	rtcp_common_t   common;			/*	 common   header   */  
	union   {  
		/*	 sender   report   (SR)   */  
		struct	 {	
			u32	ssrc;			/*	 sender   generating   this   report   */  
			u32	ntp_sec;	 /*   NTP	timestamp	*/	
			u32	ntp_frac;  
			u32	rtp_ts; 	  /*   RTP	 timestamp	 */  
			u32	psent;		   /*	packets   sent	 */  
			u32	osent;		   /*	octets	 sent	*/	
			rtcp_rr_t   rr[1];	 /*   variable-length	list   */  
		}   sr;  

		/*   reception   report	(RR)   */  
		struct   {  
			u32   ssrc; 		  /*   receiver   generating   this   report   */  
			rtcp_rr_t	rr[1];	   /*	variable-length   list	 */  
		}   rr;  

		/*   source	description   (SDES)   */  
		struct   rtcp_sdes   {  
			u32   src;			   /*	first	SSRC/CSRC	*/	
			rtcp_sdes_item_t   item[1];   /*   list   of   SDES   items   */  
		}   sdes;  

		/*   BYE	 */  
		struct   {  
			u32   src[1];		/*	 list	of	 sources   */  
									  /*   can"t   express	 trailing	text   for	 reason   */  
		}   bye;	
	}   r;  
}	rtcp_t; 

