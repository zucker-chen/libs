
## DES(支持二进制数据)
=========
### DES介绍说明：
* DES是对称性编码里面常见一种，全称为Data Encryption Standard，即数据编码标准，是一种使用密钥编码的块算法。
	密钥长度是64位(bit)，超过位数密钥被忽略。所谓对称性编码，编码和解码密钥相同。  
	对称性编码一般会按照固定长度，把待编码字符串分成块。不足一整块或者刚好最后有特殊填充字符。  
	往往跨语言做DES编码解码，经常会出现问题。往往是填充方式不对、或者编码不一致、或者选择编码解码模式(ECB,CBC,CTR,OFB,CFB,NCFB,NOFB)没有对应上造成。  
	常见的填充模式有： 'pkcs5','pkcs7','iso10126','ansix923','zero' 类型，包括DES-ECB,DES-CBC,DES-CTR,DES-OFB,DES-CFB  
	我们选用的DES源码使用DES-ECB，'pkcs7'  
* 有的DES算法源码是针对字符串编码， 所以会导致二进制编码时碰到'0x00'就会停止
	如果数据类型用char *定义则是针对字符串编码，我们选用了二进制编码算法的DES，用unsigned char *数据类型
* 编码后数据长度，'pkcs7'填充方式，
	编码数据8byte为单位，最后不足8byte则填充，每填充数据为padding = 8 - in_len%8，如果padding=8则额外编码8byte padding数据  
	所以编码后数据总长度为：8 * ((len)/8 + 1)
* DES KEY，8byte长度，可由rand()%255生成随机数
	rand随机数需要配合srand (time()) seed种子，这样就可以满足1秒更新1次种子
* 由于编码时有数据填充，输出解码数据长度需要去掉编码时的填充长度padding

### DES测试方法
```
sh make.sh
echo -n "hello world 123456789" > 1.txt 
# encrypt test
./build/bin/test_des 1.txt 2.txt 0 && cat 2.txt | base64
# decrypt test
./build/bin/test_des 2.txt 3.txt 1 && cat 3.txt && echo
# check
diff 1.txt 3.txt
```
可以通过在线[加密工具](http://tool.chacuo.net/cryptdes)验证加密算法标准  
#### 加密工具使用方法
* DES加密模式：ECB  
* 填充方式：pkcs7padding  
* 加密密码：12345678  
* 选择字符集：UTF-8编码  
* 结果验证：将加密结果(转码成base64)，与cat 2.txt | base64比较是否一致  



## BASE64(支持二进制数据)
==========
#### BASE64介绍说明：
* Base64编码要求把3个8位字节（3*8=24）转化为4个6位的字节（4*6=24），之后在6位的前面补两个0，形成8位一个字节的形式。 如果剩下的字符不足3个字节，则用0填充，输出字符使用'='，因此编码后输出的文本末尾可能会出现1或2个'='
* 输入数据长度3字节对齐，不足填充0；输出4字节对齐，所以输出长度为：(3*(len/3+((len%3) > 0)))*4/3
* 由于编码时有数据填充，解码数据长度需要去掉末尾的'='字符


## DES/BASE64混合加密(支持二进制数据)
#### DES/BASE64混合加密介绍说明：
* 先将数据进行DES加密  
* 再将DES的加密数据进行BASE64转码  
* 在IPCamera项目中重置密码模块中使用  

