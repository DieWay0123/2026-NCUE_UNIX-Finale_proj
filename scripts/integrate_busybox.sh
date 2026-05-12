#!/bin/sh
set -eu

usage()
{
    cat <<'EOF'
Usage: scripts/integrate_busybox.sh [BUSYBOX_DIR]

Copy the diagnostics toolkit applet sources into a BusyBox source tree and
regenerate BusyBox applet metadata.

BUSYBOX_DIR defaults to ./busybox.

After this script runs:
  cd busybox
  make defconfig
  ../scripts/integrate_busybox.sh .
  make -j$(nproc) busybox
  ./busybox bbfscheck --summary /
EOF
}

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
busybox_dir=${1:-"$repo_root/busybox"}

if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
    usage
    exit 0
fi

if [ ! -d "$busybox_dir" ]; then
    printf '%s\n' "error: BusyBox directory not found: $busybox_dir" >&2
    exit 1
fi

if [ ! -d "$busybox_dir/miscutils" ] ||
   [ ! -x "$busybox_dir/scripts/gen_build_files.sh" ]; then
    printf '%s\n' "error: $busybox_dir does not look like a BusyBox source tree" >&2
    exit 1
fi

for path in \
    "$repo_root/applets/bbfscheck.c" \
    "$repo_root/libdiag/diag_common.h" \
    "$repo_root/libdiag/fs_reader.c" \
    "$repo_root/libdiag/formatter.c" \
    "$repo_root/libdiag/rule_checker.c"
do
    if [ ! -f "$path" ]; then
        printf '%s\n' "error: required source file missing: $path" >&2
        exit 1
    fi
done

tmp_file=$(mktemp "${TMPDIR:-/tmp}/bbfscheck-busybox.XXXXXX")
trap 'rm -f "$tmp_file"' EXIT HUP INT TERM

awk '
BEGIN {
    print "//config:config BBFSCHECK"
    print "//config:\tbool \"bbfscheck\""
    print "//config:\tdefault n"
    print "//config:\thelp"
    print "//config:\tLightweight filesystem health checking applet for the"
    print "//config:\tBusyBox Diagnostics Toolkit project."
    print ""
    print "//applet:IF_BBFSCHECK(APPLET(bbfscheck, BB_DIR_USR_BIN, BB_SUID_DROP))"
    print ""
    print "//kbuild:lib-$(CONFIG_BBFSCHECK) += bbfscheck.o diag_fs_reader.o diag_formatter.o diag_rule_checker.o"
    print ""
    print "//usage:#define bbfscheck_trivial_usage"
    print "//usage:       \"--summary PATH | --inode PATH | --scan PATH | --check PATH\""
    print "//usage:#define bbfscheck_full_usage \"\\n\\n\""
    print "//usage:       \"Filesystem health checking and mount inspection tool\\n\""
    print "//usage:     \"\\n\t--summary PATH\tShow filesystem block usage\""
    print "//usage:     \"\\n\t--inode PATH\tShow filesystem inode usage\""
    print "//usage:     \"\\n\t--scan PATH\tScan directory metadata\""
    print "//usage:     \"\\n\t--check PATH\tShow combined filesystem health report\""
    print "//usage:     \"\\n\t--show-mount PATH Show mount information backing PATH\""
    print "//usage:     \"\\n\t--mount-tree\tShow visible mounts as a tree\""
    print "//usage:     \"\\n\t--max-depth N\tLimit scan recursion depth\""
    print "//usage:     \"\\n\t--small-file N\tCount files at or below N bytes\""
    print "//usage:     \"\\n\t-h\t\tHuman-readable sizes using powers of 1024\""
    print "//usage:     \"\\n\t-H\t\tHuman-readable sizes using powers of 1000\""
    print "//usage:     \"\\n\t-B SIZE\t\tShow summary sizes in SIZE-byte blocks\""
    print ""
}
$0 == "#include \"../libdiag/diag_common.h\"" {
    print "#include \"libbb.h\""
    print "#include \"diag_common.h\""
    next
}
$0 == "int main(int argc, char **argv)" {
    print "int bbfscheck_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;"
    print "int bbfscheck_main(int argc, char **argv)"
    next
}
{
    print
}
' "$repo_root/applets/bbfscheck.c" > "$tmp_file"

cp "$tmp_file" "$busybox_dir/miscutils/bbfscheck.c"
cp "$repo_root/libdiag/diag_common.h" "$busybox_dir/miscutils/diag_common.h"
cp "$repo_root/libdiag/fs_reader.c" "$busybox_dir/miscutils/diag_fs_reader.c"
cp "$repo_root/libdiag/formatter.c" "$busybox_dir/miscutils/diag_formatter.c"
cp "$repo_root/libdiag/rule_checker.c" "$busybox_dir/miscutils/diag_rule_checker.c"

(
    cd "$busybox_dir"
    sh scripts/gen_build_files.sh . .
)

if [ -f "$busybox_dir/.config" ]; then
    if grep -q '^# CONFIG_BBFSCHECK is not set$' "$busybox_dir/.config"; then
        sed -i 's/^# CONFIG_BBFSCHECK is not set$/CONFIG_BBFSCHECK=y/' "$busybox_dir/.config"
        printf '%s\n' "enabled CONFIG_BBFSCHECK=y in $busybox_dir/.config"
    elif grep -q '^CONFIG_BBFSCHECK=y$' "$busybox_dir/.config"; then
        printf '%s\n' "CONFIG_BBFSCHECK=y is already enabled in $busybox_dir/.config"
    else
        printf '%s\n' 'CONFIG_BBFSCHECK=y' >> "$busybox_dir/.config"
        printf '%s\n' "added CONFIG_BBFSCHECK=y to $busybox_dir/.config"
    fi
else
    printf '%s\n' "note: no .config found in $busybox_dir"
    printf '%s\n' "note: run make defconfig, then rerun this script to enable CONFIG_BBFSCHECK=y"
fi

printf '%s\n' "integrated bbfscheck into $busybox_dir"
printf '%s\n' "next: make -C \"$busybox_dir\" busybox"
