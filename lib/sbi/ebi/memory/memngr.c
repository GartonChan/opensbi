#include "memory/page_table.h"
#include <enclave/eid.h>
#include <sbi/sbi_system.h>
#include <sbi/ebi/partition_pool.h>
#include <sbi/ebi/enclave.h>
#include <sbi/ebi/ebi_debug.h>
#include <sbi/ebi/pmp.h>

int ebi_mem_alloc_handler(struct sbi_trap_regs *regs, int shared)
{
	// debug
	sbi_debug("[%s]\n", __func__);
	show(regs->a0);
	show(regs->a1);
	show(regs->a2);
	// show(regs->a3);
	// show(regs->a4);

	usize number_of_partitions = regs->a1;

	u64 current_eid = get_current_eid();
	sbi_debug("Enclave %lu allocates %lu partitions\n",
		current_eid, number_of_partitions);

	if (current_eid == HOST_EID)
		sbi_panic("Error: host memory allocation currently not supported\n");

	regs->a1 = alloc_partitions_for_enclave(
		current_eid,
		number_of_partitions,
		&regs->a0,
        0,
		shared
	);
	activate_lpmp(current_eid);

	return 0;
}

// int ebi_addr_record_handler(struct sbi_trap_regs *regs)
// {
// 	addr_record(regs->a0, regs->a1);
// 	return 0;
// }