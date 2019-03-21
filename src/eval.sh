#!/bin/bash
BM_PATH=${BENCHMARK_PATH?"Need to set environment variable BENCHMARK_PATH"}

BM_ABBR=(e{1..4} f{01..12} c{1..5} cf{01..13})
BM_LONG=(FPGA-example{1..4} FPGA{01..12} clk_design{1..5} CLK-FPGA{01..13})
BM16_NUM=16
BM_NUM=${#BM_ABBR[@]}

DATE=`date +"%m%d"`
BM=$1

DONE="false"
for ((i=0; i<$BM_NUM; ++i)); do
    if [ $BM = ${BM_ABBR[i]} ] ; then
        if [ $i -lt $BM16_NUM ] ; then
            YEAR=16
            VIVADO_VERSION=2015.4
        else
            YEAR=17
            VIVADO_VERSION=2016.4
        fi
        INPUT_DCP=${BM_PATH}/ispd20$YEAR/${BM_LONG[i]}/design.dcp
        INPUT_PL=${BM_LONG[i]}.pl
        DONE="yes"
        break
    fi
done
if [ $DONE != "yes" ] ; then
    echo "ERROR: no benchmark with name: $BM"
fi

rm -f tmp.tcl

echo "open_checkpoint $INPUT_DCP" >> tmp.tcl
echo "place_design -placement $INPUT_PL" >> tmp.tcl
# echo "report_utilization -file $BM_$DATE.util" >> tmp.tcl
echo "route_design" >> tmp.tcl
echo "report_route_status" >> tmp.tcl
# echo "write_checkpoint -force $BM_$DATE.dcp" >> tmp.tcl

echo "/opt/Xilinx/Vivado/${VIVADO_VERSION}/bin/vivado -mode batch -source tmp.tcl -log vivado_$DATE.log"
      /opt/Xilinx/Vivado/${VIVADO_VERSION}/bin/vivado -mode batch -source tmp.tcl -log vivado_$DATE.log

rm -f tmp.tcl
