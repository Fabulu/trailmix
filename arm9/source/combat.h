#ifndef COMBAT_H
#define COMBAT_H

#include "types.h"
#include "entities.h"

void combatUpdate();
void killEnemy(Enemy& e, u8 bulletColor);

#endif // COMBAT_H
