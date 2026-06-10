# memwatch — опытный образец. Объектные файлы в build/, бинарники в bin/.
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -g -O0 -Iinclude -D_GNU_SOURCE -MMD -MP
TGTFLAGS:= -std=c11 -g -O0
NOPIEFLAGS:= -std=c11 -g -O0 -no-pie

BUILD   := build
BIN     := bin

SRC     := $(wildcard src/*.c)
OBJ     := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
AGENT   := $(BIN)/memwatch

TARGET  := $(BIN)/target
TARGET_NOPIE := $(BIN)/target_nopie
SCEN_SRC:= $(wildcard scenarios/s*.c)
SCEN_BIN:= $(patsubst scenarios/%.c,$(BIN)/%,$(SCEN_SRC))

.PHONY: all clean target scenarios dirs

all: dirs $(AGENT) target scenarios

dirs:
	mkdir -p $(BUILD) $(BIN)

$(AGENT): $(OBJ) | dirs
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(BUILD)/%.o: src/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

target: $(TARGET) $(TARGET_NOPIE)

$(TARGET): target/target.c | dirs
	$(CC) $(TGTFLAGS) -o $@ $<

$(TARGET_NOPIE): target/target.c | dirs
	$(CC) $(NOPIEFLAGS) -o $@ $<

scenarios: $(SCEN_BIN)

$(BIN)/%: scenarios/%.c | dirs
	$(CC) $(TGTFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD) $(BIN)

-include $(OBJ:.o=.d)
