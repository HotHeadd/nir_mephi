#!/bin/bash
# bench_stats.sh — статистика по μс/итерацию, база на большом N
N_AGENT=1000          # под агентом: мало, иначе ждать вечность
N_BASE=100000000      # без агента: много, чтобы таймер не давал нули
REPS=30

run_mode () {
    local label="$1"; shift
    local vals=()
    for i in $(seq 1 $REPS); do
        v=$("$@" 2>/dev/null | grep 'на одну итерацию' | grep -oE '[0-9]+\.[0-9]+')
        vals+=("$v")
    done
    printf '%s\n' "${vals[@]}" | python3 -c '
import sys, statistics as st
xs=[float(l) for l in sys.stdin if l.strip()]
xs.sort()
m=st.median(xs)
# печать в мкс и в нс — нс полезны для крошечной базы
print(f"  n={len(xs)}  медиана={m:.4f} мкс ({m*1000:.2f} нс)  "
      f"среднее={st.mean(xs):.4f}  min={xs[0]:.4f}  max={xs[-1]:.4f}  "
      f"стд={st.pstdev(xs):.4f} мкс")
'
    echo "[$label]"
}

echo "=== БЕЗ агента (N=$N_BASE) ==="
run_mode "без агента" ./bin/bench_overhead $N_BASE

echo "=== ПОД агентом, регистрация (N=$N_AGENT) ==="
run_mode "регистрация" ./bin/memwatch --symbol critical_flag -- ./bin/bench_overhead $N_AGENT

echo "=== ПОД агентом, --allow main (N=$N_AGENT) ==="
run_mode "allow" ./bin/memwatch --symbol critical_flag --allow main -- ./bin/bench_overhead $N_AGENT