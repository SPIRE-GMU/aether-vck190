# AETHER

**AETHER** is a TFHE external-product accelerator for the AMD Versal
VCK190, built entirely on the AI Engine (AIE) array. It provides two
implementations of the full external product
(TRLWE × TGSW, N=1024, k=1, l=3, Bg=2^10), both bit-exact against an
exact-integer reference and verified in cycle-accurate simulation:

| Folder | Design |
|---|---|
| [`optimized-kernels/`](optimized-kernels/) | THENA-style dataflow with optimized MAC kernels: digit packing, 16-bit SIMD multiply-accumulate, systolic double buffering (215 kernels) |
| [`optimized-kernels-karatsuba/`](optimized-kernels-karatsuba/) | Multiplier chains replaced by a two-level negacyclic Karatsuba structure — 9 leaves of 256-point linear convolution + fold per chain, exact in Z_2^32 (143 kernels) |

Both designs use a sequential-read stream protocol and require no deep
FIFO (`fifo_depth`) settings anywhere in the graph.

## Requirements

- AMD Vitis **2023.2** with the VCK190 base platform
  `xilinx_vck190_base_202320_1`
- Linux host

## Quick start

```bash
cd optimized-kernels            # or optimized-kernels-karatsuba
make x86 && make x86run         # functional simulation (minutes)
make hwfull && make aiesim      # hw compile + cycle-accurate simulation
```

Bit-exact verification against the included golden reference:

```bash
diff <(grep -v '^T' aiesimulator_output/data/output_test0_acc.txt | tr -d ' ') \
     <(tr -d ' ' < data/golden_expected.txt) && echo PASS
```

See each folder's README for layout details, reference outputs, and
resource/power reproduction (`vpp/`).
