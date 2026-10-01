#!/bin/bash
# LUT/FF 资源实验: full (143-kernel Karatsuba 外积) 的板级链接 -> report_utilization
set -e
. /tools/Xilinx/Vitis/2023.2/settings64.sh
XPFM=/tools/Xilinx/Vitis/2023.2/base_platforms/xilinx_vck190_base_202320_1/xilinx_vck190_base_202320_1.xpfm
cd "$(dirname "$0")"
echo "== [1/3] v++ -c mm2s ==" && v++ -c -t hw --platform $XPFM -k mm2s mm2s.cpp -o mm2s.xo
echo "== [2/3] v++ -c s2mm ==" && v++ -c -t hw --platform $XPFM -k s2mm s2mm.cpp -o s2mm.xo
echo "== [3/3] v++ -l (Vivado synth+impl, ~2-3h) ==" && \
v++ -l -t hw --platform $XPFM ../libadf.a mm2s.xo s2mm.xo --config system.cfg --save-temps -o full_system.xsa
echo "== reports ==" && find _x -name '*utilization*.rpt' | head
