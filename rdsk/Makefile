.PHONY: all clean

ifeq ($(DESTDIR),)
DESTDIR := /
endif

ifeq ($(PREFIX),)
PREFIX := /usr
endif

all: clean rdsk

rdsk: src/main.c
	gcc -o $@ $<

clean:
	rm -f rdsk

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp rdsk $(DESTDIR)$(PREFIX)/bin/rdsk