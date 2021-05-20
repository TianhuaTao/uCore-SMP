#include "dsid.h"
#include "../mem/memory_layout.h"
#include "../utils/log.h"
#include "../arch/riscv.h"


const char *cp_reg_name[] = {
    /* Access current dsid */
    [CP_HART_DSID - CP_HART_DSID] = "hart_dsid",
    /* Access dsid selector */
    [CP_HART_SEL -
        CP_HART_DSID] = "hart_sel",
    /* Read total dsid count */
    [CP_DSID_COUNT -
        CP_HART_DSID] = "dsid_count",
    /* Access mem base with current dsid */
    [CP_MEM_BASE_LO -
        CP_HART_DSID] = "mem_base",
    [CP_MEM_BASE_HI -
        CP_HART_DSID] = "mem_base",
    /* Access mem mask with current dsid */
    [CP_MEM_MASK_LO -
        CP_HART_DSID] = "mem_mask",
    [CP_MEM_MASK_HI -
        CP_HART_DSID] = "mem_mask",
    [CP_BUCKET_FREQ -
        CP_HART_DSID] = "bucket_freq",
    [CP_BUCKET_SIZE -
        CP_HART_DSID] = "bucket_size",
    [CP_BUCKET_INC -
        CP_HART_DSID] = "bucket_inc",
    [CP_TRAFFIC -
        CP_HART_DSID] = "l1_traffic",
    [CP_WAYMASK -
        CP_HART_DSID] = "l2_waymask",
    [CP_L2_CAPACITY -
        CP_HART_DSID] = "l2_capacity",
    [CP_DSID_SEL -
        CP_HART_DSID] = "dsid_sel",
    [CP_LIMIT_INDEX -
        CP_HART_DSID] = "N/A",
    [CP_LIMIT -
        CP_HART_DSID] = "N/A",
    [CP_LOW_THRESHOLD -
        CP_HART_DSID] = "N/A",
    [CP_HIGH_THRESHOLD -
        CP_HART_DSID] = "N/A",
    [CP_MAX_QUOTA -
        CP_HART_DSID] = "N/A",
    [CP_MIN_QUOTA -
        CP_HART_DSID] = "N/A",
    [CP_QUOTA_STEP -
        CP_HART_DSID] = "N/A",
    [CP_TIMER_LO -
        CP_HART_DSID] = "timestamp",
    [CP_TIMER_HI -
        CP_HART_DSID] = "timestamp",
    [CP_AUTOCAT_EN -
        CP_HART_DSID] = "autocat_en",
    [CP_AUTOCAT_BIN_POWER -
        CP_HART_DSID] = "autocat_cycle_log2",
    [CP_AUTOCAT_WAYMASK -
        CP_HART_DSID] = "autocat_waymask",
    [CP_AUTOCAT_WATCH_DSID -
        CP_HART_DSID] = "autocat_watch",
    [CP_AUTOCAT_SET -
        CP_HART_DSID] = "autocat_set",
    [CP_AUTOCAT_GAP -
        CP_HART_DSID] = "autocat_gap",
    [CORE_PC_HI -
        CP_HART_DSID] = "N/A",
    [CORE_PC_LO -
        CP_HART_DSID] = "N/A",
    [CORE_PC_SNAP -
        CP_HART_DSID] = "N/A",
    [CORE_PC_READ_DONE -
        CP_HART_DSID] = "N/A",
    [CORE_PC_READ -
        CP_HART_DSID] = "N/A",
    [CORE_INT_DEBUG -
        CP_HART_DSID] = "N/A",
    [CORE_N_INT_DEBUG -
        CP_HART_DSID] = "N/A",
    [CORE_N_INT_DEBUG_LOCAL -
        CP_HART_DSID] = "N/A",
    [CORE_CSR_INT_VALID -
        CP_HART_DSID] = "N/A",
    [CORE_CSR_PENDING_INT_LO -
        CP_HART_DSID] = "N/A",
    [CORE_CSR_PENDING_INT_HI -
        CP_HART_DSID] = "N/A",
    [CP_HART_ID -
        CP_HART_DSID] = "vhartid",
    [CP_L2_REQ_EN -
        CP_HART_DSID] = "l2_query_miss",
    [CP_L2_REQ_MISS -
        CP_HART_DSID] = "l2_miss",
    [CP_L2_REQ_TOTAL -
        CP_HART_DSID] = "l2_total",
    [CP_L2_STAT_RESET -
        CP_HART_DSID] = "l2_stat_reset",
};

struct spinlock dsid_lock;
volatile uint32 *cpbase = (uint32 *)DSID_CP_BASE;

void init_dsid(void)
{
    init_spin_lock_with_name(&dsid_lock, "dsid_lock");
}

uint32 cp_reg_r(uint32 dm_reg)
{
    return (uint32) * ((uint32 *)DSID_CP_BASE + dm_reg);
}

void cp_reg_w(uint32 dm_reg, uint32 val)
{
    if (cpbase == NULL)
    {
        errorf("cpbase is NULL\n");
    }
    else
    {
        *((uint32 *)DSID_CP_BASE + dm_reg) = val;
        mmiowb();
    }
}
