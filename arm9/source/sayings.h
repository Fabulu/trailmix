#ifndef SAYINGS_H
#define SAYINGS_H

#include <nds/ndstypes.h>

// --- Constants ---
constexpr int MASTER_COUNT       = 434;  // total masters in ROM data
constexpr int MASTER_NAME_MAX    = 64;   // max chars for a name (includes NUL)
constexpr int ENCOUNTER_MAX_LEN  = 2048; // max chars for one encounter text (DE texts can be longer)

// --- ROM data index (built at startup from zen_masters.bin) ---
struct MasterEntry {
    const char* name;       // pointer into ROM data
    const char* e1;         // encounter 1 text, English (pointer into ROM data)
    const char* e1_de;      // encounter 1 text, German (pointer into ROM data)
    const char* e2;         // encounter 2 text, English (nullptr if "---")
    const char* e2_de;      // encounter 2 text, German (nullptr if "---")
};

extern const MasterEntry* kMasters;  // array of MASTER_COUNT, built by sayingsInit()

// --- Per-master collection state (2 bits each, packed into bytes) ---
//   0b00 = undiscovered
//   0b01 = E1 found
//   0b11 = E1 + E2 found  (E2 requires E1)
//   0b10 = invalid / reserved
struct SayingsState {
    u8 bits[112];           // ceil(448 * 2 / 8) = 112 bytes -- 2 bits per master
    u16 totalFound;         // cached count of masters with bits != 0
    u16 totalComplete;      // cached count of masters with bits == 0b11
};

extern SayingsState gSayings;

// --- Lifecycle ---
void sayingsInit();                     // parse ROM data, zero state
void sayingsLoadFromSave(const u8* src, int len);  // restore bits[] from SRAM
void sayingsSaveInto(u8* dst);          // write bits[] into buffer for SRAM

// --- Bit access (inline for performance) ---
static inline u8 sayingsGetBits(int id) {
    int byteIdx = id / 4;
    int shift   = (id % 4) * 2;
    return (gSayings.bits[byteIdx] >> shift) & 0x03;
}

static inline void sayingsSetBits(int id, u8 val) {
    int byteIdx = id / 4;
    int shift   = (id % 4) * 2;
    gSayings.bits[byteIdx] &= ~(0x03 << shift);
    gSayings.bits[byteIdx] |= (val & 0x03) << shift;
}

// --- Collection queries ---
bool sayingsMasterFound(int id);        // any encounter found?
bool sayingsMasterComplete(int id);     // both encounters found?
int  sayingsGetEncounterLevel(int id);  // 0, 1, or 2

// --- Encounter selection (called after boss kill) ---
struct EncounterResult {
    int  masterId;          // index into kMasters[]
    bool isDuplicate;       // true if player already had this encounter level
    const char* text;       // pointer to the encounter text to display (language-aware)
};

EncounterResult sayingsRollEncounter(); // RNG + duplicate logic
void sayingsMarkFound(const EncounterResult& r); // update bits[] after player reads it

// --- Language-aware text access ---
const char* sayingsGetE1(int id);       // returns e1 or e1_de based on gActiveLang
const char* sayingsGetE2(int id);       // returns e2 or e2_de based on gActiveLang

// --- Viewer helpers ---
int  sayingsFoundCount();               // how many masters have any encounter
int  sayingsCompleteCount();            // how many have both
void sayingsGetSortedIds(int* out, int* count, bool foundOnly);
    // fills out[] with master IDs sorted alphabetically
    // if foundOnly, only includes masters with bits != 0

// --- Name/text helpers ---
int sayingsMasterNameLen(int id);       // length of name (stops at ' ===' or '\n')
int sayingsEncounterLen(const char* e); // length of encounter text (stops at '\n')

#endif // SAYINGS_H
