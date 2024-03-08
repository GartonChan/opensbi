#include "enclave/eid.h"
#include <sbi/ebi/ebi_debug.h>
#include <ebi_ecall.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_version.h>
#include <sbi/sbi_system.h>
#include <sbi/ebi/enclave.h>
#include <sbi/ebi/partition_pool.h>
#include <sbi/ebi/message_channel.h>
#include <sbi/ebi/pmp.h>
#include <sbi/ebi/ipi.h>
#include <sbi/ebi/memngr.h>
#include <sbi/ebi/region.h>
#include <sbi/ebi/eval.h>
#include <sbi/ebi/eval_config.h>
#include <sbi/riscv_barrier.h>
#include <sbi/sbi_ebi.h>

static bool ebi_called = false;

bool ebi_is_called()
{
    return ebi_called;
}

int sbi_ebi_handler(struct sbi_trap_regs *regs)
{
    unsigned long funcid = regs->a6;
    int ret = 0;
    __unused u64 eid = get_current_eid();

    if (unlikely(!ebi_is_called())) {
        ipi_send_ebi_postboot_init(-1);
        ebi_called = true;
    }

    regs->mepc += 4;

    switch (funcid) {
    case SBI_EXT_EBI_CREATE:
        sbi_debug("SBI_EXT_EBI_CREATE\n");
        ret = ebi_create_handler(regs);
        break;

    case SBI_EXT_EBI_ENTER:
        sbi_debug("SBI_EXT_EBI_ENTER\n");
        ret = ebi_enter_handler(regs);
        break;

    case SBI_EXT_EBI_EXIT:
        sbi_debug("SBI_EXT_EBI_EXIT\n");
        ret = ebi_exit_handler(regs);
        break;
    
    case SBI_EXT_EBI_EXIT_THREAD:
        sbi_debug("SBI_EXT_EBI_EXIT_THREAD\n");
        ret = ebi_exit_thread_handler(regs);
        break;

    case SBI_EXT_EBI_SUSPEND:
        sbi_debug("SBI_EXT_EBI_SUSPEND\n");
        ret = ebi_suspend_handler(regs);
        break;

    case SBI_EXT_EBI_RESUME:
        sbi_debug("SBI_EXT_EBI_RESUME\n");
        ret = ebi_resume_handler(regs);
        break;

    case SBI_EXT_EBI_MEM_ALLOC:
        sbi_debug("SBI_EXT_EBI_MEM_ALLOC\n");
        START_TIMER(mem_alloc, eid);
        ret = ebi_mem_alloc_handler(regs);
        STOP_TIMER(mem_alloc, eid);
        break;

    case SBI_EXT_EBI_BLOCK_THREAD:
        sbi_debug("SBI_EXT_EBI_BLOCK_THREAD\n");
        ret = ebi_block_thread_handler(regs);
        break;

    case SBI_EXT_EBI_UNBLOCK_THREADS:
        sbi_debug("SBI_EXT_EBI_UNBLOCK_THREADS\n");
        ret = ebi_unblock_threads_handler(regs);
        break;

    case SBI_EXT_EBI_LISTEN_MESSAGE:
        sbi_debug("SBI_EXT_EBI_LISTEN_MESSAGE\n");
        ret = ebi_listen_message_handler(regs);
        break;
    
    case SBI_EXT_EBI_SEND_MESSAGE:
        sbi_debug("SBI_EXT_EBI_SEND_MESSAGE\n");
        ret = ebi_send_message_handler(regs);
        break;

    case SBI_EXT_EBI_STOP_LISTEN:
        sbi_debug("SBI_EXT_EBI_STOP_LISTEN\n");
        ret = ebi_stop_listen_handler(regs);
        break;

    case SBI_EXT_EBI_DEBUG_DUMP_STATUS:
    	// stop_other_harts();
        ret = dump_enclave_status();
        // resume_other_harts();
        break;
    
    case SBI_EXT_EBI_DEBUG_DUMP_OWNERSHIP:
    	// stop_other_harts();
        dump_partition_ownership();
        // resume_other_harts();
        break;
    
    case SBI_EXT_EBI_DEBUG_DUMP_PMP:
    	// stop_other_harts();
        __pmp_dump();
        sbi_printf("medeleg = 0x%lx\n", csr_read(CSR_MEDELEG));
        // resume_other_harts();
        break;

    case SBI_EXT_EBI_DEBUG_DUMP_REGION:
        // stop_other_harts();
        dump_region();
        // resume_other_harts();
        break;

    case SBI_EXT_EBI_DEBUG_UNMATCHED_ACC_FAULT:
        // sbi_printf("access fault from S mode\n");
        // __pmp_dump();
        // dump_region_list(eid);
        pmp_fault_handler(eid, regs->a0);
        break;

    case SBI_EXT_EVAL_SET_S_TIMER:
        SET_S_TIMER(interrupt, eid, regs->a0);
        SET_S_TIMER(syscall, eid, regs->a1);
        SET_S_TIMER(emodule, eid, regs->a2);
        break;

    case SBI_EXT_EVAL_GET_TIMER:
        if (eid != HOST_EID)
            sbi_panic("Cannot get timer in enclaves\n");
        ret = get_timer(regs->a0, regs->a1);
        break;

    case SBI_EXT_EVAL_CONFIG:
        pmp_enable = regs->a0 & CONFIG_PMP_ENABLE;
        id_split = regs->a0 & ID_SPLIT;
        tlb_cache = regs->a0 & TLB_CACHE;
        fragmented = regs->a0 & FRAGMENTED;
        max_pmp = regs->a1;
        // smp_mb();
        break;

    case SBI_EXT_EVAL_DUMP_CONFIG:
        ipi_send_ebi_get_config(-1);
        break;

    case SBI_EXT_EBI_GET_EID:
        ret = (u64)get_current_eid();
        break;
    
    case SBI_EXT_EBI_GET_TID:
        ret = (u64)get_current_tid();
        break;
    
    case SBI_EXT_EBI_GET_HARTID:
        ret = (u64)current_hartid();
        break;

    case SBI_EXT_EBI_GET_BLOCKED_THREADS:
        sbi_debug("SBI_EXT_EBI_GET_BLOCKED_THREADS\n");
        ret = get_blocked_threads(eid);
        break;
    
    case SBI_EXT_EBI_SET_CLEAR_CHILD_TID:
        set_clear_child_tid(eid, get_current_tid(), regs->a0);
        break;

    case SBI_EXT_EBI_GET_CLEAR_CHILD_TID:
        sbi_debug("SBI_EXT_EBI_GET_CLEAR_CHILD_TID\n");
        ret = get_clear_child_tid(eid, get_current_tid());
        break;
    
    case SBI_EXT_EBI_GET_ALIVE_COUNT:
        ret = (u64)get_alive_count();
        break;

    case SBI_EXT_EBI_GET_STATUS:
        if (regs->a0 == 0 || regs->a0 > NUM_ENCLAVE)
            ret = 0;
        else 
            ret = (u64)get_enclave_status(regs->a0);
        break;

    case SBI_EXT_GET_EID_COUND:
        ret = get_eid_count();
        break;

    case SBI_EXT_EBI_RESET:
        reset_coffer();
        break;

	default:
		sbi_error("Unknown extension ID: %lu\n", funcid);
		ret = SBI_ENOTSUPP;
	}

    regs->mepc -= 4;
    
    /* Only pass valid return value to regs->a0 */
    if (ret >= 0) {
		regs->a0 = ret;  
		ret = 0;
	}

    return ret;  // return ErrorCode (negative) when errors occur
}
