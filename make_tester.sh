#!/bin/sh

PYTHONHASHSEED=1

if [ -z "$1" ]
then
    RUN="all"
    echo "None of gtptester, user, or tester specified, so running all builds." 
else
    if [ "$1" = "help" ]
    then
	echo "Valid build options: gtptester, user, tester, or all. Blank is the same as all."
	exit 0
    fi
    RUN="$1"
    echo "Run set to $1."
fi

if [ "$RUN" = "all" ] || [ "$RUN" = "gtptester" ]
then
    echo "Running gtptester 100T FPGA build..."
    ./netv2mvp.py -p 100 -t gtptester
    cp ./build/gateware/top.bit ./tester-images/gtptester-100.bit
    echo "Running gtptester 35T FPGA build..."
    ./netv2mvp.py -p 35 -t gtptester
    cp ./build/gateware/top.bit ./tester-images/gtptester-35.bit
    
    echo "Running gtptester firmware build..."
    cd ./firmware-tester && make clean && make && cd ..
    cp ./firmware-tester/firmware.bin ./tester-images/gtptester-firmware.bin
fi

if [ "$RUN" = "all" ] || [ "$RUN" = "user" ]
then
    echo "Running user 100T FPGA build..."
    ./netv2mvp.py -p 100 -t video_overlay
    cp ./build/gateware/top.bit ./tester-images/user-100.bit
    echo "Running user 35T FPGA build..."
    ./netv2mvp.py -p 35 -t video_overlay
    cp ./build/gateware/top.bit ./tester-images/user-35.bit
    
    echo "Running user firmware build..."
    cd ./firmware-tester && make clean && make && cd ..
    cp ./firmware-tester/firmware.bin ./tester-images/user-firmware.bin
fi

if [ "$RUN" = "all" ] || [ "$RUN" = "tester" ]
then
    echo "Running tester 35T FPGA build..."
    ./netv2mvp.py -p 100 -t tester
    cp ./build/gateware/top.bit ./tester-images/tester-100.bit
    echo "Running tester 100T FPGA build..."
    ./netv2mvp.py -p 35 -t tester
    cp ./build/gateware/top.bit ./tester-images/tester-35.bit
    
    echo "Running tester firmware build..."
    cd ./firmware-tester && make clean && make && cd ..
    cp ./firmware-tester/firmware.bin ./tester-images/tester-firmware.bin
fi

echo "All requested builds done."

exit 0
