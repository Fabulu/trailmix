#ifndef SAYINGS_VIEWER_H
#define SAYINGS_VIEWER_H

// Collection viewer accessible from the start screen.

void sayingsViewerEnter();  // set up viewer state
bool sayingsViewerUpdate(); // handle input; returns true when player presses B to exit
void sayingsViewerRender(); // draw both screens

#endif // SAYINGS_VIEWER_H
