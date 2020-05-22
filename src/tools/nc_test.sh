#!/usr/bin/env bash

max=10;
for i in {0..4}; do
	echo "connected"
	nc localhost 1337 <<< "hello $i\n" &
done
