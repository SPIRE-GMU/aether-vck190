# Optimized Kernels + Karatsuba — TFHE External Product on Versal AIE

AIE implementation of the full TFHE external product (TRLWE × TGSW, N=1024,
k=1, l=3, Bg=2^10) with the multiplier chains replaced by a **negacyclic
Karatsuba** structure: two recursion levels map one 1024-point negacyclic
multiplication onto 9 leaves of 256×256 linear convolution (F-array method,
fold `c = F[0:N] − F[N:2N]`), all arithmetic exact in Z_2^32. Each chain is
9 leaf kernels + 1 fold kernel = **10 tiles** (vs 16 in the schoolbook
chain); 


## Requirements

- AMD Vitis **2023.2** with VCK190 base platform `xilinx_vck190_base_202320_1`
- Linux host (the Makefile auto-creates `.aie-shim-include/asm` to work
  around the CDO-generation `asm/errno.h` issue)

## Layout

```
src/        ADF graph + kernels (6 decompose, 12 x (9 leaves + fold), merge/compose)
data/       b.txt (14336-word input: TRLWE + BK), golden_expected.txt (exact-integer reference)
test/       golden_extprod.py (golden regeneration), gen_kernels.py (leaf kernel generator)
ref/        reference aiesimulator outputs with timestamps (32b@520 main; 312.5/128-bit variants)
vpp/        v++ connectivity config + run script + PDM power reports
Makefile    targets: x86 / x86run / hwfull / aiesim
```

## Build & run

```bash
make x86 && make x86run     # functional simulation (minutes)
make hwfull                 # hw compile, 143 kernels (use hwfull, not hw)
make aiesim                 # cycle-accurate simulation with profiling
```

## Verify (bit-exact)

```bash
diff <(grep -v '^T' aiesimulator_output/data/output_test0_acc.txt | tr -d ' ') \
     <(tr -d ' ' < data/golden_expected.txt) && echo PASS
```

Latency = last `T` line of `aiesimulator_output/data/output_test0_acc.txt`
(picoseconds). Expected ≈ 49,230,000 ps; see `ref/` for the recorded runs.

## Resource / power reproduction (optional)

`vpp/run_vpp.sh` performs the `v++ -l` hardware link (two AXI data movers
against the VCK190 base platform); `report_utilization` on the routed
design gives the PL numbers above and `report_power -xpe` feeds AMD Power
Design Manager (reports under `vpp/pdm/`, vector-load sweep 15/20/30%).
