#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nsf.h"
#include "cpu6502.h"

// NSF signature
static const char NSF_MAGIC[] = "NESM\x1A";

bool nsf_load(NSFPlayer *player, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return false;
    }
    
    // Read header
    size_t read_size = fread(&player->header, 1, sizeof(NSFHeader), file);
    if (read_size != sizeof(NSFHeader)) {
        printf("Error: Invalid NSF header size\n");
        fclose(file);
        return false;
    }
    
    // Verify magic
    if (memcmp(player->header.id, NSF_MAGIC, 5) != 0) {
        printf("Error: Invalid NSF signature\n");
        fclose(file);
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    player->data_size = file_size - sizeof(NSFHeader);
    
    // Read NSF data
    player->data = (uint8_t*)malloc(player->data_size);
    if (!player->data) {
        printf("Error: Cannot allocate memory for NSF data\n");
        fclose(file);
        return false;
    }
    
    fseek(file, sizeof(NSFHeader), SEEK_SET);
    read_size = fread(player->data, 1, player->data_size, file);
    if (read_size != player->data_size) {
        printf("Error: Cannot read NSF data\n");
        free(player->data);
        fclose(file);
        return false;
    }
    
    fclose(file);
    
    // Fix byte order for 16-bit values (little endian)
    player->header.load_addr = player->header.load_addr;
    player->header.init_addr = player->header.init_addr;
    player->header.play_addr = player->header.play_addr;
    player->header.ntsc_speed = player->header.ntsc_speed;
    player->header.pal_speed = player->header.pal_speed;
    
    player->is_loaded = true;
    player->current_song = player->header.starting_song;
    
    printf("NSF loaded successfully\n");
    return true;
}

bool nsf_init(NSFPlayer *player, uint8_t song_num) {
    if (!player->is_loaded) {
        printf("Error: NSF not loaded\n");
        return false;
    }
    
    if (song_num < 1 || song_num > player->header.total_songs) {
        printf("Error: Invalid song number %d (1-%d)\n", song_num, player->header.total_songs);
        return false;
    }
    
    player->current_song = song_num;
    
    printf("Initializing song %d\n", song_num);
    
    // The actual initialization will be done by CPU emulator
    // Set up CPU registers for INIT call
    // A = song number - 1
    // X = PAL/NTSC flag
    
    return true;
}

void nsf_play(NSFPlayer *player) {
    if (!player->is_loaded) {
        printf("Error: NSF not loaded\n");
        return;
    }
    
    if (!player->is_playing) {
        printf("NSF playback not started\n");
        return;
    }
    
    // The actual play routine will be called by CPU emulator
    printf("Playing frame\n");
}

void nsf_free(NSFPlayer *player) {
    if (player->data) {
        free(player->data);
        player->data = NULL;
    }
    player->is_loaded = false;
    player->is_playing = false;
}

void nsf_print_info(NSFPlayer *player) {
    if (!player->is_loaded) {
        printf("No NSF loaded\n");
        return;
    }
    
    printf("=== NSF Information ===\n");
    printf("Version:     %d\n", player->header.version);
    printf("Songs:       %d\n", player->header.total_songs);
    printf("Start Song:  %d\n", player->header.starting_song);
    printf("Load Addr:   $%04X\n", player->header.load_addr);
    printf("Init Addr:   $%04X\n", player->header.init_addr);
    printf("Play Addr:   $%04X\n", player->header.play_addr);
    printf("Song Name:   %.32s\n", player->header.song_name);
    printf("Artist:      %.32s\n", player->header.artist);
    printf("Copyright:   %.32s\n", player->header.copyright);
    printf("NTSC Speed:  %d\n", player->header.ntsc_speed);
    printf("PAL Speed:   %d\n", player->header.pal_speed);
    printf("PAL/NTSC:    $%02X\n", player->header.pal_ntsc);
    printf("Extra Chips: $%02X\n", player->header.extra_chip);
    
    if (player->header.bank_switch[0] || player->header.bank_switch[1] ||
        player->header.bank_switch[2] || player->header.bank_switch[3] ||
        player->header.bank_switch[4] || player->header.bank_switch[5] ||
        player->header.bank_switch[6] || player->header.bank_switch[7]) {
        printf("Bank Switch: ");
        for (int i = 0; i < 8; i++) {
            printf("$%02X ", player->header.bank_switch[i]);
        }
        printf("\n");
    }
    printf("=====================\n");
}