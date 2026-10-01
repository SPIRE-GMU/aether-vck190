#!/usr/bin/env python3
"""外积黄金模型: 从 data/b.txt 复算 AIE graph 的期望输出.

流格式 (14336 x int32):
  [acc_a(1024), acc_b(1024), bk1A, bk1B, bk2A, bk2B, ..., bk6A, bk6B]
分解: digit_i = ((x + OFFSET) >> shift) & 1023 - 512
  lane1..3: acc_a @ shift 22/12/2;  lane4..6: acc_b @ shift 22/12/2
每 lane: cA_i = d_i (*) bk_iA, cB_i = d_i (*) bk_iB   (负循环卷积, mod 2^32)
输出: [sum(cA), sum(cB)]  (与 data/output_test0_acc.txt 对比)
"""
import sys
import numpy as np

OFFSET = np.uint32(2149582848)          # 2^31+2^21+2^11: Bg=1024, l=3 的 decompH 偏置
N = 1024

def decomp(poly_u32, shift):
    buf = ((poly_u32 + OFFSET) >> np.uint32(shift)) & np.uint32(1023)
    return buf.astype(np.int64) - 512

def negacyclic(p, a):
    """c = p (*) a mod (X^N+1), 系数 mod 2^32 (int64 中间量足够: |p|<=512, 1024 taps)"""
    A = np.concatenate([a.astype(np.int64), -a.astype(np.int64)])  # X^N = -1
    c = np.zeros(N, dtype=np.int64)
    for k in range(N):
        # c_k = sum_t p_t * A[(k - t) mod 2N]
        idx = (k - np.arange(N)) % (2 * N)
        c[k] = np.dot(p, A[idx])
    return c.astype(np.uint32).astype(np.int32)

def main(b_path, out_path=None):
    w = np.loadtxt(b_path, dtype=np.int64).astype(np.uint32)
    assert w.size == 14 * N, f"expect 14336 words, got {w.size}"
    acc = [w[0:N], w[N:2*N]]                      # acc_a, acc_b
    bk = w[2*N:].reshape(6, 2, N)                  # [lane][A/B][coeff]
    shifts = [22, 12, 2]
    sumA = np.zeros(N, dtype=np.int64); sumB = np.zeros(N, dtype=np.int64)
    for lane in range(6):
        d = decomp(acc[lane // 3], shifts[lane % 3])
        sumA += negacyclic(d, bk[lane][0].astype(np.int32))
        sumB += negacyclic(d, bk[lane][1].astype(np.int32))
    out = np.concatenate([sumA, sumB]).astype(np.uint32).astype(np.int32)
    if out_path is None:
        np.savetxt(sys.stdout, out, fmt="%d"); return 0
    rows = [l for l in open(out_path) if not l.lstrip().startswith("T")]  # aiesim 时间戳行
    got = np.loadtxt(rows, dtype=np.int64).ravel().astype(np.uint32).astype(np.int32)  # 128-bit PLIO: 4 词/行
    if got.size != out.size:
        print(f"长度不符: sim {got.size} vs golden {out.size}"); return 1
    diff = np.nonzero(got != out)[0]
    if diff.size == 0:
        print(f"PASS: 2048 个系数全部一致")
        return 0
    print(f"FAIL: {diff.size}/2048 不一致, 首个 idx={diff[0]}: sim={got[diff[0]]} golden={out[diff[0]]}")
    return 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None))
