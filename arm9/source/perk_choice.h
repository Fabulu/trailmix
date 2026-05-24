#ifndef PERK_CHOICE_H
#define PERK_CHOICE_H

// Post-boss perk selection screen.
// Called from game.cpp when a boss wave clears.

// Enter: generate 3 random unowned perks and reset selection state.
void perkChoiceEnter();

// Update: handle touch/button input.  Returns true when the player has
// confirmed a pick and the caller should advance to Shop.
bool perkChoiceUpdate();

// Render: draw both screens for the perk choice state.
void perkChoiceRender();

#endif // PERK_CHOICE_H
