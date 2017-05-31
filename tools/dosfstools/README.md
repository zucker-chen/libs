
# dosfstools
FAT文件系统工具（格式化、修复）

## build and install
#### x86 linux
sh build-dosfstools.sh
#### arm linux
sh build-dosfstools.sh -c arm-hisiv500-linux-

## test
```
# ./fsck.fat -a /dev/mmcblk0p1 
CP437: Invalid argument
fsck.fat 4.0 (2016-05-06)
0x41: Dirty bit is set. Fs was not properly unmounted and some data may be corrupt.
 Automatically removing dirty bit.
Performing changes.
/dev/mmcblk0p1: 1860 files, 1613307/1953802 clusters
/mnt/mount # 
```

## Note
* fsck.fat可以修改错误“FAT-fs (mmcblk0p1): Volume was not properly unmounted. Some data may be corrupt. Please run fsck.”  
* dosfstools-4.1 在ARM9上测试失败，出现“Segmentation fault”， 4.0和3.0.28测试ok  



