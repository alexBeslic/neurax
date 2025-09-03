/*
 * NEURAX Performance Profiling
 * 
 * Author: NEURAX Team
 */

#include "neurax_private.h"
#include <sys/time.h>
#include <stdio.h>

static struct timeval start_time;
static bool profiling_active = false;

// Start performance measurement
neurax_error_t neurax_perf_start(neurax_perf_stats_t* stats) {
    if (!stats) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    // Initialize stats
    stats->total_time_ms = 0.0;
    stats->hw_time_ms = 0.0;
    stats->data_transfer_time_ms = 0.0;
    stats->num_operations = 0;
    
    // Record start time
    gettimeofday(&start_time, NULL);
    profiling_active = true;
    
    return NEURAX_SUCCESS;
}

// End performance measurement
neurax_error_t neurax_perf_end(neurax_perf_stats_t* stats) {
    if (!stats || !profiling_active) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    // Calculate elapsed time in milliseconds
    double elapsed_sec = (end_time.tv_sec - start_time.tv_sec) + 
                        (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    stats->total_time_ms = elapsed_sec * 1000.0;
    profiling_active = false;
    
    return NEURAX_SUCCESS;
}

// Print performance statistics
void neurax_perf_print(const neurax_perf_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    printf("\nNEURAX Performance Statistics:\n");
    printf("==============================\n");
    printf("Total execution time:    %.3f ms\n", stats->total_time_ms);
    printf("Hardware time:           %.3f ms\n", stats->hw_time_ms);
    printf("Data transfer time:      %.3f ms\n", stats->data_transfer_time_ms);
    printf("Number of operations:    %u\n", stats->num_operations);
    
    if (stats->num_operations > 0) {
        printf("Average time per op:     %.3f ms\n", 
               stats->total_time_ms / stats->num_operations);
    }
    
    if (stats->total_time_ms > 0.0) {
        printf("Hardware utilization:    %.1f%%\n", 
               (stats->hw_time_ms / stats->total_time_ms) * 100.0);
        printf("Data transfer overhead:  %.1f%%\n", 
               (stats->data_transfer_time_ms / stats->total_time_ms) * 100.0);
    }
    
    // Calculate throughput estimates
    if (stats->total_time_ms > 0.0) {
        printf("Operations per second:   %.0f\n", 
               (stats->num_operations * 1000.0) / stats->total_time_ms);
    }
    
    printf("==============================\n\n");
}
