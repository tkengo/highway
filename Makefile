vpath %.h include
vpath %.c src

GCC = gcc
.SUFFIXES: .c .o
.PHONY: all
.PHONY: clean

BUILD_DIR = tmp/build/
INCLUDE = -Iinclude
LIB = -liconv
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
		  help.c
OBJECTS = $(addprefix $(BUILD_DIR),$(SOURCES:%.c=%.o))
DEPENDS = $(OBJECTS:%.o=%.d)

TARGET = hw

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(GCC) -O2 $(INCLUDE) $(LIB) $^ -o $@

$(BUILD_DIR)%.d: %.c
	mkdir -p $(BUILD_DIR)
	$(GCC) -MM $(INCLUDE) $< | sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)\1.o: ,g' > $@
	echo "\t$(GCC) -O2 $(INCLUDE) -c -o $(subst .d,.o,$@) $$<" >> $@

-include $(DEPENDS)

clean:
	rm $(BUILD_DIR)*.o $(BUILD_DIR)*.d*
