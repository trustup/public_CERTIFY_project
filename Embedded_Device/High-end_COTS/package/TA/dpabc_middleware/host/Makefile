CC      ?= $(CROSS_COMPILE)gcc
LD      ?= $(CROSS_COMPILE)ld
AR      ?= $(CROSS_COMPILE)ar
NM      ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump
READELF ?= $(CROSS_COMPILE)readelf

TEEC_EXPORT ?= ../../../optee_client/libteec
DPABC_EXPORT ?= ../ta/lib/p-abc-main
PFECC_EXPORT ?= ../ta/lib/p-abc-main/lib/pfecCwrapper
MIRACL_EXPORT ?= ../ta/lib/p-abc-main/lib/pfecCwrapper/lib/Miracl_Core

ACEUNIT = lib/aceunit/bin/aceunit

OBJS = main.o

CFLAGS += -Wall -I../ta/include -I$(TEEC_EXPORT)/include -I./lib/aceunit/include -I./include -I$(DPABC_EXPORT)/include -I$(PFECC_EXPORT)/include -I$(MIRACL_EXPORT)
#Add/link other required libraries here
#LDADD += -lteec -L$(TEEC_EXPORT)/lib

BINARY = main

.PHONY: all
all: testcases.c
	rm $(OBJS)

testcases.c: $(OBJS)
	$(ACEUNIT) $(ACEUNIT_FLAGS) $^ >$@

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY) testcases.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

