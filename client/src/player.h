/**
 * Copyright (c) 2021-2022 Sirvoid
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#ifndef G_PLAYER_H
#define G_PLAYER_H

#include "raylib.h"
#include "raycast.h"

typedef struct Player{
    Camera camera;
    float speed;
    Vector3 position;
    Vector3 direction;
    Vector3 velocity;
    BoundingBox collisionBox;
    RaycastResult rayResult;
    int blockSelected;
    bool canJump;
    bool canFly;
} Player;
extern Player player;

//Initialize a player.
void Player_Init(void);

//Check/Do Inputs
void Player_CheckInputs(void);

//Update a player.
void Player_Update(void);

bool Player_TryPlaceBlock(Vector3 pos, int blockID);

bool Player_TestCollision(Vector3 offset);
Vector3 Player_GetForwardVector(void);

//Get player position in chunk units.
Vector3 Player_GetChunkPosition(void);

#endif