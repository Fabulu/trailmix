// strings.h — Internationalization string catalog
// Format: STR_ID | English | German
// Two-language table; index by StrLang enum.
// Generated: 2026-05-23

#ifndef STRINGS_H
#define STRINGS_H

#include "types.h"

enum class StrLang : int { EN = 0, DE = 1, COUNT = 2 };

struct StrEntry { const char* en; const char* de; };

// Helper macro — keep source readable
#define S(en, de) { en, de }

// ─────────────────────────────────────────────────────────────────
// 1. MENU / UI  (STR_UI_*)
// ─────────────────────────────────────────────────────────────────
// STR_ID                      | English          | German
// ---------------------------------------------------------------
// STR_UI_SELECT_LANG          | "Select Language" | "Sprache wählen"
// STR_UI_START_GAME           | "Start Game"      | "Spiel starten"
// STR_UI_CONTINUE             | "Continue"        | "Weiterspielen"
// STR_UI_SHOP                 | "Shop"            | "Laden"
// STR_UI_REROLL               | "Reroll"          | "Neu würfeln"
// STR_UI_SELL                 | "Sell"            | "Verkaufen"
// STR_UI_BUY                  | "Buy"             | "Kaufen"
// STR_UI_WAVE                 | "Wave"            | "Welle"
// STR_UI_GOLD                 | "Gold"            | "Gold"
// STR_UI_HP                   | "HP"              | "LP"
// STR_UI_DASH                 | "Dash"            | "Ausweichen"
// STR_UI_INTEREST             | "Interest"        | "Zinsen"
// STR_UI_START_WAVE           | "Start Wave"      | "Welle starten"
// STR_UI_GAME_OVER            | "Game Over"       | "Spiel vorbei"
// STR_UI_SCORE                | "Score"           | "Punkte"

constexpr StrEntry kUI[] = {
    /* STR_UI_SELECT_LANG  [0]  */ S("Select Language", "Sprache wählen"),
    /* STR_UI_START_GAME   [1]  */ S("Start Game",      "Spiel starten"),
    /* STR_UI_CONTINUE     [2]  */ S("Continue",        "Weiterspielen"),
    /* STR_UI_SHOP         [3]  */ S("Shop",            "Laden"),
    /* STR_UI_REROLL       [4]  */ S("Reroll",          "Neu würfeln"),
    /* STR_UI_SELL         [5]  */ S("Sell",            "Verkaufen"),
    /* STR_UI_BUY          [6]  */ S("Buy",             "Kaufen"),
    /* STR_UI_WAVE         [7]  */ S("Wave",            "Welle"),
    /* STR_UI_GOLD         [8]  */ S("Gold",            "Gold"),
    /* STR_UI_HP           [9]  */ S("HP",              "LP"),
    /* STR_UI_DASH        [10]  */ S("Dash",            "Ausweichen"),
    /* STR_UI_INTEREST    [11]  */ S("Interest",        "Zinsen"),
    /* STR_UI_START_WAVE  [12]  */ S("Start Wave",      "Welle starten"),
    /* STR_UI_GAME_OVER   [13]  */ S("Game Over",       "Spiel vorbei"),
    /* STR_UI_SCORE       [14]  */ S("Score",           "Punkte"),
    /* STR_UI_LOCK        [15]  */ S("Lock",            "Sperren"),
    /* STR_UI_PERK        [16]  */ S("PERK",            "PERK"),
    /* STR_UI_UNLOCK      [17]  */ S("Unlock",          "Entsperren"),
    /* STR_UI_LOCKED      [18]  */ S("Locked",          "Gesperrt"),
    /* STR_UI_PERMANENT   [19]  */ S("PERMANENT",       "DAUERHAFT"),
    /* STR_UI_SELL2       [20]  */ S("Sell",            "Verkaufen"),
    /* STR_UI_FREE        [21]  */ S("FREE",            "GRATIS"),
    /* STR_UI_PRESS_START [22]  */ S("PRESS START",     "DRUECKE START"),
    /* STR_UI_GAME_OVER2  [23]  */ S("GAME OVER",       "SPIEL VORBEI"),
    /* STR_UI_VICTORY     [24]  */ S("VICTORY!",        "SIEG!"),
    /* STR_UI_THE_APOTHE  [25]  */ S("THE APOTHECARY",  "DER APOTHEKER"),
    /* STR_UI_DEFEATED    [26]  */ S("HAS BEEN DEFEATED","WURDE BESIEGT"),
    /* STR_UI_ENDLESS     [27]  */ S("START: ENDLESS MODE","START: ENDLOS-MODUS"),
    /* STR_UI_RETURN_MENU [28]  */ S("A: RETURN TO MENU","A: ZUM MENUE"),
    /* STR_UI_SHOT        [29]  */ S("SHOT:",           "SCHUSS:"),
    /* STR_UI_PASSIVE     [30]  */ S("PASSIVE:",        "PASSIV:"),
    /* STR_UI_HEAL_FMT    [31]  */ S("HEAL %dg",       "HEILEN %dg"),
    /* STR_UI_RR_FREE     [32]  */ S("RR FREE",        "UW GRATIS"),
    /* STR_UI_RR_FMT      [33]  */ S("RR %dg",         "UW %dg"),
    /* STR_UI_NA          [34]  */ S("N/A",             "K.A."),
    /* STR_UI_TAP_HINT    [35]  */ S("TAP ONCE TO SELECT  TAP AGAIN TO CONFIRM", "ANTIPPEN ZUM WAEHLEN  NOCHMAL ZUM BESTAETIGEN"),
    /* STR_UI_UPGRADE     [36]  */ S("UPGRADE PATH:", "AUFWERTUNGSPFAD:"),
    /* STR_UI_WAVE        [37]  */ S("WAVE", "WELLE"),
    /* STR_UI_3MERGE      [38]  */ S("3 to merge", "3 zum Verschm."),
    /* STR_UI_2MERGE      [39]  */ S("2 to merge", "2 zum Verschm."),
    /* STR_UI_TIERMAX     [40]  */ S("MAX", "MAX"),
    /* STR_UI_PLUS1SHOT   [41]  */ S("+1 shot", "+1 Schuss"),
    /* STR_UI_PLUS2SHOTS  [42]  */ S("+2 shots", "+2 Schuesse"),
    /* STR_UI_PIERCEALL   [43]  */ S("pierce all", "durchbohrt alle"),
    /* STR_UI_SOLD        [44]  */ S("SOLD", "VERK."),
    /* STR_UI_LOCK2       [45]  */ S("LOCK", "SPERR."),
    /* STR_UI_UNLOCK2     [46]  */ S("UNLOCK", "ENTSPR."),
    /* STR_UI_GOLD_COLON  [47]  */ S("GOLD:", "GOLD:"),
    /* STR_UI_INT_ABBR    [48]  */ S("INT", "ZIN"),
    /* STR_UI_BUY2        [49]  */ S("BUY", "KAUF"),
    /* STR_UI_SELL3       [50]  */ S("SELL", "VERK"),
    /* STR_UI_DASH2       [51]  */ S("DASH", "DASH"),
    /* STR_UI_PERK_INC    [52]  */ S("PERK INCOMING...", "TALENT KOMMT..."),
    /* STR_UI_MERGE       [53]  */ S("MERGE!", "FUSION!"),
    /* STR_UI_PERK2       [54]  */ S("PERK", "TALENT"),
    /* STR_UI_PERMANENT2  [55]  */ S("PERMANENT", "DAUERHAFT"),
    /* STR_UI_SELECT_UNIT [56]  */ S("SELECT A UNIT", "WAEHLE EINHEIT"),
    /* STR_UI_COMPANIONS  [57]  */ S("COMPANIONS:", "BEGLEITER:"),
    /* STR_UI_ACTIVE      [58]  */ S("active", "aktiv"),
    /* STR_UI_UNLOCKS     [59]  */ S("unlocks!", "freigeschaltet!"),
    /* STR_UI_HP_COLON    [60]  */ S("HP:", "LP:"),
    /* STR_UI_HINTS       [61]  */ S("A:OK  B:BACK  L:LOCK  R:SELL", "A:OK  B:ZUR.  L:SPERR  R:VERK"),
};

// Color abbreviations for HUD (indexed by PillColor)
constexpr StrEntry kColorAbbr[PILL_COLOR_COUNT] = {
    /* Red    */ S("RED", "ROT"),
    /* Blue   */ S("BLU", "BLA"),
    /* Green  */ S("GRN", "GRN"),
    /* Yellow */ S("YLW", "GLB"),
    /* Purple */ S("PUR", "LIL"),
    /* Cyan   */ S("CYN", "CYN"),
};


// ─────────────────────────────────────────────────────────────────
// 2. SYNERGY NAMES  (kSynergyNames[color][tier])
//    Colors: 0=Red 1=Blue 2=Green 3=Yellow 4=Purple 5=Cyan
//    4 tiers per color, activated at 2/3/4/5 companions of that color.
//    Each tier adds a visible, impactful effect.
// ─────────────────────────────────────────────────────────────────
// RED (Wrath):   T0=fire trails, T1=kill explosion, T2=pierce, T3=fire aura
// BLUE (Arcane): T0=slow field, T1=freeze-on-hit, T2=ricochet, T3=time warp
// GREEN (Nature):T0=regen, T1=heal orbs, T2=shield, T3=companion regen
// YELLOW (Fort.):T0=+gold, T1=free reroll, T2=interest+3, T3=mega coins
// PURPLE (Hex):  T0=slow-on-hit, T1=poison-on-kill, T2=dmg reduce, T3=weakness
// CYAN (Volt):   T0=fire rate, T1=chain-on-kill, T2=EMP pulse, T3=chain-on-hit

constexpr int SYN_TIERS = 4;
constexpr StrEntry kSynergyNames[PILL_COLOR_COUNT][SYN_TIERS] = {
    /* Red — WRATH (offense escalation) */
    /* T0(2): fire trails  T1(3): kill explosion  T2(4): pierce  T3(5): fire aura */
    {
        S("FIRE TRAIL",    "FEUERSPUR"),
        S("KILL BLAST",    "TODESKNALL"),
        S("PIERCE ALL",    "DURCHBOHRUNG"),
        S("INFERNO AURA",  "INFERNO-AURA"),
    },
    /* Blue — ARCANE (control) */
    /* T0(2): slow bullets T1(3): freeze-on-hit T2(4): ricochet T3(5): time warp */
    {
        S("SLOW FIELD",    "BREMSFELD"),
        S("FROST SHOCK",   "FROSTSCHOCK"),
        S("RICOCHET",      "ABPRALLER"),
        S("TIME WARP",     "ZEITKRUMM"),
    },
    /* Green — NATURE (sustain) */
    /* T0(2): regen  T1(3): heal orbs  T2(4): shield  T3(5): companion regen */
    {
        S("REGEN",         "REGENERATION"),
        S("HEAL ORBS",     "HEILKUGELN"),
        S("SHIELD",        "SCHUTZSCHILD"),
        S("ETERNAL BLOOM", "EWIGE BLUTE"),
    },
    /* Yellow — FORTUNE (economy) */
    /* T0(2): +gold  T1(3): free reroll  T2(4): interest+3  T3(5): mega coins */
    {
        S("GOLD RUSH",     "GOLDRAUSCH"),
        S("FREE REROLL",   "GRATISWURF"),
        S("COMPOUND",      "ZINSESZINS"),
        S("MIDAS TOUCH",   "MIDASGRIFF"),
    },
    /* Purple — HEX (debuffs) */
    /* T0(2): slow-on-hit  T1(3): poison-on-kill  T2(4): damage reduce  T3(5): weakness */
    {
        S("HEX SLOW",      "HEXVERLANG"),
        S("DEATH CLOUD",   "TODESWOLKE"),
        S("DREAD AURA",    "SCHRECKENSAURA"),
        S("CURSE ALL",     "FLUCH ALLER"),
    },
    /* Cyan — VOLT (tech) */
    /* T0(2): fire rate  T1(3): chain-on-kill  T2(4): EMP pulse  T3(5): chain-on-hit */
    {
        S("OVERCLOCK",     "UBERTAKTUNG"),
        S("CHAIN ZAP",     "KETTENBLITZ"),
        S("EMP PULSE",     "EMP-PULS"),
        S("STORM FIELD",   "STURMFELD"),
    },
};

// ─────────────────────────────────────────────────────────────────
// 3. PERK NAMES + DESCRIPTIONS  (15 perks, name + desc each)
// ─────────────────────────────────────────────────────────────────
// ── OFFENSE (4) ──
// PERK_BULLET_HELL        | "Bullet Hell"        | "Kugelhagel"
//                         | Fire in all 4 directions! | Feuere in alle 4 Richtungen!
// PERK_OVERCHARGE         | "Overcharge"         | "Überladung"
//                         | Companions fire 50% faster | Begleiter feuern 50% schneller
// PERK_GLASS_CANNON       | "Glass Cannon"       | "Glaskanone"
//                         | +5 damage, half max HP | +5 Schaden, halbe max. LP
// PERK_CHAIN_LIGHTNING    | "Chain Lightning"    | "Kettenblitz"
//                         | Kills zap a nearby enemy | Kills treffen einen nahen Feind
// ── DEFENSE (4) ──
// PERK_PHOENIX_DOWN       | "Phoenix Down"       | "Phönixfeder"
//                         | First fallen ally revives per wave | Erster Verbündeter steht pro Welle auf
// PERK_FORTRESS           | "Fortress"           | "Festung"
//                         | +40 HP, allies +20 HP, 25% slow | +40 LP, Verb. +20 LP, 25% langsamer
// PERK_SHIELD_BASH        | "Shield Bash"        | "Schildstoß"
//                         | Dash deals 8 damage to enemies | Ausweichen macht 8 Schaden
// PERK_SECOND_WIND        | "Second Wind"        | "Zweite Luft"
//                         | Full heal once when near death | Volle Heilung bei Todesgefahr
// ── ECONOMY (4) ──
// PERK_GOLD_FEVER         | "Gold Fever"         | "Goldfieber"
//                         | 2x gold for 3 waves! | 2x Gold für 3 Wellen!
// PERK_WAR_CHEST          | "War Chest"          | "Kriegskasse"
//                         | Get 15g now, +3g each wave | 15g sofort, +3g jede Welle
// PERK_JACKPOT            | "Jackpot"            | "Jackpot"
//                         | Interest cap doubled to 10 | Zinsobergrenze auf 10 verdoppelt
// PERK_WHOLESALE          | "Wholesale"          | "Großhandel"
//                         | Companions cost 30% less | Begleiter kosten 30% weniger
// ── UTILITY (3) ──
// PERK_PACK_RAT           | "Pack Rat"           | "Hamsterer"
//                         | +1 companion slot    | +1 Begleiterslot
// PERK_WARP_DRIVE         | "Warp Drive"         | "Warpantrieb"
//                         | Dash 2x farther + damage trail | Dash 2x weiter + Schadensspur
// PERK_SOUL_SURGE         | "Soul Surge"         | "Seelenansturm"
//                         | Wave clear heals all 20% | Wellensieg heilt alle 20%

struct PerkEntry { StrEntry name; StrEntry desc; };

constexpr int PERK_COUNT = 30;
constexpr PerkEntry kPerks[PERK_COUNT] = {
    // ── OFFENSE ──
    /* [0]  BULLET_HELL     */ { S("Bullet Hell",     "Kugelhagel"),        S("Fire in all 4 directions!",        "Feuere in alle 4 Richtungen!") },
    /* [1]  OVERCHARGE      */ { S("Overcharge",      "UEberladung"),      S("Companions fire 50% faster",       "Begleiter feuern 50% schneller") },
    /* [2]  GLASS_CANNON    */ { S("Glass Cannon",    "Glaskanone"),        S("+5 damage, half max HP",           "+5 Schaden, halbe max. LP") },
    /* [3]  CHAIN_LIGHTNING */ { S("Chain Lightning", "Kettenblitz"),       S("Kills zap a nearby enemy",         "Kills treffen einen nahen Feind") },
    // ── DEFENSE ──
    /* [4]  PHOENIX_DOWN    */ { S("Phoenix Down",    "Phoenixfeder"),      S("First fallen ally revives/wave",   "1 Begleiter lebt pro Welle wieder auf") },
    /* [5]  FORTRESS        */ { S("Fortress",        "Festung"),           S("+40 HP, allies +20 HP, 25% slow",  "+40 LP, Verb. +20 LP, 25% langsamer") },
    /* [6]  SHIELD_BASH     */ { S("Shield Bash",     "Schildstoss"),       S("Dash deals 8 damage to enemies",   "Ausweichen macht 8 Schaden") },
    /* [7]  SECOND_WIND     */ { S("Second Wind",     "Zweite Luft"),       S("Full heal once when near death",   "Volle Heilung bei Todesgefahr") },
    // ── ECONOMY ──
    /* [8]  GOLD_FEVER      */ { S("Gold Fever",      "Goldfieber"),        S("2x gold for 3 waves!",            "2x Gold fuer 3 Wellen!") },
    /* [9]  WAR_CHEST       */ { S("War Chest",       "Kriegskasse"),       S("Get 15g now, +3g each wave",       "15g sofort, +3g jede Welle") },
    /* [10] JACKPOT         */ { S("Jackpot",         "Jackpot"),           S("Interest cap doubled to 10",       "Zinsobergrenze auf 10 verdoppelt") },
    /* [11] WHOLESALE       */ { S("Wholesale",       "Grosshandel"),       S("Companions cost 30% less",         "Begleiter kosten 30% weniger") },
    // ── UTILITY ──
    /* [12] PACK_RAT        */ { S("Pack Rat",        "Hamsterer"),         S("+1 companion slot",                "+1 Begleiterslot") },
    /* [13] WARP_DRIVE      */ { S("Warp Drive",      "Warpantrieb"),       S("Dash 2x far + damage trail",      "Dash 2x weiter + Schadensspur") },
    /* [14] SOUL_SURGE      */ { S("Soul Surge",      "Seelenansturm"),     S("Wave clear heals all 20%",         "Wellensieg heilt alle 20%") },
    // ── TRAIL MIX: OFFENSE ──
    /* [15] RICOCHET        */ { S("Ricochet",        "Ricochet"),          S("Bullets bounce off walls once",    "Kugeln prallen einmal ab") },
    /* [16] BLOODLUST       */ { S("Bloodlust",       "Blutrausch"),        S("+3% fire rate per kill this wave", "+3% Feuerrate pro Kill diese Welle") },
    /* [17] ECHO_CHAMBER    */ { S("Echo Chamber",    "Echokammer"),        S("Every 5th shot fires a bonus",     "Jeder 5. Schuss feuert Bonuskugel") },
    // ── TRAIL MIX: DEFENSE ──
    /* [18] THORNS          */ { S("Thorns",          "Dornen"),            S("Companions hurt attackers",        "Begleiter verletzen Angreifer") },
    /* [19] LAST_STAND      */ { S("Last Stand",      "Letztes Gefecht"),   S("Last companion deals 3x damage",  "Letzter Begleiter macht 3x Schaden") },
    /* [20] JUGGERNAUT      */ { S("Juggernaut",      "Koloss"),            S("Take 1 less damage from all hits","Erhalte 1 weniger Schaden pro Treffer") },
    // ── TRAIL MIX: ECONOMY ──
    /* [21] DOUBLE_OR_NOTHING*/ { S("Double or Nothing","Alles oder Nichts"),S("50/50: double gold or nothing!", "50/50: Doppelgold oder nichts!") },
    /* [22] SHORTCUT        */ { S("Shortcut",        "Abkuerzung"),        S("Merge with only 2 copies!",       "Verschmelze mit nur 2 Kopien!") },
    /* [23] BLACK_MARKET    */ { S("Black Market",    "Schwarzmarkt"),      S("Better rarity in shop",            "Bessere Seltenheit im Laden") },
    /* [24] MAGNET          */ { S("Magnet",          "Magnet"),            S("Double gold pickup range",         "Doppelte Gold-Aufsammelweite") },
    /* [25] PENSION         */ { S("Pension",         "Rente"),             S("+5g bonus at wave end",            "+5g Bonus bei Wellenende") },
    /* [26] LOAN_SHARK      */ { S("Loan Shark",      "Kredithai"),         S("Get 80g now, pay 10g/wave for 10","80g sofort, 10g/Welle fuer 10 Wellen") },
    // ── TRAIL MIX: UTILITY ──
    /* [27] REWIND          */ { S("Rewind",          "Rueckspulen"),       S("Enemies flee for 2s at wave start","Feinde fliehen 2s bei Wellenstart") },
    /* [28] BOUNTY_BOARD    */ { S("Bounty Board",    "Kopfgeldtafel"),     S("One marked enemy drops 3x gold",  "Ein markierter Feind gibt 3x Gold") },
    /* [29] WILDCARD        */ { S("Wildcard",        "Joker"),             S("All companions count for all synergies!","Alle Begl. zaehlen fuer alle Synergien!") },
};

// ─────────────────────────────────────────────────────────────────
// 4. ENEMY NAMES  (matches Enemy::type values)
// ─────────────────────────────────────────────────────────────────
// STR_ID             | English     | German
// ---------------------------------------------------------------
// STR_ENEMY_SQUARE   | "Square"    | "Quadrat"
// STR_ENEMY_TRIANGLE | "Triangle"  | "Dreieck"
// STR_ENEMY_CIRCLE   | "Circle"    | "Kreis"
// STR_ENEMY_DIAMOND  | "Diamond"   | "Raute"
// STR_ENEMY_HEXAGON  | "Hexagon"   | "Sechseck"
// STR_ENEMY_BOSS     | "Boss"      | "Boss"

constexpr int ENEMY_TYPE_COUNT = 6;
constexpr StrEntry kEnemyNames[ENEMY_TYPE_COUNT] = {
    /* [0] */ S("Square",   "Quadrat"),
    /* [1] */ S("Triangle", "Dreieck"),
    /* [2] */ S("Circle",   "Kreis"),
    /* [3] */ S("Diamond",  "Raute"),
    /* [4] */ S("Hexagon",  "Sechseck"),
    /* [5] */ S("Boss",     "Boss"),
};

// ─────────────────────────────────────────────────────────────────
// 5. GENERAL GAMEPLAY NOTIFICATIONS
// ─────────────────────────────────────────────────────────────────
// STR_ID                   | English              | German
// ---------------------------------------------------------------
// STR_GAME_WAVE_CLEAR      | "Wave Clear!"        | "Welle geschafft!"
// STR_GAME_NEW_WAVE        | "New Wave!"          | "Neue Welle!"
// STR_GAME_BOSS_INCOMING   | "Boss Incoming!"     | "Boss naht!"
// STR_GAME_MERGE           | "Merge!"             | "Verschmelzung!"
// STR_GAME_SYNERGY_ACT     | "Synergy Activated!" | "Synergie aktiv!"
// STR_GAME_NO_GOLD         | "Not Enough Gold"    | "Zu wenig Gold"
// STR_GAME_INV_FULL        | "Inventory Full"     | "Inventar voll"
// STR_GAME_SOLD            | "Sold!"              | "Verkauft!"
// STR_GAME_LEVEL_UP        | "Level Up!"          | "Aufgestiegen!"
// STR_GAME_CHOOSE_UPGRADE  | "Choose Upgrade"     | "Upgrade wählen"

constexpr StrEntry kGameplay[] = {
    /* [0]  WAVE_CLEAR     */ S("Wave Clear!",        "Welle geschafft!"),
    /* [1]  NEW_WAVE       */ S("New Wave!",           "Neue Welle!"),
    /* [2]  BOSS_INCOMING  */ S("Boss Incoming!",      "Boss naht!"),
    /* [3]  MERGE          */ S("Merge!",              "Verschmelzung!"),
    /* [4]  SYNERGY_ACT    */ S("Synergy Activated!",  "Synergie aktiv!"),
    /* [5]  NO_GOLD        */ S("Not Enough Gold",     "Zu wenig Gold"),
    /* [6]  INV_FULL       */ S("Inventory Full",      "Inventar voll"),
    /* [7]  SOLD           */ S("Sold!",               "Verkauft!"),
    /* [8]  LEVEL_UP       */ S("Level Up!",           "Aufgestiegen!"),
    /* [9]  CHOOSE_UPGRADE */ S("Choose Upgrade",      "Upgrade wählen"),
    /* [10] BOSS_DEFEATED  */ S("BOSS DEFEATED!",      "BOSS BESIEGT!"),
    /* [11] CHOOSE_A_PERK  */ S("CHOOSE A PERK",       "WAEHLE EIN TALENT"),
};

// ─────────────────────────────────────────────────────────────────
// 6. CLASS NAMES  (placeholders — fill in when final names are ready)
//    Layout: 6 colors × 6 base tiers + 6 upgraded = 72 strings
//    Naming convention: CLASS_<COLOR>_<N>  /  CLASS_<COLOR>_<N>_UP
// ─────────────────────────────────────────────────────────────────
// STR_ID              | English placeholder     | German placeholder
// ---------------------------------------------------------------
// CLASS_RED_1         | "CLASS_RED_1"           | "KLASSE_ROT_1"
// CLASS_RED_1_UP      | "CLASS_RED_1_UP"        | "KLASSE_ROT_1_AUFG"
// ... (pattern repeats for _2 through _6 and each color)

constexpr int CLASS_TIERS  = 6;
// Indexed as kClassNames[color][tier][0=base, 1=T2, 2=T3]
constexpr StrEntry kClassNames[PILL_COLOR_COUNT][CLASS_TIERS][3] = {
    /* Red */
    {
        { S("Gunner",       "Kanonier"),      S("Gatling",            "Gatling"),          S("Obliterator",      "Ausloescher")       },
        { S("Pyromaniac",   "Pyromane"),      S("Infernalist",        "Infernalist"),      S("Ragnarok Flame",   "Ragnaroekfeuer")    },
        { S("Shotgunner",   "Schrotling"),    S("Siegebreaker",       "Mauerbrecher"),     S("Ruinbringer",      "Ruinenbringer")       },
        { S("Berserker",    "Berserker"),     S("Bloodrager",         "Blutraser"),        S("Godslayer",        "Goettermoerder")  },
        { S("Detonator",    "Sprengmeister"), S("Annihilator",        "Vernichter"),       S("Worldender",       "Weltenzerstoerer")  },
        { S("Executioner",  "Scharfrichter"), S("Reaper",             "Schnitter"),        S("Death Incarnate",  "Tod Inkarniert")      },
    },
    /* Blue */
    {
        { S("Warden",       "Waechter"),      S("Bastion",            "Bastion"),          S("Eternal Bulwark",    "Ewiges Bollwerk")       },
        { S("Channeler",    "Kanalwirker"),   S("Riftweaver",         "Rissknuepfer"),     S("Planar Sovereign",   "Planarsouveraen")     },
        { S("Frost Archer", "Frostschuetze"), S("Glacial Marksman",   "Glazialschuetze"),  S("Absolute Zero",     "Absoluter Nullpunkt")   },
        { S("Orbiter",      "Orbiter"),       S("Astral Sentinel",    "Astralwache"),      S("Celestial Arbiter",  "Himmlischer Richter")   },
        { S("Chronomancer", "Chronomant"),    S("Aeonkeeper",         "Aeonhueter"),       S("Lord of Ages",       "Herr der Zeitalter")    },
        { S("Nullifier",    "Annullierer"),   S("Voidcaster",         "Leerenwerfer"),     S("Entropy Incarnate",  "Entropie Inkarniert")   },
    },
    /* Green */
    {
        { S("Thornshot",    "Dornschuss"),    S("Briarstorm",         "Dornensturm"),      S("Nature's Wrath",     "Zorn der Natur")        },
        { S("Seedling",     "Saemling"),      S("Ancient Grove",      "Urwald"),            S("World Overgrowth",   "Weltenwucherung")       },
        { S("Apothecary",   "Apotheker"),     S("Herbalist",          "Kraeuterkundige"),S("Eternal Healer",     "Ewiger Heiler")         },
        { S("Vinecaller",   "Rankenrufer"),   S("Rootwarden",         "Wurzelwacht"),       S("Primordial Titan",   "Urtitan")               },
        { S("Sporewitch",   "Sporenhexe"),    S("Blightmother",       "Seuchenmutter"),     S("Plague Sovereign",   "Seuchensouveraen")    },
        { S("Lifeshaper",   "Lebensformer"),  S("Worldtree",          "Weltenbaum"),        S("Yggdrasil",          "Yggdrasil")             },
    },
    /* Yellow */
    {
        { S("Gambler",      "Gluecksspieler"),S("High Roller",        "Hasardeur"),        S("Fate's Sovereign",   "Schicksalsherr")        },
        { S("Scavenger",    "Sammler"),       S("Hoarder",            "Hamsterer"),        S("Dragon of Greed",    "Drache der Gier")       },
        { S("Jester",       "Narr"),          S("Fool King",          "Narrenkoenig"),     S("God of Chaos",       "Gott des Chaos")        },
        { S("Mimic",        "Mimik"),         S("Doppelgaenger",      "Doppelgaenger"),    S("Primordial Mirror",  "Urspiegelbild")         },
        { S("Alchemist",    "Alchemist"),     S("Philosopher",        "Philosoph"),        S("Aurum Eternal",      "Ewiges Aurum")          },
        { S("Trickster",    "Gauner"),        S("Mastermind",         "Mastermind"),        S("Grand Architect",    "Grosser Architekt") },
    },
    /* Purple */
    {
        { S("Hexer",          "Hexer"),             S("Grand Hexer",    "Grosshexer"),         S("Damnation Lord",   "Verdammungsherr")       },
        { S("Blightling",     "Seuchenwesen"),      S("Plague Spawn",   "Seuchenbrut"),          S("Pestilence God",   "Seuchengott")           },
        { S("Plague Doctor",  "Pestdoktor"),        S("Miasma Surgeon", "Miasma-Chirurg"),       S("Death's Vessel",   "Gefaess des Todes") },
        { S("Wraith",         "Geist"),             S("Banshee",        "Banshee"),              S("Soul Devourer",    "Seelenverschlinger")    },
        { S("Voidcaller",     "Leerenrufer"),       S("Abyssal Herald", "Abyssherold"),          S("Void Sovereign",   "Leereherrscher")        },
        { S("Nightmare",      "Albtraum"),          S("Living Dread",   "Lebender Schrecken"),   S("Eternal Terror",   "Ewiger Schrecken")      },
    },
    /* Cyan */
    {
        { S("Drone Pilot",    "Drohnenpilot"),      S("Swarm Commander","Schwarmkommandant"),    S("Hive Sovereign",      "Schwarmherrscher")      },
        { S("Overclocker",    "UEbertakter"),           S("Overdriver",     "UEbertreiber"),            S("Singularity",         "Singularitaet")         },
        { S("Tesla Coil",     "Teslaspule"),        S("Arc Reactor",    "Bogenreaktor"),         S("Storm God",           "Sturmgott")             },
        { S("Signal Jammer",  "Signalstoerer"),   S("Blackout Node",  "Blackout-Knoten"),      S("Digital Apocalypse",  "Digitale Apokalypse")   },
        { S("Circuit Hacker", "Schaltungs-Hacker"), S("Ghost Protocol", "Geisterprotokoll"),     S("Omniscient",          "Allwissender")          },
        { S("Mech Pilot",     "Mechpilot"),         S("Titan Frame",    "Titanrahmen"),          S("Colossus Prime",      "Urkoloss")              },
    },
};

// ─────────────────────────────────────────────────────────────────
// 7. CLASS ABILITY DESCRIPTIONS  (shot + passive, per class)
//    Layout: kClassAbility[color][classId]  → { shot, passive }
// ─────────────────────────────────────────────────────────────────

struct ClassAbilityEntry { StrEntry shot; StrEntry passive; };
constexpr ClassAbilityEntry kClassAbility[PILL_COLOR_COUNT][6] = {
    /* Red */ {
        /* R1 Gunner       */ { S("Every 5th shot is a 3x tracer",  "Jeder 5. Schuss ist 3x Leuchtsp."),    S("All allies deal +8% damage",         "Alle Verbuendeten +8% Schaden")          },
        /* R2 Pyromaniac   */ { S("Lobs a fireball that explodes",  "Wirft einen explodierenden Feuerball"),S("Burns nearby enemies slowly",        "Verbrennt nahe Gegner langsam")          },
        /* R3 Shotgunner   */ { S("Blasts 3 bullets in a cone",     "Feuert 3 Kugeln im Kegel"),            S("+10% damage to close enemies",       "+10% Schaden auf nahe Gegner")           },
        /* R4 Berserker    */ { S("Attacks faster at low HP",       "Greift bei wenig LP schneller an"),    S("All allies +15% damage below half HP","Alle Verb. +15% Sch unter halben LP")   },
        /* R5 Detonator    */ { S("Fires a bomb with big blast",    "Feuert Bombe mit grosser Explosion"),  S("No passive",                         "Kein Passiv")                            },
        /* R6 Executioner  */ { S("Heavy slow pierce bolt at 2x dmg","Schwerer langsamer 2x Bolzen"),       S("No passive",                         "Kein Passiv")                            },
    },
    /* Blue */ {
        /* B1 Warden       */ { S("Damages all nearby enemies",     "Schadet allen nahen Gegnern"),         S("Slows nearby enemies by 10%",        "Verlangsamt nahe Gegner um 10%")         },
        /* B2 Channeler    */ { S("Rapid 3-round burst",            "Schnelle 3er-Salve"),                  S("All allies shoot 5% faster",         "Alle Verbuendeten 5% schneller")         },
        /* B3 Frost Archer */ { S("Shoots ice arrows that slow",    "Schiesst verlangsamende Eispfeile"),   S("Slowed enemies take +10% damage",    "Verlangsamte Gegner +10% Schaden")       },
        /* B4 Orbiter      */ { S("Double shot volley",             "Doppelschuss-Salve"),                  S("No passive",                         "Kein Passiv")                            },
        /* B5 Chronomancer */ { S("Freezes a target in time",       "Friert ein Ziel in der Zeit ein"),     S("No passive",                         "Kein Passiv")                            },
        /* B6 Nullifier    */ { S("Erases enemy projectiles",       "Loescht feindliche Geschosse"),        S("Enemy debuffs last 15% longer",      "Gegner-Debuffs 15% laenger")             },
    },
    /* Green */ {
        /* G1 Thornshot    */ { S("Fires a wall of thorns",         "Feuert eine Dornenwand"),              S("Regenerate 1 HP every 5 seconds",    "Regeneriere 1 LP alle 5 Sekunden")       },
        /* G2 Seedling     */ { S("Plants a timed mine",            "Pflanzt eine Zeitmine"),               S("Allies follow more tightly",         "Verbuendete folgen enger")               },
        /* G3 Apothecary   */ { S("Heals weakest ally",              "Heilt schwaechsten Verb."),          S("Picking up gold heals 1 HP",      "Gold aufheben heilt 1 LP")               },
        /* G4 Vinecaller   */ { S("Sends 3 slow tendrils",          "Sendet 3 langsame Ranken"),            S("No passive",                         "Kein Passiv")                            },
        /* G5 Sporewitch   */ { S("Triple spore spread",            "Dreifach-Sporensalve"),                S("No passive",                         "Kein Passiv")                            },
        /* G6 Lifeshaper   */ { S("Heal pulse to all allies",       "Heilpuls an alle Verb."),              S("You gain +5 max HP",                 "Du erhaeltst +5 max LP")                 },
    },
    /* Yellow */ {
        /* Y1 Gambler      */ { S("Deals random 0x to 3x damage",  "Verursacht zufaellig 0x-3x Schaden"),  S("Rerolls cost 2 gold less",           "Neuwuerfeln kostet 2 Gold weniger")      },
        /* Y2 Scavenger    */ { S("Shoots backward fan of 3",       "Schiesst 3er-Faecher nach hinten"),    S("Earn 10% more gold",                 "Verdiene 10% mehr Gold")                 },
        /* Y3 Jester       */ { S("Piercing trick shot",            "Durchdringender Trickschuss"),         S("A random ally gains +2 speed",       "Ein zufaelliger Verb. +2 Tempo")         },
        /* Y4 Mimic        */ { S("Copies nearest ally attack",     "Kopiert Angriff des naechsten Verb."), S("No passive",                         "Kein Passiv")                            },
        /* Y5 Alchemist    */ { S("Gold-seeking shot",              "Goldsuchschuss"),                      S("Shop shows 1 extra card",            "Laden zeigt 1 Karte mehr")               },
        /* Y6 Trickster    */ { S("Fires bullets in all 8 dirs",    "Feuert in alle 8 Richtungen"),         S("Enemies drop 15% more gold",         "Gegner lassen 15% mehr Gold fallen")     },
    },
    /* Purple */ {
        /* P1 Hexer        */ { S("Curse bolt + 2 spreading hexes", "Fluchbolzen + 2 Splitterflueche"),     S("All debuffs last 25% longer",        "Alle Debuffs dauern 25% laenger")        },
        /* P2 Blightling   */ { S("Shoots 3 poison darts",          "Schiesst 3 Giftpfeile"),               S("Earn 10% more gold",                 "Verdiene 10% mehr Gold")                 },
        /* P3 Plague Doctor */ { S("Launches a toxic cloud",         "Startet eine Giftwolke"),              S("All allies deal +8% damage",         "Alle Verbuendeten +8% Schaden")          },
        /* P4 Wraith       */ { S("Pierce bolt forward + backward", "Durchbohrend vor + zurueck"),          S("No passive",                         "Kein Passiv")                            },
        /* P5 Voidcaller   */ { S("Deep freeze bolt",               "Tiefkuehlbolzen"),                   S("No passive",                         "Kein Passiv")                            },
        /* P6 Nightmare    */ { S("Emits a fear pulse around self", "Sendet Furchtpuls um sich"),            S("All debuffs last 25% longer",        "Alle Debuffs dauern 25% laenger")        },
    },
    /* Cyan */ {
        /* C1 Drone Pilot  */ { S("Sends drones at 3 targets",     "Sendet Drohnen auf 3 Ziele"),          S("No passive",                         "Kein Passiv")                            },
        /* C2 Overclocker  */ { S("Fires faster until AoE overheat","Immer schneller bis AoE-Explosion"),   S("Nearby allies shoot 15% faster",     "Nahe Verb. schiessen 15% schneller")     },
        /* C3 Tesla Coil   */ { S("Lightning chains to 2 nearby",   "Blitz springt auf 2 Nahe ueber"),      S("No passive",                         "Kein Passiv")                            },
        /* C4 Signal Jammer*/ { S("Erases enemy shots",             "Loescht Gegnerschuesse"),            S("Enemy debuffs last 10% longer",      "Gegner-Debuffs 10% laenger")             },
        /* C5 Circuit Hacker*/{ S("Long freeze bolt",               "Langer Eisbolzen"),                    S("No passive",                         "Kein Passiv")                            },
        /* C6 Mech Pilot   */ { S("Fires a heavy cannon blast",     "Feuert schweren Kanonenschuss"),       S("All allies deal +15% damage",        "Alle Verbuendeten +15% Schaden")         },
    },
};

// ─────────────────────────────────────────────────────────────────
// ACCESSOR
// ─────────────────────────────────────────────────────────────────
extern StrLang gActiveLang;

inline const char* str(const StrEntry& e) {
    return (gActiveLang == StrLang::DE) ? e.de : e.en;
}

#endif // STRINGS_H
