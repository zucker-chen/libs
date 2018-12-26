
# FatFs
FatFs is a generic FAT/exFAT filesystem module for small embedded systems.

# FatFs SOURCE
* website: http://elm-chan.org/fsw/ff/00index_e.html.  
* src download:http://elm-chan.org/fsw/ff/arc/ff13c.zip.  

# FatFs Usage
* `diskio.c` 需要根据不同平台自行完成 本用例已修改，在linux下可直接使用  
* `test_libfatfs.c` 为测试用例的main文件  
* 使用时需要修改的地方有：  
** `diskio.c`中的`#define DEV_NAME    "/dev/sdd"//"/dev/mmcblk0p1""./sd.bin" //`，正常的sd卡设备应该是`/dev/mmcblk0p1`
** `diskio.c`中的`disk_ioctl GET_SECTOR_COUNT`设备扇区总数（容量）默认是128GB，如果不适用`f_fdisk`进行分区的话可以不配置  
** `make`
** `sudo ./test_libfatfs`
** 将`DEV_NAME`设备`mount`到一个文件夹中进行查看，看是否正常生成`newfile.txt`

# 备注
* `diskio.c` 容量配置暂时未手动配置，可以通过解析MBR分区数据获取设备容量  
* `FatFs` 该方法可以进行FAT32文件系统定制，满足不同需求（如：掉电保护及修复功能）  
* 如果修改`diskio.c`可以用`test_libfatfs.c`中的`test_diskio`进行验证接口正确性  



