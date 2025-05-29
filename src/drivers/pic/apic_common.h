#pragma once

#include <stdint.h>

struct LAPICNMIRedirectionInfo {
    uint8_t lint;
    bool is_level_triggered;
    bool is_active_low;
    bool is_valid;
};

extern struct LAPICNMIRedirectionInfo LAPIC_NMI_REDIRECTION;
