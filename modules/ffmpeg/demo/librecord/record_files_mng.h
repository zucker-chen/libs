

#define RECORD_TOP_FOLDERNAME	"./Record"
#define RECORD_VFILE_SUFFIX		".mp4"
#define RECORD_PFILE_SUFFIX		".jpg"
#define RECORD_INDEXFILE_NAME	"data.index"
#define RECORD_INDEXFILE_BAKUP	".bakupdata.index"

#define RECORD_FOLDERNAME_MAX_LEN	128		// 文件夹名最大字符长度
#define RECORD_FILENAME_MAX_LEN		128		// 文件名最大字符长度
#define RECORD_FILEPATH_MAX_LEN		256		// 文件全路径最大字符长度
#define RECORD_FILES_MAX_NUM		128		// 索引文件记录的最大文件(夹)个数，1天一个文件夹/1小时1个文件夹
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
	unsigned long ulStartTime;			// 对应事件录像起始时间(s)
	unsigned long ulEndTime;			// 对应事件录像结束时间(s)
} Record_EventInfo_T;


typedef struct _RecordFile_ConfData_T
{
	char cFileName[RECORD_FILENAME_MAX_LEN];
	int nFrameRate;					// Video
	int nFrameCount;				// Video	
	int nEventNum;											// 录像文件包含事件个数
	Record_EventInfo_T stTimeInfo[RECORD_EVENT_NUM];		// 对应事件录像起始结束时间
} RecordFile_ConfData_T;


typedef struct _RecordFolder_ConfData_T
{
	char cFileName[RECORD_FILENAME_MAX_LEN];
	unsigned long ulStartTime;
} RecordFolder_ConfData_T;


/**
 * 文件索引数据内部节点单元根据时间排序，方便后面检索
 */
typedef struct _RecordFile_IndexData_T
{
	int nNum;						// 录像文件个数
	unsigned int unSize;			// 数据区大小
	unsigned int unCrc32;			// data crc 
	unsigned char ucData[0];		// 文件索引数据存储位置
} RecordFile_IndexData_T;


/**
 * 根据当前系统时间创建录像文件，如文件夹不存在则创建文件夹
 * input: nType, 0=Video, 1:Picture
 * output: pOutFullPath, 创建完的文件全路径
 */
int RecordFile_Create(int nType, char *pOutFullPath);
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





