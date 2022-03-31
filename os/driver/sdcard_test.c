//
// Created by 邹先予 on 2022/3/25.
//

#include <utils/log.h>
#include <sifive/platform.h>
#include <spi/spi.h>
#include "sd.h"
#include "sdcard_test.h"

#define GPT_HEADER_LBA 1

void print_block(const uint8_t *gpt_buf);

void sdcard_test() {
    printf("Initializing sd\n");

    unsigned int peripheral_input_khz = 500000; // copy from fsbl\main.c:173
    spi_ctrl* spictrl = (spi_ctrl*) SPI2_CTRL_ADDR;

    int error = sd_init(spictrl, peripheral_input_khz, 0);
    printf("initialize_sd result %d\n", error);
    if (error) return;

    uint8_t gpt_buf[GPT_BLOCK_SIZE];
    error = sd_read_blocks(spictrl, gpt_buf, GPT_HEADER_LBA, 1);
    printf("sd_read_blocks result %d\n", error);
    print_block(gpt_buf);

//    gpt_partition_range part_range;
//    {
//        // header will be overwritten by find_sd_gpt_partition(), so locally
//        // scope it.
//        gpt_header* header = (gpt_header*) gpt_buf;
//        part_range = find_sd_gpt_partition(
//                spictrl,
//                header->partition_entries_lba,
//                header->num_partition_entries,
//                header->partition_entry_size,
//                &gpt_guid_rootfs,
//                gpt_buf
//        );
//    }
//
//    if (!gpt_is_valid_partition_range(part_range)) {
//        printf("find root partition failed\n");
//        return;
//    }
//
//    printf("root partition found\n");
//
//    printf("read before\n");
//    memset(gpt_buf, 0, GPT_BLOCK_SIZE);
//    error = sd_read_blocks(spictrl, gpt_buf, part_range.first_lba, 1);
//    if(error) {
//        printf("sd_read_blocks failed\n");
//        return;
//    }
//    print_block(gpt_buf);
//
//    uint8_t gpt_buf2[GPT_BLOCK_SIZE];
//    memset(gpt_buf2, 0, GPT_BLOCK_SIZE);
//    error = sd_write_blocks(spictrl, gpt_buf2, part_range.first_lba, 1);
//    if(error) {
//        printf("sd_write_blocks failed\n");
//        return;
//    }
//    printf("sd_write_blocks success\n");
//
//    printf("read after\n");
//    error = sd_read_blocks(spictrl, gpt_buf2, part_range.first_lba, 1);
//    if(error) {
//        printf("sd_read_blocks failed\n");
//        return;
//    }
//    print_block(gpt_buf2);
//
//    error = sd_write_blocks(spictrl, gpt_buf, part_range.first_lba, 1);
//    if(error) {
//        printf("sd_write_blocks failed\n");
//        return;
//    }
//    printf("restore success\n");

}

void print_block(const uint8_t *gpt_buf) {
    for(int i = 0; i < GPT_BLOCK_SIZE / 16; i++) {
        for (int j = 0;j<16;j++){
            printf("%x", gpt_buf[i*16+j]>>4);
            printf("%x ", gpt_buf[i*16+j]&0xf);
        }
        printf("\n");
    }
}
