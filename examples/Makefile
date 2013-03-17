
OUTPUTS := dns.o \
	   dns \
	   base64.o \
	   base64 \
	   base64_sem1.o \
	   base64_sem1 \
	   base64_sem2.o \
	   base64_sem2

TOPLEVEL := ../

include ../common.mk
CFLAGS += $(pkg-config --cflags glib-2.0)
LDFLAGS += $(pkg-config --libs glib-2.0)



all: dns base64 base64_sem1 base64_sem2

dns: LDFLAGS:=-L../src -lhammer $(LDFLAGS)
dns: dns.o rr.o dns_common.o
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

dns.o: ../src/hammer.h dns_common.h ../src/glue.h
rr.o: ../src/hammer.h rr.h dns_common.h ../src/glue.h
dns_common.o: ../src/hammer.h dns_common.h ../src/glue.h

base64: LDFLAGS:=-L../src -lhammer $(LDFLAGS)
base64: base64.o
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

base64_sem1: LDFLAGS:=-L../src -lhammer $(LDFLAGS)
base64_sem1: base64_sem1.o
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

base64_sem2: LDFLAGS:=-L../src -lhammer $(LDFLAGS)
base64_sem2: base64_sem2.o
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

base64%.o: ../src/hammer.h ../src/glue.h
