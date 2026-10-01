#!/usr/bin/env python3
"""生成 9 个 Karatsuba 叶 kernel (kleaf1..9) + kernels.h。
叶 n -> (i,j), i,j ∈ {0,1,s}: p/a 操作数 = 四分之一段组合; F 贡献 = 偏移/符号模式。
计算体 = microbench mb_leaf256 v3 (16 宽块, 已验证 II~1)。"""
import itertools, pathlib

root = pathlib.Path(__file__).resolve().parents[1]

# (i,j) 枚举与模式
L2 = {0: [(1,0),(-1,256)], 1: [(1,512),(-1,256)], 's': [(1,256)]}      # 层2: U'/V'/W'
L1 = {0: [(1,0),(-1,512)], 1: [(1,1024),(-1,512)], 's': [(1,512)]}     # 层1: U/V/W
QP = {0:{0:[0],1:[1],'s':[0,1]}, 1:{0:[2],1:[3],'s':[2,3]},
      's':{0:[0,2],1:[1,3],'s':[0,1,2,3]}}                              # (i,j)->四分段集合

def mac(acc, cb, cs, ds): return f"            acc{acc} = aie::sliding_mac<8,2>(acc{acc}, {cb}, {cs:2d}, dbuf, {ds:2d});"
def ins(slot, off):       return f"            dbuf.insert({slot}, aie::load_v<8>(pw + t + {off}));"
body = []
for p in range(4): body += [mac('B','cb0',2*p,2*p), mac('A','cb0',2*p,2*p+8)]
body.append(ins(0,32))
for p in range(4,8): body += [mac('B','cb0',2*p,2*p), mac('A','cb0',2*p,2*p+8)]
body.append(ins(1,40))
for p in range(4): body += [mac('B','cb1',2*p,16+2*p), mac('A','cb1',2*p,(24+2*p)%32)]
body.append(ins(2,48))
for p in range(4,8): body += [mac('B','cb1',2*p,16+2*p), mac('A','cb1',2*p,(24+2*p)%32)]
body.append(ins(3,56))
BODY = "\n".join(body)

hdr = ["#include <adf.h>", "#ifndef KARA_UNIT_KERNELS_H", "#define KARA_UNIT_KERNELS_H"]
combos = list(itertools.product([0,1,'s'], repeat=2))
for n,(i,j) in enumerate(combos, 1):
    first, last = (n==1), (n==9)
    qs_p = QP[i][j]                    # digit 四分段下标集合
    terms = [(s1*s2, o1+o2) for (s2,o2) in L1[i] for (s1,o1) in L2[j]]

    sig_in  = "input_stream_int32* sin1" + ("" if first else ", input_stream_int32* sin2")
    sig_out = ("output_stream_int32* sout1, output_stream_int32* sout2" if not last
               else "output_stream_int32* sout1")
    fout = "sout1" if last else "sout2"
    hdr.append(f"void kleaf{n}({sig_in}, {sig_out});")

    fwd1 = "" if last else "        writeincr(sout1, v);\n"
    # pre-add 生成: popbuf int16 (含 pad), AR 直接反转构建
    padd = " + ".join(f"aie::load_v<16>(d16 + {256*q} + t)" if k==0 else f"aie::load_v<16>(d16 + {256*q} + t)"
                      for k,q in enumerate(qs_p))
    if len(qs_p) == 1: pexpr = f"aie::load_v<16>(d16 + {256*qs_p[0]} + t)"
    elif len(qs_p) == 2: pexpr = f"aie::add(aie::load_v<16>(d16 + {256*qs_p[0]} + t), aie::load_v<16>(d16 + {256*qs_p[1]} + t))"
    else: pexpr = ("aie::add(aie::add(aie::load_v<16>(d16 + 0 + t), aie::load_v<16>(d16 + 256 + t)), "
                   "aie::add(aie::load_v<16>(d16 + 512 + t), aie::load_v<16>(d16 + 768 + t)))")
    qs_a = qs_p
    if len(qs_a) == 1: aexpr = f"aie::load_v<8>(&a[{256*qs_a[0]}] + t)"
    elif len(qs_a) == 2: aexpr = f"aie::add(aie::load_v<8>(&a[{256*qs_a[0]}] + t), aie::load_v<8>(&a[{256*qs_a[1]}] + t))"
    else: aexpr = ("aie::add(aie::add(aie::load_v<8>(&a[0] + t), aie::load_v<8>(&a[256] + t)), "
                   "aie::add(aie::load_v<8>(&a[512] + t), aie::load_v<8>(&a[768] + t)))")

    cacc = "\n".join(
        f"""    for(int x = 0; x < 512; x += 8)
    chess_prepare_for_pipelining
    {{ aie::store_v(&C[{off} + x], aie::{'add' if s>0 else 'sub'}(aie::load_v<8>(&C[{off} + x]), aie::load_v<8>(&resbuf[x]))); }}"""
        for (s, off) in terms)

    fbody = (f"""    for(int x = 0; x < 2048; x++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    {{ writeincr({fout}, C[x]); }}""" if first else
    f"""    for(int x = 0; x < 2048; x++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    {{ writeincr({fout}, readincr(sin2) + C[x]); }}""")

    src = f'''// kleaf{n}: Karatsuba 叶 (i={i}, j={j}); 生成于 test/gen_kernels.py, 勿手改
// F 贡献: {terms}
#include "../kernels.h"
#include "aie_api/aie.hpp"
#include <adf.h>

void kleaf{n}({sig_in}, {sig_out}){{
#ifdef __X86SIM__
    // x86sim 单进程: 12 实例共享函数级 static 会竞态 -> 放栈 (见 AGENTS.md)
    alignas(32) int32 dpack[512];
    alignas(32) int32 a[1024];
    alignas(32) int16 popbuf[288];
    alignas(32) int32 AR[800];
    alignas(32) int32 resbuf[512];
    alignas(32) int32 C[2048];
#else
    // hw: 每实例独占 tile, static = tile 私有内存, 不占栈配额
    alignas(32) static int32 dpack[512];
    alignas(32) static int32 a[1024];
    alignas(32) static int16 popbuf[288];
    alignas(32) static int32 AR[800];
    alignas(32) static int32 resbuf[512];
    alignas(32) static int32 C[2048];
#endif
    const int16* d16 = (const int16*)dpack;

    int v;
    for(int i = 0; i < 512; i++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    {{
        v = readincr(sin1); dpack[i] = v;
{fwd1}    }}
    for(int i = 0; i < 1024; i++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    {{
        v = readincr(sin1); a[i] = v;
{fwd1}    }}

    // pre-add -> popbuf (int16, 和<=2048 无溢出) / AR (int32 回绕安全, 反转+pad)
    for(int t = 0; t < 256; t += 16)
    chess_prepare_for_pipelining
    {{ aie::store_v(&popbuf[t], {pexpr}); }}
    for(int t = 256; t < 288; t += 16)
    chess_prepare_for_pipelining
    {{ aie::store_v(&popbuf[t], aie::zeros<int16,16>()); }}
    for(int t = 0; t < 256; t += 8)
    chess_prepare_for_pipelining
    {{ aie::store_v(&AR[504 - t], aie::reverse({aexpr})); }}
    for(int x = 0; x < 256; x += 8)
    chess_prepare_for_pipelining
    {{ aie::store_v(&AR[x], aie::zeros<int32,8>()); }}
    for(int x = 512; x < 800; x += 8)
    chess_prepare_for_pipelining
    {{ aie::store_v(&AR[x], aie::zeros<int32,8>()); }}

    // 256x256 线性卷积 (mb_leaf256 v3 结构)
    for(int m2 = 0; m2 < 32; m2++)
    {{
        int tl = 16*m2 - 255; if (tl < 0) tl = 0; tl &= ~31;
        int te = 16*m2 + 16;  if (te > 256) te = 256; te = (te + 31) & ~31;
        const int trips = (te - tl) >> 5;
        const int32* pw = AR + 496 - 16*m2 + tl;
        const int16* dw = popbuf + tl;

        aie::accum<acc80,8> accA, accB;
        accA.from_vector(aie::zeros<int32,8>(), 0);
        accB.from_vector(aie::zeros<int32,8>(), 0);
        aie::vector<int32,32> dbuf;
        dbuf.insert(0, aie::load_v<8>(pw));
        dbuf.insert(1, aie::load_v<8>(pw + 8));
        dbuf.insert(2, aie::load_v<8>(pw + 16));
        dbuf.insert(3, aie::load_v<8>(pw + 24));

        for(int c2 = 0; c2 < trips; c2++)
        chess_prepare_for_pipelining
        chess_loop_range(1,8)
        {{
            const int t = 32*c2;
            aie::vector<int16,16> cb0 = aie::load_v<16>(dw + t);
            aie::vector<int16,16> cb1 = aie::load_v<16>(dw + t + 16);
{BODY}
        }}
        aie::store_v(&resbuf[16*m2],     aie::reverse(accA.template to_vector<int32>(0)));
        aie::store_v(&resbuf[16*m2 + 8], aie::reverse(accB.template to_vector<int32>(0)));
    }}

    // C = 本叶按偏移/符号模式展开的贡献
    for(int x = 0; x < 2048; x += 8)
    chess_prepare_for_pipelining
    {{ aie::store_v(&C[x], aie::zeros<int32,8>()); }}
{cacc}

    // F 下传 (int32 回绕相加)
{fbody}
}}
'''
    (root/f"src/kernels/kleaf{n}.cc").write_text(src)

hdr.append("void kara_fold(input_stream_int32* sin1, output_stream_int32* sout1);")
hdr.append("#endif")
(root/"src/kernels.h").write_text("\n".join(hdr) + "\n")
print("generated kleaf1..9 + kernels.h")
