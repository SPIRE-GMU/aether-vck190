// kleaf7: Karatsuba 叶 (i=s, j=0); 生成于 test/gen_kernels.py, 勿手改
// F 贡献: [(1, 512), (-1, 768)]
#include "../kernels.h"
#include "aie_api/aie.hpp"
#include <adf.h>

void kleaf7(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2){
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
    {
        v = readincr(sin1); dpack[i] = v;
        writeincr(sout1, v);
    }
    for(int i = 0; i < 1024; i++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    {
        v = readincr(sin1); a[i] = v;
        writeincr(sout1, v);
    }

    // pre-add -> popbuf (int16, 和<=2048 无溢出) / AR (int32 回绕安全, 反转+pad)
    for(int t = 0; t < 256; t += 16)
    chess_prepare_for_pipelining
    { aie::store_v(&popbuf[t], aie::add(aie::load_v<16>(d16 + 0 + t), aie::load_v<16>(d16 + 512 + t))); }
    for(int t = 256; t < 288; t += 16)
    chess_prepare_for_pipelining
    { aie::store_v(&popbuf[t], aie::zeros<int16,16>()); }
    for(int t = 0; t < 256; t += 8)
    chess_prepare_for_pipelining
    { aie::store_v(&AR[504 - t], aie::reverse(aie::add(aie::load_v<8>(&a[0] + t), aie::load_v<8>(&a[512] + t)))); }
    for(int x = 0; x < 256; x += 8)
    chess_prepare_for_pipelining
    { aie::store_v(&AR[x], aie::zeros<int32,8>()); }
    for(int x = 512; x < 800; x += 8)
    chess_prepare_for_pipelining
    { aie::store_v(&AR[x], aie::zeros<int32,8>()); }

    // 256x256 线性卷积 (mb_leaf256 v3 结构)
    for(int m2 = 0; m2 < 32; m2++)
    {
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
        {
            const int t = 32*c2;
            aie::vector<int16,16> cb0 = aie::load_v<16>(dw + t);
            aie::vector<int16,16> cb1 = aie::load_v<16>(dw + t + 16);
            accB = aie::sliding_mac<8,2>(accB, cb0,  0, dbuf,  0);
            accA = aie::sliding_mac<8,2>(accA, cb0,  0, dbuf,  8);
            accB = aie::sliding_mac<8,2>(accB, cb0,  2, dbuf,  2);
            accA = aie::sliding_mac<8,2>(accA, cb0,  2, dbuf, 10);
            accB = aie::sliding_mac<8,2>(accB, cb0,  4, dbuf,  4);
            accA = aie::sliding_mac<8,2>(accA, cb0,  4, dbuf, 12);
            accB = aie::sliding_mac<8,2>(accB, cb0,  6, dbuf,  6);
            accA = aie::sliding_mac<8,2>(accA, cb0,  6, dbuf, 14);
            dbuf.insert(0, aie::load_v<8>(pw + t + 32));
            accB = aie::sliding_mac<8,2>(accB, cb0,  8, dbuf,  8);
            accA = aie::sliding_mac<8,2>(accA, cb0,  8, dbuf, 16);
            accB = aie::sliding_mac<8,2>(accB, cb0, 10, dbuf, 10);
            accA = aie::sliding_mac<8,2>(accA, cb0, 10, dbuf, 18);
            accB = aie::sliding_mac<8,2>(accB, cb0, 12, dbuf, 12);
            accA = aie::sliding_mac<8,2>(accA, cb0, 12, dbuf, 20);
            accB = aie::sliding_mac<8,2>(accB, cb0, 14, dbuf, 14);
            accA = aie::sliding_mac<8,2>(accA, cb0, 14, dbuf, 22);
            dbuf.insert(1, aie::load_v<8>(pw + t + 40));
            accB = aie::sliding_mac<8,2>(accB, cb1,  0, dbuf, 16);
            accA = aie::sliding_mac<8,2>(accA, cb1,  0, dbuf, 24);
            accB = aie::sliding_mac<8,2>(accB, cb1,  2, dbuf, 18);
            accA = aie::sliding_mac<8,2>(accA, cb1,  2, dbuf, 26);
            accB = aie::sliding_mac<8,2>(accB, cb1,  4, dbuf, 20);
            accA = aie::sliding_mac<8,2>(accA, cb1,  4, dbuf, 28);
            accB = aie::sliding_mac<8,2>(accB, cb1,  6, dbuf, 22);
            accA = aie::sliding_mac<8,2>(accA, cb1,  6, dbuf, 30);
            dbuf.insert(2, aie::load_v<8>(pw + t + 48));
            accB = aie::sliding_mac<8,2>(accB, cb1,  8, dbuf, 24);
            accA = aie::sliding_mac<8,2>(accA, cb1,  8, dbuf,  0);
            accB = aie::sliding_mac<8,2>(accB, cb1, 10, dbuf, 26);
            accA = aie::sliding_mac<8,2>(accA, cb1, 10, dbuf,  2);
            accB = aie::sliding_mac<8,2>(accB, cb1, 12, dbuf, 28);
            accA = aie::sliding_mac<8,2>(accA, cb1, 12, dbuf,  4);
            accB = aie::sliding_mac<8,2>(accB, cb1, 14, dbuf, 30);
            accA = aie::sliding_mac<8,2>(accA, cb1, 14, dbuf,  6);
            dbuf.insert(3, aie::load_v<8>(pw + t + 56));
        }
        aie::store_v(&resbuf[16*m2],     aie::reverse(accA.template to_vector<int32>(0)));
        aie::store_v(&resbuf[16*m2 + 8], aie::reverse(accB.template to_vector<int32>(0)));
    }

    // C = 本叶按偏移/符号模式展开的贡献
    for(int x = 0; x < 2048; x += 8)
    chess_prepare_for_pipelining
    { aie::store_v(&C[x], aie::zeros<int32,8>()); }
    for(int x = 0; x < 512; x += 8)
    chess_prepare_for_pipelining
    { aie::store_v(&C[512 + x], aie::add(aie::load_v<8>(&C[512 + x]), aie::load_v<8>(&resbuf[x]))); }
    for(int x = 0; x < 512; x += 8)
    chess_prepare_for_pipelining
    { aie::store_v(&C[768 + x], aie::sub(aie::load_v<8>(&C[768 + x]), aie::load_v<8>(&resbuf[x]))); }

    // F 下传 (int32 回绕相加)
    for(int x = 0; x < 2048; x++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    { writeincr(sout2, readincr(sin2) + C[x]); }
}
