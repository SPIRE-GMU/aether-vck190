// kara_fold: F(2048) -> c = F[0:1024] - F[1024:2048] (mod 2^32, X^1024=-1 折叠)
#include "../kernels.h"
#include <adf.h>

void kara_fold(input_stream_int32* sin1, output_stream_int32* sout1){
#ifdef __X86SIM__
    int32 buf[1024];        // 12 实例, static 会竞态
#else
    static int32 buf[1024]; // hw: tile 私有
#endif
    for(int i = 0; i < 1024; i++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    { buf[i] = readincr(sin1); }
    for(int i = 0; i < 1024; i++)
    chess_prepare_for_pipelining
    chess_loop_range(16,)
    { writeincr(sout1, buf[i] - readincr(sin1)); }
}
