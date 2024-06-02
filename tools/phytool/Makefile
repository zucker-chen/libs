.PHONY: all clean install dist

# Top directory for building complete system, fall back to this directory
ROOTDIR    ?= $(shell pwd)

#CROSS_PREFIX	?= aarch64-linux-gnu-
CROSS_PREFIX	?=
CC	= $(CROSS_PREFIX)gcc

VERSION = 2
TARGET    = phytool
APPLETS = mv6tool

PREFIX ?= build
CFLAGS ?= -Wall -Wextra -Werror
LDLIBS  = 

objs = $(patsubst %.c, %.o, $(wildcard *.c))
hdrs = $(wildcard *.h)

%.o: %.c $(hdrs) Makefile
	@printf "  CC      $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CC) $(CFLAGS) -c $< -o $@

phytool: $(objs)
	@mkdir -p $(PREFIX)/bin/
	@printf "  CC      $(subst $(ROOTDIR)/,,$(shell pwd)/$@)\n"
	@$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^

all: phytool

clean:
	@rm -f *.o
	@rm -f $(TARGET)
	@rm -rf $(PREFIX)

install: phytool
	@cp phytool $(PREFIX)/bin/
	@for app in $(APPLETS); do \
		ln -sf phytool $(PREFIX)/bin/$$app; \
	done
