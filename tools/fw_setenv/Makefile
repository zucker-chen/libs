
# arm-hisiv500-linux- / arm-himix200-linux-
CROSS_PREFIX	?= arm-linux-gnueabihf-
OUTPUT		    ?= ./build

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar
STRIP	= $(CROSS_PREFIX)strip
CP	= cp
RM	= rm
MKDIR = mkdir

########
LIBNAME		= fw_env
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_UNIT_TEST	= test_$(LIBNAME)

OBJS_LIB	= $(LIBNAME).o crc32.o aes.o
OBJS_UNIT_TEST	= fw_env_main.o

CFLAGS	:= -g -fPIC -Wall # -DDEBUG #-Werror -Wall
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I.

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread

.PHONY : all clean

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(LD) -o $@ $^ $(SHARED)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)
	$(STRIP) $(TGT_UNIT_TEST)

clean:
	$(RM) -f $(OBJS)
	$(RM) -f $(TGT)

install:
	$(MKDIR) -p $(OUTPUT)/include $(OUTPUT)/lib
	$(CP) -r $(TGT_LIB_H)  $(OUTPUT)/include
	$(CP) -r $(TGT_LIB_A)  $(OUTPUT)/lib
	$(CP) -r $(TGT_LIB_SO) $(OUTPUT)/lib
	$(MKDIR) -p $(OUTPUT)/bin
	$(CP) -r $(TGT_UNIT_TEST)  $(OUTPUT)/bin/fw_setenv
	cd $(OUTPUT)/bin && ln -sf fw_setenv fw_printenv
	$(CP) -r fw_env.config  $(OUTPUT)/bin/

uninstall:
	$(RM) -f $(OUTPUT)/include/$(TGT_LIB_H)
	$(RM) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM) -f $(OUTPUT)/lib/$(TGT_LIB_SO)
