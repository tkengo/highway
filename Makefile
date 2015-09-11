vpath %.h include
vpath %.c src

GCC = gcc
.SUFFIXES: .c .o
.PHONY: all
.PHONY: clean

BUILD_DIR = tmp/build/
INCLUDE = include
SOURCES = \
		  main.c \
		  queue.c
OBJECTS = $(addprefix $(BUILD_DIR),$(SOURCES:%.c=%.o))
DEPENDS = $(OBJECTS:%.o=%.d)

TARGET = hw

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(GCC) -I$(INCLUDE) $^ -o $@

$(BUILD_DIR)%.d: %.c
	$(GCC) -MM -I$(INCLUDE) $< | sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)\1.o: ,g' > $@
	echo "\t$(GCC) -I$(INCLUDE) -c -o $(subst .d,.o,$@) $$<" >> $@

-include $(DEPENDS)

clean:
	rm $(BUILD_DIR)*.o $(BUILD_DIR)*.d*
