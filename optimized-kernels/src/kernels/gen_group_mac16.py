#!/usr/bin/env python3
"""从 group2.cc (人工模板, 已 review) 派生 group3..15。
仅两处替换: 函数名 groupN; 块基址 8*(BLOCK0+j) 与转发计数 64*(N-1)。
group1/group16 是特殊形态, 不由本脚本生成。"""
import re, pathlib
here = pathlib.Path(__file__).parent
tpl = (here/"group2.cc").read_text()
for n in range(3, 16):
    s = tpl.replace("void group2(", f"void group{n}(")
    s = s.replace("const int32* ptr = A + 1016 - 8*(8 + j);",
                  f"const int32* ptr = A + 1016 - 8*({8*(n-1)} + j);")
    s = s.replace("for(int k = 0; k < 64; k++)              // group2: 转发上游 64*(g-1)=64 个",
                  f"for(int k = 0; k < {64*(n-1)}; k++)              // group{n}: 转发上游 64*(g-1) 个")
    # 模板里注释掉的旧代码含 (8+..) 常量, 不参与编译, 原样保留无妨
    (here/f"group{n}.cc").write_text(s)
    print(f"group{n}.cc: BLOCK0={8*(n-1)}, fwd={64*(n-1)}")
