#!/bin/sh

set -e

TMP1=$(mktemp)
TMP2=$(mktemp)
TMP3=$(mktemp)

i686-echidnaos-objdump -t echidna.elf | sed '/\bd\b/d' | sort > "$TMP1"
grep "\.text" < "$TMP1" | cut -d' ' -f1 > "$TMP2"
grep "\.text" < "$TMP1" | awk 'NF{ print $NF }' > "$TMP3"

cat <<EOF >symlist.gen
#include <symlist.h>

__attribute__((section(".symlist")))
struct symlist_t symlist[] = {
EOF

paste -d'$' "$TMP2" "$TMP3" | sed 's/^/    {0x/g' | sed 's/\$/, "/g' | sed 's/$/"},/g' >> symlist.gen

cat <<EOF >> symlist.gen
    {0xffffffff, ""}
};
EOF

rm "$TMP1" "$TMP2" "$TMP3"
