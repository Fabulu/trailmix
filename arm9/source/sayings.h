#ifndef SAYINGS_H
#define SAYINGS_H

#include <nds/ndstypes.h>

// --- Constants ---
constexpr int MASTER_COUNT       = 434;  // total masters in ROM data
constexpr int MASTER_NAME_MAX    = 64;   // max chars for a name (includes NUL)
constexpr int ENCOUNTER_MAX_LEN  = 2048; // max chars for one encounter text (DE texts can be longer)
constexpr int ENCOUNTER_BUF_SIZE = 2048; // DTCM decrypt buffer size

// --- ROM data index (built at startup from encrypted NitroFS data) ---
// Encounter text is NOT stored as pointers -- only offset+length into the
// encrypted buffer.  Text is decrypted on demand into a DTCM buffer.
struct MasterEntry {
    const char* name;       // pointer into persistent name buffer (decrypted)
    int         e1Len;      // length of E1 text in bytes (0 = no E1)
    int         e2Len;      // length of E2 text in bytes (0 = no E2 / "---")
    long        e1Offset;   // byte offset of E1 text in the encrypted buffer
    long        e2Offset;   // byte offset of E2 text in the encrypted buffer
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
    int  encounter;         // 0 = E1, 1 = E2
    bool isDuplicate;       // true if player already had this encounter level
};

EncounterResult sayingsRollEncounter(); // RNG + duplicate logic
void sayingsMarkFound(const EncounterResult& r); // update bits[] after player reads it

// --- On-demand encounter decryption (RAM protection) ---
// Decrypts a single encounter into a small DTCM buffer and returns a pointer.
// The buffer is overwritten on each call -- only one encounter in memory at a time.
const char* sayingsDecryptEncounter(int masterId, int encounter);

// Wipe the DTCM decrypt buffer (call after rendering encounter text each frame)
void sayingsWipeBuffer();

// Check if a master has E2 text available
bool sayingsHasE2(int id);

// --- Viewer helpers ---
int  sayingsFoundCount();               // how many masters have any encounter
int  sayingsCompleteCount();            // how many have both
void sayingsGetSortedIds(int* out, int* count, bool foundOnly);
    // fills out[] with master IDs sorted alphabetically
    // if foundOnly, only includes masters with bits != 0

// --- Name/text helpers ---
int sayingsMasterNameLen(int id);       // length of name (stops at ' ===' or '\n')
int sayingsEncounterTextLen(int masterId, int encounter); // length of encounter text

// --- Internal data pipeline (do not call directly) ---
void initEncounterDecoder();

#endif // SAYINGS_H
