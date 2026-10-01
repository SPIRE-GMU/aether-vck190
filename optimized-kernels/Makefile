# AIE component: external product (TLWE x TGSW) graph
# 用法（无需预先 source Vitis 环境，recipe 内自带）:
#   make x86        # aiecompiler -target=x86sim  (功能仿真, 快)
#   make x86run     # 运行 x86simulator
#   make hwfull     # -target=hw + CDO 补丁 (chess 编译 215 个 kernel, 很慢; 用它别用 make hw)
#   make aiesim     # 运行 aiesimulator (SystemC, 周期近似)
#   make clean

VITIS      := /tools/Xilinx/Vitis/2023.2
PLATFORM   := $(VITIS)/base_platforms/xilinx_vck190_base_202320_1/xilinx_vck190_base_202320_1.xpfm
ENV        := . $(VITIS)/settings64.sh >/dev/null 2>&1 &&

GRAPH      := src/project.cpp
WORK_X86   := Work_x86
WORK_HW    := Work

.PHONY: x86 x86run hw hwfull aiesim clean

x86:
	set -o pipefail; $(ENV) aiecompiler -target=x86sim --platform=$(PLATFORM) \
	  -include=src --Xpreproc=-I$(CURDIR)/.aie-shim-include -workdir=$(WORK_X86) $(GRAPH) 2>&1 | tail -20

x86run:
	$(ENV) x86simulator --pkg-dir=$(WORK_X86)

hw:
	set -o pipefail; $(ENV) aiecompiler -target=hw --stacksize=4096 --platform=$(PLATFORM) \
	  -include=src --Xpreproc=-I$(CURDIR)/.aie-shim-include -workdir=$(WORK_HW) $(GRAPH) 2>&1 | tail -20

# make hw 会在 CDO 阶段失败 (aiecompiler 清洗子进程 CPATH, 见 AGENTS.md 已知坑);
# hwfull = hw(容忍该失败) + 带 CPATH 手动补 CDO, 之后即可 make aiesim
hwfull:
	-$(MAKE) hw
	$(ENV) export CPATH=$(CURDIR)/.aie-shim-include && $(MAKE) -C $(WORK_HW)/ps/cdo -f Makefile all

aiesim:
	$(ENV) aiesimulator --pkg-dir=$(WORK_HW) --profile

clean:
	rm -rf $(WORK_X86) $(WORK_HW) x86simulator_output aiesimulator_output \
	  *.log *.jou .Xil Map_Report.csv sol.db pl_sample_counts *.aiecompile_summary

# shim 目录被 gitignore，clone 后首次 make 自动重建
$(shell mkdir -p .aie-shim-include; ln -sfn /usr/include/x86_64-linux-gnu/asm .aie-shim-include/asm)
