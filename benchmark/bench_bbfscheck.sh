#!/bin/sh
set -eu

BBFSCHECK_BIN="${BBFSCHECK_BIN:-./bbfscheck}"
RUNS="${RUNS:-10}"
WARMUPS="${WARMUPS:-2}"
LOOPS_PER_RUN="${LOOPS_PER_RUN:-20}"
SUMMARY_PATH="${SUMMARY_PATH:-/}"
INODE_PATH="${INODE_PATH:-$SUMMARY_PATH}"
SMALL_FILE_SIZE="${SMALL_FILE_SIZE:-4096}"
MAX_DEPTH="${MAX_DEPTH:-3}"
RESULT_DIR="${RESULT_DIR:-benchmark/results}"
WORK_DIR="${WORK_DIR:-${TMPDIR:-/tmp}/bbfscheck_bench_$$}"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"

cleanup()
{
    rm -rf "$WORK_DIR"
}

trap cleanup EXIT INT TERM

require_cmd()
{
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "missing required command: $1" >&2
        exit 1
    fi
}

check_positive_integer()
{
    case "$2" in
        ''|*[!0-9]*)
            echo "$1 must be a positive integer" >&2
            exit 2
            ;;
        0)
            echo "$1 must be greater than zero" >&2
            exit 2
            ;;
    esac
}

prepare_scan_tree()
{
    scan_dir="$WORK_DIR/scan-tree"

    mkdir -p "$scan_dir"/small "$scan_dir"/nested/a "$scan_dir"/nested/b "$scan_dir"/medium

    i=1
    while [ "$i" -le 1000 ]; do
        printf 'x\n' > "$scan_dir/small/file_$i.txt"
        i=$((i + 1))
    done

    i=1
    while [ "$i" -le 50 ]; do
        printf 'metadata benchmark payload %s\n' "$i" > "$scan_dir/nested/a/item_$i.log"
        printf 'metadata benchmark payload %s\n' "$i" > "$scan_dir/nested/b/item_$i.log"
        i=$((i + 1))
    done

    i=1
    while [ "$i" -le 8 ]; do
        dd if=/dev/zero of="$scan_dir/medium/blob_$i.bin" bs=64K count=1 >/dev/null 2>&1
        i=$((i + 1))
    done

    printf '%s\n' "$scan_dir"
}

write_environment()
{
    env_file="$1"

    {
        echo "# bbfscheck Benchmark Environment"
        echo
        echo "- Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
        echo "- Host: $(hostname 2>/dev/null || echo unknown)"
        echo "- Kernel: $(uname -srmo 2>/dev/null || uname -a)"
        echo "- OS: $(sed -n 's/^PRETTY_NAME=//p' /etc/os-release 2>/dev/null | tr -d '\"' || echo unknown)"
        echo "- CPU: $(sed -n 's/^model name[[:space:]]*: //p' /proc/cpuinfo 2>/dev/null | sed -n '1p')"
        echo "- RAM: $(awk '/MemTotal/ {printf "%.0f MiB", $2 / 1024}' /proc/meminfo 2>/dev/null)"
        echo "- GCC: $(gcc --version 2>/dev/null | sed -n '1p' || echo unavailable)"
        echo "- BusyBox: $(busybox 2>&1 | sed -n '1p' || echo unavailable)"
        echo "- bbfscheck: $BBFSCHECK_BIN"
        echo "- Runs: $RUNS"
        echo "- Warmups: $WARMUPS"
        echo "- Loops per measured run: $LOOPS_PER_RUN"
        echo "- Summary path: $SUMMARY_PATH"
        echo "- Inode path: $INODE_PATH"
        echo "- Scan max depth: $MAX_DEPTH"
        echo "- Small file threshold: $SMALL_FILE_SIZE bytes"
    } > "$env_file"
}

time_once()
{
    command_text="$1"
    metrics_file="$2"
    loop_command='i=1; while [ "$i" -le '"$LOOPS_PER_RUN"' ]; do '"$command_text"' || exit $?; i=$((i + 1)); done'

    set +e
    "$TIME_BIN" -f '%e,%U,%S,%P,%M,%I,%O,%w,%c,%x' \
        sh -c "$loop_command" >/dev/null 2>"$metrics_file"
    status=$?
    set -e

    if [ "$status" -ne 0 ]; then
        echo "command failed: $command_text" >&2
        cat "$metrics_file" >&2
        exit "$status"
    fi
}

run_case()
{
    case_name="$1"
    tool_name="$2"
    command_text="$3"
    csv_file="$4"

    i=1
    while [ "$i" -le "$WARMUPS" ]; do
        time_once "$command_text" "$WORK_DIR/time.tmp"
        i=$((i + 1))
    done

    i=1
    while [ "$i" -le "$RUNS" ]; do
        time_once "$command_text" "$WORK_DIR/time.tmp"
        metrics=$(sed -n '$p' "$WORK_DIR/time.tmp" | tr -d '%')
        printf '%s,%s,%s,%s,%s\n' "$case_name" "$tool_name" "$i" "$LOOPS_PER_RUN" "$metrics" >> "$csv_file"
        i=$((i + 1))
    done
}

summarize_csv()
{
    csv_file="$1"
    summary_file="$2"

    awk -F, '
        NR == 1 { next }
        {
            key = $1 "," $2
            if (!(key in seen)) {
                seen[key] = 1
                order[++order_count] = key
                task[key] = $1
                tool[key] = $2
            }

            count[key]++
            loops = $4 + 0
            if (loops <= 0) {
                loops = 1
            }
            elapsed[key, count[key]] = ($5 + 0) / loops
            sum_elapsed[key] += ($5 + 0) / loops
            sum_user[key] += ($6 + 0) / loops
            sum_sys[key] += ($7 + 0) / loops
            sum_cpu[key] += $8
            sum_rss[key] += $9
            sum_in[key] += ($10 + 0) / loops
            sum_out[key] += ($11 + 0) / loops
            sum_vctx[key] += ($12 + 0) / loops
            sum_ivctx[key] += ($13 + 0) / loops
            sum_exit[key] += $14
        }

        function sort_values(key, n,    i, j, tmp) {
            for (i = 1; i <= n; i++) {
                sorted[i] = elapsed[key, i]
            }
            for (i = 1; i <= n; i++) {
                for (j = i + 1; j <= n; j++) {
                    if (sorted[j] < sorted[i]) {
                        tmp = sorted[i]
                        sorted[i] = sorted[j]
                        sorted[j] = tmp
                    }
                }
            }
        }

        function median_value(n) {
            if (n % 2 == 1) {
                return sorted[(n + 1) / 2]
            }
            return (sorted[n / 2] + sorted[n / 2 + 1]) / 2
        }

        BEGIN {
            print "| Task | Tool | Runs | Avg elapsed (s) | Median elapsed (s) | Avg CPU % | Avg user (s) | Avg sys (s) | Avg max RSS (KB) | Avg fs in | Avg fs out | Avg ctx switch | Exit sum |"
            print "| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"
        }

        END {
            for (idx = 1; idx <= order_count; idx++) {
                key = order[idx]
                n = count[key]
                sort_values(key, n)
                printf "| %s | %s | %d | %.4f | %.4f | %.1f | %.4f | %.4f | %.0f | %.1f | %.1f | %.1f | %.0f |\n",
                    task[key], tool[key], n,
                    sum_elapsed[key] / n,
                    median_value(n),
                    sum_cpu[key] / n,
                    sum_user[key] / n,
                    sum_sys[key] / n,
                    sum_rss[key] / n,
                    sum_in[key] / n,
                    sum_out[key] / n,
                    (sum_vctx[key] + sum_ivctx[key]) / n,
                    sum_exit[key]
                delete sorted
            }
        }
    ' "$csv_file" > "$summary_file"
}

write_gap_table()
{
    csv_file="$1"
    gap_file="$2"

    awk -F, '
        NR == 1 { next }
        {
            key = $1 "," $2
            count[key]++
            loops = $4 + 0
            if (loops <= 0) {
                loops = 1
            }
            elapsed[key, count[key]] = ($5 + 0) / loops
            if (!($1 in task_seen)) {
                task_seen[$1] = 1
                tasks[++task_count] = $1
            }
        }

        function sort_values(key, n,    i, j, tmp) {
            for (i = 1; i <= n; i++) {
                sorted[i] = elapsed[key, i]
            }
            for (i = 1; i <= n; i++) {
                for (j = i + 1; j <= n; j++) {
                    if (sorted[j] < sorted[i]) {
                        tmp = sorted[i]
                        sorted[i] = sorted[j]
                        sorted[j] = tmp
                    }
                }
            }
        }

        function median_for(key,    n, result) {
            n = count[key]
            if (n == 0) {
                return 0
            }
            sort_values(key, n)
            if (n % 2 == 1) {
                result = sorted[(n + 1) / 2]
            } else {
                result = (sorted[n / 2] + sorted[n / 2 + 1]) / 2
            }
            delete sorted
            return result
        }

        BEGIN {
            print "| Task | Reference Tool | Reference Median (s) | Custom Median (s) | Gap | Pass <= 1.5x |"
            print "| --- | --- | ---: | ---: | ---: | --- |"
        }

        END {
            for (i = 1; i <= task_count; i++) {
                t = tasks[i]
                custom_key = t ",bbfscheck"
                ref_tool = ""
                for (key in count) {
                    split(key, parts, ",")
                    if (parts[1] == t && parts[2] != "bbfscheck") {
                        ref_tool = parts[2]
                    }
                }
                if (ref_tool == "") {
                    continue
                }
                ref_key = t "," ref_tool
                custom = median_for(custom_key)
                ref = median_for(ref_key)
                gap = ref > 0 ? custom / ref : 0
                pass = gap <= 1.5 ? "yes" : "no"
                printf "| %s | %s | %.4f | %.4f | %.2fx | %s |\n", t, ref_tool, ref, custom, gap, pass
            }
        }
    ' "$csv_file" > "$gap_file"
}

check_positive_integer RUNS "$RUNS"
check_positive_integer WARMUPS "$WARMUPS"
check_positive_integer LOOPS_PER_RUN "$LOOPS_PER_RUN"
check_positive_integer MAX_DEPTH "$MAX_DEPTH"
check_positive_integer SMALL_FILE_SIZE "$SMALL_FILE_SIZE"

require_cmd awk
require_cmd date
require_cmd dd
require_cmd df
require_cmd du
require_cmd sed
require_cmd "$TIME_BIN"

if [ ! -x "$BBFSCHECK_BIN" ]; then
    echo "bbfscheck binary is not executable: $BBFSCHECK_BIN" >&2
    echo "Build it first, or set BBFSCHECK_BIN=/path/to/bbfscheck" >&2
    exit 1
fi

mkdir -p "$RESULT_DIR" "$WORK_DIR"
SCAN_DIR=$(prepare_scan_tree)
STAMP=$(date '+%Y%m%d_%H%M%S')
RAW_CSV="$RESULT_DIR/bbfscheck_${STAMP}.csv"
SUMMARY_MD="$RESULT_DIR/bbfscheck_${STAMP}.md"
ENV_MD="$RESULT_DIR/bbfscheck_${STAMP}_environment.md"
TABLE_TMP="$WORK_DIR/summary_table.md"
GAP_TMP="$WORK_DIR/gap_table.md"

cat > "$RAW_CSV" <<'CSV'
task,tool,run,loops_per_run,elapsed_s,user_s,sys_s,cpu_pct,max_rss_kb,fs_inputs,fs_outputs,voluntary_ctx,involuntary_ctx,exit_code
CSV

write_environment "$ENV_MD"

echo "Benchmarking bbfscheck with $RUNS run(s), $WARMUPS warmup(s)"
echo "Each run loops $LOOPS_PER_RUN time(s) and reports per-invocation resource averages"
echo "Raw CSV: $RAW_CSV"

run_case "summary" "bbfscheck" "'$BBFSCHECK_BIN' --summary '$SUMMARY_PATH'" "$RAW_CSV"
run_case "summary" "df -P" "df -P '$SUMMARY_PATH'" "$RAW_CSV"

run_case "inode" "bbfscheck" "'$BBFSCHECK_BIN' --inode '$INODE_PATH'" "$RAW_CSV"
run_case "inode" "df -Pi" "df -Pi '$INODE_PATH'" "$RAW_CSV"

run_case "scan" "bbfscheck" "'$BBFSCHECK_BIN' --scan '$SCAN_DIR' --max-depth '$MAX_DEPTH' --small-file '$SMALL_FILE_SIZE'" "$RAW_CSV"
run_case "scan" "du -ak" "du -ak '$SCAN_DIR'" "$RAW_CSV"

summarize_csv "$RAW_CSV" "$TABLE_TMP"
write_gap_table "$RAW_CSV" "$GAP_TMP"

{
    cat "$ENV_MD"
    echo
    echo "## Resource Summary"
    echo
    cat "$TABLE_TMP"
    echo
    echo "## Median Runtime Gap"
    echo
    cat "$GAP_TMP"
    echo
    echo "## Metric Notes"
    echo
    echo "- elapsed_s: wall-clock elapsed time from GNU time."
    echo "- Each measured sample runs the command LOOPS_PER_RUN times; elapsed/user/sys/fs/context metrics are reported per invocation."
    echo "- user_s/sys_s: CPU time spent in user/kernel mode."
    echo "- cpu_pct: average CPU utilization during the command."
    echo "- max_rss_kb: peak resident memory size."
    echo "- fs_inputs/fs_outputs: filesystem input/output counts reported by wait4/resource usage."
    echo "- ctx switch: voluntary plus involuntary context switches."
    echo "- scan compares metadata traversal against du -ak; output is not identical, but both walk the tree."
    echo
    echo "Raw measurements: $RAW_CSV"
} > "$SUMMARY_MD"

cat "$SUMMARY_MD"
