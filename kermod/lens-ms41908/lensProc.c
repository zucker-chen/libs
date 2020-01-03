#include <linux/init.h>
#include "hi_osal.h"
#include "lensProc.h"
#include "lensDrv.h"

#define  LENS_INFO "lens_info"

#define  LENS_VERSION_INFO "IPC debug 0.0.0.1"

static int LENS_ProcShow(osal_proc_entry_t *s) 
{
	int i = 0;
	PROCLENSINFO_OBJ stLensInfo;
	memset(&stLensInfo,-1,sizeof(stLensInfo));
	
	LENS_GetInfo(&stLensInfo);
	
	osal_seq_printf(s, "[LENS] Version:["LENS_VERSION_INFO"], Build Time["__DATE__", "__TIME__"]\n");
    osal_seq_printf(s, "--------------------RegisterInfo(MS41908)--------------------\n");
	osal_seq_printf(s, "          RegisterAddr(hex)   RegisterValue(hex)      \n");
	
	for(i = 0;i < REGISTER_NUM;i++)
	{
		osal_seq_printf(s, "%16x",stLensInfo.ucRegisterInfo[i][0]);
		osal_seq_printf(s, "%20x",stLensInfo.ucRegisterInfo[i][1]);
		osal_seq_printf(s, "\n");
	}
	
	return 0;
}

int Lens_ProcInit(void)
{
	osal_proc_entry_t *lens_entry;
	lens_entry = osal_create_proc_entry(LENS_INFO, NULL);
    if (NULL == lens_entry)
    {
        osal_printk("gyro: can't create %s.\n", LENS_INFO);
        return -1;
    }
	
    lens_entry->read = LENS_ProcShow;	
	
    return 0;
}

void Lens_ProcDeinit(void)
{
	osal_remove_proc_entry(LENS_INFO, NULL);
}