#ifndef __LENS_MS41908__
#define __LENS_MS41908__

int LENS_MS41908FZInspire(void);
int LENS_MS41908IrisInspire(void);
int LENS_MS41908Read(unsigned char ucAddr);
int LENS_MS41908Write(unsigned char ucAddr, unsigned int ucData);
int LENS_MS41908Init(void);
void LENS_MS41908Deinit(void);
#endif