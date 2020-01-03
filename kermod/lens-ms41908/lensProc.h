#ifndef __LENS_PROC__
#define __LENS_PROC__

#define REGISTER_NUM 20

typedef struct ProcZoomInfo
{
	unsigned int unCurZoomStep;
	unsigned int TargetZoomStep;
	unsigned char ucRunState;
}PROCZOOMINFO_OBJ;

typedef struct ProcFocusInfo
{
	unsigned int unCurFocusStep;
	unsigned int TargetFocusStep;
	unsigned char ucRunState;
}PROCFOCUSINFO_OBJ;

typedef struct ProcLensInfo
{
	PROCZOOMINFO_OBJ stZoomInfo;
	PROCFOCUSINFO_OBJ stFocusInfo;
	unsigned int  ucRegisterInfo[REGISTER_NUM][2];
}PROCLENSINFO_OBJ;

int Lens_ProcInit(void);
void Lens_ProcDeinit(void);

#endif