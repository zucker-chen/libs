#ifndef __vofaplus_h__
#define __vofaplus_h__


#ifdef __cplusplus
extern "C" {
#endif



typedef struct vofaplus_s {
    int		fd;						// socket fd
	char 	ip[16];					// ip address
	int		port;					// socket port
} vofaplus_t;











#ifdef __cplusplus
}
#endif
#endif /* !__vofaplus_h__ */
