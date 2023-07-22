#!/bin/bash
# Nandakumar Edamana
# File started on 2021-01-30, copying the version started in 2020 for nguigen.
# env: NLEXFLAGS

set -e
set -o nounset

if [ -z "${1-}" ]; then
	>&2 echo "Usage: $0 NLXFILE"
	exit 1
fi

scriptdir="$(dirname "$0")"

nlxfile="$1"
ocfile="$1.nlexout.c"
mcfile="$1.main.c"
elffile="$1.elf"

tstfile=''

ECHOARGS=''

if [ -f "$1.test" ]; then
	tstfile="$1.test"
elif [ -f "$1.test-bashesc" ]; then
	tstfile="$1.test-bashesc"
	ECHOARGS='-e '
else
	>&2 echo 'error: test file not found.'
	exit 1
fi

nlxopts=${NLEXFLAGS-}
if [ "$(echo "$nlxfile"|grep fastkw)" ]; then
	nlxopts='--fastkeywords'
fi

echo '#include <assert.h>' > "$ocfile"
echo '#include <ctype.h>' >> "$ocfile"
echo '#include <read.h>' >> "$ocfile"
echo 'extern int ch;' >> "$ocfile"
echo 'void get_token(NlexHandle * nh) {' >> "$ocfile"
# TODO timeout?
cat "$nlxfile"|"$(dirname "$0")"/../src/nlexgen $nlxopts >> "$ocfile"
echo '}' >> "$ocfile"

rsync "$scriptdir"'/main-for-auto.c' "$mcfile"

cc -o "$elffile" -g "$mcfile" "$ocfile" "$(dirname "$0")"/../src/read.o -I"$(dirname "$0")"/../src

while IFS= read -r line; do
echo "$line"
	inp="$(echo "$line"|cut -f 1)"
	exp="$(echo "$line"|cut -f 2)"

	echo "INPUT $inp"
	test "$(echo $ECHOARGS-n "$inp"|timeout 1s "$elffile")" = "$exp"
	echo "$exp"
	echo ''
done < "$tstfile"
