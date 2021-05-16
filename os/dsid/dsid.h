#ifndef DSID_H
#define DSID_H

#define NR(arr) (sizeof(arr) / sizeof(arr[0]))
#include "../ucore/types.h"
#include "../lock/lock.h"

enum
{
    /* Access current dsid */
    CP_HART_DSID = 0x41,
    /* Access dsid selector */
    CP_HART_SEL = 0x42,
    /* Read total dsid count */
    CP_DSID_COUNT = 0x43,
    /* Access mem base with current dsid */
    CP_MEM_BASE_LO = 0x44,
    CP_MEM_BASE_HI = 0x45,
    /* Access mem mask with current dsid */
    CP_MEM_MASK_LO = 0x46,
    CP_MEM_MASK_HI = 0x47,
    CP_BUCKET_FREQ = 0x48,
    CP_BUCKET_SIZE = 0x49,
    CP_BUCKET_INC = 0x4a,
    CP_TRAFFIC = 0x4b,
    CP_WAYMASK = 0x4c,
    CP_L2_CAPACITY = 0x4d,
    CP_DSID_SEL = 0x4e,
    CP_LIMIT_INDEX = 0x4f,
    CP_LIMIT = 0x50,
    CP_LOW_THRESHOLD = 0x51,
    CP_HIGH_THRESHOLD = 0x52,
    CP_MAX_QUOTA = 0x53,
    CP_MIN_QUOTA = 0x54,
    CP_QUOTA_STEP = 0x55,
    CP_TIMER_LO = 0x56,
    CP_TIMER_HI = 0x57,
    CP_AUTOCAT_EN = 0x58,
    CP_AUTOCAT_BIN_POWER = 0x59,
    CP_AUTOCAT_WAYMASK = 0x5a,
    CP_AUTOCAT_WATCH_DSID = 0x5b,
    CP_AUTOCAT_SET = 0x5c,
    CP_AUTOCAT_GAP = 0x5d,
    CORE_PC_HI = 0x70,
    CORE_PC_LO = 0x71,
    CORE_PC_SNAP = 0x72,
    CORE_PC_READ_DONE = 0x73,
    CORE_PC_READ = 0x74,
    CORE_INT_DEBUG = 0x75,
    CORE_N_INT_DEBUG = 0x76,
    CORE_N_INT_DEBUG_LOCAL = 0x77,
    CORE_CSR_INT_VALID = 0x78,
    CORE_CSR_PENDING_INT_LO = 0x79,
    CORE_CSR_PENDING_INT_HI = 0x7a,
    CP_HART_ID = 0x7b,
    CP_L2_REQ_EN = 0x7f,
    CP_L2_REQ_MISS = 0x7c,
    CP_L2_REQ_TOTAL = 0x7d,
    CP_L2_STAT_RESET = 0x7e,
};

extern const char *cp_reg_name[];
extern struct spinlock dsid_lock;

void init_dsid(void);
void cp_reg_w(uint32 dm_reg, uint32 val);
uint32 cp_reg_r(uint32 dm_reg);

#endif // !DSID_H