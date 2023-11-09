CC := gcc

TARGET_EXEC := clox

DEBUG_TARGET := clox_debug

BUILD_DIR := ./build
SRC_DIRS := ./src

DEBUG_DIR := ./src/**/debug.*

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')
# separate all source
MAIN_SRC := $(shell find $(SRC_DIRS) -type f \( -name '*.cpp' -or -name '*.c' -or -name '*.s' \) -and -not -path "$(DEBUG_DIR)")
# MAIN_SRC := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s' | grep -v -e 'clox_debug.c' 'debug.*')
DEBUG_SRC := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s' | grep -v -s main.c)

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
# separate all objects
MAIN_OBJ := $(MAIN_SRC:%=$(BUILD_DIR)/%.o)
DEBUG_OBJ := $(DEBUG_SRC:%=$(BUILD_DIR)/%.o)

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

# build clox
$(TARGET_EXEC): $(MAIN_OBJ)
	$(CC) $(MAIN_OBJ) -o $@ $(LDFLAGS)

${DEBUG_TARGET}: ${DEBUG_OBJ}
	$(CC) $(DEBUG_OBJ) -o $@ $(LDFLAGS)
	@echo "Running debug build..."
	./${DEBUG_TARGET}
	@echo "Done."


# # The final build step.
# $(TARGET_EXEC): $(OBJS)
# 	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# ${DEBUG_TARGET}: ${OBJS}
# 	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) ${TARGET_EXEC} ${DEBUG_TARGET}

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)