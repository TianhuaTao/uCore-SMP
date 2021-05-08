#if !defined(SBI_H)
#define SBI_H

struct sbiret {
	long error;
	long value;
};

enum sbi_ext_hsm_fid {
	SBI_EXT_HSM_HART_START = 0,
	SBI_EXT_HSM_HART_STOP,
	SBI_EXT_HSM_HART_STATUS,
};


#endif // SBI_H

