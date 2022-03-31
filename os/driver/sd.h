/* Copyright (c) 2018 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-License-Identifier: GPL-2.0-or-later */
/* See the file LICENSE for further information */

#ifndef _LIBRARIES_SD_H
#define _LIBRARIES_SD_H

#define SD_INIT_ERROR_CMD0 1
#define SD_INIT_ERROR_CMD8 2
#define SD_INIT_ERROR_ACMD41 3
#define SD_INIT_ERROR_CMD58 4
#define SD_INIT_ERROR_CMD16 5

#define SD_COPY_ERROR_CMD18 1
#define SD_COPY_ERROR_CMD18_CRC 2

#define SD_COPY_ERROR_CMD25 1
#define SD_COPY_ERROR_CMD25_CRC 2

#define GPT_BLOCK_SIZE 512

#ifndef __ASSEMBLER__

#include <spi/spi.h>
#include <stdint.h>
#include <stddef.h>

int sd_init(spi_ctrl* spi, unsigned int input_clk_hz, int skip_sd_init_commands);
int sd_read_blocks(spi_ctrl* spi, void* dst, uint32_t src_lba, size_t size);
int sd_write_blocks(spi_ctrl* spi, void* src, uint32_t dst_lba, size_t size);

//gpt_partition_range find_sd_gpt_partition(
//        spi_ctrl* spictrl,
//        uint64_t partition_entries_lba,
//        uint32_t num_partition_entries,
//        uint32_t partition_entry_size,
//        const gpt_guid* partition_type_guid,
//        void* block_buf  // Used to temporarily load blocks of SD card
//);

#endif /* !__ASSEMBLER__ */

#endif /* _LIBRARIES_SD_H */
