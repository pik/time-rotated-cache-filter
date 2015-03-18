
DEPS = \
	src/murmur.c \
	
LIBOBJECTS = \
	src/trcf.c \

WORDS_FILE= ~/packages/Words/Words/en.txt	
BLDDIR = build
CFLAGS = -g -Wall -O2 -fms-extensions
LDFLAGS =
CC = gcc

all: install

clean: 
	rm $(BLDDIR)/test_trcf
	rmdir $(BLDDIR)

install: test_trcf

test_trcf:
	@mkdir -p $(BLDDIR)
	$(CC) $(CFLAGS) $(DEPS) $(LIBOBJECTS) src/test_trcf.c $(LDFLAGS) -o $(BLDDIR)/$@

test: 
	@$(BLDDIR)/test_trcf $(WORDS_FILE)

.PHONY: all clean install test
