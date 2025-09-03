/*
 * NEURAX Neural Network Accelerator Library
 * Core Implementation
 * 
 * Author: NEURAX Team
 */

#define _GNU_SOURCE
#include "neurax.h"
#include "neurax_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

// Version string
static const char* version_string = "NEURAX v1.0.0";

// Error strings
static const char* error_strings[] = {
    "Success",
    "Invalid parameter",
    "Not initialized",
    "Device not found",
    "Memory allocation failed",
    "Hardware failure",
    "Timeout",
    "Invalid model",
    "Buffer overflow"
};

// Device paths
#define NEURAX_DEVICE_PATH "/dev/neurax0"
#define NEURAX_UIO_PATH "/dev/uio0"

// Forward declarations
static neurax_error_t neurax_device_open(neurax_device_t* device);
static neurax_error_t neurax_device_close(neurax_device_t* device);

// Core API implementation

neurax_error_t neurax_init(const neurax_config_t* config, neurax_device_t** device) {
    if (!config || !device) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    // Allocate device structure
    neurax_device_t* dev = calloc(1, sizeof(neurax_device_t));
    if (!dev) {
        return NEURAX_ERROR_MEMORY_ALLOCATION;
    }
    
    // Copy configuration
    memcpy(&dev->config, config, sizeof(neurax_config_t));
    dev->initialized = false;
    dev->device_fd = -1;
    dev->mapped_memory = NULL;
    dev->register_base = NULL;
    
    // Open device
    neurax_error_t error = neurax_device_open(dev);
    if (error != NEURAX_SUCCESS) {
        free(dev);
        return error;
    }
    
    // Reset hardware
    NEURAX_WRITE_REG(dev, NEURAX_REG_CONTROL, CTRL_RESET);
    usleep(1000); // Wait 1ms
    NEURAX_WRITE_REG(dev, NEURAX_REG_CONTROL, 0);
    
    dev->initialized = true;
    *device = dev;
    
    printf("NEURAX: Device initialized successfully\n");
    printf("NEURAX: Hardware acceleration %s\n", 
           dev->hardware_available ? "enabled" : "disabled (using CPU emulation)");
    
    return NEURAX_SUCCESS;
}

neurax_error_t neurax_cleanup(neurax_device_t* device) {
    if (!device) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (device->initialized) {
        // Reset hardware
        if (device->hardware_available) {
            NEURAX_WRITE_REG(device, NEURAX_REG_CONTROL, CTRL_RESET);
        }
        
        neurax_device_close(device);
        device->initialized = false;
    }
    
    free(device);
    return NEURAX_SUCCESS;
}

const char* neurax_get_version(void) {
    return version_string;
}

const char* neurax_get_error_string(neurax_error_t error) {
    int error_index = -error; // Convert negative error code to positive index
    if (error_index >= 0 && error_index < sizeof(error_strings)/sizeof(error_strings[0])) {
        return error_strings[error_index];
    }
    return "Unknown error";
}

// Tensor management

neurax_error_t neurax_tensor_create(uint32_t width, uint32_t height, 
                                   uint32_t channels, uint32_t batch_size,
                                   neurax_data_type_t data_type,
                                   neurax_tensor_t** tensor) {
    if (!tensor || width == 0 || height == 0 || channels == 0 || batch_size == 0) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    neurax_tensor_t* t = calloc(1, sizeof(neurax_tensor_t));
    if (!t) {
        return NEURAX_ERROR_MEMORY_ALLOCATION;
    }
    
    t->width = width;
    t->height = height;
    t->channels = channels;
    t->batch_size = batch_size;
    t->data_type = data_type;
    
    // Calculate data size based on type
    size_t element_size;
    switch (data_type) {
        case NEURAX_DATA_UINT8:
        case NEURAX_DATA_INT8:
            element_size = 1;
            break;
        case NEURAX_DATA_UINT16:
        case NEURAX_DATA_INT16:
            element_size = 2;
            break;
        case NEURAX_DATA_FLOAT32:
            element_size = 4;
            break;
        default:
            free(t);
            return NEURAX_ERROR_INVALID_PARAM;
    }
    
    t->data_size = width * height * channels * batch_size * element_size;
    t->data = calloc(1, t->data_size);
    if (!t->data) {
        free(t);
        return NEURAX_ERROR_MEMORY_ALLOCATION;
    }
    
    *tensor = t;
    return NEURAX_SUCCESS;
}

neurax_error_t neurax_tensor_destroy(neurax_tensor_t* tensor) {
    if (!tensor) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (tensor->data) {
        free(tensor->data);
    }
    free(tensor);
    
    return NEURAX_SUCCESS;
}

neurax_error_t neurax_tensor_set_data(neurax_tensor_t* tensor, const void* data, size_t size) {
    if (!tensor || !data) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (size > tensor->data_size) {
        return NEURAX_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(tensor->data, data, size);
    return NEURAX_SUCCESS;
}

neurax_error_t neurax_tensor_get_data(const neurax_tensor_t* tensor, void* data, size_t size) {
    if (!tensor || !data) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    if (size > tensor->data_size) {
        return NEURAX_ERROR_BUFFER_OVERFLOW;
    }
    
    memcpy(data, tensor->data, size);
    return NEURAX_SUCCESS;
}

// Private helper functions

neurax_error_t neurax_device_open(neurax_device_t* device) {
    // Try to open hardware device first
    device->device_fd = open(NEURAX_DEVICE_PATH, O_RDWR);
    if (device->device_fd < 0) {
        // Try UIO device
        device->device_fd = open(NEURAX_UIO_PATH, O_RDWR);
        if (device->device_fd < 0) {
            printf("NEURAX: Hardware device not found, using CPU emulation\n");
            device->hardware_available = false;
            return NEURAX_SUCCESS;
        }
    }
    
    // Map device memory
    device->mapped_size = device->config.memory_size;
    if (device->mapped_size == 0) {
        device->mapped_size = 0x10000; // Default 64KB
    }
    
    device->mapped_memory = mmap(NULL, device->mapped_size, 
                                PROT_READ | PROT_WRITE, MAP_SHARED,
                                device->device_fd, 0);
    
    if (device->mapped_memory == MAP_FAILED) {
        close(device->device_fd);
        device->device_fd = -1;
        printf("NEURAX: Failed to map device memory, using CPU emulation\n");
        device->hardware_available = false;
        return NEURAX_SUCCESS;
    }
    
    device->register_base = (uint32_t*)device->mapped_memory;
    device->hardware_available = true;
    
    return NEURAX_SUCCESS;
}

static neurax_error_t neurax_device_close(neurax_device_t* device) {
    if (device->mapped_memory && device->mapped_memory != MAP_FAILED) {
        munmap(device->mapped_memory, device->mapped_size);
        device->mapped_memory = NULL;
    }
    
    if (device->device_fd >= 0) {
        close(device->device_fd);
        device->device_fd = -1;
    }
    
    device->hardware_available = false;
    return NEURAX_SUCCESS;
}

void neurax_write_reg(neurax_device_t* device, uint32_t offset, uint32_t value) {
    if (device->hardware_available && device->register_base) {
        device->register_base[offset / 4] = value;
    }
    // For CPU emulation, we would implement register emulation here
}

uint32_t neurax_read_reg(neurax_device_t* device, uint32_t offset) {
    if (device->hardware_available && device->register_base) {
        return device->register_base[offset / 4];
    }
    // For CPU emulation, we would implement register emulation here
    return 0;
}

neurax_error_t neurax_wait_for_completion(neurax_device_t* device, uint32_t timeout_ms) {
    if (!device->hardware_available) {
        return NEURAX_SUCCESS; // CPU emulation is assumed to complete immediately
    }
    
    uint32_t elapsed = 0;
    const uint32_t poll_interval_us = 100; // 100 microseconds
    
    while (elapsed < timeout_ms * 1000) {
        uint32_t status = neurax_read_reg(device, NEURAX_REG_STATUS);
        
        if (status & STAT_ERROR) {
            return NEURAX_ERROR_HARDWARE_FAILURE;
        }
        
        if (status & STAT_DONE) {
            return NEURAX_SUCCESS;
        }
        
        usleep(poll_interval_us);
        elapsed += poll_interval_us;
    }
    
    return NEURAX_ERROR_TIMEOUT;
}

neurax_error_t neurax_print_device_info(neurax_device_t* device) {
    if (!device) {
        return NEURAX_ERROR_INVALID_PARAM;
    }
    
    printf("NEURAX Device Information:\n");
    printf("==========================\n");
    printf("Version: %s\n", neurax_get_version());
    printf("Hardware acceleration: %s\n", device->hardware_available ? "Yes" : "No (CPU emulation)");
    printf("Base address: 0x%08X\n", device->config.base_address);
    printf("Memory size: %u bytes\n", device->config.memory_size);
    printf("Max kernel size: %u\n", device->config.max_kernel_size);
    printf("Data type: %d\n", device->config.data_type);
    printf("Initialized: %s\n", device->initialized ? "Yes" : "No");
    
    if (device->hardware_available) {
        uint32_t status = neurax_read_reg(device, NEURAX_REG_STATUS);
        printf("Hardware status: 0x%08X\n", status);
        printf("  Busy: %s\n", (status & STAT_BUSY) ? "Yes" : "No");
        printf("  Done: %s\n", (status & STAT_DONE) ? "Yes" : "No");
        printf("  Error: %s\n", (status & STAT_ERROR) ? "Yes" : "No");
    }
    
    return NEURAX_SUCCESS;
}
