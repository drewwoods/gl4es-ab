# Sourced by build.sh and crashtest.sh: list the demos for usage messages.
# list_demos <gl4es repo> <gl4es-ab dir>
list_demos() {
    local d name title a b
    for d in "$2"/demos/*/; do
        name=$(basename "$d")
        [ -f "$d/main.c" ] || continue
        title=$(sed -n 's/^# *//p;q' "$d/README.md" 2>/dev/null | sed 's/^[0-9][0-9]* · //')
        a=$(sed -n 's/^REF_A=//p' "$d/refs" 2>/dev/null)
        b=$(sed -n 's/^REF_B=//p' "$d/refs" 2>/dev/null)
        printf '  %-22s %s -> %s  %s\n' "$name" "${a:0:10}" "${b:0:10}" "$title"
    done
}
