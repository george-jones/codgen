#!/bin/bash

# for doing statistical testing on map generation.
# anything the program writes to stderr will be written in results.txt

> results.txt
for ((i=0; i<25; i++)) do
	echo ${i}
	./codgen ${i} > /dev/null 2>>results.txt
done

