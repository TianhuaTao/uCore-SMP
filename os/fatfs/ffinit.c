//
// Created by 邹先予 on 2022/3/31.
//

#include <ucore/ucore.h>
#include "ff.h"
#include "init.h"

void ffinit() {
    FATFS fs;           /* Filesystem object */
    FRESULT res;  /* API result code */

    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("f_mount() failed: %d\n", res);
        return;
    }

    res = f_chdrive("0:");
    if (res != FR_OK) {
        printf("f_chdrive() failed: %d\n", res);
        return;
    }
}
