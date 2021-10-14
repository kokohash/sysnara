#!/bin/bash

for i in {1..100}
do
    echo $(time ./mdu -j $i /pkg/ > data.txt)
done