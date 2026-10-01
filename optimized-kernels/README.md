# Optimized Kernels — TFHE External Product on Versal AIE

AIE implementation of the full TFHE external product (TRLWE × TGSW, N=1024,
k=1, l=3, Bg=2^10) using THENA's dataflow topology with **optimized MAC
kernels**: digit packing, 16-bit SIMD multiply-accumulate
(`aie::sliding_mac`, 16 MACs/instr), and systolic double buffering.
215 kernels, sequential-read protocol 

## Requirements

- AMD Vitis **2023.2** with VCK190 base platform `xilinx_vck190_base_202320_1`
- Linux host (the Makefile auto-creates `.aie-shim-include/asm` to work
  around the CDO-generation `asm/errno.h` issue)

## Layout

```
src/        ADF graph (project.h/.cpp) + kernels (6 decompose, 16x12 MAC chain, merge/compose)
data/       b.txt (14336-word input: TRLWE + BK), golden_expected.txt (exact-integer reference)
test/       golden_extprod.py — regenerates the golden output from b.txt
ref/        reference aiesimulator outputs with timestamps (32b@520 main; 312.5/128-bit variants)
vpp/        v++ connectivity config + PDM power reports (resource/power reproduction)
Makefile    targets: x86 / x86run / hwfull / aiesim
```

## Build & run

```bash
make x86 && make x86run     # functional simulation (minutes)
make hwfull                 # hw compile, 215 kernels (use hwfull, not hw)
make aiesim                 # cycle-accurate simulation with profiling
```

## Verify (bit-exact)

```bash
diff <(grep -v '^T' aiesimulator_output/data/output_test0_acc.txt | tr -d ' ') \
     <(tr -d ' ' < data/golden_expected.txt) && echo PASS
```

Latency = last `T` line of `aiesimulator_output/data/output_test0_acc.txt`
(picoseconds). Expected ≈ 47,330,000 ps; see `ref/` for the recorded runs.

## Resource / power reproduction (optional)

`v++ -l` with `vpp/system.cfg` (two AXI data movers, mm2s/s2mm from the AMD
examples) against the VCK190 base platform produces the routed Vivado
project; `report_utilization` gives the PL numbers above and
`report_power -xpe` feeds AMD Power Design Manager (reports under
`vpp/pdm/`, vector-load sweep 15/20/30%).
