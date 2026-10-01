// Karatsuba 全系统: 12 链 x (9 叶 + fold) + decompose/split/merge/compose = 143 kernel
// 接口与 dropin/clean_system 完全一致: in0 = b.txt (14336 词), out0 = 2048 词
#include <adf.h>
#include "kernels.h"
using namespace adf;

class karaFullGraph : public adf::graph {
private:
    kernel decompose[6], ksplit[6], kmerge[6], compose[5];
    kernel kl[12][9], kfold[12];
public:
    input_plio in0;
    output_plio out0;
    karaFullGraph(){
        in0  = input_plio::create("Datain",  plio_32_bits, "data/b.txt", 520);
        out0 = output_plio::create("Dataout", plio_32_bits, "data/output_test0_acc.txt");

        decompose[0] = kernel::create(acc_decompose1);
        decompose[1] = kernel::create(acc_decompose2);
        decompose[2] = kernel::create(acc_decompose3);
        decompose[3] = kernel::create(acc_decompose4);
        decompose[4] = kernel::create(acc_decompose5);
        decompose[5] = kernel::create(acc_decompose6);
        source(decompose[0]) = "src/kernels/acc_decompose1.cc";
        source(decompose[1]) = "src/kernels/acc_decompose2.cc";
        source(decompose[2]) = "src/kernels/acc_decompose3.cc";
        source(decompose[3]) = "src/kernels/acc_decompose4.cc";
        source(decompose[4]) = "src/kernels/acc_decompose5.cc";
        source(decompose[5]) = "src/kernels/acc_decompose6.cc";
        for(int i = 0; i < 6; i++){
            ksplit[i] = kernel::create(split);  source(ksplit[i]) = "src/kernels/split.cc";
            kmerge[i] = kernel::create(merge);  source(kmerge[i]) = "src/kernels/merge.cc";
            runtime<ratio>(decompose[i]) = 1; runtime<ratio>(ksplit[i]) = 1; runtime<ratio>(kmerge[i]) = 1;
        }
        for(int i = 0; i < 5; i++){
            compose[i] = kernel::create(acc_compose);
            source(compose[i]) = "src/kernels/acc_compose.cc";
            runtime<ratio>(compose[i]) = 1;
        }
        for(int c = 0; c < 12; c++){
            kl[c][0] = kernel::create(kleaf1); kl[c][1] = kernel::create(kleaf2);
            kl[c][2] = kernel::create(kleaf3); kl[c][3] = kernel::create(kleaf4);
            kl[c][4] = kernel::create(kleaf5); kl[c][5] = kernel::create(kleaf6);
            kl[c][6] = kernel::create(kleaf7); kl[c][7] = kernel::create(kleaf8);
            kl[c][8] = kernel::create(kleaf9); kfold[c] = kernel::create(kara_fold);
            for(int n = 0; n < 9; n++){
                char buf[40]; snprintf(buf, 40, "src/kernels/kleaf%d.cc", n+1);
                source(kl[c][n]) = buf;  runtime<ratio>(kl[c][n]) = 1;
            }
            source(kfold[c]) = "src/kernels/kara_fold.cc";  runtime<ratio>(kfold[c]) = 1;
        }

        // decompose 级联: 右传 split, 下传下一级
        connect<stream>(in0.out[0], decompose[0].in[0]);
        for(int i = 0; i < 6; i++) connect<stream>(decompose[i].out[0], ksplit[i].in[0]);
        for(int i = 0; i < 5; i++) connect<stream>(decompose[i].out[1], decompose[i+1].in[0]);

        // split -> 链首; 链内: 操作数 + F 两条流; 链尾 fold
        for(int i = 0; i < 6; i++){
            connect<stream>(ksplit[i].out[0], kl[2*i][0].in[0]);
            connect<stream>(ksplit[i].out[1], kl[2*i+1][0].in[0]);
        }
        for(int c = 0; c < 12; c++){
            for(int n = 0; n < 8; n++){
                connect<stream>(kl[c][n].out[0], kl[c][n+1].in[0]);   // 操作数
                connect<stream>(kl[c][n].out[1], kl[c][n+1].in[1]);   // F 部分和
            }
            connect<stream>(kl[c][8].out[0], kfold[c].in[0]);         // leaf9 唯一出口 = F
        }

        // fold -> merge -> compose 链 -> out
        for(int i = 0; i < 6; i++){
            connect<stream>(kfold[2*i].out[0],   kmerge[i].in[0]);
            connect<stream>(kfold[2*i+1].out[0], kmerge[i].in[1]);
        }
        connect<stream>(kmerge[0].out[0], compose[0].in[0]);
        connect<stream>(kmerge[1].out[0], compose[0].in[1]);
        for(int i = 1; i < 5; i++){
            connect<stream>(compose[i-1].out[0], compose[i].in[0]);
            connect<stream>(kmerge[i+1].out[0],  compose[i].in[1]);
        }
        connect<stream>(compose[4].out[0], out0.in[0]);
    }
};
