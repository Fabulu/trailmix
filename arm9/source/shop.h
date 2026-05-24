#ifndef SHOP_H
#define SHOP_H

#include "types.h"

constexpr int SHOP_CARDS = 6;

struct ShopCard {
    PillColor color;
    u8 classId;      // 0-5 within color
    u8 rarity;       // 0=common, 1=uncommon, 2=rare
    u16 price;
    bool sold;
    bool isPerk;
    u8 perkId;       // valid when isPerk
    bool locked;
};

struct ShopState {
    ShopCard cards[SHOP_CARDS];
    ShopCard perkCard;
    u8 rerollCount;
    s8 selectedCard;   // 0-5 = companion, 6 = perk
    s8 selectedCompanion; // inventory slot for sell preview (-1 = none)
    s8 lockTarget;     // -1 = none
    s8 errorCard;      // index of card that failed to buy (-1 = none)
    u8 errorTimer;     // frames remaining for red flash
};

extern ShopState gShop;

void shopGenerate(int wave);
// Returns true when player wants to start next wave
bool shopUpdate();
void shopRender();

#endif
