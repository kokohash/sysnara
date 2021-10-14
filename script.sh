#!/bin/bash

TIMEFORMAT=%R
for i in {1..100}
do
    { time ./mdu -j $i /pkg/ ; } 2>> data.txt
done

for j in {1..100}
do
    { time ./mdu -j $j /pkg/ ; } 2>> data2.txt
done

for k in {1..100}
do
    { time ./mdu -j $k /pkg/ ; } 2>> data3.txt
done

for l in {1..100}
do
    { time ./mdu -j $l /pkg/ ; } 2>> data4.txt
done

for m in {1..100}
do
    { time ./mdu -j $m /pkg/ ; } 2>> data5.txt
done

for n in {1..100}
do
    { time ./mdu -j $n /pkg/ ; } 2>> data6.txt
done