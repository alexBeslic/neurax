/**
 * @file neurax_test.c
 * @brief Neurax BSP Integration Test
 *
 * Tests: ID check, register R/W, small convolution.
 *
 * DMA transfers use the Linux msgdma character device driver (/dev/msgdma).
 * The driver handles DMA buffer management and descriptor submission
 * internally; userspace just calls write() to send data and read() to
 * receive data.
 *
 * Usage:
 *   ./neurax_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "neurax_bsp.h"

/* =========================================================================
 * Test configuration
 * ========================================================================= */

/* msgdma character device path */
#define MSGDMA_DEV_PATH     "/dev/msgdma"

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
 * Test: Small Convolution (uniform input, all-ones kernel)
 * ========================================================================= */

static void test_convolution(neurax_bsp_t *bsp) {
    printf("\n--- Test: Convolution (%dx%d input, %dx%d kernel) ---\n",
           HW_INPUT_H, HW_INPUT_W, TEST_KERNEL, TEST_KERNEL);

    /* Open the msgdma character device */
    int fd_dma = open(MSGDMA_DEV_PATH, O_RDWR);
    if (fd_dma < 0) {
        perror("  open " MSGDMA_DEV_PATH);
        FAIL("Could not open msgdma device");
        tests_failed++;
        return;
    }

    /* Allocate RAM image buffer */
    uint32_t *ram_buf = (uint32_t *)calloc(RAM_TOTAL_WORDS, sizeof(uint32_t));
    if (!ram_buf) {
        FAIL("Could not allocate RAM buffer");
        tests_failed++;
        close(fd_dma);
        return;
    }

    /* ---- Prepare input data: 100x100, all 1.0 in Q8.8 ---- */
    int16_t val_one = float_to_q8_8(1.0f);
    int total_input = HW_INPUT_H * HW_INPUT_W;
    for (int i = 0; i < total_input; i++) {
        ram_buf[RAM_INPUT_BASE + i] = (uint32_t)(uint16_t)val_one;
    }
    printf("  Input: %dx%d uniform 1.0 (%d elements)\n",
           HW_INPUT_H, HW_INPUT_W, total_input);

    /* ---- Prepare weights: 3x3, all 1.0 in Q8.8 ---- */
    int total_weights = TEST_KERNEL * TEST_KERNEL * TEST_IN_CH * TEST_OUT_CH;
    for (int i = 0; i < total_weights; i++) {
        ram_buf[RAM_WEIGHT_BASE + i] = (uint32_t)(uint16_t)val_one;
    }
    printf("  Weights: %dx%d all-ones kernel (%d elements)\n",
           TEST_KERNEL, TEST_KERNEL, total_weights);

    /* ---- Prepare bias: 0.0 ---- */
    ram_buf[RAM_BIAS_BASE] = (uint32_t)(uint16_t)float_to_q8_8(0.0f);
    printf("  Bias: 0.0\n");

    /* ---- Configure accelerator BEFORE DMA write so ST Sink is ready ---- */
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

    /* ---- Send RAM image to FPGA via /dev/msgdma ---- */
    uint32_t total_bytes = RAM_TOTAL_WORDS * 4;
    printf("  Sending %d words (%u bytes) via /dev/msgdma...\n",
           RAM_TOTAL_WORDS, total_bytes);

    ssize_t nw = write(fd_dma, ram_buf, total_bytes);
    if (nw < 0) {
        perror("  write /dev/msgdma");
        FAIL("DMA write failed");
        tests_failed++;
        goto cleanup;
    }
    if ((uint32_t)nw != total_bytes) {
        printf("  Short write: %zd / %u bytes\n", nw, total_bytes);
        FAIL("DMA write incomplete");
        tests_failed++;
        goto cleanup;
    }
    printf("  DMA send complete (%u bytes)\n", total_bytes);

    /* ---- Start convolution ---- */
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

    /* Wait for completion */
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

    /* ---- Read results back from FPGA via /dev/msgdma ---- */
    printf("  Reading results via /dev/msgdma...\n");

    uint32_t *output_buf = (uint32_t *)calloc(RAM_TOTAL_WORDS, sizeof(uint32_t));
    if (!output_buf) {
        FAIL("Could not allocate output buffer");
        tests_failed++;
        goto cleanup;
    }

    ssize_t nr = read(fd_dma, output_buf, total_bytes);
    if (nr < 0) {
        perror("  read /dev/msgdma");
        FAIL("DMA read failed");
        tests_failed++;
        free(output_buf);
        goto cleanup;
    }
    if ((uint32_t)nr != total_bytes) {
        printf("  Short read: %zd / %u bytes\n", nr, total_bytes);
        FAIL("DMA read incomplete");
        tests_failed++;
        free(output_buf);
        goto cleanup;
    }
    printf("  DMA recv complete (%u bytes)\n", total_bytes);

    /* ---- Raw memory dump for debugging ---- */
    printf("\n  === RAW MEMORY DUMP ===\n");

    printf("  Input area [0..7]:");
    for (int i = 0; i < 8; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    printf("  Weight area [10000..10011]:");
    for (int i = 10000; i < 10012; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    printf("  Bias area [13000..13003]:");
    for (int i = 13000; i < 13004; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    printf("  Output area [13016..13031] (first 16 outputs):\n    ");
    for (int i = 13016; i < 13032; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    printf("  Output area [13016+990..+1005] (row 10, col 10 region):\n    ");
    for (int i = 13016+990; i < 13016+1006; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

    printf("  Output area [13016+9590..+9605] (last outputs):\n    ");
    for (int i = 13016+9590; i < 13016+9606; i++)
        printf(" %08X", output_buf[i]);
    printf("\n");

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
    float expected_val = 9.0f;
    printf("  Checking output values (expected %.1f for interior positions):\n", expected_val);

    int check_positions[][2] = { {10, 10}, {50, 50}, {90, 90}, {1, 1}, {97, 97} };
    int num_checks = sizeof(check_positions) / sizeof(check_positions[0]);
    int output_ok = 1;

    for (int c = 0; c < num_checks; c++) {
        int oh = check_positions[c][0];
        int ow = check_positions[c][1];
        int out_idx = oh * TEST_OUTPUT_W + ow;

        uint32_t word = output_buf[RAM_OUTPUT_BASE + out_idx];
        int16_t val = (int16_t)(word & 0xFFFF);

        float fval = q8_8_to_float(val);
        printf("    output[%d][%d] (idx=%d) = %.4f (raw=0x%04X, expected=%.1f)\n",
               oh, ow, out_idx, fval, (uint16_t)val, expected_val);

        if (fval < expected_val - 0.5f || fval > expected_val + 0.5f)
            output_ok = 0;
    }

    check(output_ok, "Convolution output values correct");

    free(output_buf);

cleanup:
    free(ram_buf);
    close(fd_dma);
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
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("  Neurax BSP Integration Test\n");
    printf("========================================\n");

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
    dump_registers(&bsp);
    test_convolution(&bsp);

    /* Summary */
    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n");

    /* Cleanup */
    neurax_bsp_deinit(&bsp);

    return (tests_failed > 0) ? 1 : 0;
}
