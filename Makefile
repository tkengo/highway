vpath %.h include
vpath %.c src

GCC = gcc
.SUFFIXES: .c .o
.PHONY: all
.PHONY: clean

BUILD_DIR = tmp/build/
SOURCES = main.c
OBJECTS = $(addprefix $(BUILD_DIR),$(SOURCES:%.c=%.o))
DEPENDS = $(OBJECTS:%.o=%.d)

TARGET = hw

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(GCC) $^ -o $@

$(BUILD_DIR)%.d: %.c
	$(GCC) -MM $< | sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)\1.o: ,g' > $@
	echo "\t$(GCC) -c -o $(subst .d,.o,$@) $$<" >> $@

-include $(DEPENDS)

clean:
	rm $(BUILD_DIR)*.o $(BUILD_DIR)*.d*
