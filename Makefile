vpath %.h include
vpath %.c src

GCC = gcc
.SUFFIXES: .c .o
.PHONY: all
.PHONY: clean

BUILD_DIR = tmp/build/
INCLUDE = -Iinclude -Ivendor/onigmo
LIB = -L/usr/local/lib -liconv
SOURCES = \
		  highway.c \
		  file.c \
		  queue.c \
		  worker.c \
		  search.c \
		  ignore.c \
		  log.c \
		  option.c \
		  util.c \
		  regex.c \
		  help.c \
		  vendor/onigmo/regcomp.c \
		  vendor/onigmo/regenc.c \
		  vendor/onigmo/regerror.c \
		  vendor/onigmo/regexec.c \
		  vendor/onigmo/regext.c \
		  vendor/onigmo/reggnu.c \
		  vendor/onigmo/regparse.c \
		  vendor/onigmo/regposerr.c \
		  vendor/onigmo/regposix.c \
		  vendor/onigmo/regsyntax.c \
		  vendor/onigmo/regtrav.c \
		  vendor/onigmo/regversion.c \
		  vendor/onigmo/st.c \
		  vendor/onigmo/enc/ascii.c \
		  vendor/onigmo/enc/big5.c \
		  vendor/onigmo/enc/cp1251.c \
		  vendor/onigmo/enc/cp932.c \
		  vendor/onigmo/enc/euc_jp.c \
		  vendor/onigmo/enc/euc_kr.c \
		  vendor/onigmo/enc/euc_tw.c \
		  vendor/onigmo/enc/gb18030.c \
		  vendor/onigmo/enc/iso8859_1.c \
		  vendor/onigmo/enc/iso8859_10.c \
		  vendor/onigmo/enc/iso8859_11.c \
		  vendor/onigmo/enc/iso8859_13.c \
		  vendor/onigmo/enc/iso8859_14.c \
		  vendor/onigmo/enc/iso8859_15.c \
		  vendor/onigmo/enc/iso8859_16.c \
		  vendor/onigmo/enc/iso8859_2.c \
		  vendor/onigmo/enc/iso8859_3.c \
		  vendor/onigmo/enc/iso8859_4.c \
		  vendor/onigmo/enc/iso8859_5.c \
		  vendor/onigmo/enc/iso8859_6.c \
		  vendor/onigmo/enc/iso8859_7.c \
		  vendor/onigmo/enc/iso8859_8.c \
		  vendor/onigmo/enc/iso8859_9.c \
		  vendor/onigmo/enc/koi8.c \
		  vendor/onigmo/enc/koi8_r.c \
		  vendor/onigmo/enc/mktable.c \
		  vendor/onigmo/enc/sjis.c \
		  vendor/onigmo/enc/unicode.c \
		  vendor/onigmo/enc/utf16_be.c \
		  vendor/onigmo/enc/utf16_le.c \
		  vendor/onigmo/enc/utf32_be.c \
		  vendor/onigmo/enc/utf32_le.c \
		  vendor/onigmo/enc/utf8.c
OBJECTS = $(addprefix $(BUILD_DIR),$(SOURCES:%.c=%.o))
DEPENDS = $(OBJECTS:%.o=%.d)

TARGET = hw

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(GCC) -O2 $(INCLUDE) $(LIB) $^ -o $@

$(BUILD_DIR)%.d: %.c
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)vendor/onigmo/enc
	$(GCC) -MM $(INCLUDE) $< | sed 's,\($(@F:.d=)\)\.o[ :]*,$(@D)/\1.o: ,g' > $@
	echo "\t$(GCC) -O2 $(INCLUDE) -c -o $(subst .d,.o,$@) $$<" >> $@

-include $(DEPENDS)

clean:
	rm $(BUILD_DIR)*.o $(BUILD_DIR)*.d*
