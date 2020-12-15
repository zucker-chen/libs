
CROSS_PREFIX	?=
OUTPUT		?= ./build

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar
CP	= cp
RM	= rm
MKDIR = mkdir

########
LIBNAME		= libcmd
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_UNIT_TEST	= test_$(LIBNAME)
TGT_UNIT_TESTS	= test_$(LIBNAME)s
TGT_UNIT_TESTC	= test_$(LIBNAME)c

OBJS_LIB	= $(LIBNAME).o
OBJS_UNIT_TEST	= test_$(LIBNAME).o ../libmsgq/libmsgq.o
OBJS_UNIT_TESTS	= test_$(LIBNAME)s.o ../libmsgq/libmsgq.o
OBJS_UNIT_TESTC	= test_$(LIBNAME)c.o ../libmsgq/libmsgq.o

CFLAGS	:= -g -Wall -fPIC #-Werror
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I. -I ../libmsgq/

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread
#LDFLAGS	+= -L . -lmsgq

.PHONY : all clean

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST) $(TGT_UNIT_TESTS) $(TGT_UNIT_TESTC)

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST) $(OBJS_UNIT_TESTS) $(OBJS_UNIT_TESTC)

all: $(TGT)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(LD) -o $@ $^ $(SHARED)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

$(TGT_UNIT_TESTS): $(OBJS_UNIT_TESTS)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

$(TGT_UNIT_TESTC): $(OBJS_UNIT_TESTC)
	$(CC) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM) -f $(OBJS)
	$(RM) -f $(TGT)

install:
	$(MKDIR) -p $(OUTPUT)/include $(OUTPUT)/lib
	$(CP) -r $(TGT_LIB_H)  $(OUTPUT)/include
	$(CP) -r $(TGT_LIB_A)  $(OUTPUT)/lib
	$(CP) -r $(TGT_LIB_SO) $(OUTPUT)/lib

uninstall:
	$(RM) -f $(OUTPUT)/include/$(TGT_LIB_H)
	$(RM) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM) -f $(OUTPUT)/lib/$(TGT_LIB_SO)
