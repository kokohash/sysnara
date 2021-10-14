#!/bin/bash

TIMEFORMAT=%R
for i in {1..100}
do
    { time ./mdu -j $i /pkg/ ; } 2>> data.txt
done