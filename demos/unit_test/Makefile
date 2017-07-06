#
# Makefile module
# make single file(.c .cpp) to execution
#

# cross compile swich
MCC ?= 0
ifeq ($(MCC), 1)
CROSS_COMPILE := arm-hisiv500-linux-
EXT_INCS      :=    # -I$(REL_INC)
EXT_LIBS      :=    # $(MPI_LIBS) $(AUDIO_LIBA) -lpthread -lm -ldl
else
CROSS_COMPILE :=
endif

AS		:= $(CROSS_COMPILE)as
LD		:= $(CROSS_COMPILE)ld
CC		:= $(CROSS_COMPILE)gcc
CXX		:= $(CROSS_COMPILE)g++
AR		:= $(CROSS_COMPILE)ar
STRIP	:= $(CROSS_COMPILE)strip
CFLAGS 		:= -O3 -Wall -fPIC $(EXT_INCS)
LIBS		:= $(EXT_LIBS)


SRCS		:= $(wildcard *.c) $(wildcard *.cpp)
OBJS		:= $(addsuffix .o,$(basename $(SRCS))) 
TARGETS		:= $(OBJS:%.o=%)	#$(basename $(SRCS))

.c.o: $(filter %.c %.s,$(wildcard *.c))
	$(CC) $(CFLAGS) -c -o $@ $< 

.cpp.o: $(filter %.cpp,$(wildcard *.cpp))
	$(CXX) $(CFLAGS) -c -o $@ $<

	.PHONY: all $(TARGETS)

all: $(TARGETS)

$(TARGETS):%: %.o
	$(CC) -o $@ $< $(LIBS) $(CFLAGS)
	
clean: 
	rm -fr *.o $(TARGETS)

rebuild: clean all

	
# tips:
# * Makefile static mode:
# 	<targets ...>: <target-pattern>: <prereq-patterns ...>
# * 'for' function need use '$$value' to get the value (shell cmd) like:
#	@for i in $(TARGETS); do\
#		$(CC) -o $${i} $${i}.o $(LIBS) $(CFLAGS);\
#	done

	
