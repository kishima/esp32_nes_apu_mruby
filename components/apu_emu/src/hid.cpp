#include <string.h>

extern "C" {
    void hid_init() {
    }

    void hid_update() {
    }

    int hid_get(unsigned char* buf, int len) {
        return 0;
    }

    void wii_map(unsigned char* buf) {
    }

    void sys_get_pref(const char* key, char* buf, int len) {
        buf[0] = 0;
    }

    void sys_set_pref(const char* key, const char* value) {
    }

    void ir_sample() {
    }

    int get_hid_ir(unsigned char* buf) {
        return 0;
    }
}