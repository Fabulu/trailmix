#ifndef ENCOUNTER_H
#define ENCOUNTER_H

// Post-boss encounter screen (GameState::Encounter).
// Modeled after perk_choice.h.

void encounterEnter();      // roll encounter, set up display state
bool encounterUpdate();     // handle input; returns true when done -> advance to PerkChoice
void encounterRender();     // draw both screens

#endif // ENCOUNTER_H
