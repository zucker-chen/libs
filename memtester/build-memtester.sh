#!/bin/sh

memtester_ver="memtester-4.3.0"
enable_cross_compile="disable"	# enable/disable
output_path="$(pwd)/$memtester_ver/build"

# Fetch Sources
if [ ! -d $memtester_ver ]; then
	mkdir $memtester_ver
	wget http://pyropus.ca/software/memtester/old-versions/memtester-4.3.0.tar.gz
	tar xf ${memtester_ver}.tar.bz2
fi

# Cross compile cflags
if [ "$enable_cross_compile" = "enable" ]; then
	cross_prefix="arm-fullhan-linux-uclibcgnueabi-"
fi

sed -i "s/^.*cc /${cross_prefix}cc /" ./$memtester_ver/conf-cc
sed -i "s/^.*cc /${cross_prefix}cc /" ./$memtester_ver/conf-ld

# fix Makefile
esc_path=$(echo $output_path | sed '/\//s/\//\\\//g')	# 全路径'/' 转义 '\/'
#sed -n "s/^INSTALLPATH\t= .*$/INSTALLPATH\t= $esc_path/p" ./$memtester_ver/Makefile		# 整行替换 - 测试打印
sed -i "s/^INSTALLPATH\t= .*$/INSTALLPATH\t= $esc_path/" ./$memtester_ver/Makefile		# 整行替换

cd $memtester_ver

# make & install
make -j4 && make install

