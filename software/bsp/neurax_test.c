/**
 * @file neurax_test.c
 * @brief Neurax BSP Integration Test
 *
 * Tests: ID check, register R/W, DMA transfers, small convolution.
 *
 * IMPORTANT: DMA needs physically contiguous memory. For userspace we use
 * a RESERVED memory region at the top of DDR. You can pass the address via
 * command line or it defaults to 0x3E000000 (last 32 MB of a 1 GB system).
 *
 * Usage:
 *   ./neurax_test [dma_buffer_phys_addr_hex]
 *
 * Example:
 *   ./neurax_test 3E000000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "neurax_bsp.h"

/* =========================================================================
 * Test configuration
 * ========================================================================= */

/* Default DMA buffer physical address - top of 1 GB DDR */
#define DEFAULT_DMA_PHYS_BASE   0x3E000000
#define DMA_REGION_SIZE         (4 * 1024 * 1024)   /* 4 MB for testing */

/* Buffer layout within the DMA region */
#define INPUT_OFFSET    0x00000
#define OUTPUT_OFFSET   0x100000   /* 1 MB in */

/*
 * Hardware dimensions (must match FPGA generics g_FB_HEIGHT/g_FB_WIDTH):
 *   INPUT_HEIGHT = 100, INPUT_WIDTH = 100
 * RAM layout (FPGA_accelerator generics, one Q8.8 per 32-bit word):
 *   INPUT_BASE_ADDR  = 0      (input data: 100*100 = 10000 words)
 *   WEIGHT_BASE_ADDR = 10000  (weights: k*k*in_ch*out_ch words)
 *   BIAS_BASE_ADDR   = 13000  (biases: out_ch words)
 *   OUTPUT_BASE_ADDR = 13016  (output results: 98*98 = 9604 words)
 * RAM size: 23000 words (fits in 2^15 = 32768 M10K)
 */
#define HW_INPUT_H      100
#define HW_INPUT_W      100
#define TEST_KERNEL      3
#define TEST_IN_CH       1
#define TEST_OUT_CH      1
#define TEST_STRIDE      1
#define TEST_PADDING     0

/* Output size: floor((100 - 3)/1) + 1 = 98 → 98x98 */
#define TEST_OUTPUT_H   98
#define TEST_OUTPUT_W   98

/* RAM word addresses (one Q8.8 value per 32-bit word) */
#define RAM_INPUT_BASE   0
#define RAM_WEIGHT_BASE  10000
#define RAM_BIAS_BASE    13000
#define RAM_OUTPUT_BASE  13016
#define RAM_TOTAL_WORDS  23000

/* =========================================================================
 * Test helpers
 * ========================================================================= */

#define PASS(msg)   printf("  [PASS] %s\n", (msg))
#define FAIL(msg)   printf("  [FAIL] %s\n", (msg))

static int tests_passed = 0;
static int tests_failed = 0;

static void check(int cond, const char *msg) {
    if (cond) {
        PASS(msg);
        tests_passed++;
    } else {
        FAIL(msg);
        tests_failed++;
    }
}

/* =========================================================================
 * Test: Magic ID
 * ========================================================================= */

static void test_magic_id(neurax_bsp_t *bsp) {
    printf("\n--- Test: Magic ID ---\n");
    uint32_t id = neurax_reg_read(bsp, REG_READ_ONLY);
    printf("  REG_READ_ONLY = 0x%08X (expected 0x%08X)\n", id, NEURAX_MAGIC_ID);
    check(id == NEURAX_MAGIC_ID, "Magic ID matches");
}

/* =========================================================================
 * Test: Register Read/Write
 * ========================================================================= */

static void test_register_rw(neurax_bsp_t *bsp) {
    printf("\n--- Test: Register R/W ---\n");

    /* Write a pattern to a temp register and read it back */
    uint32_t pattern = 0xDEADBEEF;
    neurax_reg_write(bsp, REG_TEMP_0, pattern);
    uint32_t readback = neurax_reg_read(bsp, REG_TEMP_0);
    printf("  TEMP_0: wrote 0x%08X, read 0x%08X\n", pattern, readback);
    check(readback == pattern, "TEMP_0 readback matches");

    /* Test CMD register — only bottom bits should be writable */
    neurax_reg_write(bsp, REG_CMD, CMD_ENABLE);
    readback = neurax_reg_read(bsp, REG_CMD);
    printf("  CMD: wrote 0x%08X, read 0x%08X\n", CMD_ENABLE, readback);
    check((readback & CMD_ENABLE) != 0, "CMD enable bit set");
}

/* =========================================================================
 * Test: Accelerator Enable/Reset
 * ========================================================================= */

static void test_enable_reset(neurax_bsp_t *bsp) {
    printf("\n--- Test: Enable / Reset ---\n");

    neurax_reset(bsp);
    uint32_t cmd = neurax_reg_read(bsp, REG_CMD);
    check((cmd & CMD_ENABLE) != 0, "Enable after reset");

    uint32_t status = neurax_reg_read(bsp, REG_STATUS);
    printf("  STATUS = 0x%08X\n", status);
    check((status & STATUS_BUSY) == 0, "Not busy after reset");
}

/* =========================================================================
 * Test: DMA Status
 * ========================================================================= */

static void test_dma_status(neurax_bsp_t *bsp) {
    printf("\n--- Test: DMA Engine Status ---\n");

    neurax_dma_reset(bsp->dma_write_csr);
    neurax_dma_reset(bsp->dma_read_csr);

    uint32_t write_status = bsp->dma_write_csr[0];
    uint32_t read_status  = bsp->dma_read_csr[0];

    printf("  DMA write CSR status = 0x%08X\n", write_status);
    printf("  DMA read  CSR status = 0x%08X\n", read_status);

    check((write_status & MSGDMA_CSR_BUSY) == 0, "DMA write not busy");
    check((read_status  & MSGDMA_CSR_BUSY) == 0, "DMA read not busy");
    check((write_status & MSGDMA_CSR_DESC_EMPTY) != 0, "DMA write desc buffer empty");
    check((read_status  & MSGDMA_CSR_DESC_EMPTY) != 0, "DMA read desc buffer empty");
}

/* =========================================================================
 * Test: Small Convolution (vertical edge detector on 4×4 input)
 * ========================================================================= */

static void test_convolution(neurax_bsp_t *bsp, int fd_mem, uint32_t dma_phys_base) {
    printf("\n--- Test: Convolution (%dx%d input, %dx%d kernel) ---\n",
           HW_INPUT_H, HW_INPUT_W, TEST_KERNEL, TEST_KERNEL);

    /*
     * Map the DMA physical region into userspace so we can write test data.
     * The DMA engines will access this memory via their physical address.
     */
    void *dma_region = mmap(NULL, DMA_REGION_SIZE,
                            PROT_READ | PROT_WRITE, MAP_SHARED,
                            fd_mem, dma_phys_base);
    if (dma_region == MAP_FAILED) {
        perror("  mmap DMA region");
        FAIL("Could not map DMA buffer region");
        tests_failed++;
        return;
    }

    uint32_t *input_buf  = (uint32_t *)((uint8_t *)dma_region + INPUT_OFFSET);
    uint32_t *output_buf = (uint32_t *)((uint8_t *)dma_region + OUTPUT_OFFSET);

    /* ---- Clear entire RAM image ---- */
    memset(input_buf, 0, RAM_TOTAL_WORDS * 4);

    /* ---- Prepare input data: 100x100, all 1.0 in Q8.8 ---- */
    /*
     * Fill a uniform 100x100 input with value 1.0.
     * With a 3x3 kernel of all 1.0, every output should be 9.0 (sum of 9 ones).
     * Hardware stores one Q8.8 value per 32-bit RAM word (in lower 16 bits).
     */
    int16_t val_one = float_to_q8_8(1.0f);
    int total_input = HW_INPUT_H * HW_INPUT_W;  /* 10000 elements */
    for (int i = 0; i < total_input; i++) {
        input_buf[RAM_INPUT_BASE + i] = (uint32_t)(uint16_t)val_one;
    }
    printf("  Input: %dx%d uniform 1.0 (%d elements, %d words)\n",
           HW_INPUT_H, HW_INPUT_W, total_input, total_input);

    /* ---- Prepare weights: 3x3, all 1.0 in Q8.8 ---- */
    int total_weights = TEST_KERNEL * TEST_KERNEL * TEST_IN_CH * TEST_OUT_CH;  /* 9 */
    for (int i = 0; i < total_weights; i++) {
        input_buf[RAM_WEIGHT_BASE + i] = (uint32_t)(uint16_t)val_one;
    }
    printf("  Weights: %dx%d all-ones kernel (%d elements, %d words)\n",
           TEST_KERNEL, TEST_KERNEL, total_weights, total_weights);

    /* ---- Prepare bias: 0.0 ---- */
    input_buf[RAM_BIAS_BASE] = (uint32_t)(uint16_t)float_to_q8_8(0.0f);
    printf("  Bias: 0.0\n");

    /* ---- Clear output readback area ---- */
    memset(output_buf, 0xAA, RAM_TOTAL_WORDS * 4);

    /* ---- DMA: send RAM image to FPGA ---- */
    neurax_dma_reset(bsp->dma_write_csr);
    neurax_dma_reset(bsp->dma_read_csr);

    printf("  Sending %d words (%d bytes) via DMA...\n",
           RAM_TOTAL_WORDS, RAM_TOTAL_WORDS * 4);

    uint32_t input_phys  = dma_phys_base + INPUT_OFFSET;
    uint32_t output_phys = dma_phys_base + OUTPUT_OFFSET;
    uint32_t total_send  = RAM_TOTAL_WORDS * 4;
    int rc;

    /* Single descriptor: MAX_BYTE=131072 covers 92000 bytes */
    {
        volatile uint32_t *desc = bsp->dma_write_desc;
        desc[MSGDMA_DESC_READ_ADDR / 4]  = input_phys;
        desc[MSGDMA_DESC_WRITE_ADDR / 4] = 0;
        desc[MSGDMA_DESC_LENGTH / 4]     = total_send;
        desc[MSGDMA_DESC_CONTROL / 4]    = MSGDMA_DESC_CTL_GO
                                          | MSGDMA_DESC_CTL_GENERATE_SOP
                                          | MSGDMA_DESC_CTL_GENERATE_EOP
                                          | MSGDMA_DESC_CTL_TX_CHANNEL(0);
    }

    rc = neurax_dma_send_wait(bsp);
    if (rc) {
        uint32_t dma_sts = bsp->dma_write_csr[MSGDMA_CSR_STATUS / 4];
        uint32_t dma_fill = bsp->dma_write_csr[MSGDMA_CSR_RW_FILL / 4];
        printf("  DMA write CSR STATUS=0x%08X  RW_FILL=0x%08X\n", dma_sts, dma_fill);
        printf("  (busy=%d desc_empty=%d resetting=%d irq=%d)\n",
               (dma_sts >> 0) & 1, (dma_sts >> 1) & 1,
               (dma_sts >> 6) & 1, (dma_sts >> 9) & 1);
        FAIL("DMA send timeout");
        tests_failed++;
        goto cleanup;
    }
    printf("  DMA send complete (%u bytes)\n", total_send);

    /* ---- Verify DMA write: immediate read-back before convolution ---- */
    printf("  Verifying RAM contents (read back before convolution)...\n");
    memset(output_buf, 0xBB, RAM_TOTAL_WORDS * 4);  /* fill with sentinel */

    neurax_dma_reset(bsp->dma_read_csr);
    {
        volatile uint32_t *desc = bsp->dma_read_desc;
        desc[MSGDMA_DESC_READ_ADDR / 4]  = 0;
        desc[MSGDMA_DESC_WRITE_ADDR / 4] = output_phys;
        desc[MSGDMA_DESC_LENGTH / 4]     = total_send;
        desc[MSGDMA_DESC_CONTROL / 4]    = MSGDMA_DESC_CTL_GO
                                          | MSGDMA_DESC_CTL_END_ON_EOP;

        rc = neurax_dma_recv_wait(bsp);
        if (rc) {
            printf("  WARNING: verify readback DMA timeout\n");
            uint32_t dma_sts = bsp->dma_read_csr[MSGDMA_CSR_STATUS / 4];
            printf("  DMA read CSR STATUS=0x%08X\n", dma_sts);
        } else {
            printf("  Verify input[0..3]:  %08X %08X %08X %08X\n",
                   output_buf[0], output_buf[1], output_buf[2], output_buf[3]);
            printf("  Verify weight[10000..10011]: %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\n",
                   output_buf[10000], output_buf[10001], output_buf[10002], output_buf[10003],
                   output_buf[10004], output_buf[10005], output_buf[10006], output_buf[10007],
                   output_buf[10008], output_buf[10009], output_buf[10010], output_buf[10011]);
            printf("  Verify bias[13000..13001]: %08X %08X\n",
                   output_buf[13000], output_buf[13001]);
            printf("  Verify output_area[13016..13019]: %08X %08X %08X %08X\n",
                   output_buf[13016], output_buf[13017], output_buf[13018], output_buf[13019]);
            /* Count non-zero in weight region */
            int wt_nz = 0;
            for (int i = 10000; i < 10000 + 9; i++)
                if (output_buf[i] != 0) wt_nz++;
            printf("  Verify: %d/9 weights non-zero\n", wt_nz);
        }
    }

    /* Need to re-send data since readback consumed it (buffer_full cleared) */
    printf("  Re-sending data after verification readback...\n");
    neurax_dma_reset(bsp->dma_write_csr);
    {
        volatile uint32_t *desc = bsp->dma_write_desc;
        desc[MSGDMA_DESC_READ_ADDR / 4]  = input_phys;
        desc[MSGDMA_DESC_WRITE_ADDR / 4] = 0;
        desc[MSGDMA_DESC_LENGTH / 4]     = total_send;
        desc[MSGDMA_DESC_CONTROL / 4]    = MSGDMA_DESC_CTL_GO
                                          | MSGDMA_DESC_CTL_GENERATE_SOP
                                          | MSGDMA_DESC_CTL_GENERATE_EOP
                                          | MSGDMA_DESC_CTL_TX_CHANNEL(0);
    }
    rc = neurax_dma_send_wait(bsp);
    if (rc) {
        FAIL("DMA re-send timeout");
        tests_failed++;
        goto cleanup;
    }
    printf("  Re-send complete (%u bytes)\n", total_send);

    /* ---- Configure and start convolution ---- */
    neurax_reset(bsp);

    neurax_conv_config_t conv_cfg = {
        .kernel_size    = TEST_KERNEL,
        .stride         = TEST_STRIDE,
        .padding        = TEST_PADDING,
        .input_channels = TEST_IN_CH,
        .output_channels = TEST_OUT_CH,
    };
    neurax_config_conv(bsp, &conv_cfg);
    neurax_set_batch_size(bsp, 1);

    printf("  Starting convolution (expect 98x98 = 9604 outputs)...\n");
    neurax_start(bsp, OP_CONVOLUTION);

    /* Check that busy goes high */
    usleep(100);
    uint32_t status_after_start = neurax_reg_read(bsp, REG_STATUS);
    printf("  STATUS after start = 0x%08X (busy=%d, done=%d)\n",
           status_after_start,
           (status_after_start >> 5) & 1,
           (status_after_start >> 4) & 1);
    check((status_after_start & STATUS_BUSY) != 0, "Accelerator is busy after start");

    /* Wait for completion — 98x98x9 MACs at ~4 clk/MAC = ~346K cycles = ~7ms at 50MHz */
    /* Give generous timeout of 5 seconds */
    printf("  Waiting for done (timeout 5s)...\n");
    int timeout = 5000000;
    while (!neurax_is_done(bsp) && timeout > 0) {
        usleep(1);
        timeout--;
    }

    uint32_t cycles = neurax_reg_read(bsp, REG_DEBUG_CYCLES);
    uint32_t final_status = neurax_reg_read(bsp, REG_STATUS);
    printf("  Final STATUS = 0x%08X (busy=%d, done=%d)\n",
           final_status,
           (final_status >> 5) & 1,
           (final_status >> 4) & 1);
    printf("  Cycle count = %u\n", cycles);

    if (timeout <= 0) {
        FAIL("Convolution TIMEOUT (5s)");
        tests_failed++;
        goto cleanup;
    }
    check(neurax_is_done(bsp), "Convolution completed (done flag set)");

    /* ---- DMA: read results back from FPGA ---- */
    printf("  Reading results via DMA...\n");

    neurax_dma_reset(bsp->dma_read_csr);

    uint32_t total_recv = RAM_TOTAL_WORDS * 4;
    {
        volatile uint32_t *desc = bsp->dma_read_desc;
        desc[MSGDMA_DESC_READ_ADDR / 4]  = 0;
        desc[MSGDMA_DESC_WRITE_ADDR / 4] = output_phys;
        desc[MSGDMA_DESC_LENGTH / 4]     = total_recv;
        desc[MSGDMA_DESC_CONTROL / 4]    = MSGDMA_DESC_CTL_GO
                                          | MSGDMA_DESC_CTL_END_ON_EOP;
    }

    rc = neurax_dma_recv_wait(bsp);
    if (rc) {
        uint32_t dma_sts = bsp->dma_read_csr[MSGDMA_CSR_STATUS / 4];
        printf("  DMA read CSR STATUS=0x%08X\n", dma_sts);
        FAIL("DMA recv timeout");
        tests_failed++;
        goto cleanup;
    }
    printf("  DMA recv complete (%u bytes)\n", total_recv);

    /* ---- Raw memory dump for debugging ---- */
    printf("\n  === RAW MEMORY DUMP ===\n");
    
    /* Check first few input words (should be 0x0100 = 1.0 in Q8.8) */
    printf("  Input area [0..7]:");
    for (int i = 0; i < 8; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");
    
    /* Check weight area */
    printf("  Weight area [10000..10011]:");
    for (int i = 10000; i < 10012; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");
    
    /* Check bias area */
    printf("  Bias area [13000..13003]:");
    for (int i = 13000; i < 13004; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    /* Check output area at new base (13016) */
    printf("  Output area [13016..13031] (first 16 outputs):\n    ");
    for (int i = 13016; i < 13032; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");
    
    /* Check output at middle */
    printf("  Output area [13016+990..+1005] (row 10, col 10 region):\n    ");
    for (int i = 13016+990; i < 13016+1006; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    /* Check output at end */
    printf("  Output area [13016+9590..+9605] (last outputs):\n    ");
    for (int i = 13016+9590; i < 13016+9606; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    /* Also check OLD output base (8016) in case hardware still writes there */
    printf("  OLD output area [8016..8031]:");
    for (int i = 8016; i < 8032; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    /* Count non-zero words in output region */
    int nonzero = 0;
    int first_nz = -1, last_nz = -1;
    for (int i = 0; i < 9604; i++) {
        if (output_buf[RAM_OUTPUT_BASE + i] != 0) {
            nonzero++;
            if (first_nz < 0) first_nz = i;
            last_nz = i;
        }
    }
    printf("  Non-zero outputs: %d / 9604 (first=%d, last=%d)\n", nonzero, first_nz, last_nz);

    /* Show first 10 non-zero */
    if (nonzero > 0) {
        printf("  First non-zero values:");
        int shown = 0;
        for (int i = 0; i < 9604 && shown < 10; i++) {
            if (output_buf[RAM_OUTPUT_BASE + i] != 0) {
                printf(" [%d]=0x%08X", i, output_buf[RAM_OUTPUT_BASE + i]);
                shown++;
            }
        }
        printf("\n");
    }
    printf("  === END DUMP ===\n\n");

    /* ---- Verify a few output values ---- */
    /*
     * With uniform 1.0 input and all-ones 3x3 kernel:
     *   Interior output elements = sum of 9 ones = 9.0
     * In Q8.8: 9.0 = 0x0900
     * Expected at RAM word 8016 onwards.
     *
     * Check a few interior positions (away from edges to avoid any
     * potential boundary effects).
     */
    float expected_val = 9.0f;
    printf("  Checking output values (expected %.1f for interior positions):\n", expected_val);

    /* Check output positions [10][10], [50][50], [90][90] — all interior */
    int check_positions[][2] = { {10, 10}, {50, 50}, {90, 90}, {1, 1}, {97, 97} };
    int num_checks = sizeof(check_positions) / sizeof(check_positions[0]);
    int output_ok = 1;

    for (int c = 0; c < num_checks; c++) {
        int oh = check_positions[c][0];
        int ow = check_positions[c][1];
        int out_idx = oh * TEST_OUTPUT_W + ow;  /* Linear index */

        /* Hardware writes one Q8.8 value per 32-bit RAM word (low 16 bits) */
        uint32_t word = output_buf[RAM_OUTPUT_BASE + out_idx];
        int16_t val = (int16_t)(word & 0xFFFF);

        float fval = q8_8_to_float(val);
        printf("    output[%d][%d] (idx=%d) = %.4f (raw=0x%04X, expected=%.1f)\n",
               oh, ow, out_idx, fval, (uint16_t)val, expected_val);

        if (fval < expected_val - 0.5f || fval > expected_val + 0.5f)
            output_ok = 0;
    }

    check(output_ok, "Convolution output values correct");

cleanup:
    munmap(dma_region, DMA_REGION_SIZE);
}

/* =========================================================================
 * Test: Register Dump
 * ========================================================================= */

static void dump_registers(neurax_bsp_t *bsp) {
    printf("\n--- Register Dump ---\n");
    const char *names[] = {
        "CMD", "STATUS", "CONFIG", "CONV_CFG_0", "CONV_CFG_1",
        "POOL_CFG", "ACT_CFG", "ACT_ALPHA", "BATCH_SIZE",
        "TEMP_0", "TEMP_1", "TEMP_2", "TEMP_3",
        "DBG_CYCLES", "DBG_STATUS", "READ_ONLY"
    };
    for (int i = 0; i < 16; i++) {
        printf("  [%2d] %-12s = 0x%08X\n", i, names[i], neurax_reg_read(bsp, i));
    }
}

/* =========================================================================
 * Main
 * ========================================================================= */

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("  Neurax BSP Integration Test\n");
    printf("========================================\n");

    /* Parse optional DMA buffer address */
    uint32_t dma_phys_base = DEFAULT_DMA_PHYS_BASE;
    if (argc > 1) {
        dma_phys_base = (uint32_t)strtoul(argv[1], NULL, 16);
    }
    printf("DMA buffer physical base: 0x%08X\n", dma_phys_base);

    /* Initialize BSP */
    neurax_bsp_t bsp;
    if (neurax_bsp_init(&bsp) != 0) {
        fprintf(stderr, "Failed to initialize BSP\n");
        return 1;
    }
    printf("BSP initialized OK\n");

    /* Run tests */
    test_magic_id(&bsp);
    test_register_rw(&bsp);
    test_enable_reset(&bsp);
    test_dma_status(&bsp);
    dump_registers(&bsp);
    test_convolution(&bsp, bsp.fd_mem, dma_phys_base);

    /* Summary */
    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    /* Cleanup */
    neurax_bsp_deinit(&bsp);

    return (tests_failed > 0) ? 1 : 0;
}
