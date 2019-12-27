/******************************************************************************
 * Copyright (C), 2008-2011, Grandstream Co., Ltd.        
 ******************************************************************************
 File Name     : gs_sensor.c
 Version       : Initial Draft
 Author        : Grandstream video software group
 Created       : 2009/11/19
 Last Modified :
 Description   : 
 Function List :
 Note		   : created 2009/12/11
 History       :
 1.Date        : 2009/12/11
   Author      : lwx
   Modification: 
 ******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#define 	MODLE_NAME		"Misc "

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <net/route.h>
#include <linux/sockios.h>

#include "rs_misc.h"
//#include "rs_cfg.h"

int 				fsCnt = 0;
UsbSdStat_t			gUsbSdUmount = {USB_SD_IS_MOUNT, USB_SD_IS_MOUNT};
pthread_mutex_t 	gUsbSdLock;
pthread_mutex_t		gLocalTimeLock;
sd_mmc_device_t		sd_mmc_device;
extern char 		*tzname[2];
extern long			timezone; 
extern int			daylight;
static int			gIsGmtTime = 0;
static int			gMin = 0;
static int			gTimeZoneUpdateTime = 0;

char *gWriteTzCmd[] = {
		"cp /usr/share/zoneinfo/Etc/GMT+12 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+11 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+10 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+9 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+8 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+7 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+6 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+5 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+5 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+4 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+4 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+4 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+3 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+2 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+1 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT+0 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-1 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-2 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-2 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-2 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-2 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-2 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-3 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-3 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-3 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-4 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-4 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-5 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-5 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-6 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-7 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-8 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-9 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-9 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-10 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-10 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-11 /data/zone/tz",
		"cp /usr/share/zoneinfo/Etc/GMT-12 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT-9 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT-8 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT-7 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+1 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+1 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+2 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+3 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+3 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+9 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+10 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+10 /data/zone/tz",
		"cp /usr/share/zoneinfo/dn/dn_GMT+12 /data/zone/tz",

};

char *gTimeZoneInfo[] = {
		"GMT+12\n",     	// 1	
		"GMT+11\n", 	
		"GMT+10\n", 	
		"GMT+09\n", 	
		"GMT+08\n", 	
		"GMT+07\n", 	
		"GMT+06\n", 	
		"GMT+05\n", 	
		"GMT+05\n", 	
		"GMT+04\n", 	
		"GMT+04\n", 	
		"GMT+04\n", 	
		"GMT+03\n", 	
		"GMT+02\n", 	
		"GMT+01\n", 	
		"GMT+00\n",		
		"GMT-01\n", 	
		"GMT-02\n", 	
		"GMT-02\n", 	
		"GMT-02\n", 	
		"GMT-02\n", 	
		"GMT-02\n", 	
		"GMT-03\n", 	
		"GMT-03\n", 	
		"GMT-03:30\n",	
		"GMT-04\n",  	
		"GMT-04:30\n",  	
		"GMT-05\n",     	
		"GMT-05:30\n",  	
		"GMT-06\n",     	
		"GMT-07\n",     	
		"GMT-08\n",     	
		"GMT-09\n",     	
		"GMT-09:30\n",  	
		"GMT-10\n", 	
		"GMT-10\n", 	
		"GMT-11\n", 	
		"GMT-12\n", 		// 38
		/* daylight saving time */
		"AKST9AKDT\n", 
		"PST8PDT,M3.2.0/02:00:00,M11.1.0/02:00:00\n", // 40
		"MST7MDT\n", 	
		"GMT+0IST-1,M3.5.0/01:00:00,M10.5.0/02:00:00\n", // 42
		"WET-0WEST-1,M3.5.0/01:00:00,M10.5.0/02:00:00\n",
		"GMT+0BST-1,M3.5.0/01:00:00,M10.5.0/02:00:00\n", // 44
		"CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00\n", 		
		"CFT-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00\n",// 46 
		"EET-2EEST-3,M3.5.0/03:00:00,M10.5.0/04:00:00\n", 		
		"MSK-3MSD,M3.5.0/2,M10.5.0/3\n", 		 // 48
		"MST-3MDT,M3.5.0/2,M10.5.0/3\n", 	
		"CST-9:30CDT-10:30,M10.5.0/02:00:00,M3.5.0/03:00:00\n", // 50
		"EST-10EDT-11,M10.5.0/02:00:00,M3.5.0/03:00:00\n", 		
		"EST-10EDT-11,M10.1.0/02:00:00,M3.5.0/03:00:00\n", 	// 52
		"NZST-12NZDT-13,M10.1.0/02:00:00,M3.3.0/03:00:00\n", 
		"\0",		
};

	
int Net_TcpSendN(int sockFd, char *buf, int size)
{
	int n,left;
	char *pBuf;
	
	n = 0;
	left = size;
	pBuf = (char *)buf;
	while(left > 0)
	{
		n = send(sockFd, pBuf, left, MSG_NOSIGNAL);
		if (n <= 0)
		{
			/* EINTR A signal occurred before any data was transmitted. */
			if(errno == EINTR)
			{
				n = 0;
				fprintf(stderr, "socket send_n eintr error");
			} 
			else
			{
				//dbg(Err, DbgPerror, "socket error");
				fprintf(stderr, "\n %s socket error %d \n", __FUNCTION__, n);
				return(-1);
			}
		}
		left -= n;
		pBuf += n;
	}
	return size;
}

int Net_TcpRecvN(int sockFd, char *buf, int len)
{
	int ret = 0;
	int rLen = 0;

	while(rLen != len){
		
		ret = recv(sockFd, buf+rLen, len-rLen, 0);
		if(ret <= 0){
			if(errno == EINTR){
				//dbg(Err, DbgPerror, "errno = %d EINTR \n", errno);
				continue;
			}
			//dbg(Err, DbgPerror, "recv error \n");
			return GS_FAIL;
		}
		rLen += ret;
		
	}
	return rLen;
}

int Net_UdpRecvN(int sock, struct sockaddr* sockAddr, 
							size_t *addrLen, int recvLen,  void *recvBuf)
{
	int left, recvN;
	char *pRecv = (char *)recvBuf;

	if(recvBuf == NULL){
		//dbg(Err, DbgNoPerror, "Input params recvBuf is NULL \n");
		return GS_FAIL; 
	}
	recvN = 0;
	left = recvLen;
	while(left > 0){
		recvN = recvfrom(sock, pRecv, left, 0, sockAddr, addrLen);
		if(recvN <= 0){
			return recvN;
		}
		left  -= recvN;
		pRecv += recvN;
	}
	return recvLen;
	
}

int sock_close(int sock_fd)
{
	shutdown(sock_fd, SHUT_RDWR);
	return close(sock_fd);
}

int GetLocalIpAddr(signed   char* pIfName,signed   char* pIPAddr)
{
    struct ifreq sIfr;
    struct sockaddr_in	*pSockAddr;
    char*				pTmp="0.0.0.0";
    int					nSockFd;

	
    if((pIfName == NULL) || (pIPAddr== NULL)){
	   //dbg(Err, DbgNoPerror, "Input params is NULL!\n");
	   return GS_FAIL;
    }
	#if 0
	//* test lwx
	memset(pIfName, 0, 16);
	strcpy(pIfName, "eth0");

	#endif 
	
    nSockFd = socket ( AF_INET, SOCK_DGRAM, 0 );
    if ( nSockFd <= 0 ){
        //dbg(Err, DbgPerror, "socket error \n");
		return GS_FAIL;
    }
    strcpy ( sIfr.ifr_name, (char *)pIfName);
    sIfr.ifr_addr.sa_family=AF_INET;
    if ( ioctl ( nSockFd, SIOCGIFADDR, &sIfr ) < 0 ){
        //dbg(Err, DbgPerror, "ioctl SIOCGIFADDR error \n");
		close(nSockFd);
        return GS_FAIL;
    }else{
        pSockAddr= ( struct sockaddr_in * ) &sIfr.ifr_addr;
        pTmp = inet_ntoa ( pSockAddr->sin_addr );
    }
    strncpy((char *)pIPAddr,pTmp,16);
    close(nSockFd);
	
    return GS_SUCCESS;
}

int LocalSocketCreate(char *path)
{
	int sock;
	int	sockFlag, addrLen;
	struct sockaddr_un	svrAddr;

	unlink(path);
	sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if(sock <= 0){
		//dbg(Err, DbgPerror, "socket AF_UNIX %s error \n", path);
		return GS_FAIL; 
	}
	
	sockFlag = fcntl(sock, F_GETFL, 0);
	sockFlag |= O_NONBLOCK;

	if(-1 == fcntl(sock, F_SETFL, sockFlag)){
		//dbg(Err, DbgPerror, "fcntl F_SETFL error \n");
		close(sock);
		return GS_FAIL; 
	}
	
	svrAddr.sun_family 	= AF_LOCAL;
	strncpy(svrAddr.sun_path, path, sizeof(svrAddr.sun_path));
	addrLen = SUN_LEN(&svrAddr);
	
	if(-1 == bind(sock, (struct sockaddr*)&svrAddr, addrLen)){
		//dbg(Err, DbgPerror, "bind sock = %d error \n", sock);
		close(sock);
		return GS_FAIL; 
	}

	return sock; 
}

int  LocalTcpSocketCreate (const char *filename)
{
	struct sockaddr_un	name;
	int					sock;
	size_t				size;

	/* 删除local filename, 避免bind出错 */
	remove(filename);

	/* Create the socket. */
	sock = socket (PF_LOCAL, SOCK_STREAM, 0);
	if (sock < 0)
	 {
	   //dbg(Err, DbgPerror,  "socket\n");
	   return(-1);
	 }

	/* Bind a name to the socket. */
	name.sun_family = AF_LOCAL;
	strncpy (name.sun_path, filename, sizeof (name.sun_path));
	name.sun_path[sizeof (name.sun_path) - 1] = '\0';
     
	/* The size of the address is
	  the offset of the start of the filename,
	  plus its length,
	  plus one for the terminating null byte.
	  Alternatively you can just do:
	  size = SUN_LEN (&name);
	*/
	size = SUN_LEN(&name);
	if(bind (sock, (struct sockaddr *) &name, size) < 0)
	 {
		//dbg(Err, DbgPerror,  "bind\n");
		return(-1);
		//perror ("bind");
		//exit(1);
	 }

    if(listen(sock, 5) < 0) 
    {
        //dbg(Err, DbgPerror, "socket %s listen error", filename); 
		return (-1);
		//perror ("bind");
		//exit(1);
    }
	return sock;
}

int LocalSocketClientCreate(char *path, struct sockaddr_un *svr)
{
	int sock;
	//int	sockFlag;
	
	sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if(sock <= 0 ){
		//dbg(Err, DbgPerror, "socket error \n");
		return GS_FAIL ;
	}
#if 0
	sockFlag = fcntl(sock, F_GETFL, 0);
    if ( fcntl(sock, F_SETFL, sockFlag|O_NONBLOCK) == -1 )
    {
        dbg(Err, DbgPerror, "cannot set socket to non-block!");
		close(sock);
        return GS_FAIL;
    }
#endif 

	svr->sun_family 	= AF_LOCAL;
	strncpy(svr->sun_path, path, sizeof(svr->sun_path));
	svr->sun_path[sizeof(svr->sun_path) - 1] = '\0';

	return sock;

}

int LocalSocketSend(char *path, int len, void *buf)
{
	int sock;
	//int	sockFlag;
	int addrLen;
	struct sockaddr_un	svr;

	sock = socket(PF_LOCAL, SOCK_DGRAM, 0);
	if(sock <= 0 ){
		//dbg(Err, DbgPerror, "socket error \n");
		return GS_FAIL ;
	}
	#if 0
	//dbg(Dbg, DbgNoPerror, "%s, Create sockFd = %d\n", path, sock);
	sockFlag = fcntl(sock, F_GETFL, 0);
    if ( fcntl(sock, F_SETFL, sockFlag|O_NONBLOCK) == -1 )
    {
        dbg(Err, DbgPerror, "cannot set socket to non-block!");
		close(sock);
        return GS_FAIL;
    }
	#endif 
	
	svr.sun_family 	= PF_LOCAL;
	strncpy(svr.sun_path, path, sizeof(svr.sun_path));
	svr.sun_path[sizeof(svr.sun_path) - 1] = '\0';

	addrLen = SUN_LEN(&svr);
	if( -1 == sendto(sock, buf, len, 0, (struct sockaddr*)&svr, addrLen)){
		//dbg(Err, DbgPerror, "sendto error \n");
		close(sock);
		return GS_FAIL;
	}
	close(sock);
	return GS_SUCCESS;
}

int _Msg_Init(char *path, char cKey)
{
	key_t	key;

	key = ftok(path, cKey);
	return msgget(key, IPC_CREAT | 0666);	
}

int _Msg_Recv(int msgId, MsgData_t *msgData, int flag)
{
	int len; 
	len = sizeof(*msgData) - sizeof(long);
	return msgrcv(msgId, msgData, len, 0, flag);
}

int _Msg_Send(int msgId, MsgData_t *msgData, int flag)
{
	int len; 
	len = sizeof(*msgData) - sizeof(long);
	return msgsnd(msgId, msgData, len, flag);
}

int _Msg_Del(int msgId)
{
	return  msgctl(msgId, IPC_RMID, NULL);
}

int DeviceDetect(char *device)
{
	return access(device,F_OK);
}


int detect_sd_card(void)
{
	int pos = 0;
	char *p, *u, *tmp, *n;
	DIR *pDir = NULL;
	struct dirent *ptr = NULL; 
	
	pDir = opendir("/dev");
	if(!pDir){
		perror("open /dev error ");
		return -1;
	}

	memset(&sd_mmc_device, 0, sizeof(sd_mmc_device_t));
	sd_mmc_device.device_n = 0;
	while(NULL != (ptr = readdir(pDir))){
		if(ptr->d_name[0] == '.'){	//* 排除 ../ ./
			continue;
		}

		p = strstr(ptr->d_name, "mmcblk0");
		if(p != NULL){
			//printf("name: %s \n", ptr->d_name);
			tmp = strstr(ptr->d_name, "mmcblk0p");
			if( tmp != NULL ){
				n = tmp+8;
				if( n != NULL ){
					pos = atoi(n);
					if( pos < 11 && pos >= 0){
						sd_mmc_device.device[pos].iActive = 1;
						sprintf(sd_mmc_device.device[pos].f_name, "/dev/%s",ptr->d_name);
						sprintf(sd_mmc_device.device[pos].mount_cmd, 
							"mount -t vfat %s /opt/dm365/web/www/mnt/sd;", 
							sd_mmc_device.device[pos].f_name);
						sd_mmc_device.device_n ++;
					}
					
				}
			}else if( strcmp(ptr->d_name, "mmcblk0") == 0){
				//printf("mmcblk0 is active ... \n");
				pos = 11;
				sd_mmc_device.device[pos].iActive = 1;
				sprintf(sd_mmc_device.device[pos].f_name, "/dev/%s", ptr->d_name);
				sprintf(sd_mmc_device.device[pos].mount_cmd, 
					"mount -t vfat %s /opt/dm365/web/www/mnt/sd;", 
					sd_mmc_device.device[pos].f_name);
				sd_mmc_device.device_n ++;
			}

			if( sd_mmc_device.device_n > 11 ){
				break; 
			}
			
		}

		u = strstr(ptr->d_name, "sd");
		if(u != NULL){
			//printf("%s \n", ptr->d_name);
		}
	}
	closedir(pDir);
	return sd_mmc_device.device_n;
}

#if 0
/* lwx add  */
int detect_sd_card(void)
{
	char *p, *u;
	DIR *pDir = NULL;
	struct dirent *ptr = NULL; 
	
	pDir = opendir("/dev");
	if(!pDir){
		perror("open /dev error ");
		return -1;
	}

	memset(&sd_mmc_device, 0, sizeof(sd_mmc_device_t));
	sd_mmc_device.device_n = 0;
	while(NULL != (ptr = readdir(pDir))){
		if(ptr->d_name[0] == '.'){	//* 排除 ../ ./
			continue;
		}

		p = strstr(ptr->d_name, "mmcblk0p");
		if(p != NULL){
			sprintf(sd_mmc_device.device[sd_mmc_device.device_n].f_name,
				"/dev/%s", ptr->d_name);
			sprintf(sd_mmc_device.device[sd_mmc_device.device_n].mount_cmd, 
				"mount -t vfat %s /opt/dm365/web/www/mnt/sd;", 
				sd_mmc_device.device[sd_mmc_device.device_n].f_name);
			sd_mmc_device.device_n ++;
			if(sd_mmc_device.device_n == 2){
				break;
			}
		}

		u = strstr(ptr->d_name, "sd");
		if(u != NULL){
			//printf("%s \n", ptr->d_name);
		}
	}
	closedir(pDir);
	return sd_mmc_device.device_n;
}
#endif 

int USB_SD_DeviceDetect(void)
{
	int mask  = 0;

	/* 有设备但存在文件(已经umount)则不能写 */
	if(0 == DeviceDetect(DRV_USB) && 0 != DeviceDetect(UMOUNT_USB_FLAG_FILE)){
		mask |= USB_DETECT_BIT;
	}

	if( detect_sd_card() > 0 && 0 != DeviceDetect(UMOUNT_SD_FLAG_FILE)){
		mask |= SD_DETECT_BIT;
	}
	
	return mask;
}

void LocalTimeLock(void)
{
	pthread_mutex_lock(&gLocalTimeLock);
}

void LocalTimeUnlock(void)
{
	pthread_mutex_unlock(&gLocalTimeLock);
}

int USB_SD_DeviceLockInit(void)
{
	pthread_mutex_init(&gLocalTimeLock, NULL);
	return pthread_mutex_init(&gUsbSdLock, NULL);
}

void USB_SD_DeviceLock(void)
{
	pthread_mutex_lock(&gUsbSdLock);
}

void USB_SD_DeviceUnlock(void)
{
	pthread_mutex_unlock(&gUsbSdLock);
}


int USB_SD_DeviceUmount(int device)
{
	int result = GS_SUCCESS;
	char cmd[256];
	//dbg(Dbg, DbgNoPerror, "lock \n");
	
	pthread_mutex_lock(&gUsbSdLock);
	if(device & USB_DETECT_BIT){
		gUsbSdUmount.usb = USB_SD_IS_UMOUNT;
		//dbg(Dbg, DbgNoPerror, "usb umount \n");
		memset(cmd, 0, 256);
		
		sprintf(cmd, "touch %s ;", UMOUNT_USB_FLAG_FILE);
		//dbg(Dbg, DbgNoPerror, "touch %s\n", cmd);
		system(cmd);
		sync();
		if( -1 == umount((const char *)USB_MOUNT_PATH)){
			if(errno == EBUSY){
				result = EN_UMOUNT_ERR_BUSY;
			}
			//dbg(Err, DbgPerror, "umount %s error \n", USB_MOUNT_PATH);
			#if 0
			printf("umount %s error [%d]\n", USB_MOUNT_PATH, errno);
			perror("err");
			#endif 
		}else{
			//dbg(Info, DbgNoPerror, "umount %s success! \n", USB_MOUNT_PATH);
			#if 0
			printf("umount %s successful! \n", USB_MOUNT_PATH);
			#endif 
			
		}
		sync();
		gUsbSdUmount.usb = USB_SD_IS_MOUNT;
	}

	if(device & SD_DETECT_BIT){
		gUsbSdUmount.sd = USB_SD_IS_UMOUNT;
		memset(cmd, 0, 256);
		sprintf(cmd, "touch %s ;", UMOUNT_SD_FLAG_FILE);
		//dbg(Dbg, DbgNoPerror, "touch %s\n", cmd);
		system(cmd);
		sync();
		if( -1 == umount((const char *)SD_MOUNT_PATH)){
			if(errno == EBUSY){
				result = EN_UMOUNT_ERR_BUSY;
			}
			//dbg(Err, DbgPerror, "umount %s error  \n", SD_MOUNT_PATH);
			#if 0
			printf("umount %s error [%d][EBUSY = %d]\n", 
						SD_MOUNT_PATH, errno, EBUSY);
			perror("err");
			#endif 
		}else{
			//dbg(Info, DbgNoPerror, "umount %s success !\n", SD_MOUNT_PATH);
			#if 0
			printf("umount %s successful! \n", SD_MOUNT_PATH);
			#endif 
		}
		sync();
		gUsbSdUmount.sd = USB_SD_IS_MOUNT;
	}
	//dbg(Dbg, DbgNoPerror, "unlock \n");
	pthread_mutex_unlock(&gUsbSdLock);
	return result;
}


int USB_SD_SaveFile(int inMask, char *fileName, int fileLen, char *context)
{
	int 	fd = -1;
	char	fName[128];
	int 	mask = 0;
	int		deviceMask = 0;

	deviceMask = USB_SD_DeviceDetect();
	if(inMask == NO_MASK){
		if(deviceMask == 0){
			goto __err;
		}
		mask = deviceMask;
	}else {
		mask = inMask;
	}
	
	memset(fName, 0, 128);
	if((mask & USB_DETECT_BIT) && (gUsbSdUmount.usb == USB_SD_IS_MOUNT) &&
												(deviceMask & mask)){
		sprintf(fName, "%s/%s", USB_MOUNT_PATH, fileName);

		fd = open(fName, O_RDWR | O_NONBLOCK | O_CREAT | O_TRUNC | O_SYNC, 00644);
		if(fd <= 0){
			//dbg(Err, DbgPerror, "open %s error \n", fName);
			goto __saveSd;
		}
		lseek(fd, 0, SEEK_SET);
		write(fd, context, fileLen);
		fdatasync(fd);
		fsync(fd);
		close(fd);
		sync();
		goto __ok;
	}
	
	__saveSd:
	if((mask & SD_DETECT_BIT) &&(gUsbSdUmount.sd == USB_SD_IS_MOUNT) && 
								(deviceMask & mask)){
		sprintf(fName, "%s/%s", SD_MOUNT_PATH, fileName);
		fd = open(fName, O_RDWR | O_NONBLOCK | O_CREAT | O_TRUNC | O_SYNC, 00644);
		if(fd <= 0){
			//dbg(Err, DbgPerror, "open %s error \n", fName);
			goto __err;
		}
		lseek(fd, 0, SEEK_SET);
		write(fd, context, fileLen);
		fdatasync(fd);
		fsync(fd);
		close(fd);
		sync();
		goto __ok;
	}
__err:
	if(mask != 0){
		//dbg(Err, DbgNoPerror, "Save file error \n");
	}
	return GS_FAIL; 
__ok:
	//dbg(Info, DbgNoPerror, "Save file ok \n");
	return GS_SUCCESS;
}


int SaveYUV(FILE *pw, char *data, int width, int height)
{
	int w = width, h = height,  dw = width/2, dh = height/2;
	int i,j;
	char tmp[width];
	char *py, *pu, *pv, *p;
	p = data;
	py = p;
	/* save y -------- */
	for(i=0; i<h; i++){
		for(j=0; j<w; j++)
		{
			tmp[j] = *py;
			py++;
		}
		fwrite(tmp, w, 1, pw);
		
	}
	fflush(pw);
	/* save u --------  */
	pu = p + h*w;
	for(i=0; i<dh; i++){
		for(j=0; j<dw; j++){
			tmp[j] = *pu;
			pu += 2;
		}
		fwrite(tmp, dw, 1, pw);
		
	}
	fflush(pw);
	/* save v --------- */
	pv = p + h*w + 1;
	for(i=0; i<dh; i++){
		for(j=0; j<dw; j++){
			tmp[j] = *pv;
			pv += 2;
		}
		fwrite(tmp, dw, 1, pw);
		
	}
	fflush(pw);
	fsCnt ++;
	return fsCnt;
}


u32	Time_GetMsec(void)
{
	u32 	msec;
	struct timeval 	tv;
	struct timezone	tz;

	gettimeofday(&tv, &tz);

	msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return msec;
}

/* =================  读取 /etc/TZ 中的内容 ==============
 * @output		: pTzEnv 将/etc/TZ 中的内容加上 "TZ=" 的头
 * 				: 放入此 buf 
 * @return		: 返回读取的字符串长度
 */
int get_tzenv_buf(char *pTzEnv)
{
	int fd,count = 0;
	
	sprintf(pTzEnv,"TZ=");
	fd = open(TZ_FILE_PATH,O_RDONLY);
	if(fd < 0){
		printf("%s open fail!,using default TZ=GMT+00\n",TZ_FILE_PATH);
		sprintf(pTzEnv + 3,"GMT+00");
		count = strlen("GMT+00");
	}else{
		count = read(fd,pTzEnv+3,120);
		if(count < 0){
			printf("%s read fail!using default TZ=GMT+00\n",TZ_FILE_PATH);
			sprintf(pTzEnv + 3,"GMT+00");
			count = strlen("GMT+00");
		}
	}
	close(fd);

	return count;
}



/* ================ 检查 envTZ 的类型 ===========================
 * @return		: 若为用户定义 		enTimeZoneUserDefined -- 1
 *	            : 若为列表中已有 	enTimeZoneNormail	  -- 2
 *				: 出错				-1
 * @input		: envTz 为从 /etc/TZ 中读取的 TZ环境变量值
 * ============================================================== 
 */
int	get_zone_type(char *envTZ)
{
	int	  iZoneType = -1;
	char *pEnvTz = NULL; 
	char **ppExistTzEnv;

	if( envTZ == NULL){
		printf("input envTZ is NULL \n");
		return -1;
	}
	pEnvTz = strstr(envTZ, "TZ=");
	if( pEnvTz ){
		pEnvTz += 3;
		if( pEnvTz){
			if(!strcmp(pEnvTz, "GMT+00\n")){
				//printf("is gmt time pEnvTz = %s \n", pEnvTz);
				gIsGmtTime = 1;
			}else{
				gIsGmtTime = 0;
			}
			iZoneType = enTimeZoneUserDefined;
			for(ppExistTzEnv=gTimeZoneInfo; *ppExistTzEnv != NULL; 
											ppExistTzEnv ++){

				//printf("*ppExistTzEnv: %s  \n", *ppExistTzEnv);

				/* 到了未尾 */
				if( !strcmp(*ppExistTzEnv, "\0")){
					//printf("is user defined zone \n");
					iZoneType = enTimeZoneUserDefined;
					break; 
				}
				
				//printf("*p = %s pEvnTz = %s\n", *ppExistTzEnv, pEnvTz);
				/* 比较是否和已定义的相同 */
				if(!strcmp(*ppExistTzEnv, pEnvTz)){
					iZoneType = enTimeZoneNormail;
					break; 
				}
			}
		}
	}

	return iZoneType;
}

/* =========================  修改当前时间  ==========================
 * @input		: pEnvTz  /etc/TZ 中的内容
 * @output		: curTime 当前的UTC时间, 外部传入，修改后存入此变量
 * 				: sign    加号还是减号, 外部传入，修改后存入此变量
 * @return 		: 成功返回　０，　否则返回 -1
 * ===================================================================
 */
int modify_half_time_zone(int *min, int *sign, 
										int *hour, char *pEnvTz)
{
	int		iMin = 0, iHour = 0, len = 0,i;
	char	ascMin[16];
	char 	ascHour[16];
	char   *pGmtCst = NULL, *pSign = NULL;

	/* 输入参数检查 */
	if(pEnvTz == NULL || min == NULL || sign == NULL ){
		printf("Input params error \n");
		return -1;
	}

	pGmtCst = strstr(pEnvTz, "GMT");
	if(!pGmtCst){
		pGmtCst = strstr(pEnvTz, "CST");
	}
	if(pGmtCst){
		pSign = pGmtCst+3;
		/* 获取符号 */
		if(pSign){
			if(*pSign == '+'){
				*sign = ADD;
			}else if(*pSign == '-'){
				*sign = SUB;
			}else{
				*sign = UNDEFINED;
			}
		}

		memset(ascHour, 0, 2);
		memset(ascMin, 0, 2);
		pGmtCst = pSign + 1;
		len = 0;
		if(pGmtCst){
			/* find out the hour */
			for(i=0; i<2; i++){
				if(isdigit(pGmtCst[i])){
					len ++;	
				}else{
					break; 
				}
			}
			memcpy(ascHour, pGmtCst, len);
			iHour = atoi(ascHour);

			/* 有 ':' 则表明有非整数倍时区存在 */
			pSign = strchr(pGmtCst, ':');
			if(pSign){
				pSign += 1;
				if( pSign){
					for(i=0; i<2; i++){
						if(isdigit(pSign[i])){
							len ++;	
						}else{
							break; 
						}
					}
					memcpy(ascMin, pSign, len); 
					iMin = atoi(ascMin);
				}
			}
		}
	}

	*hour   = iHour;
	*min 	= iMin;
	
	return 0;
}


int prase_user_defined_time_zone(TimeZoneInfo_t *pDstTzInfo, 
													char *pTzEnv)
{
	int  i, len = 0;
	char tmp[16];
	char *pSrc = pTzEnv;
	char *pSign = NULL, *pTmp = NULL; 

	if(pDstTzInfo == NULL || pTzEnv == NULL ){
		printf("input params is NULL \n");
		return -1;
	}

	pSrc = pTzEnv;
	/* example: pSrc=pTzEnv="MTZ+6MDT+5,M3.2.0,M11.1.0"; */
	pSign = strchr(pSrc, '+');
	if(!pSign){
		pSign = strchr(pSrc, '-');
	}else{
		/* 字符串同时存在 '+'和'-', 有两种情况: 
		 * 若 '+' 在前, 则 pSign 指向第一个符号的位置,(pSign - pTmp) < 0;
		 * 若 '+' 在后边，则 pSign 指向的是 '+' 的位置, 在第一个符号的后边，
		 * 而 pTmp 又指向的是第一个符号的位置,故 (pSign - pTmp) > 0;
		 */
		pTmp = strchr(pSrc, '-');
		if(pTmp){
			if((pSign - pTmp) > 0){
				pSign = pTmp;
			}
		}
	}
	/* example: pSign = "+6MDT+5,M3.2.0,M11.1.0" */
	if(pSign){
		/* find time zone offset */
		if( *pSign == '+'){
			pDstTzInfo->offsetSign = ADD;
		}else if(*pSign == '-'){
			pDstTzInfo->offsetSign = SUB; 
		}
		pSign += 1;
		/* example: pSign = "6MDT+5,M3.2.0,M11.1.0" */
		if(pSign){
			len = 0;
			/* 获取时区的偏移值 */
			for(i=0; i<2; i++){
				if(isdigit(pSign[i])){
					len ++;
				}else{
					break; 
				}
			}
			memcpy(tmp, pSign, len);
			pDstTzInfo->timeZoneOffset = atoi(tmp);
			pSrc = pSign + len; 
			/* example: pSrc = "MDT+5,M3.2.0,M11.1.0" */
		}

		
		pSign = strchr(pSrc, '+');
		if(!pSign){
			pSign = strchr(pSrc, '-');
		} 
		/* example: pSign = "+5,M3.2.0,M11.1.0" */
		
		if(pSign){
			/* 用户设置了夏令时 */
			pDstTzInfo->isDayLightTime = 1;
			if( *pSign == '+'){
				pDstTzInfo->dayLightTimeZoneSign = ADD;
			}else if(*pSign == '-'){
				pDstTzInfo->dayLightTimeZoneSign = SUB; 
			}
			pSign += 1;
			/* example: pSign = "5,M3.2.0,M11.1.0" */
			/* 获取夏令时时区的偏移值 */
			if(pSign){
				len = 0;
				for(i=0; i<2; i++){
					if(isdigit(pSign[i])){
						len ++;
					}else{
						break; 
					}
				}
				memcpy(tmp, pSign, len);
				pDstTzInfo->dayLightTimeZoneOffset = atoi(tmp);
				pSrc = pSign + len; 
				/* example: pSrc = ",M3.2.0,M11.1.0" */
			}
			//printf("src = %s \n", pSrc);
			/* example: pSrc = ",M3.2.0,M11.1.0"; */
			pSign = strstr(pSrc, ",M");
			if(pSign){
				//printf("0 pSign = %s \n", pSign);
				pSrc = pSign + 2;
				/* example: pSrc = "3.2.0,M11.1.0"; */
				if(pSrc){
					/* 获取夏令时的开始月份 */
					pSign = strchr(pSrc, '.');
					if(pSign){
						//printf("1 pSign = %s \n", pSign);
						/* example: pSign = ".2.0,M11.1.0"; */
						memset(tmp, 0, 16);
						memcpy(tmp, pSrc, pSign-pSrc);
						pDstTzInfo->startDayLightTimeMon = atoi(tmp);
						pSrc = pSign+1;
						/* example: pSrc = "2.0,M11.1.0"; */
						/* 获取夏令时的开始周 */
						pSign = strchr(pSrc, '.');
						if(pSign){
							//printf("2 pSign = %s \n", pSign);
							/* example: pSign = ".0,M11.1.0"; */
							memset(tmp, 0, 16);
							memcpy(tmp, pSrc, pSign-pSrc);
							pDstTzInfo->startDayLightTimeWeek = atoi(tmp);
							pSrc = pSign+1;
							/* example: pSrc = "0,M11.1.0"; */
							/* 获取夏令时的开始星期 */
							pSign = strstr(pSrc, ",M");
							if(pSign){
								//printf("3 pSign = %s \n", pSign);
								/* example: pSign = ",M11.1.0"; */
								memset(tmp, 0, 16);
								memcpy(tmp, pSrc, pSign-pSrc);
								pDstTzInfo->startDayLightTimeWeekDay = atoi(tmp);
								pSrc = pSign+2;
								/* example: pSrc = "11.1.0"; */
								/* 获取夏令时的结束月份 */
								pSign = strchr(pSrc, '.');
								if(pSign){
									//printf("4 pSign = %s \n", pSign);
									/* example: pSrc = ".1.0"; */
									memset(tmp, 0, 16);
									memcpy(tmp, pSrc, pSign-pSrc);
									pDstTzInfo->endDayLightTimeMon = atoi(tmp);
									pSrc = pSign+1;
									
									/* example: pSrc = "1.0"; */
									/* 获取夏令时的结束周 */
									pSign = strchr(pSrc, '.');
									if(pSign){
										//printf("5 pSign = %s \n", pSign);
										/* example: pSrc = ".0"; */
										memset(tmp, 0, 16);
										memcpy(tmp, pSrc, pSign-pSrc);
										pDstTzInfo->endDayLightTimeWeek = atoi(tmp);
										pSrc = pSign+1;
										memset(tmp, 0, 16);
										tmp[0] = *pSrc;
										/* example: pSrc = "0"; */
										/* 获取夏令时的结束星期 */
										pDstTzInfo->endDayLightTimeWeekDay = atoi(tmp);
									}
								}
							}
						}
					}
				}
			}
		}else{
		/* 没有设置夏令时 */
			pDstTzInfo->isDayLightTime = 0;
		}
	}else{
		printf("Err: Check you input self-defined timezone: %s\n", pTzEnv);
		return -1;
	}

	return 0;
}

int get_mday(int week, int wday, struct tm *pstTm)
{
	int iMday, iWday, weekDayOffset = 0;
	int nWeek, findDay = -1, firstday = 0;

	if(pstTm == NULL){
		printf("input params is NULL \n");
		return -1;
	}

	iMday = pstTm->tm_mday;
	iWday = pstTm->tm_wday;

	if(iMday > 7){
		weekDayOffset = wday - iWday;
		iMday += weekDayOffset;
		firstday = iMday%7;
		if(firstday == 0){
			firstday = 7;
		}
	}else{
		if(iMday > iWday){
			while(iMday){
				if(iWday == wday){
					firstday = iMday;
					break; 
				}
				iMday --;
				iWday --;
				if(iWday < 0){
					iWday = 6;
				}
			}
		}else{
			while(iMday <= 7){
				if(iWday == wday){
					firstday = iMday;
					break; 
				}
				iMday ++;
				iWday ++;
				if(iWday > 6){
					iWday = 0;
				}
			}
		}
	}

	nWeek = 0;
	if( week > 0){
		nWeek = week - 1;
	}
	findDay = nWeek * 7 + firstday;
	
	return findDay;
}

int check_if_change_daylight_time(int *isDayLightTime, 
					struct tm *pstTm, TimeZoneInfo_t *pstTzInfo)
{
	int findDay = -1;
	if(pstTm == NULL || pstTzInfo == NULL || isDayLightTime == NULL){
		printf("input params is NULL \n");
		return -1;
	}

	//printf("startMon = %d, endMon = %d, curMon = %d \n", 
	//	pstTzInfo->startDayLightTimeMon, 
	//	pstTzInfo->endDayLightTimeMon, pstTm->tm_mon+1);

	if((pstTzInfo->startDayLightTimeMon <= (pstTm->tm_mon+1)) &&
		(pstTzInfo->endDayLightTimeMon >= (pstTm->tm_mon+1))){
		
		findDay = get_mday(pstTzInfo->startDayLightTimeWeek, 
						pstTzInfo->startDayLightTimeWeekDay, pstTm);
		if(findDay == -1){
			printf("Error: get_mday error \n");
			return -1;
		}
		*isDayLightTime = enIsDayLightTime;

		/* 开始月跟当前月在同一月，则检查日期 */
		if(pstTzInfo->startDayLightTimeMon == (pstTm->tm_mon+1)){
			//printf("Start: today: %d, findDay = %d \n", pstTm->tm_mday, findDay);
			if( pstTm->tm_mday <= findDay){
				*isDayLightTime = enIsNormailTime;
			}
		}

		findDay = get_mday(pstTzInfo->endDayLightTimeWeek, 
						pstTzInfo->endDayLightTimeWeekDay, pstTm);
		if(findDay == -1){
			printf("Error: get_mday error \n");
			return -1;
		}
		if(pstTzInfo->endDayLightTimeMon == (pstTm->tm_mon+1)){
			//printf("End: today: %d, findDay = %d \n", pstTm->tm_mday, findDay);
			if( pstTm->tm_mday >= findDay){
				*isDayLightTime = enIsNormailTime;
			}
		}
		
	}else{
		*isDayLightTime = enIsNormailTime;
	}
	
	return 0;
}




struct tm *sTime_GetLocalTime(struct tm *pstTm)
{
	time_t	curTime;
	int  iTimeZoneType, sign;
	int  iHour, iMin, ret, isDayLightTime = enIsDayLightUnknown;
	char ascTzEnv[128];
	char tzCmd[256];
	char cFlag = 0;
	struct tm *pCurTime;
	TimeZoneInfo_t stTzInfo;

	memset(ascTzEnv, 0, 128);
	tzset();
	
	/* --------------- test ----------- */	
	/* 读取 /etc/TZ 中的内容 */
	if(get_tzenv_buf(ascTzEnv) < 0){
		printf("get_tzenv_buf error \n");
		return NULL; 
	}

	/* 获取时区的定义类型 */
	//printf("get_zone_type.... \n");
	iTimeZoneType = get_zone_type(ascTzEnv);
	if(iTimeZoneType == -1){
		printf("get_zone_type error \n");
		return NULL; 
	}
	//printf("get_zone_type.... %d\n", iTimeZoneType);

	/* 为用户自定义 */
	if( iTimeZoneType == enTimeZoneUserDefined ){
		//printf("UseDefinedTimeZone \n");
		memset(&stTzInfo, 0, sizeof(TimeZoneInfo_t));
		if( 0 != prase_user_defined_time_zone(&stTzInfo, ascTzEnv)){
			printf("prase_user_defined_time_zone error , set tz to GMT\n");
			system("/usr/share/zoneinfo/Etc/GMT /data/zone/tz");
			tzset();
			time(&curTime);
			pCurTime = localtime(&curTime);
			memcpy(pstTm, pCurTime, sizeof(struct tm));
		}else{
			/* 成功解析用户自定义时区 */
			memset(tzCmd, 0, 256);
			if(stTzInfo.offsetSign == ADD){
				cFlag = '+';
			}else if (stTzInfo.offsetSign == SUB){
				cFlag = '-';
			}
			sprintf(tzCmd, "cp %s%s%c%d %s", TZINFO_PATH,"GMT", cFlag, 
				stTzInfo.timeZoneOffset, TZ_DST);
			//printf("syscmd: %s \n", tzCmd);
			system(tzCmd);
			tzset();
			time(&curTime);
			pCurTime = localtime(&curTime);
			memcpy(pstTm, pCurTime, sizeof(struct tm));
			if(check_if_change_daylight_time(&isDayLightTime,
										pCurTime, &stTzInfo) == -1){
				printf("check_if_change_daylight_time error \n");
			}

			if( isDayLightTime == enIsDayLightTime ){
				memset(tzCmd, 0, 256);
				if(stTzInfo.dayLightTimeZoneSign == ADD){
					cFlag = '+';
				}else if (stTzInfo.dayLightTimeZoneSign == SUB){
					cFlag = '-';
				}
				
				sprintf(tzCmd, "cp %s%s%c%d %s", TZINFO_PATH,"GMT", cFlag, 
					stTzInfo.dayLightTimeZoneOffset, TZ_DST);
				//printf("daylight saving time: syscmd: %s \n", tzCmd);
				time(&curTime);
				curTime += 3600;
				pCurTime = localtime(&curTime);
				memcpy(pstTm, pCurTime, sizeof(struct tm));
				//printf("%02d:%02d:%02d \n", pCurTime->tm_hour, pCurTime->tm_min,
				//	pCurTime->tm_sec);
				//system(tzCmd);
				//sync();
				//tzset();
			}
		}
	}else if(iTimeZoneType == enTimeZoneNormail ){
	//printf("normail time \n");
	/* 为已有选项的时区 */
		
		ret = modify_half_time_zone(&iMin, &sign, &iHour, ascTzEnv);
		tzset();
		time(&curTime);
		if(sign == ADD){
			curTime += iMin*60;
		}else if(sign == SUB){
			curTime -= iMin*60;
		}
		pCurTime = localtime(&curTime);
		memcpy(pstTm, pCurTime, sizeof(struct tm));
	}
	
	return pstTm;
}

char gascTzEnvBuf[256];

int update_tzenv_buf(void)
{
	int fd,count = 0;
	//struct tm stTm;
	//struct tm *pstTm = NULL; 
	memset(gascTzEnvBuf, 0, 256);

	#if 1
	sprintf(gascTzEnvBuf,"TZ=");
	fd = open(TZ_FILE_PATH,O_RDONLY);
	if(fd < 0){
		printf("%s open fail!,using default TZ=GMT+00\n",TZ_FILE_PATH);
		sprintf(gascTzEnvBuf + 3,"GMT+00");
		count = strlen("GMT+00");
	}else{
		count = read(fd,gascTzEnvBuf+3,120);
		if(count < 0){
			printf("%s read fail!using default TZ=GMT+00\n",TZ_FILE_PATH);
			sprintf(gascTzEnvBuf + 3,"GMT+00");
			count = strlen("GMT+00");
		}
	}
	close(fd);
	#endif 
	//printf("sTime_GetLocalTime: %s  \n", gascTzEnvBuf);
	//sTime_GetLocalTime(&stTm);
	update_dn();
	return 0; 
}


#define DN_F	"/data/zone/isdnt"
static int gIsDayLightTime = 0;
int update_dn(void)
{
	int fd,count = 0;
	char 	buf[128];
	char 	*p;
	
	memset(buf, 0, 128);

	fd = open(DN_F,O_RDONLY);
	if(fd < 0){
		printf("open %s error \n", DN_F);
		gIsDayLightTime = 0;
	}else{
		count = read(fd,buf,128);
		//printf("buf: %s \n", buf);
		if(count < 0){
			printf("%s read fail!using default TZ=GMT+00\n",DN_F);
			gIsDayLightTime = 0;
		}else{
			p = strstr(buf, "dnt=");
			if(p){
				//printf("p=%s\n", p);
				p += 4;
				if(p){
					//printf("p=%s\n", p);
					gIsDayLightTime = atoi(p);
					//printf("isDayLightTime = %d \n", gIsDayLightTime);
				}
			}
		}
	}
	close(fd);
	return 0; 
}

int get_tm_time(char *t_src, struct tm *pTm)
{
	char tmp[6];
	char *p = t_src;

	memset(tmp, 0, 6);
	memcpy(tmp, p, 2);
	pTm->tm_hour = atoi(tmp);
	p+=3;
	memset(tmp, 0, 6);
	memcpy(tmp, p, 2);
	pTm->tm_min  = atoi(tmp) +  gMin;
	p+=3;
	pTm->tm_sec  = atoi(p);

	return 0; 
}

typedef struct tm_mon_s{
	int 		mon;
	char		ascMon[4];
}tm_mon_t;

tm_mon_t gMonInfo[] = {
		{1, "Jan"},
		{2, "Feb"},
		{3, "Mar"},
		{4, "Apr"},
		{5, "May"},
		{6, "Jun"},
		{7, "Jul"},
		{8, "Aug"},
		{9, "Sep"},
		{10, "Oct"},
		{11, "Nov"},
		{12, "Dec"},
		{-1, "End"},
};

int get_tm_mon(char *mon)
{
	tm_mon_t *p;

	for(p=gMonInfo; p->mon != -1; p++){
		if(!strcmp(p->ascMon, mon)){
			//printf("%s, %s, %d\n", p->ascMon, mon, p->mon);
			return p->mon;
		}
	}
	return -1;
}

int get_date_time(char *time, struct tm *pstTm)
{
	int		n = 0;
	char 	tmp[4];
	char 	*src, *p;

	src = time;
	p = src;
	while(p){
		p = strchr(src, ':');
		memset(tmp, 0, 4);
		memcpy(tmp, src, p-src);

		switch(n){
			case 0:
				pstTm->tm_hour = atoi(tmp);
				break; 

			case 1:
				pstTm->tm_min = atoi(tmp) + gMin;
				break; 

			default:
				break; 
		}
		
		src = p + 1;
		if( n == 1){
			pstTm->tm_sec = atoi(src);
			break;
		}
		n ++;
	}
	return GS_SUCCESS; 
}

int get_date_info(struct tm *pstTm)
{
	int		n = 0;
	FILE *fp;
	char v_str[128];
	char tmp[32];
	char *src, *p;
	
	fp=popen("date","r");
	if (fp==NULL){
		printf("popen err\n\n");
		pclose(fp);
		return -1;
	}else{
		memset(v_str,0,sizeof(v_str));
		fgets(v_str,sizeof(v_str),fp);
		pclose(fp);
	}
	src = v_str;
	//printf("date: %s \n", v_str);

	p = src;

	n = 0;
	while(p){
		p = strchr(src, ' ');
		memset(tmp, 0, 32);
		memcpy(tmp, src, p - src);
		switch(n){
			case 0:
				
				break; 

			case 1:
				pstTm->tm_mon = get_tm_mon(tmp) - 1;
				break; 

			case 2:
				pstTm->tm_mday = atoi(tmp);
				break; 

			case 3:
				get_date_time(tmp, pstTm);
				break; 

			default:
				break;
		}
		src = p + 1;
		n ++;
		if(n == 5){
			pstTm->tm_year = atoi(src) - 1900;
			break;
		}
	}

	return 0;
}


time_t gCurTime;
struct tm *Time_GetLocalTime(struct tm *pstTm)
{
	int ret; 
	int iMin, sign, iHour;
	int iTimeZoneType;
	
	time(&gCurTime);
	iTimeZoneType = get_zone_type(gascTzEnvBuf);
	if(iTimeZoneType == -1){
		printf("get_zone_type error \n");
		return NULL; 
	}

	/* 自定义时区时，只要使能了夏令时，
	 * 则时间比原来的时间要快一个小时 
	 */
	if(iTimeZoneType == enTimeZoneUserDefined){
		if(gIsDayLightTime  == enIsDayLightTime){
			gCurTime += 3600;
		}
	}else{
		ret = modify_half_time_zone(&iMin, &sign, &iHour, gascTzEnvBuf);		
		if(sign == ADD){
			gCurTime += iMin*60;
			gMin = iMin;
		}else if(sign == SUB){
			gCurTime -= iMin*60;
			gMin = -iMin;
		}
	}
	tzset();
	memcpy(pstTm, localtime(&gCurTime), sizeof(struct tm));
	#if 0
	
	if(gIsGmtTime == 0){
		if(timezone == 0){
			#if 0
			printf("Err: %04d-%02d-%02d %02d:%02d:%02d \n", 
				pstTm->tm_year + 1900, pstTm->tm_mon+1, 
				pstTm->tm_mday, pstTm->tm_hour, 
				pstTm->tm_min, pstTm->tm_sec);
			#endif 
			get_date_info(pstTm);
		}
	}
	#endif 
	return pstTm; 
}

struct tm *Time_GetTzTime(struct tm *pstTm, time_t *curTime)
{
	int  iTimeZoneType, sign;
	int  iHour, iMin, ret; 

	iTimeZoneType = get_zone_type(gascTzEnvBuf);
	if(iTimeZoneType == -1){
		printf("get_zone_type error \n");
		return NULL; 
	}

	if(iTimeZoneType == enTimeZoneUserDefined){
		if(gIsDayLightTime  == enIsDayLightTime){
			*curTime += 3600;
		}
	}else{
		ret = modify_half_time_zone(&iMin, &sign, &iHour, gascTzEnvBuf);
		//printf("env: %s \n", gascTzEnvBuf);
		//time(&gCurTime);
		if(sign == ADD){
			*curTime += iMin*60;
			gMin      = iMin;
		}else if(sign == SUB){
			*curTime -= iMin*60;
			gMin      = -iMin; 
		}
	}
	tzset();
	memcpy(pstTm,  localtime(curTime), sizeof(struct tm));
	#if 0
	if(gIsGmtTime == 0){
		if(timezone == 0){
			#if 0
			printf("Err: %04d-%02d-%02d %02d:%02d:%02d \n", 
				pstTm->tm_year + 1900, pstTm->tm_mon+1, 
				pstTm->tm_mday, pstTm->tm_hour, 
				pstTm->tm_min, pstTm->tm_sec);
			#endif 
			get_date_info(pstTm);
		}
	}
	#endif 
	
	return pstTm;
}


int Proc_GetProductId(void)
{
	char buf[64];
	char *p = NULL; 
	FILE *pPn = fopen(PN_FILE, "r");
	if(pPn == NULL){
		//dbg(Err, DbgPerror, "fopen PN file error \n");
		return GS_FAIL; 
	}
	memset(buf, 0, 64);
	if(NULL == fgets(buf, 64, pPn)){
		//dbg(Err, DbgPerror, "fget PN error \n");
		fclose(pPn);
		pPn = NULL; 
		return GS_FAIL; 
	}
	fclose(pPn);
	
	
	
	//dbg(Err, DbgNoPerror, "Unknown PN = %s \n", buf);
	return GS_FAIL;
}

int Proc_GetOsdTextLang(void)
{
	int lang;// = EN_OSD_LANG_ENGLISH; 
	char buf[64];
	//char *p = NULL; 
	FILE *pLang = fopen(LANG_FILE, "r");
	
	if(pLang == NULL){
		//dbg(Err, DbgPerror, "fopen PN file error \n");
		return GS_FAIL; 
	}
	memset(buf, 0, 64);
	if(NULL == fgets(buf, 64, pLang)){
		//dbg(Err, DbgPerror, "fget PN error \n");
		fclose(pLang);
		pLang = NULL; 
		return GS_FAIL; 
	}
	fclose(pLang);
#if 0
	switch(buf[0]){
		case '0':
			lang = EN_OSD_LANG_ENGLISH;
			printf("[OSD]: English\n");
			break;

		case '1':
			lang = EN_OSD_LANG_CHINESE;
			printf("[OSD]: Chinese\n");
			break; 

		default:
			dbg(Err, DbgPerror, "Unknown language, use default language !\n");
			break;
	}
#endif
	return lang; 
}

int Get_LanguageConf(void)
{
	int	lang;// = EN_OSD_LANG_ENGLISH;
	int	 tmp = -1;
	char line_buf[256] = {0};
	FILE * fd;
	char *p = NULL;
	char *q = NULL;

	if ( NULL == (fd=fopen(LANGUAGE_CONF,"r")) )
	{
		perror("open language config file error ");
		return lang;
	}

	while(NULL != fgets(line_buf,256,fd))
	{		
		if((NULL == strchr(line_buf,'='))||(strlen(line_buf) < 6))
		{
			continue;
		}
		if ((p = strtok(line_buf, "=")) != (char *)NULL) 
		{
			if((q = strtok(NULL, "=")) != (char *)NULL)
			{
				if(0 == strcmp(p,"LANGUAGE"))
				{
					tmp = (int)atoi(q);		
				}
				else
				{
				}
			}
		}
	}

	fclose(fd);


	
	return lang;
}


int Get_EncodeDecodeConf(void)
{
	int	 type = EN_TYPE_ENCODE;
	int	 tmp = -1;
	char line_buf[256] = {0};
	FILE * fd;
	char *p = NULL;
	char *q = NULL;

	if ( NULL == (fd=fopen(ENC_DEC_CONF,"r")) )
	{
		perror("open encode/decode config file error ");
		return type;
	}

	while(NULL != fgets(line_buf,256,fd))
	{		
		if((NULL == strchr(line_buf,'='))||(strlen(line_buf) < 6))
		{
			continue;
		}
		if ((p = strtok(line_buf, "=")) != (char *)NULL) 
		{
			if((q = strtok(NULL, "=")) != (char *)NULL)
			{
				if(0 == strcmp(p,"default_type"))
				{
					tmp = (int)atoi(q);		
				}
				else
				{
				}
			}
		}
	}

	fclose(fd);

	if(tmp == 0){
		type = EN_TYPE_ENCODE;
	}else{
		type = EN_TYPE_DECODE;
	}
	
	return type;
}


int GetMaxFrameRate(int width, int height)
{
	int productId = 0;
	int maxFrameRate = 20;

	
	
	return maxFrameRate;
}

int GetFrameSkipBit(int mainFrameRate, int subFrameRate)
{
	int i;
	int frameRateBit = 0;
	
	for(i=0; i<mainFrameRate; i++){
		frameRateBit |= (1<<i);
	}	

	switch(mainFrameRate){
		case 30:
			switch(subFrameRate){
				case 25:
					frameRateBit &= 0x3efbefbe;
					break; 

				case 20:
					frameRateBit &= 0x1b6db6db;
					break;

				case 15:
					frameRateBit &= 0x15555555;
					break; 

				case 13:
					frameRateBit &= 0x15155155;
					break; 

				case 10:
					frameRateBit &= (~0x36db6db6);
					break; 

				case 5:
					frameRateBit &= (~0x3efbefbe);
					break; 

				case 4:
					frameRateBit &= (~0x3fdfbf7e);
					break; 

				case 3:
					frameRateBit &= (~0x3feffbfe);
					break; 

				case 2:
					frameRateBit &= (~0x3fff7ffe);
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break;

		case 25:
			switch(subFrameRate){
				case 20:
					frameRateBit &= 0x1ef7bde;
					break;

				case 15:
					frameRateBit &= 0x16d6b5a;
					break; 

				case 13:
					frameRateBit &= 0x1555555;
					break; 

				case 10:
					frameRateBit &= (~0x16d6b5a);
					break; 

				case 5:
					frameRateBit &= (~0x1ef7bde);
					break; 

				case 4:
					frameRateBit &= (~0x1fbefbe);
					break; 

				case 3:
					frameRateBit &= (~0x1fefefe);
					break; 

				case 2:
					frameRateBit &= (~0x1ffeffe);
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		case 20:
			switch(subFrameRate){
				case 15:
					frameRateBit &= 0xeeeee;
					break; 

				case 13:
					frameRateBit &= 0x57577;
					break; 

				case 10:
					frameRateBit &= 0x55555;
					break; 

				case 5:
					frameRateBit &= (~0xeeeee);
					break; 

				case 4:
					frameRateBit &= (~0xf7bde);
					break; 

				case 3:
					frameRateBit &= (~0xfbf7e);
					break; 

				case 2:
					frameRateBit &= (~0xffbfe);
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break;

		case 15:
			switch(subFrameRate){
				case 13:
					frameRateBit &= 0x7efe;
					break; 

				case 10:
					frameRateBit &= 0x6db6;
					break; 

				case 5:
					frameRateBit &= (~0x6db6);
					break; 

				case 4:
					frameRateBit &= (~0x6eee);
					break; 

				case 3:
					frameRateBit &= (~0x7bde);
					break; 

				case 2:
					frameRateBit &= (~0x7efe);
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		case 13:
			switch(subFrameRate){
				case 10:
					frameRateBit &= 0xdef;
					break; 

				case 5:
					frameRateBit &= (~0x15b6);
					break; 

				case 4:
					frameRateBit &= (~0xeee);
					break; 

				case 3:
					frameRateBit &= (~0x1bde);
					break; 

				case 2:
					frameRateBit &= (~0x1fbe);
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		case 10:
			switch(subFrameRate){
				case 5:
					frameRateBit &= (~0x155);
					break; 

				case 4:
					frameRateBit &= (~0x1b6);
					break; 

				case 3:
					frameRateBit &= (~0x2ee);
					break; 

				case 2:
					frameRateBit &= (~0x3de);
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		case 5:
			switch(subFrameRate){
				case 4:
					frameRateBit &= (~0x1e);
					break; 

				case 3:
					frameRateBit &= (~0xa);
					break; 

				case 2:
					frameRateBit &= (~0x15);
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		case 4:
			switch(subFrameRate){
				case 3:
					frameRateBit &= (~(1<<3));
					break; 

				case 2:
					frameRateBit &= (~(0x5<<1));
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		case 3:
			switch(subFrameRate){
				case 2:
					frameRateBit &= (~(1<<2));
					break; 

				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		case 2:
			switch(subFrameRate){
				case 1:
					frameRateBit &= 1;
					break; 

				default:
					break; 
			}
			break; 

		default:
			break; 
	}
	
	return frameRateBit;
}

int ADC_CheckTest(void)
{
	return access(TEST_TMP_FILE, F_OK);
}

int Device_GetIdName(char *name)
{
#if 0
	char *p = name;
	int id = CFG_GetProductId();

	if( id == ID_IP5150 ){
		p += sprintf(p, "IP");
	}else{
		p += sprintf(p, "GXV");
	}

	p += sprintf(p, "%x", id&0xffff);
	
#endif

	return GS_SUCCESS; 
}


int Net_GetDeviceName(char *dev)
{
	return GS_SUCCESS; 
}


int UTIL_GetLine(int fd, char *pcLineBuf)
{
	char *pcPose;
	char cChar;

	if ( !pcLineBuf )
	{
		printf("NULL");
		return -1;
	}

	pcPose = pcLineBuf;

	for (;;)
	{
		if ( -1 == read(fd, &cChar, 1) )
		{
			return -1;
		}

		if ( (cChar=='\r') ||(cChar=='\n') )
		{
			break;
		}

		*pcPose++ = cChar;
	}

	*pcPose = 0;
	if ( cChar == '\r' )
	{
		/*skip the '\n'*/
		read(fd, &cChar, 1);
	}

	return 0;
}

/***********************************************************************************
*Function Name   	:UTIL_GetTagIntByName()

*Output          	:sip_conf     sip  配置 信息结构体对象指针
*Return          	:
*Other           	:
***********************************************************************************/

int UTIL_GetTagIntByName(char *pcTagName, unsigned int *pulTagVal)
{
    char acLineBuf[1024] = {0};
    int fd;
    char *pcHead;
    char cFind = 0;
	int n = 0;
    if ( !pcTagName || !pulTagVal )
    {
        printf("NULL");
        return -1;
    }

    if ( -1 == (fd=open(SIP_CONF_FILE, O_RDONLY)) )
    {
        printf("open file Error!");
        return -1;
    }

    while ( 0 == UTIL_GetLine(fd, acLineBuf) )
    {

		//printf("n=%d\n", n); 
		//printf("buf = %s , len = %d \n", acLineBuf, strlen(acLineBuf));

		if(n > 15 && strlen(acLineBuf) == 0){
			break; 
		}
				
        if ( acLineBuf[0] == ';' )
            continue;
		n++;
        /*search the tag string*/
        if ( 0 == strncmp(acLineBuf, pcTagName, strlen(pcTagName)) )
        {
            cFind = 1;
            break;
        }
		//printf("t %d \n", n);
    }

    close(fd);
    if ( cFind == 1 )
    {
        pcHead = strchr(acLineBuf, '=');
        if ( !pcHead )
        {
            printf("Illegal sytax!");
            return -1;
        }
        pcHead++;

        while ( *pcHead == 0x20 )
        {
            pcHead++;
        }

        *pulTagVal = (unsigned int)atol(pcHead);
    }
    else
    {
        *pulTagVal = 0;
    }

    return 0;
}


unsigned int GetSipStreamNo(void)
{
	unsigned int sipStream;// = SIP_STREAM_SUB;
	/* PRIMARY_STREAM=x, x=0, 子码流, x=1, 主码流 
	 * PRIMARY_STREAM不存在默认为子码流
	 */
	
	UTIL_GetTagIntByName("PRIMARY_STREAM", &sipStream);
	printf("[SIP]: %d \n", sipStream);
	return sipStream; 
}


int Thread_SetAttr(pthread_attr_t *pAttr, int pri)
{
	int statu;
	
	struct sched_param schedprm;

	statu = pthread_attr_init(pAttr);
	if( statu != 0 ){
		//dbg(Err, DbgPerror, "pthread_attr_init capture thread error \n");
		return GS_FAIL; 
	}

	statu |= pthread_attr_setinheritsched(pAttr, PTHREAD_EXPLICIT_SCHED);
	statu |= pthread_attr_setschedpolicy(pAttr, SCHED_FIFO);
#if 0
	if( pri > THREAD_PRI_MAX ){
		pri = THREAD_PRI_MAX; 
	}

	if ( pri < THREAD_PRI_MIN ){
		pri = THREAD_PRI_MIN; 
	}
#endif
	schedprm.sched_priority = pri; 
	statu |= pthread_attr_setschedparam(pAttr, &schedprm);

	if( statu != 0 ){
		//dbg(Err, DbgPerror, "set thread attr error \n");
		return GS_FAIL;
	}

	return GS_SUCCESS; 
}

char get_gateway(char* pIfName,char* pGateway)
{
#if 0
	struct rtentry sGw;
       char cBuf[1024] = {0};
    	   char cIface[16] = {0};
    struct sockaddr_in *pDst, *pGw, *pMask;
    int nIflags, nMetric, nRefcnt, nUse, nMss, nWindow, nIrtt;
    long lGateAddr = 0, lMaskAddr, lNetAddr;
    FILE* pFp;
	 
    if ( ( pIfName == NULL ) || ( pGateway == NULL ) )
//if ( pGateway == NULL )
    {
        printf ( "get_gateway_addr: input the invalid argument!\n" );
        return -1;
    }

    pGw = ( struct sockaddr_in * ) &sGw.rt_gateway;
    pDst = ( struct sockaddr_in * ) &sGw.rt_dst;
    pMask = ( struct sockaddr_in * ) &sGw.rt_genmask;
    memset ( &sGw, 0, sizeof ( struct rtentry ) );
	
    //纭畾鍗忚绫诲瀷
    pDst->sin_family = AF_INET;
    pGw->sin_family = AF_INET;
    pMask->sin_family = AF_INET;	
	
    pFp = fopen ( _PATH_PROCNET_ROUTE, "r" );
    if ( !pFp )
    {
        printf ( "get_gateway_addr: Unable to open %s: %s\n", _PATH_PROCNET_ROUTE,
                 strerror ( errno ) );
        return -2;
    }
	
    fgets ( cBuf, 1023, pFp );
    while ( fgets ( cBuf, 1023, pFp ) )
    {
        sscanf ( cBuf, "%16s %lX %lX %X %d %d %d %lX %d %d %d\n", cIface, &lNetAddr, &lGateAddr,
                 &nIflags, &nRefcnt, &nUse, &nMetric, &lMaskAddr, &nMss, &nWindow, &nIrtt );
        if ( lNetAddr != 0 )
            continue;
        if ( strcmp ( cIface, pIfName ) != 0 )
            continue;         
        sGw.rt_flags = nIflags;
        sGw.rt_metric = nMetric;
        pGw->sin_addr.s_addr = lGateAddr;
        pDst->sin_addr.s_addr = lNetAddr;
        pMask->sin_addr.s_addr = lMaskAddr;
        strcpy ( pGateway, inet_ntoa ( pGw->sin_addr ) );
        break;
    }
    fclose ( pFp );
    if ( lGateAddr== 0 )
    {
        printf( "get_default_gw(): No default gateway found.\n" );
        return 0;
    }
#endif
    return 0;
}

char get_mask(  char* pIfName,  char* pNetmask)
{
    int nSockFd;
    struct ifreq sIfr;
    struct sockaddr_in *pSockAddr;
      char* pTmp="0.0.0.0";
      char cRet=0;
	
    if((pIfName == NULL) || (pNetmask== NULL)){
	   printf("get_mask_addr: invalid argument!\n");
	   cRet=-1;
    }
	
    nSockFd = socket ( AF_INET, SOCK_DGRAM, 0 );
    if ( nSockFd == -1 ){
        printf ( "get_mask_addr: creat socket error\n" );
    }
    strcpy ( sIfr.ifr_name, pIfName);
    sIfr.ifr_addr.sa_family=AF_INET;
    if ( ioctl ( nSockFd, SIOCGIFNETMASK, &sIfr ) < 0 )
    {
        printf( "get_mask_addr: ioctl: %s\n",strerror(errno) );
        cRet=-2;
    }
    else{
        pSockAddr= ( struct sockaddr_in * ) &sIfr.ifr_netmask;
        pTmp = inet_ntoa ( pSockAddr->sin_addr );
    	}
    strcpy(pNetmask, pTmp);
    close(nSockFd);
	
    return cRet;
}

int get_mac(const char *pIfName,unsigned char *pMacBuf)
{
	struct ifreq ifreq;
	int nSockFd;
	

	if ( pIfName == NULL )
    {
        printf ( "get_mac_addr: input the invalid argument!\n" );
        return -1;
    }
	if((nSockFd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
        printf ( "get_mac_addr: creat socket error\n" );
		return -1;
	}
	strcpy(ifreq.ifr_name,pIfName);
	if(ioctl(nSockFd,SIOCGIFHWADDR,&ifreq)<0)
	{
         printf( "get_mac_addr: ioctl: %s\n",strerror(errno));
		return -1;
	}

	*(pMacBuf + 0) = (unsigned char)ifreq.ifr_hwaddr.sa_data[0];
	*(pMacBuf + 1) = (unsigned char)ifreq.ifr_hwaddr.sa_data[1];
	*(pMacBuf + 2) = (unsigned char)ifreq.ifr_hwaddr.sa_data[2];
	*(pMacBuf + 3) = (unsigned char)ifreq.ifr_hwaddr.sa_data[3];
	*(pMacBuf + 4) = (unsigned char)ifreq.ifr_hwaddr.sa_data[4];
	*(pMacBuf + 5) = (unsigned char)ifreq.ifr_hwaddr.sa_data[5];

	close(nSockFd);
	return 0;
}

int SetGatway(int gatway)
{
#if 0
	struct ifreq temp;
	struct rtentry  rt;
    struct sockaddr_in *addr;
    int fd = 0;
   // int ret = -1;
    strcpy(temp.ifr_name, "eth0");
    if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        return -1;
    }
	
    addr = (struct sockaddr_in *)&(temp.ifr_addr);
    addr->sin_family = AF_INET;
    //addr->sin_addr.s_addr = inet_addr(ip);
    addr->sin_port = 0;
	addr->sin_addr.s_addr = gatway;
	printf("set gatway to: %s \n", inet_ntoa(addr->sin_addr));
	memset(&rt, 0, sizeof(rt));
    memcpy ( &rt.rt_gateway, addr, sizeof(struct sockaddr_in));
    ((struct sockaddr_in *)&rt.rt_dst)->sin_family=AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family=AF_INET;
    rt.rt_flags = RTF_GATEWAY;
    if (ioctl(fd, SIOCADDRT, &rt)<0)
    {
        printf( "ioctl(SIOCADDRT) error in set_default_route\n");
		perror("ioctl SIOCADDRT error ");
        close(fd);
        return -1;
    }
    close(fd);
#endif
    return 0;
}

char * getip(char *ip_buf)
{
    struct ifreq temp;
    struct sockaddr_in *myaddr;
    int fd = 0;
    int ret = -1;
    strcpy(temp.ifr_name, "eth0");
    if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        return NULL;
    }
    ret = ioctl(fd, SIOCGIFADDR, &temp);
    close(fd);
    if(ret < 0)
        return NULL;
    myaddr = (struct sockaddr_in *)&(temp.ifr_addr);
    strcpy(ip_buf, inet_ntoa(myaddr->sin_addr));
    return ip_buf;
}

/* Input: dstWeek 		:目标周， 一个月的第几周
 * Input: dstWeekDay	:目标天， 星期几
 * Input: p				:当前时间的指针
 * return: 返回要找的当月日期
 */
int find_day(int dstWeek, int dstWeekDay, struct tm *p)
{
	int	dstDay, iDisDay, days;	

	iDisDay = p->tm_wday - dstWeekDay;
	if( iDisDay == 0 ){
		days = p->tm_mday;
	}else if ( iDisDay > 0 ){
		days = p->tm_mday - iDisDay;
	}else{
		days = p->tm_mday + abs(iDisDay);
	}

	if( days >= 28 ){
		days -= 7;
	}

	if( days <= 0 ){
		days += 7;
	}
	
	iDisDay = days%7;
	if( iDisDay <= 0 ){
		iDisDay += 7;
	}

	dstDay = iDisDay + (dstWeek-1)*7 ;
	
	return dstDay;
}


/* gIsUseDayLightTime
 * 0:　非夏时制
 * 1:  夏时制
 */
static int gIsUseDayLightTime = 0;
int check_now_time_statu( TimeZoneInfo_t timeInfo)
{
	int day;
	time_t curTime; 
	struct tm stTm;
	struct tm *p;	
	p = &stTm;

	if( timeInfo.isDayLightTime == 1 ){
		time(&curTime);
		Time_GetTzTime(p, &curTime);

		if( p->tm_mon+1 < timeInfo.startDayLightTimeMon || 
			p->tm_mon+1 > timeInfo.endDayLightTimeMon ){
			gIsUseDayLightTime = 0;
		}else{
			if( p->tm_mon+1 >= timeInfo.startDayLightTimeMon ){
				if( p->tm_mon+1 == timeInfo.startDayLightTimeMon){
					day = find_day(timeInfo.startDayLightTimeWeek, 
						timeInfo.startDayLightTimeWeekDay, p);
					if( p->tm_mday >= day ){
						gIsUseDayLightTime = 1;
					}else{
						gIsUseDayLightTime = 0;
					}
				}else { /* 当前月已经超过了开始月 */
					gIsUseDayLightTime = 1;
				}
			}

			if( p->tm_mon+1 >= timeInfo.endDayLightTimeMon ){
				if( p->tm_mon+1 == timeInfo.endDayLightTimeMon){
					day = find_day(timeInfo.endDayLightTimeWeek, 
						timeInfo.endDayLightTimeWeekDay, p);
					if( p->tm_mday >= day ){
						gIsUseDayLightTime = 0;
					}else{
						gIsUseDayLightTime = 1;
					}
				}else { /* 当前月已经超过了结束月 */
					gIsUseDayLightTime = 0;
				}
			}
		}
	}

	return gIsUseDayLightTime;
}

int set_user_defined_tz(char *ascTzBuf)
{
	char cFlag = 0;
	char tzCmd[256];
	int 	timeOffset, sign, isDayLight;
	TimeZoneInfo_t stTzInfo;

	if( 0 != prase_user_defined_time_zone(&stTzInfo, ascTzBuf)){
		printf("prase_user_defined_time_zone error , set tz to GMT\n");
		system("/usr/share/zoneinfo/Etc/GMT /data/zone/tz");
		tzset();
	}else{
		memset(tzCmd, 0, 256);

		isDayLight = check_now_time_statu(stTzInfo);

		if(isDayLight == 1 ){
			timeOffset = stTzInfo.dayLightTimeZoneOffset;
			sign = stTzInfo.dayLightTimeZoneSign;
		}else{
			timeOffset = stTzInfo.timeZoneOffset;
			sign = stTzInfo.offsetSign;
		}
		
		if(sign == ADD){
			cFlag = '+';
		}else if(sign == SUB){
			cFlag = '-';
		}
		
		sprintf(tzCmd, "cp %sGMT%c%d %s", TZINFO_PATH, cFlag, 
			timeOffset, TZ_DST);
	}
	system(tzCmd);
	tzset();
	
	return 0; 
}

#define UPDATE_TZ_TIME		60
int update_system_timezone(void)
{
	/* 1分钟检查一次是否修改时区 */
	if( gTimeZoneUpdateTime < UPDATE_TZ_TIME ){
		gTimeZoneUpdateTime ++;
		return 0;
	}
	update_tzenv_buf();
	if(enTimeZoneUserDefined == get_zone_type(gascTzEnvBuf)){
		set_user_defined_tz(gascTzEnvBuf);
	}
	gTimeZoneUpdateTime = 0;
	return 0; 
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


