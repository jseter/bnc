INSTALL_DIR=/usr/local
BIN_DIR=$(INSTALL_DIR)/bin

OBJS = bnc.c
C_OPT = -O3 -Wall -Wunused
STATIC = -static

all: bnc

bnc: $(OBJS)
	gcc $(C_OPT) $(LIBS) -o $@ $^
	strip bnc

bnc-static: $(OBJS)
	gcc $(STATIC) $(C_OPT) $(LIBS) -o $@ $^
	strip bnc

clean: 
	rm -f core bnc

install: all
	install -s -m 755 bnc $(BIN_DIR)
