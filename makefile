CC := gcc

TARGET_EXEC := clox

BUILD_DIR := ./build
SRC_DIRS := ./src

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.i
# .i files are preprocessed source files
EXTENDS := $(OBJS:.o=.i)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP 
PPFLAGS := -E
CFLAGS := -Wall -Wextra
# link libmath
LDFLAGS := -lm

# Optional debug flag (-g)
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS += -g
endif

# disassemble lox script flag
DISASSEMBLE ?= 0
ifeq ($(DISASSEMBLE), 1)
	CPPFLAGS += -DCLOX_DEBUG_DISASSEMBLE
endif

# trace lox instruction flag
TRACE ?= 0
ifeq ($(TRACE), 1)
	CPPFLAGS += -DCLOX_DEBUG_TRACE_EXECUTION
endif

LOXPATH := lox/test.lox

# build clox
$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) ${CFLAGS}

# run clox
.PHONY: REPL
REPL: $(TARGET_EXEC)
	@echo "Running build..."
	./${TARGET_EXEC}
	@echo "Done."

.PHONY: script
script: ${TARGET_EXEC}
	@echo "Running build..."
	./${TARGET_EXEC} ${LOXPATH}
	@echo "Done."

# preprocess clox
.PHONY: preprocess
preprocess: ${EXTENDS}

# Target to generate object files
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ ${GDBFLAGS}

# Target to generate preprocessed source file
$(BUILD_DIR)/%.c.i: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(PPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) ${TARGET_EXEC}

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)