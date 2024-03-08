#ifndef __SBI_EBI_H__
#define __SBI_EBI_H__

#include <sbi/sbi_types.h>
#include <sbi/ebi/enclave.h>

int sbi_ebi_handler(struct sbi_trap_regs *regs);

#endif