
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
	attr_bool \
	indirect

BACKENDS := \
	packrat \
	regex \
	llk \
	lalr \
	glr

HAMMER_PARTS := \
	bitreader.o \
	hammer.o \
	bitwriter.o \
	pprint.o \
	allocator.o \
	desugar.o \
	datastructures.o \
	system_allocator.o \
	benchmark.o \
	cfgrammar.o \
	glue.o \
	backends/lr.o \
	backends/lr0.o \
	$(PARSERS:%=parsers/%.o) \
	$(BACKENDS:%=backends/%.o)

TESTS := t_benchmark.o \
	 t_bitreader.o \
	 t_bitwriter.o \
	 t_parser.o \
	 t_grammar.o \
	 t_misc.o \
	 test_suite.o

OUTPUTS := libhammer.a \
	   test_suite.o \
	   test_suite \
	   $(HAMMER_PARTS) \
	   $(TESTS)

TOPLEVEL := ../

include ../common.mk

$(TESTS): CFLAGS += $(TEST_CFLAGS)
$(TESTS): LDFLAGS += $(TEST_LDFLAGS)

CFLAGS += -fPIC

all: libhammer.a

libhammer.a: $(HAMMER_PARTS)

bitreader.o: test_suite.h
hammer.o: hammer.h export.h allocator.h
glue.o: hammer.h glue.h export.h allocator.h

all: libhammer.a

test: test_suite
	./test_suite -v

test_suite: $(TESTS) libhammer.a
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS) $(TEST_LDFLAGS)

backends/regex.o: backends/regex_debug.c
