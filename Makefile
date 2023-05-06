CC := gcc
CFLAGS := -O0 -g

TARGET := buddy

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf $(TARGET)
