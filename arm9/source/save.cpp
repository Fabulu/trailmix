#include "save.h"
#include "sayings.h"
#include "strings.h"   // for gActiveLang, StrLang
#include "game.h"      // for gameGetWave()
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>    // for memset

static const char SAVE_PATH[] = "fat:/trailmix.sav";

static const u32 SAVE_MAGIC   = 0x584D5254;  // "TRMX" little-endian
static const u8  SAVE_VERSION = 1;
static const int SAVE_SIZE    = 138;

static bool sFatOk   = false;
static bool sValid   = false;
static u8   sSavedLanguage = 0xFF;
static u8   sSavedBestWave = 0;

// ---------------------------------------------------------------------------
// XOR checksum over bytes 0x00..0x88 (137 bytes), stored at byte 0x89
// ---------------------------------------------------------------------------
static u8 computeChecksum(const u8* data, int len) {
    u8 cs = 0;
    for (int i = 0; i < len; i++) cs ^= data[i];
    return cs;
}

// ---------------------------------------------------------------------------
// saveInit -- init libfat, read save file, validate magic + version + checksum
// ---------------------------------------------------------------------------
void saveInit() {
    // Saves disabled — fatInitDefault() crashes on some R4 hardware.
    // Sayings collection persists within a session but not across power cycles.
    // TODO: find a save method that works on both melonDS and real R4.
    sFatOk = false;
    sValid = false;
    return;

    FILE* f = fopen(SAVE_PATH, "rb");
    if (!f) { sValid = false; return; }

    u8 buf[SAVE_SIZE];
    size_t bytesRead = fread(buf, 1, SAVE_SIZE, f);
    fclose(f);

    if (bytesRead != SAVE_SIZE) { sValid = false; return; }

    // Check magic (little-endian: T=0x54, R=0x52, M=0x4D, X=0x58)
    u32 magic = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    if (magic != SAVE_MAGIC) { sValid = false; return; }

    // Check version
    if (buf[4] != SAVE_VERSION) { sValid = false; return; }

    // Check checksum
    u8 expected = computeChecksum(buf, 137);
    if (buf[137] != expected) { sValid = false; return; }

    sValid = true;

    // Cache language and bestWave for getters
    sSavedLanguage = buf[5];
    sSavedBestWave = buf[6];

    // Do NOT override gActiveLang here — the player's fresh language
    // selection from languageSelect() takes priority over saved preference.

    // Restore sayings bits from save into gSayings
    sayingsLoadFromSave(&buf[9], 112);
}

// ---------------------------------------------------------------------------
// saveWrite -- serialize current state to SD card
// ---------------------------------------------------------------------------
void saveWrite() {
    if (!sFatOk) return;  // No SD card available, skip silently

    u8 buf[SAVE_SIZE];
    memset(buf, 0, SAVE_SIZE);

    // Magic: "TRMX" little-endian
    buf[0] = 0x54; buf[1] = 0x52; buf[2] = 0x4D; buf[3] = 0x58;

    // Version
    buf[4] = SAVE_VERSION;

    // Language
    buf[5] = static_cast<u8>(gActiveLang);

    // Best wave (keep highest of current run vs saved)
    u8 wave = static_cast<u8>(gameGetWave());
    buf[6] = (wave > sSavedBestWave) ? wave : sSavedBestWave;

    // Total runs (increment from last known value)
    u16 runs = 0;
    if (sValid) {
        // Read previous totalRuns from the save file
        FILE* fr = fopen(SAVE_PATH, "rb");
        if (fr) {
            u8 lo = 0, hi = 0;
            fseek(fr, 7, SEEK_SET);
            fread(&lo, 1, 1, fr);
            fread(&hi, 1, 1, fr);
            fclose(fr);
            runs = lo | (hi << 8);
        }
    }
    runs++;
    buf[7] = runs & 0xFF;
    buf[8] = (runs >> 8) & 0xFF;

    // Sayings bits (112 bytes at offset 9)
    sayingsSaveInto(&buf[9]);

    // Reserved area (bytes 0x79..0x88) already zeroed by memset

    // Checksum: XOR of bytes 0x00..0x88 (137 bytes), stored at 0x89
    buf[137] = computeChecksum(buf, 137);

    // Write to SD card
    FILE* f = fopen(SAVE_PATH, "wb");
    if (f) {
        fwrite(buf, 1, SAVE_SIZE, f);
        fclose(f);
        sValid = true;

        // Update cached values
        sSavedLanguage = buf[5];
        sSavedBestWave = buf[6];
    }
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------
bool saveIsValid() {
    return sValid;
}

u8 saveGetLanguage() {
    return sValid ? sSavedLanguage : 0xFF;
}

u8 saveGetBestWave() {
    return sValid ? sSavedBestWave : 0;
}
