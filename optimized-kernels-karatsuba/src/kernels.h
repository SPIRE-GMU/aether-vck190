#include <adf.h>
#ifndef KARA_UNIT_KERNELS_H
#define KARA_UNIT_KERNELS_H
void kleaf1(input_stream_int32* sin1, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf2(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf3(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf4(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf5(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf6(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf7(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf8(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1, output_stream_int32* sout2);
void kleaf9(input_stream_int32* sin1, input_stream_int32* sin2, output_stream_int32* sout1);
void kara_fold(input_stream_int32* sin1, output_stream_int32* sout1);
void split( input_stream_int32* sin1, output_stream_int32* sout1,output_stream_int32* sout2);
void merge( input_stream_int32* sin1, input_stream_int32* sin2,output_stream_int32* sout1);
void acc_decompose1( input_stream_int32* sin1,output_stream_int32* sout1,output_stream_int32* sout2);
void acc_decompose2( input_stream_int32* sin1,output_stream_int32* sout1,output_stream_int32* sout2);
void acc_decompose3( input_stream_int32* sin1,output_stream_int32* sout1,output_stream_int32* sout2);
void acc_decompose4( input_stream_int32* sin1,output_stream_int32* sout1,output_stream_int32* sout2);
void acc_decompose5( input_stream_int32* sin1,output_stream_int32* sout1,output_stream_int32* sout2);
void acc_decompose6( input_stream_int32* sin1,output_stream_int32* sout1);
void acc_compose( input_stream_int32* sin1, input_stream_int32* sin2,output_stream_int32* sout1);
#endif
