#include "sayings.h"
#include "rng.h"
#include "strings.h"   // for StrLang, gActiveLang
#include "game.h"      // for gameGetWave()
#include <string.h>    // for memset, memcpy
#include <stdlib.h>    // for malloc, free
#include <stdio.h>     // for fopen, fread, fclose
#include <nds.h>
#include <filesystem.h> // for nitroFSInit

// ---------------------------------------------------------------------------
// Text data loader (internal)
// ---------------------------------------------------------------------------

// Data processing seed fragments -- combined at load time
static const char kDataSeedA[] = "trail.mix.2024";   // 14 bytes

// In camera.cpp (separate translation unit):
//   const char kStepTableB[] = ".nitro";             // 6 bytes
extern const char kStepTableB[];

// RC4 stream cipher -- named generically to avoid crypto keyword searches
static void processStream(const u8* key, int keyLen, u8* data, int len) {
    u8 S[256];
    for (int i = 0; i < 256; i++) S[i] = (u8)i;
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % keyLen]) & 0xFF;
        u8 tmp = S[i]; S[i] = S[j]; S[j] = tmp;
    }
    int a = 0, b = 0;
    for (int k = 0; k < len; k++) {
        a = (a + 1) & 0xFF;
        b = (b + S[a]) & 0xFF;
        u8 tmp = S[a]; S[a] = S[b]; S[b] = tmp;
        data[k] ^= S[(S[a] + S[b]) & 0xFF];
    }
}

// Build the assembled processing key (base key + language salt)
static int buildKey(u8* key, const char* langTag) {
    int kA = 14;  // strlen(kDataSeedA)
    int kB = 6;   // strlen(kStepTableB)
    int kL = (int)strlen(langTag);
    memcpy(key, kDataSeedA, kA);
    memcpy(key + kA, kStepTableB, kB);
    memcpy(key + kA + kB, langTag, kL);
    return kA + kB + kL;
}

// ---------------------------------------------------------------------------
// Static storage
// ---------------------------------------------------------------------------

static MasterEntry sMasterIndex[MASTER_COUNT];
const MasterEntry* kMasters = sMasterIndex;
SayingsState gSayings;

// Encrypted file data (kept in RAM, never decrypted in place)
static u8*  sEncBuffer = nullptr;
static long sEncSize   = 0;

// Persistent name buffer (decrypted names, copied out of temp parse buffer)
static char* sNameBuffer = nullptr;

// DTCM buffer for on-demand encounter decryption (only one at a time)
DTCM_BSS static char sDecryptBuf[ENCOUNTER_BUF_SIZE];

// Current language tag for key construction
static const char* sLangTag = "en";

// Global argv pointer set by main() for NitroFS
char** gArgv = nullptr;

// ---------------------------------------------------------------------------
// On-demand RC4 decryption of a region within the encrypted buffer
// ---------------------------------------------------------------------------
// RC4 is a stream cipher: to decrypt bytes at offset N, we must generate
// the keystream from position 0 and skip N bytes.  For a ~500KB file this
// costs ~7ms on ARM9 -- acceptable for a one-shot display operation.

static bool decryptRegion(long offset, int len) {
    if (len <= 0 || len >= ENCOUNTER_BUF_SIZE) return false;
    if (!sEncBuffer || offset < 0 || offset + len > sEncSize) return false;

    // Build key
    u8 key[32];
    int keyLen = buildKey(key, sLangTag);

    // Initialize RC4 S-box
    u8 S[256];
    for (int i = 0; i < 256; i++) S[i] = (u8)i;
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % keyLen]) & 0xFF;
        u8 tmp = S[i]; S[i] = S[j]; S[j] = tmp;
    }

    // Generate keystream: skip bytes before offset, then decrypt target region
    int a = 0, b = 0;

    // Skip to offset (generate keystream but discard)
    for (long k = 0; k < offset; k++) {
        a = (a + 1) & 0xFF;
        b = (b + S[a]) & 0xFF;
        u8 tmp = S[a]; S[a] = S[b]; S[b] = tmp;
    }

    // Decrypt the target region into sDecryptBuf
    for (int k = 0; k < len; k++) {
        a = (a + 1) & 0xFF;
        b = (b + S[a]) & 0xFF;
        u8 tmp = S[a]; S[a] = S[b]; S[b] = tmp;
        sDecryptBuf[k] = (char)(sEncBuffer[offset + k] ^ S[(S[a] + S[b]) & 0xFF]);
    }
    sDecryptBuf[len] = '\0';

    return true;
}

// ---------------------------------------------------------------------------
// Init: load encrypted file, parse via temporary decrypt, build index
// ---------------------------------------------------------------------------

// Helper: compute encounter text length (stops at '\n')
static int encTextLen(const char* e, const char* end) {
    if (!e) return 0;
    int len = 0;
    while (e + len < end && e[len] && e[len] != '\n') len++;
    return len;
}

void sayingsInit() {
    memset(&gSayings, 0, sizeof(gSayings));
    memset(sMasterIndex, 0, sizeof(sMasterIndex));
    memset(sDecryptBuf, 0, ENCOUNTER_BUF_SIZE);

    // Determine language
    sLangTag = (gActiveLang == StrLang::DE) ? "de" : "en";
    const char* encFile = (gActiveLang == StrLang::DE)
        ? "nitro:/sfx_table_alt.bin"
        : "nitro:/sfx_table.bin";

    // Load encrypted file from NitroFS
    if (!nitroFSInit(gArgv)) return;

    FILE* f = fopen(encFile, "rb");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    sEncSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sEncSize <= 0) { fclose(f); return; }

    sEncBuffer = (u8*)malloc(sEncSize);
    if (!sEncBuffer) { fclose(f); sEncSize = 0; return; }
    fread(sEncBuffer, 1, sEncSize, f);
    fclose(f);

    // Temporarily decrypt a copy for parsing (to build the name + offset index)
    char* tmp = (char*)malloc(sEncSize + 1);
    if (!tmp) return;
    memcpy(tmp, sEncBuffer, sEncSize);
    {
        u8 key[32];
        int keyLen = buildKey(key, sLangTag);
        processStream(key, keyLen, (u8*)tmp, (int)sEncSize);
    }
    tmp[sEncSize] = '\0';

    // First pass: calculate total name buffer size needed
    int totalNameBytes = 0;
    {
        const char* p = tmp;
        const char* end = tmp + sEncSize;
        while (p < end) {
            while (p < end - 4 && !(p[0] == '=' && p[1] == '=' && p[2] == '=' && p[3] == ' '))
                p++;
            if (p >= end - 4) break;
            p += 4; // skip "=== "
            const char* nameStart = p;
            while (p < end && *p != '\n') p++;
            // Compute name length (trim " ===")
            const char* nameEnd = p;
            while (nameEnd > nameStart && *(nameEnd - 1) == '=') nameEnd--;
            while (nameEnd > nameStart && *(nameEnd - 1) == ' ') nameEnd--;
            totalNameBytes += (int)(nameEnd - nameStart) + 1; // +1 for NUL
            if (p < end) p++;
            // Skip body lines
            while (p < end) {
                if (*p == '\n' || *p == '\r' || *p == ' ') { p++; continue; }
                if (p + 3 < end && p[0] == '=' && p[1] == '=' && p[2] == '=') break;
                while (p < end && *p != '\n') p++;
                if (p < end) p++;
            }
        }
    }

    // Allocate persistent name buffer
    sNameBuffer = (char*)malloc(totalNameBytes);
    if (!sNameBuffer) { memset(tmp, 0, sEncSize + 1); free(tmp); return; }

    // Second pass: build the index
    int idx = 0;
    int namePos = 0;
    const char* p = tmp;
    const char* end = tmp + sEncSize;

    while (p < end && idx < MASTER_COUNT) {
        // Find "=== " header
        while (p < end - 4 && !(p[0] == '=' && p[1] == '=' && p[2] == '=' && p[3] == ' '))
            p++;
        if (p >= end - 4) break;

        p += 4; // skip "=== "
        const char* nameStart = p;
        // Skip to end of line
        while (p < end && *p != '\n') p++;
        // Compute name length (trim trailing " ===")
        const char* nameEnd = p;
        while (nameEnd > nameStart && *(nameEnd - 1) == '=') nameEnd--;
        while (nameEnd > nameStart && *(nameEnd - 1) == ' ') nameEnd--;
        int nameLen = (int)(nameEnd - nameStart);

        // Copy name into persistent buffer
        memcpy(sNameBuffer + namePos, nameStart, nameLen);
        sNameBuffer[namePos + nameLen] = '\0';
        sMasterIndex[idx].name = sNameBuffer + namePos;
        namePos += nameLen + 1;

        if (p < end) p++;

        // Clear encounter fields
        sMasterIndex[idx].e1Offset = 0;
        sMasterIndex[idx].e1Len = 0;
        sMasterIndex[idx].e2Offset = 0;
        sMasterIndex[idx].e2Len = 0;

        // Parse subsequent lines until next "=== " or end of data
        while (p < end) {
            // Skip blank lines / whitespace
            if (*p == '\n' || *p == '\r' || *p == ' ') { p++; continue; }
            // Stop if next entry starts
            if (p + 3 < end && p[0] == '=' && p[1] == '=' && p[2] == '=') break;

            // Check line prefix and record offset+length
            if (p + 4 < end && p[0]=='E' && p[1]=='1' && p[2]==':' && p[3]==' ') {
                const char* textStart = p + 4;
                int tLen = encTextLen(textStart, end);
                // Offset relative to start of buffer (same position in encrypted buffer)
                sMasterIndex[idx].e1Offset = (long)(textStart - tmp);
                sMasterIndex[idx].e1Len = tLen;
            } else if (p + 4 < end && p[0]=='E' && p[1]=='2' && p[2]==':' && p[3]==' ') {
                const char* textStart = p + 4;
                // Check for "---" placeholder
                if (!(textStart + 2 < end && textStart[0]=='-' && textStart[1]=='-' && textStart[2]=='-')) {
                    int tLen = encTextLen(textStart, end);
                    sMasterIndex[idx].e2Offset = (long)(textStart - tmp);
                    sMasterIndex[idx].e2Len = tLen;
                }
            }
            // Advance to next line
            while (p < end && *p != '\n') p++;
            if (p < end) p++;
        }

        idx++;
    }

    // Wipe and free the temporary decrypted buffer -- no plaintext stays in RAM
    memset(tmp, 0, sEncSize + 1);
    free(tmp);
}

// ---------------------------------------------------------------------------
// On-demand encounter decryption (public API)
// ---------------------------------------------------------------------------

const char* sayingsDecryptEncounter(int masterId, int encounter) {
    if (masterId < 0 || masterId >= MASTER_COUNT) return nullptr;

    long offset;
    int  len;
    if (encounter == 0) {
        offset = kMasters[masterId].e1Offset;
        len    = kMasters[masterId].e1Len;
    } else {
        offset = kMasters[masterId].e2Offset;
        len    = kMasters[masterId].e2Len;
    }

    if (len <= 0) return nullptr;
    if (!decryptRegion(offset, len)) return nullptr;
    return sDecryptBuf;
}

void sayingsWipeBuffer() {
    memset(sDecryptBuf, 0, ENCOUNTER_BUF_SIZE);
    DC_FlushRange(sDecryptBuf, ENCOUNTER_BUF_SIZE);
}

// ---------------------------------------------------------------------------
// Save integration
// ---------------------------------------------------------------------------

void sayingsLoadFromSave(const u8* src, int len) {
    if (len > 112) len = 112;
    memcpy(gSayings.bits, src, len);

    // Rebuild cached counts
    gSayings.totalFound = 0;
    gSayings.totalComplete = 0;
    for (int i = 0; i < MASTER_COUNT; i++) {
        u8 b = sayingsGetBits(i);
        if (b != 0) gSayings.totalFound++;
        if (b == 3) gSayings.totalComplete++;
    }
}

void sayingsSaveInto(u8* dst) {
    memcpy(dst, gSayings.bits, 112);
}

// ---------------------------------------------------------------------------
// Collection queries
// ---------------------------------------------------------------------------

bool sayingsMasterFound(int id) {
    return sayingsGetBits(id) != 0;
}

bool sayingsMasterComplete(int id) {
    return sayingsGetBits(id) == 3;
}

int sayingsGetEncounterLevel(int id) {
    u8 b = sayingsGetBits(id);
    if (b == 3) return 2;
    if (b == 1) return 1;
    return 0;
}

int sayingsFoundCount() {
    return gSayings.totalFound;
}

int sayingsCompleteCount() {
    return gSayings.totalComplete;
}

// ---------------------------------------------------------------------------
// Encounter availability check
// ---------------------------------------------------------------------------

bool sayingsHasE2(int id) {
    if (id < 0 || id >= MASTER_COUNT) return false;
    return kMasters[id].e2Len > 0;
}

// ---------------------------------------------------------------------------
// Name / text helpers
// ---------------------------------------------------------------------------

int sayingsMasterNameLen(int id) {
    const char* n = kMasters[id].name;
    if (!n) return 0;
    // Names are now NUL-terminated in the name buffer
    return (int)strlen(n);
}

int sayingsEncounterTextLen(int masterId, int encounter) {
    if (masterId < 0 || masterId >= MASTER_COUNT) return 0;
    if (encounter == 0) return kMasters[masterId].e1Len;
    return kMasters[masterId].e2Len;
}

// ---------------------------------------------------------------------------
// Encounter selection logic
// ---------------------------------------------------------------------------

EncounterResult sayingsRollEncounter() {
    EncounterResult r;
    r.masterId = 0;
    r.encounter = 0;
    r.isDuplicate = false;

    int found = gSayings.totalFound;

    // Determine if this roll is a duplicate
    bool forceDuplicate = false;
    bool forceNew = false;

    if (found < 10) {
        // First 10 finds: guaranteed new
        forceNew = true;
    } else if (found >= MASTER_COUNT - 10) {
        // Last 10 undiscovered: 95% duplicate (finding the last few is very hard)
        forceDuplicate = (rngRange(100) < 95);
    } else {
        // Middle range: 50% base duplicate rate, with erratic escalation
        int baseDupeChance = 50;
        int wave = gameGetWave();
        int jitter = ((wave * 7 + 13) % 30) - 15;  // -15 to +14
        int dupeChance = baseDupeChance + jitter;
        if (dupeChance < 20) dupeChance = 20;   // floor
        if (dupeChance > 80) dupeChance = 80;   // cap
        forceDuplicate = (rngRange(100) < dupeChance);
    }

    if (forceNew || (!forceDuplicate && found < MASTER_COUNT)) {
        // Pick a new (undiscovered or partially discovered) master
        // Static arrays to avoid 1792-byte stack allocations
        static int eligible[MASTER_COUNT];
        int eligCount = 0;

        for (int i = 0; i < MASTER_COUNT; i++) {
            u8 bits = sayingsGetBits(i);
            if (bits == 0) {
                // Completely undiscovered -- eligible for E1
                eligible[eligCount++] = i;
            } else if (bits == 1 && kMasters[i].e2Len > 0) {
                // Has E1 but not E2, and E2 exists -- eligible for E2
                eligible[eligCount++] = i;
            }
        }

        if (eligCount > 0) {
            int pick = rngRange(eligCount);
            r.masterId = eligible[pick];
            r.isDuplicate = false;
            u8 bits = sayingsGetBits(r.masterId);
            r.encounter = (bits == 0) ? 0 : 1;
            return r;
        }
        // All masters fully discovered -- fall through to duplicate
    }

    // Duplicate: pick any master that has at least E1
    static int known[MASTER_COUNT];
    int knownCount = 0;
    for (int i = 0; i < MASTER_COUNT; i++) {
        if (sayingsGetBits(i) > 0) known[knownCount++] = i;
    }

    if (knownCount == 0) {
        // Edge case: no masters found yet AND forced duplicate (shouldn't happen)
        // Fall back to random new master
        r.masterId = rngRange(MASTER_COUNT);
        r.isDuplicate = false;
        r.encounter = 0;
        return r;
    }

    int pick = rngRange(knownCount);
    r.masterId = known[pick];
    r.isDuplicate = true;
    // Show a random known encounter from this master
    u8 bits = sayingsGetBits(r.masterId);
    if (bits == 3 && kMasters[r.masterId].e2Len > 0 && rngRange(2) == 0) {
        r.encounter = 1;
    } else {
        r.encounter = 0;
    }
    return r;
}

// ---------------------------------------------------------------------------
// Mark encounter as found
// ---------------------------------------------------------------------------

void sayingsMarkFound(const EncounterResult& r) {
    if (r.isDuplicate) return;  // nothing to save

    u8 bits = sayingsGetBits(r.masterId);
    if (bits == 0) {
        sayingsSetBits(r.masterId, 1);   // E1 found
        gSayings.totalFound++;
    } else if (bits == 1) {
        sayingsSetBits(r.masterId, 3);   // E1 + E2 found
        gSayings.totalComplete++;
    }
}

// ---------------------------------------------------------------------------
// Viewer helpers: sorted ID list
// ---------------------------------------------------------------------------

// Simple comparison for alphabetical sort by master name
// Names are now NUL-terminated in the name buffer
static int nameCompare(int idA, int idB) {
    const char* a = kMasters[idA].name;
    const char* b = kMasters[idB].name;
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (true) {
        bool endA = (*a == '\0');
        bool endB = (*b == '\0');
        if (endA && endB) return 0;
        if (endA) return -1;
        if (endB) return 1;

        // Case-insensitive comparison
        char ca = *a;
        char cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca < cb) return -1;
        if (ca > cb) return 1;

        a++;
        b++;
    }
}

// Insertion sort -- fine for 448 elements on a one-time init call
static void sortIds(int* ids, int count) {
    for (int i = 1; i < count; i++) {
        int key = ids[i];
        int j = i - 1;
        while (j >= 0 && nameCompare(ids[j], key) > 0) {
            ids[j + 1] = ids[j];
            j--;
        }
        ids[j + 1] = key;
    }
}

void sayingsGetSortedIds(int* out, int* count, bool foundOnly) {
    int n = 0;
    for (int i = 0; i < MASTER_COUNT; i++) {
        if (foundOnly && sayingsGetBits(i) == 0) continue;
        out[n++] = i;
    }
    sortIds(out, n);
    *count = n;
}
