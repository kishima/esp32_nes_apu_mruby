#ifndef NSF_H
#define NSF_H

#include <stdint.h>
#include <stdbool.h>

// NSF Header Format (128 bytes)
typedef struct {
    char id[5];           // "NESM\x1A"
    uint8_t version;      // Version number (currently 1)
    uint8_t total_songs;  // Total number of songs
    uint8_t starting_song;// Starting song number (1-based)
    uint16_t load_addr;   // Load address ($8000-$FFFF)
    uint16_t init_addr;   // Init address ($8000-$FFFF)
    uint16_t play_addr;   // Play address ($8000-$FFFF)
    char song_name[32];   // Song name (null-terminated)
    char artist[32];      // Artist name (null-terminated)
    char copyright[32];   // Copyright (null-terminated)
    uint16_t ntsc_speed;  // NTSC playback speed (1/1000000 seconds)
    uint8_t bank_switch[8]; // Initial bank values
    uint16_t pal_speed;   // PAL playback speed
    uint8_t pal_ntsc;     // PAL/NTSC flags
    uint8_t extra_chip;   // Extra sound chip support
    uint8_t expansion[4]; // Expansion bytes (should be 0)
} NSFHeader;

// NSF Player Context
typedef struct {
    NSFHeader header;
    uint8_t *data;        // NSF data (loaded at load_addr)
    uint32_t data_size;   // Size of NSF data
    uint8_t current_song; // Currently playing song
    bool is_loaded;       // NSF loaded flag
    bool is_playing;      // Playing status
} NSFPlayer;

// Function prototypes
bool nsf_load(NSFPlayer *player, const char *filename);
bool nsf_init(NSFPlayer *player, uint8_t song_num);
void nsf_play(NSFPlayer *player);
void nsf_free(NSFPlayer *player);
void nsf_print_info(NSFPlayer *player);

#endif // NSF_H