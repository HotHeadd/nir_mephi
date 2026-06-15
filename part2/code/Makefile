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

.PHONY: all clean target scenarios dirs run-tests

all: dirs $(AGENT) target scenarios

dirs:
	@mkdir -p $(BUILD) $(BIN)

# --- агент ---
$(AGENT): $(OBJ) | dirs
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(BUILD)/%.o: src/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

# --- модельные цели ---
target: $(TARGET) $(TARGET_NOPIE)

$(TARGET): target/target.c | dirs
	$(CC) $(TGTFLAGS) -o $@ $<

$(TARGET_NOPIE): target/target.c | dirs
	$(CC) $(NOPIEFLAGS) -o $@ $<

# --- сценарии валидации ---
# s1, s3, s5 — обычные PIE-цели; s2 — main + чужая .so; s4 — no-pie.
scenarios: dirs \
	$(BIN)/s1_legit_write \
	$(BIN)/s2_foreign_module \
	$(BIN)/s3_foreign_func \
	$(BIN)/s4_buffer_overflow \
	$(BIN)/s5_control_unguarded \
	$(BIN)/bench_overhead

$(BIN)/s1_legit_write: scenarios/s1_legit_write.c | dirs
	$(CC) $(TGTFLAGS) -o $@ $<

$(BIN)/s3_foreign_func: scenarios/s3_foreign_func.c | dirs
	$(CC) $(TGTFLAGS) -o $@ $<

$(BIN)/s5_control_unguarded: scenarios/s5_control_unguarded.c | dirs
	$(CC) $(TGTFLAGS) -o $@ $<

# s4 собирается no-pie: адрес поля стабилен, охраняем через --addr
$(BIN)/s4_buffer_overflow: scenarios/s4_buffer_overflow.c | dirs
	$(CC) $(NOPIEFLAGS) -o $@ $<

# s2: сначала библиотека-нарушитель, затем main линкуется с ней
$(BIN)/libs2attacker.so: scenarios/s2_attacker_lib.c | dirs
	$(CC) $(TGTFLAGS) -fPIC -shared -o $@ $<

$(BIN)/s2_foreign_module: scenarios/s2_foreign_module.c $(BIN)/libs2attacker.so | dirs
	$(CC) $(TGTFLAGS) -o $@ $< -L$(BIN) -ls2attacker -Wl,-rpath,$(BIN)

$(BIN)/bench_overhead: scenarios/bench_overhead.c | dirs
	$(CC) $(NOPIEFLAGS) -o $@ $<
clean:
	rm -rf $(BUILD) $(BIN)

-include $(OBJ:.o=.d)
