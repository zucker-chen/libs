
# gpioconfig
海思3516CV500下的2相4线电机驱动   

# function
* 8拍：static unsigned char LensStep_8P[] = {0x1, 0x5, 0x4, 0x6, 0x2, 0xA, 0x8, 0x9};	 // 8拍正向	A -> AB -> B -> A`B -> A` -> A`B` -> B` -> AB`
    * 4拍：static unsigned char LensStep_4P[] = {0x9, 0x5, 0x6, 0xA};	// 双8拍正向 	AB` -> AB -> A`B -> A`B`
    * 备注：反向的话逆向即可
    * 8拍方式的时序如下：
```
         A     B     C     D     E     F     G     H（时序）
A       1     1     0     0     0     0     0     1
A-      0     0     0     1     1     1     0     0
B       0     1     1     1     0     0     0     0
B-      0     0     0     0     0     1     1     1
```

# TEST
* make  

