#include "vmi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char error_buffer[256];

bool init_vmi(vmi_instance_t *vmi, char *name) {
    if (!name) {
        snprintf(error_buffer, sizeof(error_buffer), "VM name is NULL");
        return false;
    }

    if (vmi_init(vmi, VMI_XEN, name, VMI_INIT_COMPLETE | VMI_INIT_EVENTS, 
                 NULL, NULL) == VMI_FAILURE) {
        snprintf(error_buffer, sizeof(error_buffer), "Failed to initialize VMI");
        return false;
    }

    return true;
}

void cleanup_vmi(vmi_instance_t *vmi) {
    if (vmi) {
        vmi_destroy(*vmi);
    }
}

addr_t get_kernel_base(vmi_instance_t vmi) {
    addr_t kernel_base = 0;
    
    if (vmi_translate_ksym2v(vmi, "_text", &kernel_base) == VMI_SUCCESS) {
        return kernel_base;
    }

    addr_t dtb = 0;
    if (vmi_get_dtb(vmi, &dtb) == VMI_SUCCESS) {
        // Search for kernel base in high memory region
        // This is a simplified approach and might need adjustment
        addr_t addr = 0xffffffff80000000;
        while (addr < 0xffffffffa0000000) {
            uint8_t buf[2] = {0};
            if (vmi_read_8_va(vmi, addr, 0, buf) == VMI_SUCCESS) {
                if (buf[0] == 0x48 && buf[1] == 0x8d) {
                    kernel_base = addr;
                    break;
                }
            }
            addr += 0x1000;
        }
    }

    if (!kernel_base) {
        snprintf(error_buffer, sizeof(error_buffer), "Failed to find kernel base");
    }

    return kernel_base;
}

addr_t get_kaslr_offset(vmi_instance_t vmi) {
    addr_t kernel_base = get_kernel_base(vmi);
    if (!kernel_base) {
        return 0;
    }

    return kernel_base - 0xffffffff81000000;
}

const char* vmi_get_last_error(void) {
    return error_buffer;
}
