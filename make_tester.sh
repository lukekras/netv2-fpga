#!/bin/sh

PYTHONHASHSEED=1

if [ -z "$1" ]
then
    RUN="all"
    echo "None of gtptester or tester specified, so running all builds." 
else
    if [ "$1" = "help" ]
    then
	echo "Valid build options: gtptester, tester, or all. Blank is the same as all."
	exit 0
    fi
    RUN="$1"
    echo "Run set to $1."
fi

if [ -z "$2" ]
then
    TYPE="all"
    echo "None of 35T or 100T specified, so making for both types."
else
    if [ "$2" = "35T" ]
    then
	TYPE="35T"
    elif [ "$2" = "100T" ]
    then
	TYPE="100T"
    else
	TYPE="all"
    fi
fi

if [ "$RUN" = "all" ] || [ "$RUN" = "gtptester" ]
then
    if [ "$TYPE" = "all" ] || [ "$TYPE" = "100T" ]
       then
	   echo "Running gtptester 100T FPGA build..."
	   ./netv2mvp.py -p 100 -t gtptester
	   cp ./build/gateware/top.bit ./tester-images/gtptester-100.bit
    fi
    if [ "$TYPE" = "all" ] || [ "$TYPE" = "35T" ]
       then
	   echo "Running gtptester 35T FPGA build..."
	   ./netv2mvp.py -p 35 -t gtptester
	   cp ./build/gateware/top.bit ./tester-images/gtptester-35.bit
    fi
    
    echo "Running gtptester firmware build..."
    cd ./firmware-tester && make clean && make && cd ..
    cp ./firmware-tester/firmware.bin ./tester-images/gtptester-firmware.bin
fi

# if [ "$RUN" = "all" ] || [ "$RUN" = "user" ]
# then
#     echo "Running user 100T FPGA build..."
#     ./netv2mvp.py -p 100 -t video_overlay
#     cp ./build/gateware/top.bit ./tester-images/user-100.bit
#     echo "Running user 35T FPGA build..."
#     ./netv2mvp.py -p 35 -t video_overlay
#     cp ./build/gateware/top.bit ./tester-images/user-35.bit
    
#     echo "Running user firmware build..."
#     cd ./firmware-tester && make clean && make && cd ..
#     cp ./firmware-tester/firmware.bin ./tester-images/user-firmware.bin
# fi

if [ "$RUN" = "all" ] || [ "$RUN" = "tester" ]
then
    if [ "$TYPE" = "all" ] || [ "$TYPE" = "100T" ]
       then
	   echo "Running tester 100T FPGA build..."
	   ./netv2mvp.py -p 100 -t tester
	   cp ./build/gateware/top.bit ./tester-images/tester-100.bit
    fi

    if [ "$TYPE" = "all" ] || [ "$TYPE" = "35T" ]
       then
	   echo "Running tester 35T FPGA build..."
	   ./netv2mvp.py -p 35 -t tester
	   cp ./build/gateware/top.bit ./tester-images/tester-35.bit
    fi
    
    echo "Running tester firmware build..."
    cd ./firmware-tester && make clean && make && cd ..
    cp ./firmware-tester/firmware.bin ./tester-images/tester-firmware.bin
fi

echo "All requested builds done."

exit 0
