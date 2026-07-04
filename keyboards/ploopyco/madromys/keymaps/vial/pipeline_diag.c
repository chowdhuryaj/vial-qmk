#include "pipeline_diag.h"

static uint32_t diag_last_tick = 0;
static uint16_t diag_max_gap   = 0;

void pipeline_diag_tick(void) {
    uint32_t now = timer_read32();
    if (diag_last_tick != 0) {
        uint32_t gap = now - diag_last_tick;
        if (gap > 0xFFFF) gap = 0xFFFF;
        if ((uint16_t)gap > diag_max_gap) diag_max_gap = (uint16_t)gap;
    }
    diag_last_tick = now;
}

uint16_t pipeline_diag_max_gap_ms(void) {
    return diag_max_gap;
}

void pipeline_diag_reset(void) {
    diag_max_gap = 0;
}

uint16_t pipeline_diag_uptime_s(void) {
    return (uint16_t)(timer_read32() / 1000); // wraps at ~18 h; fine for "did it just reboot?"
}
