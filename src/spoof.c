#include "spoof.h"
#include "mem.h"
#include "vmi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static addr_t original_offset = 0;
static addr_t current_offset = 0;
static bool spoofer_active = false;

#define KERNEL_TEXT_START      0xffffffff81000000
#define KERNEL_TEXT_END        0xffffffffa2000000
#define KASLR_MASK            0x3fffffff
#define KASLR_ALIGNMENT       0x200000
#define MAX_KASLR_OFFSET      0x40000000

bool init_spoofer(vmi_instance_t vmi, spoof_config_t *config) {
    if (!config) return false;
    srand(time(NULL));
    original_offset = get_kaslr_offset(vmi);
    if (!original_offset) return false;
    current_offset = original_offset;
    spoofer_active = true;
    return true;
}

bool spoof_kaslr_offset(vmi_instance_t vmi, addr_t new_offset) {
    if (!spoofer_active) return false;
    if (!validate_kaslr_offset(new_offset)) return false;

    addr_t kernel_base = get_kernel_base(vmi);
    if (!kernel_base) return false;

    addr_t real_kernel_text = kernel_base + new_offset;
    if (!write_memory(vmi, kernel_base + KERNEL_TEXT_START - KERNEL_TEXT_END, &real_kernel_text, sizeof(real_kernel_text))) {
        return false;
    }

    current_offset = new_offset;
    return true;
}

bool restore_original_offset(vmi_instance_t vmi) {
    if (!spoofer_active || !original_offset) return false;
    bool result = spoof_kaslr_offset(vmi, original_offset);
    if (result) {
        spoofer_active = false;
    }
    return result;
}

bool validate_kaslr_offset(addr_t offset) {
    if (offset > MAX_KASLR_OFFSET) return false;
    if (offset & ~KASLR_MASK) return false;
    if (offset & (KASLR_ALIGNMENT - 1)) return false;
    return true;
}

bool is_spoofing_active(void) {
    return spoofer_active;
}

addr_t generate_random_offset(void) {
    addr_t offset;
    do {
        offset = (addr_t)rand() & KASLR_MASK;
        offset &= ~(KASLR_ALIGNMENT - 1);
    } while (!validate_kaslr_offset(offset));
    return offset;
}

addr_t get_current_spoofed_offset(void) {
    return current_offset;
}
