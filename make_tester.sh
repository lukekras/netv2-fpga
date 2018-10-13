#!/bin/sh

PYTHONHASHSEED=1

./netv2mvp.py -p 100 -t gtptester
cp ./build/gateware/top.bit ./tester-images/gtptester-100.bit
./netv2mvp.py -p 35 -t gtptester
cp ./build/gateware/top.bit ./tester-images/gtptester-35.bit

cd ./firmware-tester && make clean && make && cd ..
cp ./firmware-tester/firmware.bin ./tester-images/gtptester-firmware.bin

./netv2mvp.py -p 100 -t tester
cp ./build/gateware/top.bit ./tester-images/tester-100.bit
./netv2mvp.py -p 35 -t tester
cp ./build/gateware/top.bit ./tester-images/tester-35.bit

cp ./firmware-tester/firmware.bin ./tester-images/tester-firmware.bin


