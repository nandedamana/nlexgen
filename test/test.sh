#!/bin/bash
# Nandakumar Edamana
# File started on 2021-01-30, copying the version started in 2020 for nguigen.

if [ "$1" = '-e' ]; then
	dieonerr=1
fi

if [ "$dieonerr" ]; then
	set -e
fi

npass=0
nfail=0
fails=()

function onexit {
	if [ "$?" -ne 0 ]; then
		echo -e '\x1b[31mFAIL\x1b[0m'
	fi
}

trap onexit EXIT

while read t; do
	if [ -f "$t/Makefile" ]; then
		usemake=1
	elif [ -f "$t" ]; then # auto
		unset usemake
	else # a dir withou a Makefile
		continue
	fi
	
	echo "$t"
	echo '===='

	if [ "$usemake" ]; then
		make -C "$t" clean
		timeout 10s make -C "$t"
	else
		./test-auto.sh "$t"
	fi

	if [ "$?" -eq 0 ]; then
		echo -e '\x1b[32mPASS\x1b[0m'
		npass="$(expr $npass + 1)"
	else # won't enter if dieonerr is set; that is why I have an exit trap.
		echo -e '\x1b[31mFAIL\x1b[0m'
		nfail="$(expr $nfail + 1)"
		fails+=("$t")
	fi

	echo '----'
	echo ''
done < <(find tests-make -type d; find tests-auto -name '*.nlx' -type f)

echo ''
echo "Pass Count: $npass"
echo "Fail Count: $nfail"
echo 'Failed Tests:'
for t in ${fails[@]}; do
	echo "  $t"
done
