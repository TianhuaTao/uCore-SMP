//
// Created by 邹先予 on 2022/3/31.
//

#include <ucore/ucore.h>
#include "ff.h"
#include "fftest.h"

void fftest() {
    FATFS fs;           /* Filesystem object */
    FIL fil;            /* File object */
    FRESULT res;  /* API result code */
    UINT bw;            /* Bytes written */
    BYTE mm[50];
    UINT i;

    printf("FatFs test\n");

    /* 挂载文件系统 */
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("f_mount() failed: %d\n", res);
        return;
    }
    printf("f_mount() succeeded.\n");

    res = f_chdrive("0:");
    if (res != FR_OK) {
        printf("f_chdrive() failed: %d\n", res);
        return;
    }
    printf("f_chdrive() succeeded.\n");

    res = f_open(&fil, "/test.txt", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
    if (res != FR_OK) {
        printf("f_open() failed: %d\n", res);
        return;
    }
    printf("f_open() succeeded.\n");

    /* Write a message */
    res = f_write(&fil, "Hello,World!", 12, &bw);
    if (res != FR_OK) {
        printf("f_write() failed: %d\n", res);
        return;
    }
    printf("f_write() succeeded.\n");

    res = f_size(&fil);
    printf("f_size() = %d\n", res);

    memset(mm, 0x0, 50);
    f_lseek(&fil, 0);
    res = f_read(&fil, mm, 12, &i);
    if (res != FR_OK) {
        printf("f_read() failed: %d\n", res);
        return;
    }
    printf("f_read() = %s\n", mm);

    /* Close the file */
    res = f_close(&fil);
    if (res != FR_OK) {
        printf("f_close() failed: %d\n", res);
        return;
    }
    printf("f_close() succeeded.\n");
    /*卸载文件系统*/
    f_mount(0, "0:", 0);

    printf("FatFs test done\n");
}