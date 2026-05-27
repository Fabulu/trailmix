#ifndef SAVE_H
#define SAVE_H

#include <nds/ndstypes.h>

// Save file layout v1 (138 bytes, stored on SD via libfat)
struct SaveData {
    u32 magic;              // 0x00  "TRMX" = 0x584D5254
    u8  version;            // 0x04  = 1
    u8  language;           // 0x05  0=EN, 1=DE
    u8  bestWave;           // 0x06
    u16 totalRuns;          // 0x07  (serialized as 2 bytes, not aligned)
    u8  sayingsBits[112];   // 0x09  2 bits per master x 448 masters
    u8  reserved[16];       // 0x79  future expansion
    u8  checksum;           // 0x89  XOR of bytes 0x00..0x88
};
// Total: 138 bytes

void saveInit();            // init libfat, read save file, validate, populate gSayings
void saveWrite();           // serialize current state to SD card
bool saveIsValid();         // was save data valid on last load?
u8   saveGetLanguage();     // 0xFF if no valid save
u8   saveGetBestWave();

#endif // SAVE_H
