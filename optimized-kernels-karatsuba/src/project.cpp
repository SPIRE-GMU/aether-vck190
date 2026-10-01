#include "project.h"
karaFullGraph kfg;
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(){ kfg.init(); kfg.run(1); kfg.wait(); kfg.end(); return 0; }
#endif
