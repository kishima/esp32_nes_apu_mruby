/* HID Server Stub Implementation
 * Temporary stub functions for when Bluetooth HID is disabled
 */

#include <stdio.h>
#include <stdint.h>

extern "C" {

// Stub implementations for HID functions
int hid_init(const char* local_name) {
    printf("HID init (stubbed): %s\n", local_name);
    return 0; // Return success
}

int hid_update() {
    // No-op for stub
    return 0;
}

int hid_close() {
    printf("HID close (stubbed)\n");
    return 0;
}

int hid_get(uint8_t* dst, int dst_len) {
    // No-op for stub, return no data
    return 0;
}

// gui_msg is implemented in gui.cpp

// Stub wii state structures
typedef struct {
    uint32_t flags;
    uint8_t report[32];
    uint16_t common()   { return 0; }
    uint16_t classic()  { return 0; }
} wii_state;

wii_state wii_states[4] = {0};

uint32_t wii_map(int index, const uint32_t* common, const uint32_t* classic) {
    return 0; // No input for stub
}

} // extern "C"