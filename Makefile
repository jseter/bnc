INSTALL_DIR=/usr/local
BIN_DIR=$(INSTALL_DIR)/bin

OBJS = bnc.c
STATIC = -static

all: bnc

bnc: $(OBJS)
	gcc -o bnc bnc.c
	strip bnc

bnc-static: $(OBJS)
	gcc -o bnc bnc.c
	strip bnc

clean: 
	rm -f core bnc *~

install: all
	install -s -m 755 bnc $(BIN_DIR)
