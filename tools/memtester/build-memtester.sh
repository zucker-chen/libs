#!/bin/sh

memtester_ver="memtester-4.3.0"
enable_cross_compile="disable"	# enable/disable
cross_prefix="arm-hisiv500-linux-"
output_path="$(pwd)/$memtester_ver/build"

# Fetch Sources
if [ ! -d $memtester_ver ]; then
	mkdir $memtester_ver
	wget http://pyropus.ca/software/memtester/old-versions/memtester-4.3.0.tar.gz
	tar xvf ${memtester_ver}.tar.gz
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_pri_cflags="${cross_prefix}"
fi

sed -i "s/^.*cc /${cross_pri_cflags}gcc /" ./$memtester_ver/conf-cc
sed -i "s/^.*cc /${cross_pri_cflags}gcc /" ./$memtester_ver/conf-ld

# fix Makefile
esc_path=$(echo $output_path | sed '/\//s/\//\\\//g')	# 全路径'/' 转义 '\/'
#sed -n "s/^INSTALLPATH\t= .*$/INSTALLPATH\t= $esc_path/p" ./$memtester_ver/Makefile		# 整行替换 - 测试打印
sed -i "s/^INSTALLPATH\t= .*$/INSTALLPATH\t= $esc_path/" ./$memtester_ver/Makefile		# 整行替换
#sed -n "s/{bin,man\/man8}/bin \$(INSTALLPATH)\/man\/man8/p" ./$memtester_ver/Makefile	# 将"{bin,man/man8}"替换成"$(INSTALLPATH)/man/man8" - 测试打印
sed -i "s/{bin,man\/man8}/bin \$(INSTALLPATH)\/man\/man8/" ./$memtester_ver/Makefile	# 将"{bin,man/man8}"替换成"$(INSTALLPATH)/man
    /man8"

cd $memtester_ver

# make & install
make -j4 && make install


# tips
# Makefile 中 mkdir -p {bin,man/man8} 语法解析会存在问题，不能正确创建目录
