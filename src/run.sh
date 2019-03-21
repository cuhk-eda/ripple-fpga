#!/bin/bash
BM_PATH=${BENCHMARK_PATH?"Need to set environment variable BENCHMARK_PATH"}

BM_ABBR=(e{1..4} f{01..12} c{1..5} cf{01..13})
BM_LONG=(FPGA-example{1..4} FPGA{01..12} clk_design{1..5} CLK-FPGA{01..13})
BM16_NUM=16
BM_NUM=${#BM_ABBR[@]}

if [ -z $1 ] ; then
    echo "Usage:"
    echo "$0 [mode] <benchmark>|all [option1] [option2]"
    echo "Mode:"
    echo " place        place by RippleFPGA only (default)"
    echo " route        route by VIVADO only"
    echo " place_route  place and route"
    echo " gdb          place and debug with gdb"
    echo " valgrind     place and check memory error with valgrind"
    echo " vgdb         place run valgrind with gdb debugging"
    echo ""
    echo "Available benchmarks:"
    for ((i=0; i<$BM_NUM; ++i)); do
        echo ${BM_ABBR[i]} - ${BM_LONG[i]}
    done
    exit
fi

PREFIX=""
MODE=$1
if [ $1 = "gdb" ] ; then
    shift
    PREFIX="gdb --args "
    MODE=place_$MODE
elif [ $1 = "valgrind" ] ; then
    shift
    PREFIX="valgrind "
    MODE=place_$MODE
elif [ $1 = "vgdb" ] ; then
    shift
    PREFIX="valgrind --vgdb-error=0"
    MODE=place_$MODE
elif [ $1 = "route" -o $1 = "place_route" -o $1 = "place" ] ; then
    shift
else
    MODE="place"
fi
echo "Mode: $MODE"

DATE=`date +"%m%d"`
BM=$1
shift
OPTIONS="$@"
BINARY="placer"

FOUND="false"

for ((i=0; i<$BM_NUM; ++i)); do
    if [ $BM = "all" -o $BM = ${BM_ABBR[i]} \
    -o \( $BM = "all16" -a $i -lt $BM16_NUM \) \
    -o \( $BM = "all17" -a $i -ge $BM16_NUM \) ] ; then
        if [ $i -lt $BM16_NUM ] ; then
            YEAR=16
        else
            YEAR=17
        fi
        INPUT_AUX=$BM_PATH/ispd20$YEAR/${BM_LONG[i]}/design.aux
        OUTPUT_PL=${BM_LONG[i]}.pl
        BENCHMARK=${BM_ABBR[i]}
        BM_DATED=${BENCHMARK}_${DATE}

        if [[ $MODE == *"place"* ]] ; then
            mkdir -p $BENCHMARK/
            cd $BENCHMARK/
            echo "$PREFIX ../$BINARY -aux $INPUT_AUX -out $OUTPUT_PL $OPTIONS"
                $PREFIX ../$BINARY -aux $INPUT_AUX -out $OUTPUT_PL $OPTIONS | tee $BM_DATED.log
            cd ..
        fi
        if [[ $MODE == *"route"* ]] ; then
            cd $BENCHMARK
            ../eval.sh $BENCHMARK
            cd ..
        fi
        
        FOUND="yes"
    fi
done

if [ $FOUND != "yes" ] ; then
    echo "ERROR: no benchmark with name: $BM"
fi
