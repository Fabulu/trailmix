#include "sayings.h"
#include "rng.h"
#include "strings.h"   // for StrLang, gActiveLang
#include "game.h"      // for gameGetWave()
#include <string.h>    // for memset, memcpy
#include <stdlib.h>    // for malloc
#include <stdio.h>     // for fopen, fread, fclose
#include <nds.h>
#include <filesystem.h> // for nitroFSInit

// bin2s-generated header for embedded zen_masters.bin data
#include "zen_masters_bin.h"

// ---------------------------------------------------------------------------
// Static storage
// ---------------------------------------------------------------------------

static MasterEntry sMasterIndex[MASTER_COUNT];
const MasterEntry* kMasters = sMasterIndex;
SayingsState gSayings;

// ---------------------------------------------------------------------------
// ROM parsing
// ---------------------------------------------------------------------------

// Buffer for German NitroFS data (kept alive for the entire session)
static char* sDeBuffer = nullptr;

// Find master index by name (case-insensitive, stops at ' ===' or '\n')
static int findMasterByName(const char* name, const char* nameEnd) {
    int nameLen = (int)(nameEnd - name);
    // Trim trailing spaces
    while (nameLen > 0 && name[nameLen - 1] == ' ') nameLen--;

    for (int i = 0; i < MASTER_COUNT; i++) {
        const char* mn = sMasterIndex[i].name;
        if (!mn) continue;

        // Compare character by character
        int j = 0;
        bool match = true;
        while (j < nameLen) {
            char a = name[j];
            char b = mn[j];
            // Stop if master name ended early
            if (b == '=' || b == '\n' || b == '\0') { match = false; break; }
            // Case-insensitive
            if (a >= 'a' && a <= 'z') a -= 32;
            if (b >= 'a' && b <= 'z') b -= 32;
            if (a != b) { match = false; break; }
            j++;
        }
        if (match) {
            // Verify master name also ends here
            char next = mn[j];
            if (next == ' ' || next == '=' || next == '\n' || next == '\0')
                return i;
        }
    }
    return -1;
}

// Global argv pointer set by main() for NitroFS
char** gArgv = nullptr;

// Load German text from NitroFS and fill in e1_de/e2_de pointers
static void loadGermanFromNitroFS() {
    if (!nitroFSInit(gArgv)) return;
    FILE* f = fopen("nitro:/zen_masters_de.txt", "rb");
    if (!f) return;

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fclose(f); return; }

    sDeBuffer = (char*)malloc(size + 1);
    if (!sDeBuffer) { fclose(f); return; }

    fread(sDeBuffer, 1, size, f);
    fclose(f);
    sDeBuffer[size] = '\0';

    // Parse the German file: same format as English but E1/E2 lines
    // map to e1_de/e2_de pointers on the matching master
    const char* p = sDeBuffer;
    const char* end = sDeBuffer + size;

    while (p < end) {
        // Find "=== " header
        while (p < end - 4 && !(p[0] == '=' && p[1] == '=' && p[2] == '=' && p[3] == ' '))
            p++;
        if (p >= end - 4) break;

        p += 4; // skip "=== "
        const char* nameStart = p;
        // Find end of name (before " ===")
        while (p < end && *p != '\n') p++;
        const char* lineEnd = p;
        // Trim trailing " ===" from name
        const char* nameEnd = lineEnd;
        while (nameEnd > nameStart && *(nameEnd - 1) == '=') nameEnd--;
        while (nameEnd > nameStart && *(nameEnd - 1) == ' ') nameEnd--;

        if (p < end) p++;

        int idx = findMasterByName(nameStart, nameEnd);

        // Parse E1/E2 lines for this block
        while (p < end) {
            if (*p == '\n' || *p == '\r' || *p == ' ') { p++; continue; }
            if (p + 3 < end && p[0] == '=' && p[1] == '=' && p[2] == '=') break;

            if (idx >= 0 && p + 4 < end && p[0]=='E' && p[1]=='1' && p[2]==':' && p[3]==' ') {
                sMasterIndex[idx].e1_de = p + 4;
            } else if (idx >= 0 && p + 4 < end && p[0]=='E' && p[1]=='2' && p[2]==':' && p[3]==' ') {
                const char* v = p + 4;
                if (!(v + 2 < end && v[0]=='-' && v[1]=='-' && v[2]=='-'))
                    sMasterIndex[idx].e2_de = v;
            }
            // Advance to next line
            while (p < end && *p != '\n') p++;
            if (p < end) p++;
        }
    }
}

// Buffer for NitroFS-loaded text data (kept alive entire session)
static char* sMainBuffer = nullptr;

void sayingsInit() {
    memset(&gSayings, 0, sizeof(gSayings));
    memset(sMasterIndex, 0, sizeof(sMasterIndex));

    // Load the appropriate language file from NitroFS
    const char* filename = (gActiveLang == StrLang::DE)
        ? "nitro:/zen_masters_de.txt"
        : "nitro:/zen_masters_en.txt";

    // Try NitroFS first
    const char* data = nullptr;
    const char* end  = nullptr;

    if (nitroFSInit(gArgv)) {
        FILE* f = fopen(filename, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (size > 0) {
                sMainBuffer = (char*)malloc(size + 1);
                if (sMainBuffer) {
                    fread(sMainBuffer, 1, size, f);
                    sMainBuffer[size] = '\0';
                    data = sMainBuffer;
                    end = sMainBuffer + size;
                }
            }
            fclose(f);
        }
    }

    // Fallback: use embedded ROM data (English only)
    if (!data) {
        data = reinterpret_cast<const char*>(zen_masters_bin);
        end  = reinterpret_cast<const char*>(zen_masters_bin_end);
    }

    int idx = 0;
    const char* p = data;

    while (p < end && idx < MASTER_COUNT) {
        // Find "=== " header
        while (p < end - 4 && !(p[0] == '=' && p[1] == '=' && p[2] == '=' && p[3] == ' '))
            p++;
        if (p >= end - 4) break;

        p += 4; // skip "=== "
        sMasterIndex[idx].name = p;
        // Skip to next line
        while (p < end && *p != '\n') p++;
        if (p < end) p++;

        // Clear fields
        sMasterIndex[idx].e1 = nullptr;
        sMasterIndex[idx].e1_de = nullptr;
        sMasterIndex[idx].e2 = nullptr;
        sMasterIndex[idx].e2_de = nullptr;

        // Parse subsequent lines until next "=== " or end of data
        // NOTE: With split EN/DE files, the embedded data has only E1/E2 lines
        // (no E1_DE/E2_DE). We still parse those prefixes for backward compat
        // in case the bilingual file is used directly.
        while (p < end) {
            // Skip blank lines / whitespace
            if (*p == '\n' || *p == '\r' || *p == ' ') { p++; continue; }
            // Stop if next entry starts
            if (p + 3 < end && p[0] == '=' && p[1] == '=' && p[2] == '=') break;

            // Check line prefix and assign pointer
            if (p + 7 < end && p[0]=='E' && p[1]=='1' && p[2]=='_' && p[3]=='D' && p[4]=='E' && p[5]==':' && p[6]==' ') {
                sMasterIndex[idx].e1_de = p + 7;
            } else if (p + 7 < end && p[0]=='E' && p[1]=='2' && p[2]=='_' && p[3]=='D' && p[4]=='E' && p[5]==':' && p[6]==' ') {
                const char* v = p + 7;
                if (!(v + 2 < end && v[0]=='-' && v[1]=='-' && v[2]=='-'))
                    sMasterIndex[idx].e2_de = v;
            } else if (p + 4 < end && p[0]=='E' && p[1]=='1' && p[2]==':' && p[3]==' ') {
                sMasterIndex[idx].e1 = p + 4;
            } else if (p + 4 < end && p[0]=='E' && p[1]=='2' && p[2]==':' && p[3]==' ') {
                const char* v = p + 4;
                if (!(v + 2 < end && v[0]=='-' && v[1]=='-' && v[2]=='-'))
                    sMasterIndex[idx].e2 = v;
            }
            // Advance to next line
            while (p < end && *p != '\n') p++;
            if (p < end) p++;
        }

        idx++;
    }

    // When loaded from NitroFS, the language-specific file has E1/E2 lines
    // that go directly into e1/e2 (not e1_de/e2_de). The language-aware
    // accessors sayingsGetE1/E2 will just return e1/e2 since e1_de is null.
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
// Language-aware text access
// ---------------------------------------------------------------------------

const char* sayingsGetE1(int id) {
    if (gActiveLang == StrLang::DE && kMasters[id].e1_de)
        return kMasters[id].e1_de;
    return kMasters[id].e1;
}

const char* sayingsGetE2(int id) {
    if (gActiveLang == StrLang::DE && kMasters[id].e2_de)
        return kMasters[id].e2_de;
    return kMasters[id].e2;  // may be nullptr
}

// ---------------------------------------------------------------------------
// Name / text helpers
// ---------------------------------------------------------------------------

int sayingsMasterNameLen(int id) {
    const char* n = kMasters[id].name;
    if (!n) return 0;
    int len = 0;
    while (n[len] && n[len] != '=' && n[len] != '\n') len++;
    // Trim trailing space before " ==="
    while (len > 0 && n[len - 1] == ' ') len--;
    return len;
}

int sayingsEncounterLen(const char* e) {
    if (!e) return 0;
    int len = 0;
    while (e[len] && e[len] != '\n') len++;
    return len;
}

// ---------------------------------------------------------------------------
// Encounter selection logic
// ---------------------------------------------------------------------------

EncounterResult sayingsRollEncounter() {
    EncounterResult r;
    r.masterId = 0;
    r.isDuplicate = false;
    r.text = nullptr;

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
            } else if (bits == 1 && kMasters[i].e2 != nullptr) {
                // Has E1 but not E2, and E2 exists -- eligible for E2
                eligible[eligCount++] = i;
            }
        }

        if (eligCount > 0) {
            int pick = rngRange(eligCount);
            r.masterId = eligible[pick];
            r.isDuplicate = false;
            u8 bits = sayingsGetBits(r.masterId);
            // Use language-aware accessors
            r.text = (bits == 0) ? sayingsGetE1(r.masterId)
                                 : sayingsGetE2(r.masterId);
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
        r.text = sayingsGetE1(r.masterId);
        return r;
    }

    int pick = rngRange(knownCount);
    r.masterId = known[pick];
    r.isDuplicate = true;
    // Show a random known encounter from this master (language-aware)
    u8 bits = sayingsGetBits(r.masterId);
    if (bits == 3 && kMasters[r.masterId].e2 && rngRange(2) == 0) {
        r.text = sayingsGetE2(r.masterId);
    } else {
        r.text = sayingsGetE1(r.masterId);
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
// Compares character by character, stopping at '=' or '\n' (end of name)
static int nameCompare(int idA, int idB) {
    const char* a = kMasters[idA].name;
    const char* b = kMasters[idB].name;
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (true) {
        // End-of-name detection: stop at '=' or '\n' or '\0'
        bool endA = (*a == '=' || *a == '\n' || *a == '\0');
        bool endB = (*b == '=' || *b == '\n' || *b == '\0');
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
