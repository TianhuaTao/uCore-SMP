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

void sbi_console_putchar(int c);

int sbi_console_getchar();
#endif // SBI_H
