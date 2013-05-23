JSOURCES := Action.java Hammer.java ParsedToken.java ParseResult.java Parser.java Predicate.java
JSOURCES_NATIVE := Hammer ParsedToken Parser ParseResult
CSOURCES := com_upstandinghackers_hammer_Hammer.c com_upstandinghackers_hammer_ParsedToken.c com_upstandinghackers_hammer_Parser.c com_upstandinghackers_hammer_ParseResult.c

# ls *.h *.o *.so com/upstandinghackers/hammer/*.class | grep -v jhammer.h | tr '\n' ' '; replace single $ with $$
OUTPUTS := com/upstandinghackers/hammer/Action.class com/upstandinghackers/hammer/Hammer.class com_upstandinghackers_hammer_Hammer.h com_upstandinghackers_hammer_Hammer.o com/upstandinghackers/hammer/Hammer\$TokenType.class com_upstandinghackers_hammer_Hammer_TokenType.h com/upstandinghackers/hammer/ParsedToken.class com_upstandinghackers_hammer_ParsedToken.h com_upstandinghackers_hammer_ParsedToken.o com/upstandinghackers/hammer/Parser.class com/upstandinghackers/hammer/ParseResult.class com_upstandinghackers_hammer_ParseResult.h com_upstandinghackers_hammer_ParseResult.o com_upstandinghackers_hammer_Parser.h com_upstandinghackers_hammer_Parser.o com/upstandinghackers/hammer/Predicate.class libjhammer.so

TOPLEVEL := ../

JC=javac
JH=javah
CP=com/upstandinghackers/hammer
PACKAGE=com.upstandinghackers.hammer

include ../common.mk

JNI_INCLUDE := /usr/lib/jvm/java-6-openjdk/include/
CFLAGS += -fPIC -I. -I $(TOPLEVEL)/src/ -I jni -I $(JNI_INCLUDE) 

%.java: $(call ifsilent,| $(HUSH))
	$(call hush, "Compiling Java source $@") $(JC) $(CP)/$@

all: javacc prepare compile link

link: compile
	$(call hush, "Generating libjhammer.so") $(CC) -shared $(CFLAGS) -o libjhammer.so *.o ../src/*.o ../src/backends/*.o ../src/parsers/*.o

$(CSOURCES): prepare
	$(call hush, "Compiling $@") $(CC) -c $(CFLAGS) $@

compile: prepare $(CSOURCES)

prepare: javacc $(JSOURCES_NATIVE)

$(JSOURCES_NATIVE): javacc
	$(call hush, "Generating JNI headers for $@") $(JH) $(PACKAGE).$@

javacc: $(JSOURCES)

#TODO make this not-as-hardcoded
#clean:
#	rm $(CP)/*.class && rm com_upstandinghackers_*.h && rm com_upstandinghackers_*.o && rm libjhammer.so
