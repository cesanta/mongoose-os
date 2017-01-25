MAKEFLAGS += --warn-undefined-variables
.DEFAULT_GOAL := all

BUILD_DIR ?= $(CURDIR)/.build

CFLAGS ?= -Os -Wall -Werror

LOCAL_CFLAGS = -MD \
               -Wno-missing-field-initializers \
               -Wno-old-style-declaration \
               -Wno-unused-parameter \
               -Wno-unused-variable

.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/libatca.a

SRCS = $(notdir $(wildcard *.c basic/*.c host/*.c))
OBJS = $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(SRCS)))

VPATH = basic host
IPATH = $(addprefix -I,$(VPATH))

$(BUILD_DIR)/libatca.a: $(OBJS)
	@echo "  AR    $@"
	$(AR) -rc $@ $^

$(BUILD_DIR)/%.o: %.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) $(IPATH) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf libatca.a $(BUILD_DIR)

# Pull in auto-generated .c dependencies (if any).
-include $(wildcard $(BUILD_DIR)/*.d)
