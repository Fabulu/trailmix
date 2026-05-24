#ifndef CAMERA_H
#define CAMERA_H
#include "types.h"

void cameraInit();
void cameraUpdate(Vec2 target);  // center camera on target (player pos)
int camX();  // current camera X pixel offset
int camY();  // current camera Y pixel offset
// Screen shake: adds random offset for N frames
void cameraShake(u8 intensity, u8 frames);

#endif
