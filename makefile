# ATTRIBUTION: Heavily based on makefile from
# https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

TARGET_EXEC ?= prog

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

CC := g++

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d) /usr/local/include ./include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

HEADER_DIVIDER_STRING := '----------------------------------------'
BOLD := \033[1m
RESET := \033[0m

# -flto: Link-time optimization
# -pg  : Instrument code for gprof
# -g   : Instrument code for gdb
INSTRUMENTATION_FLAGS := -g # -pg
OPTIMIZATION_LEVEL := #-O3
LTO_FLAG := #-flto
PPROF_FLAGS := -Wl --no-as-needed -lprofiler --as-needed 
CPPFLAGS := $(INSTRUMENTATION_FLAGS) $(INC_FLAGS) $(LTO_FLAG) -MMD -MP -std=c++17 -Wall $(OPTIMIZATION_LEVEL)
#FINAL_ARGS := -framework OpenGL -lglfw -lglew  # OSX flags
FINAL_ARGS := -lGLEW -lglfw -lGL -lX11 $(OPTIMIZATION_LEVEL) $(LTO_FLAG)

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	@printf '\n$(BOLD)$(HEADER_DIVIDER_STRING)\n'
	@printf 'Linking Object Files\n'
	@printf '\tInstrumentation Flags:  $(INSTRUMENTATION_FLAGS)\n'
	@printf '\tOptimization Level:     $(OPTIMIZATION_LEVEL)\n'
	@printf '\tLink-Time Optimization: $(LTO_FLAG)\n'
	@printf '$(HEADER_DIVIDER_STRING)$(RESET)\n'

	$(CC) $(INSTRUMENTATION_FLAGS) $(OBJS) $(FINAL_ARGS) -o $@ $(LDFLAGS)
	mv $(BUILD_DIR)/$(TARGET_EXEC) ./$(TARGET_EXEC)

# Assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# C Source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# C++ source
#@echo $($<)
$(BUILD_DIR)/%.cpp.o: %.cpp
	@printf '\n$(BOLD)$(HEADER_DIVIDER_STRING)\n'
	@printf '$< \n'
	@printf '\tInstrumentation Flags:  $(INSTRUMENTATION_FLAGS)\n'
	@printf '\tOptimization Level:     $(OPTIMIZATION_LEVEL)\n'
	@printf '\tLink-Time Optimization: $(LTO_FLAG)\n'
	@printf '$(HEADER_DIVIDER_STRING)$(RESET)\n'

	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
