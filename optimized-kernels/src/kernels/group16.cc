
#include "../kernels.h"
#include "adf/intrinsics.h"
#include "adf/stream/types.h"
#include "aie_api/aie.hpp"
#include <stdint.h>
#include <adf.h>

void group16( input_stream_int32* sin1,input_stream_int32* sin2, output_stream_int32* sout1){

#ifdef __X86SIM__
    // x86sim 单进程: 多实例共享函数级 static 会竞态 -> 放栈
    alignas(32) int32 dpack[512];
    alignas(32) int32 A[2080];
    alignas(32) int32 res64[64];
#else
    // hw: 每实例独占 tile, static = tile 私有数据内存, 不占栈配额
    alignas(32) static int32 dpack[512];
    alignas(32) static int32 A[2080];
    alignas(32) static int32 res64[64];
#endif
    const int16* d = (const int16*)dpack;
    int v;

    // 链尾: 只读不转发
    for(int i = 0; i < 512; i++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    { dpack[i] = readincr(sin1); }

    for(int i = 0; i < 1024; i++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    { A[1023-i] = readincr(sin2); }

    for(int i = 0; i < 1024; i += 8)
    chess_prepare_for_pipelining
    { aie::store_v(&A[1024+i], aie::neg(aie::load_v<8>(&A[i]))); }

    for(int j = 0; j < 8; j++)
    {
        const int32* ptr = A + 1016 - 8*(120 + j);
        aie::accum<acc80,8> acc_e, acc_o;      // 双独立累加链: 破除串行 mac 依赖
        acc_e.from_vector(aie::zeros<int32,8>(), 0);
        acc_o.from_vector(aie::zeros<int32,8>(), 0);

        aie::vector<int32,32> dbuf;
        dbuf.insert(0, aie::load_v<8>(ptr));
        dbuf.insert(1, aie::load_v<8>(ptr + 8));
        dbuf.insert(2, aie::load_v<8>(ptr + 16));

        for(int c2 = 0; c2 < 32; c2++)      // 每迭代 32 taps
        chess_prepare_for_pipelining
        chess_loop_range(32,)
        {
            const int t = 32*c2;
            aie::vector<int16,16> cb0 = aie::load_v<16>(d + t);
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0,  0, dbuf,  0);
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0,  2, dbuf,  2);
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0,  4, dbuf,  4);
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0,  6, dbuf,  6);
            dbuf.insert(3, aie::load_v<8>(ptr + t + 24));
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0,  8, dbuf,  8);
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0, 10, dbuf, 10);
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0, 12, dbuf, 12);
            acc_e = aie::sliding_mac<8,2>(acc_e, cb0, 14, dbuf, 14);
            dbuf.insert(0, aie::load_v<8>(ptr + t + 32));
            aie::vector<int16,16> cb1 = aie::load_v<16>(d + t + 16);
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1,  0, dbuf, 16);
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1,  2, dbuf, 18);
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1,  4, dbuf, 20);
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1,  6, dbuf, 22);
            dbuf.insert(1, aie::load_v<8>(ptr + t + 40));
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1,  8, dbuf, 24);
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1, 10, dbuf, 26);
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1, 12, dbuf, 28);
            acc_o = aie::sliding_mac<8,2>(acc_o, cb1, 14, dbuf, 30);
            dbuf.insert(2, aie::load_v<8>(ptr + t + 48));
        }

        aie::store_v(&res64[8*j], aie::reverse(aie::add(acc_e.template to_vector<int32>(0),
                                                acc_o.template to_vector<int32>(0))));
    }

    for(int k = 0; k < 960; k++)              // 转发上游 64*15 个结果
    chess_prepare_for_pipelining
    chess_loop_range(64,)
    { writeincr(sout1, readincr(sin1)); }

    for(int k = 0; k < 64; k++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    { writeincr(sout1, res64[k]); }
}
