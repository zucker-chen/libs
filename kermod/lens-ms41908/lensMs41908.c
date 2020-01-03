#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <asm/io.h>
#include "lensMs41908.h"
#include "lensGpio.h"

#define SPI1_BASE 0x200E0000
#define SPISR 0x0c
#define SPICR1 0x04
#define SPIDR  0x08
#define	SPISR_TFE_EMPTY 0x01
#define SPISR_BSY	(1<<4)
#define SPISR_RNE_EMPTY (1<<2)
#define SPI_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY)// | SPI_TX_DUAL \
				//| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)

struct spi_master *hi_master = NULL;
struct spi_device *hi_spi = NULL;
struct device		*dev = NULL;
static void *pSpiBase;

static int LENS_MS41908Reset(void)
{
	LENS_GpioSet(14,1,1);
	udelay(1000);
	LENS_GpioSet(14,1,0);
	udelay(1000);
	LENS_GpioSet(14,1,1);
}

int LENS_MS41908FZInspire(void)
{	
	LENS_GpioSet(13,3,1);
	udelay(80);
	LENS_GpioSet(13,3,0);
}

int LENS_MS41908IrisInspire(void)
{
	LENS_GpioSet(10,5,0);
	udelay(80);
	LENS_GpioSet(10,5,1);
	udelay(80);
	LENS_GpioSet(10,5,0);
	//printk(KERN_INFO "fun = %s line = %d\n",__FUNCTION__,__LINE__);
}

// value:  0:low, 1:high, else:low->high
int LENS_MS41908CS(int value)
{
	if (value == 0) {
		LENS_GpioSet(8,7,0);
	} else if (value == 1) {
		LENS_GpioSet(8,7,1);
	} else {
		LENS_GpioSet(8,7,0);
		udelay(100);
		LENS_GpioSet(8,7,1);
	}

	return 0;
}

#if 0
static int LENS_SpiWaitIdle(void)
{
	unsigned int i = 0;
	unsigned int unStatus = 0;
	unsigned int nRet = 0;
	
	while(1)
	{
		if(pSpiBase != NULL)
		{
			unStatus = readl(pSpiBase+SPISR);
			//printk(KERN_INFO "fun = %s unStatus = %d\n",__FUNCTION__,unStatus);
			if((unStatus & SPISR_TFE_EMPTY) && (!(unStatus & SPISR_BSY))&&(unStatus & SPISR_RNE_EMPTY))
			{
				break;
			}
		}
		
		if(i++ >1000)
		{
			printk(KERN_ERR "%s ERROR,unStatus:%d!\n",__FUNCTION__,unStatus);
			nRet =  -EFAULT;
			break;
		}
		
	}
	
	return nRet;
}


static int LENS_SpiFlushRxFifo(unsigned char ucFlag)
{
	unsigned int i = 0;
	unsigned int unStatus = 0;
	unsigned int nRet = 0,nRetLast=0;
	
	if (ucFlag)
		nRet = -EFAULT;
	
	while(1)
	{
		unStatus = readl(pSpiBase+SPISR);
/*		if ((unStatusPre!=unStatus) && (ucFlag != 1))
		{
			unStatusPre = unStatus;
			printk(KERN_INFO "fun = %s unStatus = %d\n",__FUNCTION__,unStatus);
		}*/

		if(!(unStatus & SPISR_RNE_EMPTY))
		{
			break;
		}
		
		nRetLast = nRet;
		nRet = readl(pSpiBase+SPIDR);
//		if (2 == ucFlag)
//			printk(KERN_INFO "fun = %s nRet = %x,i:%d\n",__FUNCTION__,nRet,i);
		if(i++ >256)
		{
			printk(KERN_ERR "%s ERROR:%d,ucFlag:%d!\n",__FUNCTION__,unStatus,ucFlag);
			nRet =  -EFAULT;
			break;
		}
	}
	if (2 == ucFlag)
	{
		if (i >= 2)
		{
			nRet = (nRet<<8)|nRetLast;
//			printk(KERN_INFO "fun = %s unStatus = %d\n",__FUNCTION__,unStatus);
//			printk(KERN_INFO "fun = %s getValue = %d,i:%d\n",__FUNCTION__,nRet,i);
		}
		else
		{
			printk(KERN_INFO "%s error,nRet:%d,i:%d\n",__FUNCTION__,nRet,i);
			nRet =  -EFAULT;
		}
	}
	
	return nRet;
}



int LENS_MS41908Write(unsigned char ucAddr, unsigned int ucData)
{
	unsigned char  ucDataLow =  (unsigned char)((ucData>>0)&0xff);
	unsigned char ucDataHigh = (unsigned char)((ucData>>8)&0xff);
	unsigned int unValue = 0;
	
	if(pSpiBase == NULL)
	{
		printk(KERN_INFO "fun = %s line = %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	LENS_MS41908CS(1);

	unValue = readl(pSpiBase+SPICR1);
	unValue |= 0x02;
	writel(unValue,pSpiBase+SPICR1);

	//printk(KERN_INFO "%s,SPISR:%d\n",__FUNCTION__,readl(pSpiBase+SPISR));
	writel(ucAddr,pSpiBase+SPIDR);
	writel(ucDataLow,pSpiBase+SPIDR);
	writel(ucDataHigh,pSpiBase+SPIDR);
	LENS_SpiWaitIdle();
	LENS_SpiFlushRxFifo(0);
	
	unValue &= ~0x02;
	writel(unValue,pSpiBase+SPICR1);
	//printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,ucData);

	//LENS_MS41908Read(ucAddr);
	LENS_MS41908CS(0);

	return 0;
}

int LENS_MS41908Read(unsigned char ucAddr)
{
	unsigned int unValue = 0;
	int nRet = 0;
	
	LENS_MS41908CS(1);
	
	unValue = readl(pSpiBase+SPICR1);
	unValue |= 0x02;
	writel(unValue,pSpiBase+SPICR1);
	//清空FIFO
	LENS_SpiFlushRxFifo(0);
	
	writel(0x40 | ucAddr,pSpiBase+SPIDR);
	writel(0,pSpiBase+SPIDR);
	writel(0,pSpiBase+SPIDR);
	
	if (LENS_SpiWaitIdle() < 0)
	{
		LENS_SpiFlushRxFifo(2);
		unValue &= ~0x02;
		writel(unValue,pSpiBase+SPICR1);
		printk(KERN_INFO "fun = %s,ucAddr:%d,error\n",__FUNCTION__,ucAddr);
		return -1;
	}
	//读取FIFO数据	
	nRet = LENS_SpiFlushRxFifo(2);
	if (nRet < 0)
		printk(KERN_INFO "fun = %s,ucAddr:%d,error\n",__FUNCTION__,ucAddr);
	
	unValue &= ~0x02;
	writel(unValue,pSpiBase+SPICR1);
	//printk("addr=%x,value:%x\n",ucAddr,nRet);
	
	LENS_MS41908CS(0);
	
	return nRet;
}


#else
int LENS_MS41908Read(unsigned char ucAddr)
{
	struct spi_master	*master = hi_master;
	struct spi_device	*spi = hi_spi;
	static struct spi_transfer t;
	static struct spi_message	m;
	int nRet = 0;
	unsigned long		        flags;
	static unsigned char  ucTxBuf[8] = {0};
    static unsigned char  ucRxBuf[8] = {0};
	
#if 0
	/* check spi_message is or no finish */
	spin_lock_irqsave(&master->queue_lock, flags);
	if (m.state != NULL)
	{
		spin_unlock_irqrestore(&master->queue_lock, flags);
		dev_err(&spi->dev, "\n********** %s, %d line: spi_message no finish!*********\n", __func__, __LINE__);
		return -EFAULT;
	}
	spin_unlock_irqrestore(&master->queue_lock, flags);
#endif	
	//LENS_MS41908CS();
	spi->mode = SPI_MODE_3 | SPI_LSB_FIRST;
	
	ucTxBuf[0] = 0x40 | ucAddr;
	ucTxBuf[1] = 0;
	ucTxBuf[2] = 0;

	t.tx_buf    = ucTxBuf;
	t.rx_buf    = ucTxBuf;
	t.len       = 3;
	t.cs_change = 1;
	t.speed_hz  = 2000000;
	t.bits_per_word = 8;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	m.state = &m;
	nRet  = spi_sync(spi, &m);
	if (nRet)
	{
		dev_err(&spi->dev, "%s %d: LENS_sync() error!\n", __func__,nRet);
		nRet = -EFAULT;
		return nRet;
	}
	//udelay(500);
	
	nRet = ucTxBuf[1]| ucTxBuf[2]<< 8;
//	printk("addr = %x value  %x \n\n\n",ucAddr,nRet);
	
	return nRet;
}

/*****************************************************************
This function will be called in interrupt route.
So use spi_async, can't call spi_sync here.
*****************************************************************/
int LENS_MS41908Write(unsigned char ucAddr, unsigned int ucData)
{
	if((hi_master == NULL) || (hi_spi == NULL))
	{
		return -1;
	}
	struct spi_master	*master = hi_master;
	struct spi_device	*spi = hi_spi;
	static struct spi_transfer t;
	static struct spi_message	m;
	int	status = 0;
	unsigned long		        flags;
	unsigned char  ucDataLow =  (unsigned char)((ucData>>0)&0xff);
	unsigned char ucDataHigh = (unsigned char)((ucData>>8)&0xff);
	static unsigned char  ucTxBuf[8];
    static unsigned char  ucRxBuf[8];

#if 0
    //LENS_MS41908CS();
	spin_lock_irqsave(&master->queue_lock, flags);
	if (m.state != NULL)
	{
		spin_unlock_irqrestore(&master->queue_lock, flags);
		dev_err(&spi->dev, "\n********** %s, %d line: de  sssss spi_message no finish!ucAddr = %x*********\n", __func__, __LINE__,ucAddr);
		return -EFAULT;
	}
	spin_unlock_irqrestore(&master->queue_lock, flags);
#endif

	spi->mode = SPI_MODE_3 | SPI_LSB_FIRST;

	ucTxBuf[0] = ucAddr;
    ucTxBuf[1] = ucDataLow;
    ucTxBuf[2] = ucDataHigh;
	t.tx_buf	 = ucTxBuf;
	t.rx_buf	 = ucRxBuf;
	t.len		 = 3;
	t.cs_change = 1;
	t.speed_hz  = 2000000;
	t.bits_per_word = 8;
	//t.delay_usecs = 0;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	m.state = &m;
	status  = spi_sync(spi, &m);

	if (status)
	{
		dev_err(&spi->dev, "%s %d: spi_async() error!\n", __func__,status);
		status = -EFAULT;
	}
	
//	printk(KERN_INFO "fun = %s line = %d, addr = 0x%x, data = 0x%x\n",__FUNCTION__,__LINE__, ucAddr, ucData);
    //LENS_MS41908Read(ucAddr);
	
	return status;
}
#endif

static int LENS_MS41908SpiInit(void)
{
	struct spi_master 	*master;
	
	unsigned int unValue = 0;
	char 			    spi_name[128] = {0};
	int bus_num = 1;	
	int csn = 0;
	int	status = 0;
	int	retval = 0;
	master = spi_busnum_to_master(bus_num);
	
	if(master)
	{
		hi_master = master;
		snprintf(spi_name, sizeof(spi_name), "%s.%u", dev_name(&master->dev), csn);
		dev = bus_find_device_by_name(&spi_bus_type, NULL, spi_name);
		if (dev == NULL)
		{
			dev_err(NULL, "chipselect %d has not been used\n", csn);
			status = -ENXIO;
			goto end1;
		}
		
		hi_spi = to_spi_device(dev);
		if(hi_spi == NULL)
		{
			dev_err(dev, "to_spi_device() error!\n");
			status = -ENXIO;
			goto end1;
		}
		#if 0
		unValue = SPI_MODE_3 | SPI_LSB_FIRST;
		retval = __put_user(hi_spi->mode,(__u8 __user *)unValue);	// & SPI_MASK
		
		unValue = 2000000;
		retval = __put_user(hi_spi->max_speed_hz, (__u32 __user *)unValue);
		#endif

		printk(KERN_INFO "fun = %s line = %d retval = %d\n",__FUNCTION__,__LINE__,retval);
	}
	else
	{
		dev_err(NULL, "spi_busnum_to_master() error!\n");
		status = -ENXIO;
		goto end0;
	}
	
end1:
	put_device(dev);
end0:
	return status;	
}

int LENS_MS41908Init(void)
{
	int	nStatus = 0;
#if 0
	pSpiBase = ioremap(SPI1_BASE,0x10);
	writel(0xc7,pSpiBase);	// SPI_MODE_3
	unsigned int unValue = 0;
	unValue = readl(pSpiBase+SPICR1);
	unValue |= 0x1<<4;
	writel(unValue,pSpiBase+SPICR1);	// SPI_LSB_FIRST 大端
#else
	nStatus = LENS_MS41908SpiInit();
	if(nStatus < 0)
	{
		printk("LENS_MS41908SpiInit failed!!\n");
		
		return nStatus;
	}
#endif
	LENS_MS41908Reset();

	LENS_MS41908Read(0x20);
	LENS_MS41908Write(0x20,0x1e07);	 //设定频率�?   DT1延时设为 3ms
	LENS_MS41908Write(0x22,0x0004);	 //DT2延时设为 0.6ms
	LENS_MS41908Write(0x23,0xd8d8);	 //设置AB占空比为 90%
	LENS_MS41908Write(0x24,0x0400);	 //AB 256细分	设定电流方向
	LENS_MS41908Write(0x25,0x0000);  //
	LENS_MS41908Write(0x21,0x0000);  //
	
	LENS_MS41908Write(0x27,0x0004);	 //DT2延时设为 0.6ms
	LENS_MS41908Write(0x28,0xd8d8);	 //设置CD占空比为 90%
	LENS_MS41908Write(0x29,0x0400);	 //AB 256细分	设定电流方向
	LENS_MS41908Write(0x2a,0x0000);  //

	//光圈相关寄存器初始化
	LENS_MS41908Write(0x00,0x0000);
	LENS_MS41908Write(0x01,0x3E00);
	LENS_MS41908Write(0x02,0x1000);
	LENS_MS41908Write(0x03,0x0E10);
	LENS_MS41908Write(0x04,0xD640);
	LENS_MS41908Write(0x05,0x0004);
	LENS_MS41908Write(0x0A,0x0000);
	LENS_MS41908Write(0x0B,0x0480);
	LENS_MS41908Write(0x0E,0x0300);

	//光圈激励信�?
	LENS_MS41908IrisInspire();

#if 0
	unsigned char ucAddr;
	ucAddr = 0x00;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x01;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x02;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x03;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x04;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x05;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x0A;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x0B;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x0E;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));

	ucAddr = 0x20;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x21;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x22;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x23;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x24;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x25;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x27;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x28;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x29;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));
	ucAddr = 0x2A;
	printk(KERN_INFO "fun = %s,ucAddr:%x,ucData:%x\n",__FUNCTION__,ucAddr,LENS_MS41908Read(ucAddr));

#endif
	return nStatus;
}

void LENS_MS41908Deinit(void)
{
	put_device(dev);
}
