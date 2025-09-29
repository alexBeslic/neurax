#include <cassert>
#include "neurax/bsp/neurax_bsp.h"

int main(int argc, char const *argv[])
{
    uint32_t value = 0;
    neurax_bsp_init();
    neurax_bsp_read_reg(NEURAX_READ_ONLY_OFFSET, &value);

    printf("Read NEURAX ID register: 0x%08X\n", value);

    // Check if the read value matches the expected ID
    assert(value == NEURAX_READ_ONLY_ID);

    neurax_bsp_deinit();
    return 0;
}
