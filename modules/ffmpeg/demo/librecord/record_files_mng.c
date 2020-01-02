#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <libgen.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>
#include "record_files_mng.h"
#include "crc32.h"


static int _RecordFile_Create(const char *name)
{
	return close(open(name, O_CREAT|O_RDWR|O_SYNC, 0775));
}

static int _RecordFile_MkDir(const char *path)
{
	mode_t mode = 0755;
	
    if (!path) {
        return -1;
    }
    char *temp = strdup(path);
    char *pos = temp;
    int ret = 0;

    if (strncmp(temp, "/", 1) == 0) {
        pos += 1;
    } else if (strncmp(temp, "./", 2) == 0) {
        pos += 2;
    }
    for ( ; *pos != '\0'; ++ pos) {
        if (*pos == '/') {
            *pos = '\0';
            if (-1 == (ret = mkdir(temp, mode))) {
                if (errno == EEXIST) {
                    ret = 0;
                } else {
                    fprintf(stderr, "failed to mkdir %s: %d:%s\n", temp, errno, strerror(errno));
                    break;
                }
            }
            *pos = '/';
        }
    }
    if (*(pos - 1) != '/') {
        if (-1 == (ret = mkdir(temp, mode))) {
            if (errno == EEXIST) {
                ret = 0;
            } else {
                fprintf(stderr, "failed to mkdir %s: %d:%s\n", temp, errno, strerror(errno));
            }
        }
    }
    free(temp);
    return ret;
}

static int  _RecordFile_RemoveDir(const char *path)
{
    DIR *pdir = NULL;
    struct dirent *ent = NULL;
    char full_path[PATH_MAX];
    int ret = 0;
    pdir = opendir(path);
    if (!pdir) {
        printf("can not open path: %s\n", path);
        if (errno == EMFILE) {
            return -EMFILE;
        } else {
            return -1;
        }
    }
    while (NULL != (ent = readdir(pdir))) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
            continue;
        }
        memset(full_path, 0, sizeof(full_path));
        sprintf(full_path, "%s/%s", path, ent->d_name);
        if (ent->d_type == DT_DIR) {
            ret = _RecordFile_RemoveDir(full_path);
            if (ret != 0) {
                printf("dfs_remove_dir %s ret=%d\n", full_path, ret);
            }
        }
        ret = remove(full_path);
    }
    closedir(pdir);
	rmdir(path);
	
    return ret;
}

/**
 * 根据当前系统时间创建录像文件，如文件夹不存在则创建文件夹
 * 文件命名根据当前时间命名
 * input: nType, 0=Video, 1:Picture
 * output: pOutFullPath, 创建完的文件全路径
 */
int RecordFile_Create(int nType, char *pOutFullPath)
{
	char cFolderName[RECORD_FOLDERNAME_MAX_LEN] = {0};
	char cFileName[RECORD_FILENAME_MAX_LEN] = {0};
	char cDayTimeStr[12] = {0};		// 年月日
	char cHourTimeStr[16] = {0};	// 年月日天
	char cSecTimeStr[24] = {0};		// 年月日天时分秒
	int nRet = 0;
	time_t t;
	struct tm *tm;

	if (pOutFullPath == NULL) {
		printf("%s:%d Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	t = time(NULL);
	tm = localtime(&t);
	strftime(cDayTimeStr, 12, "%Y-%m-%d", tm);
	strftime(cHourTimeStr, 16, "%Y-%m-%d-%H", tm);
	strftime(cSecTimeStr, 24, "%Y-%m-%d-%H-%M-%S", tm);

	if (0 == nType) {
		// 录像文件路径
		sprintf(cFolderName, RECORD_TOP_RECFOLDER"/%s/%s", cDayTimeStr, cHourTimeStr);
		sprintf(cFileName, "%s%s", cSecTimeStr, RECORD_VFILE_SUFFIX);
	} else {
		// 图片文件路径
		sprintf(cFolderName, RECORD_TOP_PICFOLDER"/%s/%s", cDayTimeStr, cHourTimeStr);
		sprintf(cFileName, "%s%s", cSecTimeStr, RECORD_PFILE_SUFFIX);
	}

	sprintf(pOutFullPath, "%s/%s", cFolderName, cFileName);
	//printf("%s:%d pOutFullPath = %s\n", __FUNCTION__, __LINE__, pOutFullPath);

	nRet = _RecordFile_MkDir(cFolderName);
	if (nRet < 0) {
		return nRet;
	}
	nRet = _RecordFile_Create(pOutFullPath);

	return nRet;
}


/**
 * 索引文件数据读取
 * input: pName, 索引文件全路径名
 * output: pRFID, 索引数据存储地址
 * result: 0 = success, <0 = fail
 */
int _RecordIndexFile_DataRead(char *pName, RecordFile_IndexData_T *pRFID)
{
	int fd = 0;

	fd = open(pName, O_RDWR|O_SYNC, 0775);
	if (fd < 0) {
		printf("%s:%d %s open error!\n", __FUNCTION__, __LINE__, pName);
		return -1;
	}
	if (sizeof(RecordFile_IndexData_T) != read(fd, (void *)pRFID, sizeof(RecordFile_IndexData_T))) {
		printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
		close(fd);
		return -1;
	}
	if (pRFID->unSize > 0) {
		if (pRFID->unSize != read(fd, (void *)pRFID->ucData, pRFID->unSize)) {
			printf("%s:%d read Error!\n", __FUNCTION__, __LINE__);
			close(fd);
			return -1;
		}
	}
	close(fd);

	return 0;
}

/**
 * 索引文件数据写
 * input: pName, 索引文件全路径名
 * output: pRFID, 索引数据存储地址
 * result: 0 = success, <0 = fail
 */
int _RecordIndexFile_DataWrite(char *pName, RecordFile_IndexData_T *pRFID)
{
	int fd = 0, nRet;

	fd = open(pName, O_CREAT|O_RDWR|O_SYNC, 0775);
	if (fd < 0) {
		printf("%s:%d %s open error!\n", __FUNCTION__, __LINE__, pName);
		return -1;
	}

	nRet = write(fd, pRFID, sizeof(RecordFile_IndexData_T) + pRFID->unSize);
	if (nRet < sizeof(RecordFile_IndexData_T) + pRFID->unSize) {
		printf("%s:%d write error\n", __FUNCTION__, __LINE__);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}


/**
 * 删除最旧录像，每次删除1小时数据(包括视频及图片)
 * input: 无
 * result: 0 = success, <0 = fail
 */
int RecordFile_OldestFileDel(void)
{
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	char cIndex_TopBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};	// 根目录索引buf
	char cIndex_DayBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};	// 天目录索引buf
	RecordFile_IndexData_T *pRFID = NULL;
	RecordFolder_ConfData_T *pRFCD = NULL;
	int nRet = 0;

	// Record
	pRFID = (RecordFile_IndexData_T *)cIndex_TopBuf;
	sprintf(cIndexFullPath, "%s/%s", RECORD_TOP_RECFOLDER, RECORD_INDEXFILE_NAME);
	nRet = _RecordIndexFile_DataRead(cIndexFullPath, pRFID);		// Root
	if (nRet >= 0 && pRFID->nNum > 0) {
		printf("%s:%d \n", __FUNCTION__, __LINE__);
		pRFCD = (RecordFolder_ConfData_T *)pRFID->ucData;
		sprintf(cIndexFullPath, "%s/%s", pRFCD->cFileName, RECORD_INDEXFILE_NAME);
		pRFID = (RecordFile_IndexData_T *)cIndex_DayBuf;
		nRet = _RecordIndexFile_DataRead(cIndexFullPath, pRFID);	// 天
		if (nRet >= 0 && pRFID->nNum > 1) {
			printf("%s:%d \n", __FUNCTION__, __LINE__);
			pRFCD = (RecordFolder_ConfData_T *)pRFID->ucData;
			nRet = _RecordFile_RemoveDir(pRFCD->cFileName);			// 删除最旧1小时文件夹
			// 更新天目录索引文件
			pRFID->nNum--;
			pRFID->unSize = pRFID->nNum * sizeof(RecordFolder_ConfData_T);
			if (pRFID->unSize > 0)
				memmove((void *)pRFID->ucData, (void *)(pRFID->ucData + sizeof(RecordFolder_ConfData_T)), pRFID->unSize);
			pRFID->unCrc32 = crc32((const char*)pRFID->ucData, pRFID->unSize);		// 文件CRC校验码
			_RecordIndexFile_DataWrite(cIndexFullPath, pRFID);		// 索引文件暂未做备份??
		} else if (nRet >= 0 && pRFID->nNum == 1) { 				// 该天只存在一个文件夹录像，删除该天文件夹	
			pRFID = (RecordFile_IndexData_T *)cIndex_TopBuf;
			nRet = _RecordFile_RemoveDir(pRFCD->cFileName);
			// 更新根目录索引文件
			pRFID->nNum--;
			pRFID->unSize = pRFID->nNum * sizeof(RecordFolder_ConfData_T);
			if (pRFID->unSize > 0)
				memmove((void *)pRFID->ucData, (void *)(pRFID->ucData + sizeof(RecordFolder_ConfData_T)), pRFID->unSize);
			pRFID->unCrc32 = crc32((const char*)pRFID->ucData, pRFID->unSize);		// 文件CRC校验码
			sprintf(cIndexFullPath, "%s/%s", RECORD_TOP_RECFOLDER, RECORD_INDEXFILE_NAME);
			_RecordIndexFile_DataWrite(cIndexFullPath, pRFID);		// 索引文件暂未做备份??
		}
	}
	if (nRet < 0) {
		printf("%s:%d Delete record folder error!\n", __FUNCTION__, __LINE__);
	}

	// Picture
	sprintf(cIndexFullPath, "%s/%s", RECORD_TOP_PICFOLDER, RECORD_INDEXFILE_NAME);
	nRet = _RecordIndexFile_DataRead(cIndexFullPath, pRFID);		// Root
	if (nRet >= 0 && pRFID->nNum > 0) {
		printf("%s:%d \n", __FUNCTION__, __LINE__);
		pRFCD = (RecordFolder_ConfData_T *)pRFID->ucData;
		sprintf(cIndexFullPath, "%s/%s", pRFCD->cFileName, RECORD_INDEXFILE_NAME);
		pRFID = (RecordFile_IndexData_T *)cIndex_DayBuf;
		nRet = _RecordIndexFile_DataRead(cIndexFullPath, pRFID);	// 天
		if (nRet >= 0 && pRFID->nNum > 1) {
			printf("%s:%d \n", __FUNCTION__, __LINE__);
			pRFCD = (RecordFolder_ConfData_T *)pRFID->ucData;
			nRet = _RecordFile_RemoveDir(pRFCD->cFileName);			// 删除最旧1小时文件夹
			// 更新天目录索引文件
			pRFID->nNum--;
			pRFID->unSize = pRFID->nNum * sizeof(RecordFolder_ConfData_T);
			if (pRFID->unSize > 0)
				memmove((void *)pRFID->ucData, (void *)(pRFID->ucData + sizeof(RecordFolder_ConfData_T)), pRFID->unSize);
			pRFID->unCrc32 = crc32((const char*)pRFID->ucData, pRFID->unSize);		// 文件CRC校验码
			_RecordIndexFile_DataWrite(cIndexFullPath, pRFID);
		} else if (nRet >= 0 && pRFID->nNum == 1) {
			pRFID = (RecordFile_IndexData_T *)cIndex_TopBuf;
			nRet = _RecordFile_RemoveDir(pRFCD->cFileName); 		// 该天只存在一个文件夹录像，删除该天文件夹
			// 更新根目录索引文件
			pRFID->nNum--;
			pRFID->unSize = pRFID->nNum * sizeof(RecordFolder_ConfData_T);
			if (pRFID->unSize > 0)
				memmove((void *)pRFID->ucData, (void *)(pRFID->ucData + sizeof(RecordFolder_ConfData_T)), pRFID->unSize);
			pRFID->unCrc32 = crc32((const char*)pRFID->ucData, pRFID->unSize);		// 文件CRC校验码
			sprintf(cIndexFullPath, "%s/%s", RECORD_TOP_PICFOLDER, RECORD_INDEXFILE_NAME);
			_RecordIndexFile_DataWrite(cIndexFullPath, pRFID);
		}
	}
	if (nRet < 0) {
		printf("%s:%d Delete picture folder error!\n", __FUNCTION__, __LINE__);
	}

	return 0;
}


/**
 * 索引文件完整性检查
 * input: pName, 索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_CRCCheck(char *pName)
{
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	int nRet = 0;

	nRet = _RecordIndexFile_DataRead(pName, pRFID);
	if (nRet < 0) {
		printf("%s:%d Read Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (pRFID->unCrc32 != (nRet = crc32((const char*)pRFID->ucData, pRFID->unSize))) {
		printf("%s:%d check %s crc32(%u -> %u) error!\n", __FUNCTION__, __LINE__, pName, pRFID->unCrc32, nRet);
		return -1;
	}

	return 0;
}


/**
 * 索引文件还原
 * input: pName, 备份索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_Restore(char *pName)
{
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	int nRet = 0;

	// 读取备份文件数据
	sprintf(cIndexFullPath, "%s/%s", dirname(strcpy(cIndexFullPath, pName)), RECORD_INDEXFILE_NAME);
	nRet = _RecordIndexFile_DataRead(cIndexFullPath, pRFID);
	if (nRet < 0) {
		printf("%s:%d Read Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	// 数据写入索引文件
	nRet = _RecordIndexFile_DataWrite(pName, pRFID);
	if (nRet < 0) {
		printf("%s:%d Write Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}




/**
 * 索引文件备份，如果当前文件校验失败则还原
 * input: pName, 索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_Bakup(char *pName)
{
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	int nRet = 0;

	// step1: 检查当前索引文件完整性
	nRet = RecordIndexFile_CRCCheck(pName);
	if (nRet < 0) {
		printf("%s:%d %s crc check error, restoring...!\n", __FUNCTION__, __LINE__, pName);
		// step1-1: 还原索引文件(从备份文件)
		RecordIndexFile_Restore(pName);
		return -1;
	}
	
	// step2-1: 读出当前索引文件数据
	nRet = _RecordIndexFile_DataRead(pName, pRFID);
	if (nRet < 0) {
		printf("%s:%d Read Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	// step2-2: 数据写入到备份索引文件中
	char cFileName[RECORD_FILENAME_MAX_LEN] = {0};	// dirname会修改输入字符串
	sprintf(cIndexFullPath, "%s/"RECORD_INDEXFILE_BAKUP, dirname(strcpy(cFileName, pName)));
	nRet = _RecordIndexFile_DataWrite(cIndexFullPath, pRFID);
	if (nRet < 0) {
		printf("%s:%d Write Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}


/**
 * 目前索引文件更新，将最新的录像文件目录信息更新到索引文件中，如索引文件不存在则创建
 * 每次新写入数据到索引文件之前会进行备份，如果当前索引文件被破坏(不完整)，则会从备份索引文件恢复
 * input: pRFCD, 录像文件夹（天）关键信息
 * result: 0 = success, <0 = fail
 */
static int _RecordIndexFolder_Update(RecordFolder_ConfData_T *pRFCD)
{
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	RecordFolder_ConfData_T *pInsertPos = NULL;
	int i, nRet;

	if (pRFCD == NULL){
		printf("%s:%d Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	char cFileName[RECORD_FILENAME_MAX_LEN] = {0};	// dirname会修改输入字符串，保存上一级文件路径名
	sprintf(cIndexFullPath, "%s/%s", dirname(strcpy(cFileName, pRFCD->cFileName)), RECORD_INDEXFILE_NAME);
	if (access(cIndexFullPath, F_OK|W_OK|R_OK) == 0) {	// exist
		RecordIndexFile_Bakup(cIndexFullPath);
		nRet = _RecordIndexFile_DataRead(cIndexFullPath, pRFID);
		if (nRet < 0) {
			printf("%s:%d Read Error!\n", __FUNCTION__, __LINE__);
			return -1;
		}
	} else {	// 索引文件不存在，创建
		memset(pRFID, 0, sizeof(RecordFile_IndexData_T));

		printf("%s:%d Update folder index file -> %s  %s\n", __FUNCTION__, __LINE__, cFileName, pRFCD->cFileName);
		if (strcmp(cFileName, RECORD_TOP_RECFOLDER) && strcmp(cFileName, RECORD_TOP_PICFOLDER)) {	// 录像顶级目录开始创建索引文件
			RecordFolder_ConfData_T stRFCD;
			strcpy(stRFCD.cFileName, cFileName);	// cFileName: 录像文件夹名
			stRFCD.ulStartTime = pRFCD->ulStartTime;
			nRet = _RecordIndexFolder_Update(&stRFCD);
			if (nRet < 0) {
				printf("%s:%d Update folder index file(%s) error!\n", __FUNCTION__, __LINE__, stRFCD.cFileName);
			}
		}
	}

	//printf("%s:%d pRFID->nNum = %d\n", __FUNCTION__, __LINE__, pRFID->nNum);
	pInsertPos = (RecordFolder_ConfData_T *)(pRFID->ucData + pRFID->nNum * sizeof(RecordFolder_ConfData_T));
	for (i = pRFID->nNum; i > 0; i--) {
		pInsertPos--;
		if (pRFCD->ulStartTime >= pInsertPos->ulStartTime) {	// 需要比较结束时间？？
			//printf("%s:%d Folder Name = %s\n", __FUNCTION__, __LINE__, pInsertPos->cFileName);
			pInsertPos++;	// 定位到插入位置
			break;
		} else {	// 新录像文件夹开始时间不在最后,删除此时间节点之后的所有文件夹
			printf("%s:%d Del Folder = %s\n", __FUNCTION__, __LINE__, pInsertPos->cFileName);
			_RecordFile_RemoveDir(pInsertPos->cFileName);
			// 更新索引头部信息
			pRFID->nNum = i - 1;
			pRFID->unSize = (i - 1) * sizeof(RecordFolder_ConfData_T);
		}
	}
	
	// 追加写入最新文件信息到索引文件
	pRFID->nNum++;
	if (pRFID->nNum > RECORD_FILES_MAX_NUM) {
		printf("%s:%d Num out of range(%d > %d) Error!\n", __FUNCTION__, __LINE__, pRFID->nNum, RECORD_FILES_MAX_NUM);
		return -1;
	}
	pRFID->unSize += sizeof(RecordFolder_ConfData_T);
	if ((sizeof(RecordFile_IndexData_T) + pRFID->unSize) > RECORD_INDEXFILE_MAX_SIZE) {
		printf("%s:%d Out of range(RECORD_INDEXFILE_MAX_SIZE) Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}
	printf("%s:%d Append Folder = %s\n", __FUNCTION__, __LINE__, pRFCD->cFileName);
	memcpy(pInsertPos, pRFCD, sizeof(RecordFolder_ConfData_T));
	pRFID->unCrc32 = crc32((const char*)pRFID->ucData, pRFID->unSize);		// 需要完善CRC
	nRet = _RecordIndexFile_DataWrite(cIndexFullPath, pRFID);
	if (nRet < 0) {
		printf("%s:%d Write Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}



/**
 * 索引文件更新，将最新的录像文件信息更新到索引文件中，如索引文件不存在则创建
 * 每次新写入数据到索引文件之前会进行备份，如果当前索引文件被破坏(不完整)，则会从备份索引文件恢复
 * input: pRFCD, 录像文件关键信息
 * result: 0 = success, <0 = fail
 * note: 当索引文件数据较大时会导致更新速度慢(1~3s，因为写文件是sync)，索引建议用线程后台更新索引文件数据
 *		 图片索引文件更新建议该方式(避免录像索引与图片索引文件同时更新导致更新失败问题)
 */
int RecordIndexFile_Update(RecordFile_ConfData_T *pRFCD)
{
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	RecordFile_IndexData_T *pRFID = (RecordFile_IndexData_T *)cIndexBuf;
	RecordFile_ConfData_T *pInsertPos = NULL;
	int i, nRet;

	if (pRFCD == NULL){
		printf("%s:%d Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}
		
	char cFileName[RECORD_FILENAME_MAX_LEN] = {0};	// dirname会修改输入字符串
	sprintf(cIndexFullPath, "%s/%s", dirname(strcpy(cFileName, pRFCD->cFileName)), RECORD_INDEXFILE_NAME);
	if (access(cIndexFullPath, F_OK|W_OK|R_OK) == 0) {	// exist，判断索引文件是否存在，存在则备份
		RecordIndexFile_Bakup(cIndexFullPath);
		nRet = _RecordIndexFile_DataRead(cIndexFullPath, pRFID);
		if (nRet < 0) {
			printf("%s:%d Read Error!\n", __FUNCTION__, __LINE__);
			return -1;
		}
	} else {	// 索引文件不存在，创建；此时(文件夹第一个录像文件创建时)上级目录索引文件需要同步更新
		memset(pRFID, 0, sizeof(RecordFile_IndexData_T));

		RecordFolder_ConfData_T stRFCD;
		strcpy(stRFCD.cFileName, cFileName);	// cFileName: 录像文件夹名
		stRFCD.ulStartTime = pRFCD->stTimeInfo[0].ulStartTime;
		nRet = _RecordIndexFolder_Update(&stRFCD);
		if (nRet < 0) {
			printf("%s:%d Update folder index file(%s) error!\n", __FUNCTION__, __LINE__, stRFCD.cFileName);
		}
	}

	//printf("%s:%d pRFID->nNum = %d\n", __FUNCTION__, __LINE__, pRFID->nNum);
	// 分析索引数据，根据录像文件时间比较，选择插入位置; 倒序查找效率更高
	pInsertPos = (RecordFile_ConfData_T *)(pRFID->ucData + pRFID->nNum * sizeof(RecordFile_ConfData_T));
	for (i = pRFID->nNum; i > 0; i--) {
		pInsertPos--;
		if (pRFCD->stTimeInfo[0].ulStartTime >= pInsertPos->stTimeInfo[0].ulEndTime) {
			//printf("%s:%d File Name = %s\n", __FUNCTION__, __LINE__, pInsertPos->cFileName);
			pInsertPos++;	// 定位到插入位置
			break;
		} else {	// 新录像文件开始时间不在最后,删除此时间节点之后的所有文件
			printf("%s:%d Del File = %s\n", __FUNCTION__, __LINE__, pInsertPos->cFileName);
			remove(pInsertPos->cFileName);
			// 更新索引头部信息
			pRFID->nNum = i - 1;
			pRFID->unSize = (i - 1) * sizeof(RecordFile_ConfData_T);
		}
	}
	
	// 追加写入最新文件信息到索引文件
	pRFID->nNum++;
	if (pRFID->nNum > RECORD_FILES_MAX_NUM) {
		printf("%s:%d Num out of range(%d > %d) Error!\n", __FUNCTION__, __LINE__, pRFID->nNum, RECORD_FILES_MAX_NUM);
		return -1;
	}
	pRFID->unSize += sizeof(RecordFile_ConfData_T);
	if ((sizeof(RecordFile_IndexData_T) + pRFID->unSize) > RECORD_INDEXFILE_MAX_SIZE) {
		printf("%s:%d Size out of range(%d > %d) Error!\n", __FUNCTION__, __LINE__, (int)(sizeof(RecordFile_IndexData_T) + pRFID->unSize), RECORD_INDEXFILE_MAX_SIZE);
		return -1;
	}
	printf("%s:%d Append File = %s\n", __FUNCTION__, __LINE__, pRFCD->cFileName);
	memcpy(pInsertPos, pRFCD, sizeof(RecordFile_ConfData_T));
	pRFID->unCrc32 = crc32((const char*)pRFID->ucData, pRFID->unSize);		// 文件CRC校验码
	nRet = _RecordIndexFile_DataWrite(cIndexFullPath, pRFID);
	if (nRet < 0) {
		printf("%s:%d Write Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}


static int nUpdatingFlag = 0;
static void * _RecordIndexFile_UpdateFn(void * pArg)
{
	RecordFile_ConfData_T stRFCD;
	
	memcpy((void *)&stRFCD, pArg, sizeof(RecordFile_ConfData_T));	// 需要及时拷贝走，不然源数据容易被覆盖
	RecordIndexFile_Update(&stRFCD);
	pthread_detach(pthread_self());
	nUpdatingFlag = 0;

	return NULL;
}

/**
 * 功能同RecordIndexFile_Update函数，添加线程后台执行操作
 * 考虑索引文件数据较大时会导致更新速度慢(1~3s，因为写文件是sync)，改用线程方式后台更新索引文件数据
 * input: pRFCD, 录像文件关键信息
 * result: 0 = success, <0 = fail
 * note: 当索引文件数据较大时会导致更新速度慢(1~3s，因为写文件是sync)，索引建议用线程后台更新索引文件数据
 */
int RecordIndexFile_UpdateThr(RecordFile_ConfData_T *pRFCD)
{
    pthread_t pid;

	if (nUpdatingFlag == 0) {
		nUpdatingFlag = 1;
		pthread_create(&pid, NULL, _RecordIndexFile_UpdateFn, (void *)pRFCD);
	} else {
		printf("%s:%d Last update file do not complete...\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	return 0;
}


/**
 * 按时间段搜索，支持录像及图片搜索
 * input: nType, 0=Video, 1:Picture; ulStartTime，开始时间; ulEndTime，结束时间;
 * output: pRFID, 检索输出的结果
 * result: 0 = success, <0 = fail
 */
int RecordFile_TimeSearch(int nType, unsigned long ulStartTime, unsigned long ulEndTime, RecordFile_IndexData_T *pOutRFID)
{
	char cFolderName[RECORD_FOLDERNAME_MAX_LEN] = {0};
	char cIndexFullPath[RECORD_FILEPATH_MAX_LEN] = {0};
	char cIndexBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};
	char cIndex_TopBuf[RECORD_INDEXFILE_MAX_SIZE/4] = {0};	// 根目录索引buf
	char cIndex_DayBuf[RECORD_INDEXFILE_MAX_SIZE/4] = {0};	// 天目录索引buf
	char cIndex_HourBuf[RECORD_INDEXFILE_MAX_SIZE] = {0};	// 小时目录索引buf
	RecordFile_IndexData_T *pTopRFID = (RecordFile_IndexData_T *)cIndex_TopBuf;
	RecordFile_IndexData_T *pDayRFID = (RecordFile_IndexData_T *)cIndex_DayBuf;
	RecordFile_IndexData_T *pHourRFID = (RecordFile_IndexData_T *)cIndex_HourBuf;
	RecordFolder_ConfData_T *pDayRFCD = NULL, *pHourRFCD = NULL;
	RecordFile_ConfData_T *pRFCD = NULL;
	RecordFile_SearchInfo_T *pRFSI = NULL;
	int nRet = 0, i = 0, j = 0, k = 0;
	char cSecTimeStr[24] = {0};		// 年月日天时分秒
	struct tm *tm;


	if (pOutRFID == NULL) {
		printf("%s:%d Error!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	tm = localtime((time_t *)&ulStartTime);
	strftime(cSecTimeStr, 24, "%Y-%m-%d-%H-%M-%S", tm);
	printf("%s:%d Search start time: %s\n", __FUNCTION__, __LINE__, cSecTimeStr);
	tm = localtime((time_t *)&ulEndTime);
	strftime(cSecTimeStr, 24, "%Y-%m-%d-%H-%M-%S", tm);
	printf("%s:%d Search end time: %s\n", __FUNCTION__, __LINE__, cSecTimeStr);
	
	// 跟目录索引文件名
	sprintf(cIndexFullPath, "%s/%s", (0 == nType) ? RECORD_TOP_RECFOLDER : RECORD_TOP_PICFOLDER, RECORD_INDEXFILE_NAME);
	if (access(cIndexFullPath, F_OK|R_OK) != 0) {	// not exist，判断索引文件是否可读
		return -1;
	}

	nRet = _RecordIndexFile_DataRead(cIndexFullPath, pTopRFID);		// Root
	for (i = 0; i < pTopRFID->nNum; i++)
	{
		pDayRFCD = (RecordFolder_ConfData_T *)pTopRFID->ucData + i;	// 天
		if (pDayRFCD->ulStartTime >= (ulStartTime/86400 + 1) * 86400  && pDayRFCD->ulStartTime <= ulEndTime) {	// 该天在搜索时间范围之内
			//printf("%s:%d %s\n", __FUNCTION__, __LINE__, pDayRFCD->cFileName);
			sprintf(cIndexFullPath, "%s/%s", pDayRFCD->cFileName, RECORD_INDEXFILE_NAME);
			if (access(cIndexFullPath, F_OK|R_OK) != 0) {	// not exist，判断索引文件是否可读
				continue;
			}
			nRet = _RecordIndexFile_DataRead(cIndexFullPath, pDayRFID);		// 天索引文件读取
			for (j = 0; j < pDayRFID->nNum; j++)
			{
				pHourRFCD = (RecordFolder_ConfData_T *)pDayRFID->ucData + j;	// 小时
				if (pHourRFCD->ulStartTime >= (ulStartTime/3600 + 1) * 3600  && pHourRFCD->ulStartTime <= ulEndTime) {	// 该小时在搜索时间范围之内
					//printf("%s:%d %s\n", __FUNCTION__, __LINE__, pHourRFCD->cFileName);
					sprintf(cIndexFullPath, "%s/%s", pHourRFCD->cFileName, RECORD_INDEXFILE_NAME);
					if (access(cIndexFullPath, F_OK|R_OK) != 0) {	// not exist，判断索引文件是否可读
						continue;
					}
					nRet = _RecordIndexFile_DataRead(cIndexFullPath, pHourRFID);		// 小时索引文件读取
					for (k = 0; k < pHourRFID->nNum; k++)
					{
						pRFCD = (RecordFile_ConfData_T *)pHourRFID->ucData + k;	// 小时
						//printf("%s:%d ulStartTime = %d\n", __FUNCTION__, __LINE__, pHourRFCD->ulStartTime);
						if (pRFCD->stTimeInfo[0].ulEndTime >= ulStartTime && pRFCD->stTimeInfo[0].ulStartTime <= ulEndTime) {	// 该文件在搜索时间范围之内
							printf("%s:%d %s\n", __FUNCTION__, __LINE__, pRFCD->cFileName);
							sprintf(cIndexFullPath, "%s/%s", pHourRFCD->cFileName, RECORD_INDEXFILE_NAME);
							if (access(pHourRFCD->cFileName, F_OK|R_OK) != 0) {	// not exist，判断文件是否可读
								continue;
							}

							pRFSI = pOutRFID->ucData + sizeof(RecordFile_SearchInfo_T) * pOutRFID->nNum;
							pRFSI->ucType = pRFCD->ucType;
							pRFSI->ucEventNum = pRFCD->ucEventNum;
							memcpy((void *)pRFSI->stTimeInfo, (void *)pRFCD->stTimeInfo, sizeof(Record_EventInfo_T));
							pOutRFID->nNum++;
						}
					}
				}
			}
			
		}
	}

	return 0;
}





