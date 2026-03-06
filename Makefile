OUT := ucd-gen
FT_MACROS := -D_DEFAULT_SOURCE
LIB_FLAGS :=

CC := gcc
CC_FLAGS := -O3 -Wall -MMD -MP

INC_DIR := include
SRC_DIR := src
BUILD_DIR := build

rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
SRC_FILES := $(call rwildcard,$(SRC_DIR)/,*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))
INC_FLAGS := $(addprefix -I,$(INC_DIR))
CC_BUILD := $(CC) $(FT_MACROS) $(INC_FLAGS) $(PP_FLAGS) $(CC_FLAGS)

all: $(OUT)

$(OUT): $(BUILD_DIR) $(OBJ_FILES)
	$(CC) -o $(OUT) $(LIB_FLAGS) $(OBJ_FILES)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC_BUILD) -c "$<" -o "$@"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

-include $(OBJ_FILES:.o=.d)

test: $(OUT)
	./$(OUT)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(OUT)

.PHONY:
	all clean test
