#!/usr/bin/env bash
set -euo pipefail

# ----------------------------------------
# CONFIGURATION
# ----------------------------------------

TEST_BIN=./stress_syscalls
KP_MODULE=logsyscall.ko 

# Directory for logs & output
OUTDIR=strace_data
LOGDIR=$OUTDIR/logs
mkdir -p "$LOGDIR"

# CSV output file
CSVFILE=$OUTDIR/results.csv
echo "test,syscall,calls,total_time_s" > "$CSVFILE"

# strace options
STRACE_OPTS="-e trace=process,signal -c"

# Define your tests as "label:pre-action"
# (pre-action is run before invoking strace)
declare -a TESTS=(
  "baseline:echo '(no setup)'"
  "with_kprobe:sudo insmod $KP_MODULE uid=1000  syscall_syms="__x64_sys_execve,__x64_sys_kill,__x64_sys_exit_group,__x64_sys_clone""
  "after_remove:sudo rmmod $KP_MODULE"
)

# ----------------------------------------
# RUN TESTS
# ----------------------------------------

NUM_RUNS=10

for entry in "${TESTS[@]}"; do
  IFS=":" read -r label pre_action <<< "$entry"

  echo "=== [$label] ==="
  echo "running pre-action: $pre_action"
  eval "$pre_action"


  for ((i=0; i< NUM_RUNS; i++)); do
    logfile="$LOGDIR/${label}${i}.log"
    echo "running strace -> $logfile"
    strace -o "$logfile" $STRACE_OPTS "$TEST_BIN"

    echo "parsing summary for CSV…"
    # Skip header (first 2 lines) and footer ("total"), capture syscall lines
    # Fields: % time | seconds | usecs/call | calls | errors | syscall
    tail -n +3 "$logfile" \
        | sed '/^------/d;/^total/,$d' \
        | awk -v test="$label" '
            {
            # calls is field 4, seconds is field 2, syscall is last field
            calls = $4
            total_s = $2
            syscall = $NF
            print test "," syscall "," calls "," total_s
            }
        ' >> "$CSVFILE"

    echo
    done 
done

echo "All tests done."
echo "Data ready in $CSVFILE"