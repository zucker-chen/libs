

#define RECORD_TOP_RECFOLDER	"./Record"		// 	录像根目录
#define RECORD_TOP_PICFOLDER	"./Picture"		//	图片根目录
#define RECORD_VFILE_SUFFIX		".mp4"
#define RECORD_PFILE_SUFFIX		".jpg"
#define RECORD_INDEXFILE_NAME	"data.index"
#define RECORD_INDEXFILE_BAKUP	".bakupdata.index"

#define RECORD_FOLDERNAME_MAX_LEN	128		// 文件夹名最大字符长度
#define RECORD_FILENAME_MAX_LEN		128		// 文件名最大字符长度
#define RECORD_FILEPATH_MAX_LEN		256		// 文件全路径最大字符长度
#define RECORD_FILES_MAX_NUM		360		// 索引文件记录的最大文件(夹)个数，1天一个文件夹/1小时1个文件夹(10s一个文件总共360个)
#define RECORD_INDEXFILE_MAX_SIZE	262144	// 索引文件最大文件大小

typedef enum _Record_EventType_E
{
	RECORD_EVENT_NONE = 0,
	RECORD_EVENT_GENERAL,		// 常规录像
	RECORD_EVENT_MOTION,		// 移动侦测录像
	RECORD_EVENT_IO,			// I/O报警录像
	RECORD_EVENT_NET,			// NET断网录像
	RECORD_EVENT_NUM,			// 报警录像事件个数
} Record_EventType_E;


typedef struct _Record_EventInfo_T
{
	Record_EventType_E eEventType;		// 事件类型
	unsigned long ulStartTime;			// 对应事件起始时间(s)
	unsigned long ulEndTime;			// 对应事件结束时间(s)，当为图片时ulEndTime=ulStartTime
} Record_EventInfo_T;


/**
 * 录像文件关键信息结构体，每个录像文件对应一个该结构体数据，最终保存到索引文件中（录像与图片共用）
 */
typedef struct _RecordFile_ConfData_T
{
	char cFileName[RECORD_FILENAME_MAX_LEN];				// 录像文件名
	unsigned int unFileSize;								// 录像文件大小，用于判断录像文件是否完整（由于掉电等操作导致文件数据不完整）
	unsigned char ucType;									// 0:H264; 1:H265
	unsigned char ucFrameRate;								// Only Video，视频帧率，回放时用到
	unsigned char ucRev;
	unsigned char ucEventNum;								// 录像文件包含事件个数	--> Only Video
	Record_EventInfo_T stTimeInfo[RECORD_EVENT_NUM];		// 对应事件录像起始结束时间
} RecordFile_ConfData_T;									// 256 bytes


/**
 * 录像文件夹内（天）对应文件夹关键信息，每个录像文件夹（天）对应一个该结构体数据，最终保存到索引文件中
 */
typedef struct _RecordFolder_ConfData_T
{
	char cFileName[RECORD_FILENAME_MAX_LEN];	// 录像文件夹名
	unsigned long ulStartTime;					// 起始时间(s)
} RecordFolder_ConfData_T;


/**
 * 录像文件检索时间段单元信息
 */
typedef struct _RecordFile_SearchInfo_T
{
	//char cFileName[RECORD_FILENAME_MAX_LEN];	// 减少索引数据量，不保存文件名，可以通过时间推导出文件名
	unsigned char ucType;						// 0:H264; 1:H265, 下载时某一时间区间出现多种编码类型的文件时需要区分
	unsigned char ucRev[2];
	unsigned char ucEventNum;								// 录像文件包含事件个数	--> Only Video
	Record_EventInfo_T stTimeInfo[RECORD_EVENT_NUM];		// 对应事件录像起始结束时间
} RecordFile_SearchInfo_T;						// 20 bytes


/**
 * 文件索引数据内部节点单元根据时间排序，方便后面检索，如果出现中间时间点插入数据，这该时间点往后的数据删除（局限于该索引文件）
 */
typedef struct _RecordFile_IndexData_T
{
	int nNum;						// 录像文件个数
	unsigned int unSize;			// 数据区大小
	unsigned int unCrc32;			// data crc 
	unsigned char ucData[0];		// 文件索引数据存储位置
} RecordFile_IndexData_T;			// 12 bytes


/**
 * 根据当前系统时间创建录像文件，如文件夹不存在则创建文件夹
 * input: nType, 0=Video, 1:Picture
 * output: pOutFullPath, 创建完的文件全路径
 */
int RecordFile_Create(int nType, char *pOutFullPath);
/**
 * 删除最旧录像，每次删除1小时数据(包括视频及图片)
 * input: 无
 * result: 0 = success, <0 = fail
 */
int RecordFile_OldestFileDel(void);
/**
 * 索引文件完整性检查
 * input: pName, 索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_CRCCheck(char *pName);
/**
 * 索引文件还原
 * input: pName, 备份索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_Restore(char *pName);
/**
 * 索引文件备份，如果当前文件校验失败则还原
 * input: pName, 索引文件全路径名
 * result: 0 = success, <0 = fail
 */
int RecordIndexFile_Bakup(char *pName);


/**
 * 索引文件更新，将最新的录像文件信息更新到索引文件中，如索引文件不存在则创建
 * 每次新写入数据到索引文件之前会进行备份，如果当前索引文件被破坏(不完整)，则会从备份索引文件恢复
 * input: pRFCD, 录像文件关键信息
 * result: 0 = success, <0 = fail
 * note: 当索引文件数据较大时会导致更新速度慢(1~3s，因为写文件是sync)，索引建议用线程后台更新索引文件数据
 */
int RecordIndexFile_Update(RecordFile_ConfData_T *pRFCD);

/**
 * 功能同RecordIndexFile_Update函数，添加线程后台执行操作
 * 考虑索引文件数据较大时会导致更新速度慢(1~3s，因为写文件是sync)，改用线程方式后台更新索引文件数据
 * input: pRFCD, 录像文件关键信息
 * result: 0 = success, <0 = fail
 * note: 当索引文件数据较大时会导致更新速度慢(1~3s，因为写文件是sync)，索引建议用线程后台更新索引文件数据
 *		 图片索引文件更新建议该方式(避免录像索引与图片索引文件同时更新导致更新失败问题)
 */
int RecordIndexFile_UpdateThr(RecordFile_ConfData_T *pRFCD);

/**
 * 按时间段搜索，支持录像及图片搜索
 * input: nType, 0=Video, 1:Picture; ulStartTime，开始时间; ulEndTime，结束时间;
 * output: pRFID, 检索输出的结果
 * result: 0 = success, <0 = fail
 */
int RecordFile_TimeSearch(int nType, unsigned long ulStartTime, unsigned long ulEndTime, RecordFile_IndexData_T *pOutRFID);


