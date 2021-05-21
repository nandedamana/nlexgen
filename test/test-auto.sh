#!/bin/bash
# Nandakumar Edamana
# File started on 2021-01-30, copying the version started in 2020 for nguigen.

set -e

if [ -z "$1" ]; then
	echo "Usage: $0 NLXFILE"
	exit 1
fi

scriptdir="$(dirname "$0")"

nlxfile="$1"
ocfile="$1.nlexout.c"
mcfile="$1.main.c"
elffile="$1.elf"
tstfile="$1.test"

echo '#include <assert.h>' > "$ocfile"
echo '#include <read.h>' >> "$ocfile"
echo 'extern int ch;' >> "$ocfile"
echo 'void get_token(NlexHandle * nh) {' >> "$ocfile"
# TODO timeout?
cat "$nlxfile"|"$(dirname "$0")"/../src/nlexgen >> "$ocfile"
echo '}' >> "$ocfile"

rsync "$scriptdir"'/main-for-auto.c' "$mcfile"

cc -o "$elffile" "$mcfile" "$ocfile" "$(dirname "$0")"/../src/read.o -I"$(dirname "$0")"/../src

while IFS= read -r line; do
	inp="$(echo "$line"|cut -f 1)"
	exp="$(echo "$line"|cut -f 2)"

	echo "INPUT $inp"
	test "$(echo -n "$inp"|timeout 4s "$elffile")" = "$exp"
done < "$tstfile"
