
CROSS_PREFIX	?=
OUTPUT			?= ./build

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar
CP	= cp
RM	= rm
MKDIR = mkdir

ROOT_PATH = ../../../..
MEDIA_PATH = $(ROOT_PATH)/demos/rtsp/ireader-rtsp/media-server
SDK_PATH = $(ROOT_PATH)/demos/rtsp/ireader-rtsp/sdk

########
TGT_BIN	= ff_rtspsrv
TGT_LIB	= librtsps_api

OBJS_TGT	= ff_rtspsrv.o 
OBJS_TGT	+= $(ROOT_PATH)/modules/ffmpeg/demo/media_demux.o
OBJS_LIB	+= rtsps_api.o 
OBJS_LIB	+= $(ROOT_PATH)/demos/libringbuf/ringbuf.o

SOURCE_FILES += $(wildcard $(MEDIA_PATH)/librtp/payload/*.c)
SOURCE_FILES += $(wildcard $(MEDIA_PATH)/librtp/source/*.c)
SOURCE_FILES += $(wildcard $(MEDIA_PATH)/librtsp/source/*.c)
SOURCE_FILES += $(wildcard $(MEDIA_PATH)/librtsp/source/server/*.c)
SOURCE_FILES += $(wildcard $(MEDIA_PATH)/librtsp/source/client/*.c)
SOURCE_FILES += $(wildcard $(MEDIA_PATH)/librtsp/source/server/aio/*.c)
SOURCE_FILES += $(wildcard $(SDK_PATH)/libaio/src/*.c)
SOURCE_FILES += $(wildcard $(SDK_PATH)/libhttp/source/*.c)
SOURCE_FILES += $(SDK_PATH)/source/uri-parse.c
SOURCE_FILES += $(SDK_PATH)/source/urlcodec.c
SOURCE_FILES += $(SDK_PATH)/source/ntp-time.c
SOURCE_FILES += $(SDK_PATH)/source/time64.c
SOURCE_FILES += $(SDK_PATH)/source/twtimer.c
SOURCE_FILES += $(SDK_PATH)/source/base64.c
#SOURCE_FILES += $(SDK_PATH)/source/unicode.c
#SOURCE_FILES += $(SDK_PATH)/source/thread-pool.c
#SOURCE_FILES += $(SDK_PATH)/source/app-log.c
SOURCE_FILES += $(SDK_PATH)/source/port/ip-route.c
SOURCE_FILES += $(SDK_PATH)/source/port/aio-socket-epoll.c
SOURCE_FILES += $(SDK_PATH)/source/i18n/nls_cp936.c
SOURCE_FILES += $(SDK_PATH)/source/sockpair.c
SOURCE_FILES += $(SDK_PATH)/source/darray.c
#SOURCE_FILES += $(SDK_PATH)/source/channel.c
#SOURCE_FILES += $(SDK_PATH)/source/uuid.c
SOURCE_FILES += $(SDK_PATH)/source/random.c
SOURCE_FILES += $(SDK_PATH)/source/digest/md5.c
OBJS_LIB	+= $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SOURCE_FILES)))

CFLAGS	:= -g -Wall -fPIC #-Werror
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I. -I $(ROOT_PATH)/demos/libringbuf -I $(ROOT_PATH)/modules/ffmpeg/demo/libffmpeg/include
CFLAGS	+= -I $(ROOT_PATH)/modules/ffmpeg/demo
CFLAGS	+= -I $(SDK_PATH)/include
CFLAGS	+= -I $(SDK_PATH)/include/sys
CFLAGS	+= -I $(SDK_PATH)/libhttp/include
CFLAGS	+= -I $(SDK_PATH)/libaio/include
CFLAGS	+= -I $(MEDIA_PATH)/librtsp/include
CFLAGS	+= -I $(MEDIA_PATH)/librtp/include

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS) -L .
LDFLAGS	+= -L $(ROOT_PATH)/modules/ffmpeg/demo/libffmpeg/lib
LDFLAGS	+= -L $(ROOT_PATH)/modules/ffmpeg/demo/libffmpeg/lib -lavformat -lavcodec -lswresample -lavutil
LDFLAGS	+= -lm -lrt -ldl -lpthread -lz


.PHONY : all clean

TGT	:= $(TGT_LIB) $(TGT_BIN)
OBJS	:= $(OBJS_LIB) $(OBJS_TGT)
all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
%.o : %.cpp
	$(CXX) -c $(CFLAGS) $< -o $@

$(TGT_LIB): $(OBJS_LIB)
	$(AR) rcs $@.a $^
	$(LD) -o $@.so $^ $(SHARED)

$(TGT_BIN): $(OBJS_TGT)
	$(CC) -o $@ $^ $(LDFLAGS) $(TGT_LIB).a

clean:
	$(MAKE) -C $(SDK_PATH) clean
	$(MAKE) -C $(MEDIA_PATH) clean
	$(RM) -f $(OBJS) $(OBJS_TGT)
	$(RM) -f $(TGT) $(TGT_LIB).*

install:
	$(MKDIR) -p $(OUTPUT)/bin
	$(CP) -rf $(TGT_BIN)  $(OUTPUT)/bin
	$(MKDIR) -p $(OUTPUT)/include
	$(CP) -rf $(TGT_LIB).h  $(OUTPUT)/include
	

uninstall:
	$(RM) -rf $(OUTPUT)/bin/$(TGT_BIN)
	
