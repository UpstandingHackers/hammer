
PARSERS := \
	unimplemented \
	bits \
	token \
	whitespace \
	ignoreseq \
	ch \
	action \
	charset \
	int_range \
	sequence \
	choice \
	nothing \
	end \
	butnot \
	difference \
	many \
	xor \
	optional \
	ignore \
	epsilon \
	and \
	not \
	attr_bool

OUTPUTS := bitreader.o \
	   hammer.o \
	   libhammer.a \
	   pprint.o \
	   allocator.o \
	   datastructures.o \
	   test_suite \
	   $(PARSERS:%=parsers/%.o)

TOPLEVEL := ../

include ../common.mk


all: libhammer.a test_suite

test_suite: test_suite.o libhammer.a
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

libhammer.a: bitreader.o hammer.o pprint.o allocator.o datastructures.o \
	$(PARSERS:%=parsers/%.o)

bitreader.o: test_suite.h
hammer.o: hammer.h

test: test_suite
	./test_suite -v
