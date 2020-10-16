
# arm-hisiv500-linux- / arm-himix200-linux-
#CROSS_PREFIX	?= arm-linux-gnueabihf-
OUTPUT		    ?= ./build

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar
CP	= cp
RM	= rm
MKDIR = mkdir

########
LIBNAME		= sparse
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_IMG2SIMG	= img2simg
TGT_SIMG2IMG	= simg2img

OBJS_LIB	= $(LIBNAME).o sparse_crc32.o sparse_err.o backed_block.o sparse_read.o output_file.o
OBJS_IMG2SIMG	= img2simg.o
OBJS_SIMG2IMG	= simg2img.o

CFLAGS	:= -g -fPIC -w #-Werror -Wall
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I. -I./include

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -L. -lpthread -lz

.PHONY : all clean

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_IMG2SIMG)
TGT	+= $(TGT_SIMG2IMG)

OBJS	:= $(OBJS_LIB) $(OBJS_IMG2SIMG) $(OBJS_SIMG2IMG)

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(LD) -o $@ $^ $(SHARED)

$(TGT_IMG2SIMG): $(OBJS_IMG2SIMG)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

$(TGT_SIMG2IMG): $(OBJS_SIMG2IMG)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS)
	$(RM) -f $(TGT)

install:
	$(MKDIR) -p $(OUTPUT)/include $(OUTPUT)/lib
	$(CP) -r include/sparse/$(TGT_LIB_H)  $(OUTPUT)/include
	$(CP) -r $(TGT_LIB_A)  $(OUTPUT)/lib
	$(CP) -r $(TGT_LIB_SO) $(OUTPUT)/lib
	$(MKDIR) -p $(OUTPUT)/bin
	$(CP) -r $(TGT_IMG2SIMG)  $(OUTPUT)/bin
	$(CP) -r $(TGT_SIMG2IMG)  $(OUTPUT)/bin

uninstall:
	$(RM) -f $(OUTPUT)/include/$(TGT_LIB_H)
	$(RM) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM) -f $(OUTPUT)/lib/$(TGT_LIB_SO)

# Note:
#	1. 交叉编译会提示找不到-lz，zlib库，需要手动交叉编译改库并拷贝相关库和头文件到该目录后进行编译，zlib库的编译路径：[../zlib]
