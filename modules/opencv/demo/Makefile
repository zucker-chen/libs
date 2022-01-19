

CROSS_PREFIX	?=
OUTPUT		?= ./build

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar
CP	= cp
RM	= rm
MKDIR = mkdir



MAKEFILE_NAMES += "Makefile.tenengrad"
MAKEFILE_NAMES += "Makefile.plane"


all:
	for file in $(MAKEFILE_NAMES); do \
	echo "===> make -f $$file CROSS_PREFIX=$(CROSS_PREFIX)"; \
	make -f $$file CROSS_PREFIX=$(CROSS_PREFIX) ARCH=$(ARCH); \
	done	


clean:
	for file in $(MAKEFILE_NAMES); do \
	echo "===> make -f $$file CROSS_PREFIX=$(CROSS_PREFIX) clean"; \
	make -f $$file CROSS_PREFIX=$(CROSS_PREFIX) ARCH=$(ARCH) clean; \
	done	


