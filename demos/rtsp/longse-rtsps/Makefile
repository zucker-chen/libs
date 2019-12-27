
CROSS_PREFIX	?=
OUTPUT		?= ./build

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar
CP	= cp
RM	= rm
MKDIR = mkdir

ROOT_PATH = ../../..
########
TGT_BIN	= ff_rtspsrv
OBJS_TGT	= ff_rtspsrv.o 
OBJS_TGT	+= $(ROOT_PATH)/demos/libringbuf/ringbuf.o
OBJS_TGT	+= $(ROOT_PATH)/modules/ffmpeg/demo/media_demux.o

CFLAGS	:= -g -Wall -fPIC #-Werror
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I. -I $(ROOT_PATH)/demos/libringbuf -I $(ROOT_PATH)/modules/ffmpeg/demo/libffmpeg/include
CFLAGS	+= -I $(ROOT_PATH)/modules/ffmpeg/demo/
CFLAGS	+= -I ./librtsp

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -lpthread -L $(ROOT_PATH)/modules/ffmpeg/demo/libffmpeg/lib
LDFLAGS	+= -lpthread -L $(ROOT_PATH)/modules/ffmpeg/demo/libffmpeg/lib -lavformat -lavcodec -lswresample -lavutil
LDFLAGS	+= ./librtsp/librtsp.a
LDFLAGS	+= -lm -lrt -ldl -lpthread -lz

.PHONY : all clean

TGT	:= $(TGT_BIN)

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@


$(TGT_BIN): $(OBJS_TGT)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) -f $(OBJS)
	$(RM) -f $(TGT)

install:
	$(MKDIR) -p $(OUTPUT)/bin
	$(CP) -rf $(TGT_BIN)  $(OUTPUT)/bin

uninstall:
	$(RM) -rf $(OUTPUT)/bin/$(TGT_BIN)
	
