/**
 * Copyright (c) 2021-2022 Sirvoid
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <limits.h>
#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "world.h"
#include "raycast.h"
#include "screens.h"
#include "chat.h"
#include "block.h"
#include "networkhandler.h"
#include "packet.h"

#define MOUSE_SENSITIVITY 0.003f

Vector2 Player_oldMousePos = {0.0f, 0.0f};
Vector2 Player_cameraAngle = {0.0f, 0.0f};
double Player_LastPositionPacketTime;
Player player;

void Player_Init(void) {

    Camera camera = { 0 };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 65.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    player.camera = camera;
    
    player.velocity = (Vector3) {0, 0, 0};
    player.position = (Vector3) { 0, 80, 0 };
    player.speed = 0.125f / 6;
    
    player.collisionBox.min = (Vector3) { 0.2f, 0, 0.2f };
    player.collisionBox.max = (Vector3) { 0.8f, 1.5f, 0.8f };

    player.blockSelected = 15;

    Player_LastPositionPacketTime = 0;

    UpdateCamera(&player.camera, CAMERA_CUSTOM);
    DisableCursor();
}

void Player_CheckInputs() {
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (Screen_cursorEnabled) {
            DisableCursor();
            Chat_open = false;
            Screen_Switch(SCREEN_GAME);
        } else {
            EnableCursor();
            Screen_Switch(SCREEN_PAUSE);
        }
        Screen_cursorEnabled = !Screen_cursorEnabled;
    } else if (IsKeyPressed(KEY_T)) {
        if (Screen_cursorEnabled && !Chat_open) {
            DisableCursor();
            Screen_cursorEnabled = false;
            Screen_Switch(SCREEN_GAME);
        } else {
            Chat_open = true;
            EnableCursor();
            Screen_cursorEnabled = true;
        }
    }
    
    
    Vector2 mousePositionDelta = { 0.0f, 0.0f };
    Vector2 mousePos = GetMousePosition();
    
    mousePositionDelta.x = mousePos.x - Player_oldMousePos.x;
    mousePositionDelta.y = mousePos.y - Player_oldMousePos.y;
    
    Player_oldMousePos = GetMousePosition();
    
    if (!Screen_cursorEnabled) {
        Player_cameraAngle.x -= (mousePositionDelta.x * -MOUSE_SENSITIVITY);
        Player_cameraAngle.y -= (mousePositionDelta.y * -MOUSE_SENSITIVITY);
        
        //Limit head rotation
        float maxCamAngleY = PI - 0.01f;
        float minCamAngleY = 0.01f;
        
        if (Player_cameraAngle.y >= maxCamAngleY) 
            Player_cameraAngle.y = maxCamAngleY;
        else if (Player_cameraAngle.y <= minCamAngleY) 
            Player_cameraAngle.y = minCamAngleY;
    }
    
    
    //Calculate direction vectors of the camera angle
    float cx = cosf(Player_cameraAngle.x);
    float sx = sinf(Player_cameraAngle.x);
    
    float cx90 = cosf(Player_cameraAngle.x + PI / 2);
    float sx90 = sinf(Player_cameraAngle.x + PI / 2);
    
    float sy = sinf(Player_cameraAngle.y);
    float cy = cosf(Player_cameraAngle.y);
    
    float forwardX = cx * sy;
    float forwardY = cy;
    float forwardZ = sx * sy;
    
    if (!Screen_cursorEnabled) {
        //Handle keys & mouse
        switch(player.canFly) {
        case true:
            if (IsKeyDown(KEY_SPACE)) {
                player.velocity.y += 0.2f;
                player.velocity.y = Clamp(player.velocity.y, 0.0f, 0.2f);
            } else if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                player.velocity.y -= 0.2f;
                player.velocity.y = Clamp(player.velocity.y, 0.0f, -0.2f);
            } else {
                player.velocity.y = 0.0f;
            }
        case false:
            if (IsKeyDown(KEY_SPACE) && player.canJump) {
                player.velocity.y += 0.2f;
                player.canJump = false;
            }
        }
        Vector3 moveDir = { 0 };

        if (IsKeyPressed(KEY_F))
            player.canFly = !player.canFly;
        
        if (IsKeyDown(KEY_W)) {
            moveDir.z += sx;
            moveDir.x += cx;
        }
        
        if (IsKeyDown(KEY_S)) {
            moveDir.z -= sx;
            moveDir.x -= cx;
        }
        
        if (IsKeyDown(KEY_A)) {
            moveDir.z -= sx90;
            moveDir.x -= cx90;
        }
        
        if (IsKeyDown(KEY_D)) {
            moveDir.z += sx90;
            moveDir.x += cx90;
        }

        moveDir = Vector3ClampValue(moveDir, 0.0f, 1.0f); // normalize
        Vector3 moveVel = Vector3Scale(moveDir, player.speed);
        player.velocity = Vector3Add(player.velocity, moveVel);
        
        float wheel = GetMouseWheelMove();
        if (wheel > 0.35f) player.blockSelected++;
        if (wheel < -0.35f) player.blockSelected--;
        if (player.blockSelected > 18) player.blockSelected = 1;
        if (player.blockSelected < 1) player.blockSelected = 18;
        
        player.rayResult = Raycast_Do(player.camera.position, (Vector3) { forwardX, forwardY, forwardZ}, true);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { //Break Block
            if (player.rayResult.hitBlockID != -1) {
                World_SetBlock(player.rayResult.hitPos, 0, true);
                Network_Send(Packet_SetBlock(0, player.rayResult.hitPos));
            }
        } else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) { //Place Block
            Vector3 placePos = Vector3Add(player.rayResult.hitPos, player.rayResult.normal);
            
            if (player.rayResult.hitBlockID != -1) {
                int bottomBlockID = World_GetBlock(Vector3Add(placePos, (Vector3){0, -1, 0}));
                switch (player.blockSelected)
                {
                    case -1: // null
                    case 0: // air
                        break;

                    case 12:
                    case 13:
                        if (bottomBlockID == 2 || bottomBlockID == 3 || bottomBlockID == 6) { // dirt, grass, sand
                            Player_TryPlaceBlock(placePos, player.blockSelected);
                        }
                        break;
                        
                    case 17: // stone_slab
                        if (player.rayResult.normal.y == 1 && player.rayResult.hitBlockID == 17) {
                            Player_TryPlaceBlock(player.rayResult.hitPos, 1);
                            break;
                        }

                    case 18: // wood_slab
                        if (player.rayResult.normal.y == 1 && player.rayResult.hitBlockID == 18) {
                            Player_TryPlaceBlock(player.rayResult.hitPos, 4);
                            break;
                        }

                    default:
                        Player_TryPlaceBlock(placePos, player.blockSelected);
                        break;
                }
            }
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) { //Pick Block
            int pickedID = World_GetBlock(player.rayResult.hitPos);
            player.blockSelected = pickedID;
        }
    }
    //Place camera's target to the direction looking at.
    player.camera.target.x = player.camera.position.x + forwardX;
    player.camera.target.y = player.camera.position.y + forwardY;
    player.camera.target.z = player.camera.position.z + forwardZ;
}

bool Player_TryPlaceBlock(Vector3 pos, int blockID)
{
    int oldBlock = World_GetBlock(pos);
    World_SetBlock(pos, blockID, true);
    if (Player_TestCollision((Vector3){ 0 }))
    {
        World_SetBlock(pos, oldBlock, true);
        return false;
    }

    Network_Send(Packet_SetBlock(blockID, pos));
    return true;
}



void Player_Update(void) {
    
    if(GetTime() - Player_LastPositionPacketTime > 0.05) {
        Network_Send(Packet_PlayerPosition((Vector3) { player.position.x + 0.5f, player.position.y, player.position.z + 0.5f }, (Vector2) { -Player_cameraAngle.x + PI / 2,  Player_cameraAngle.y - PI / 2}));
        Player_LastPositionPacketTime = GetTime();
    }

    //Gravity
    if (!player.canFly) {
        player.velocity.y -= 0.012f * (GetFrameTime() * 60);
        if (player.velocity.y <= -1) player.velocity.y = -1;
    }
    
    //Calculate velocity with delta time
    Vector3 velXdt = Vector3Scale(player.velocity, GetFrameTime() * 60);
    
    int steps = 8;
    
    //Move Y & Test Collisions
    for (int i = 0; i < steps; i++) {
        player.position.y += velXdt.y / steps;
        if (Player_TestCollision((Vector3){ 0 })) {
            player.position.y -= velXdt.y / steps;
            if (player.velocity.y <= 0) player.canJump = true;
            player.velocity.y = 0;
            break;
        } else {
            player.canJump = false;
        }
    }

    //Move X & Test Collisions
    for (int i = 0; i < steps; i++) {
        player.position.x += velXdt.x / steps;
        if (Player_TestCollision((Vector3){ 0 })) {
            if (player.velocity.y != 0 || Player_TestCollision((Vector3){0,0.51f,0})) {
                player.position.x -= velXdt.x / steps;
            } else {
                player.position.y += 0.1f / steps;
            }
        }
    }

    //Move Z & Test Collisions
    for (int i = 0; i < steps; i++) {
        player.position.z += velXdt.z / steps;
        if (Player_TestCollision((Vector3){ 0 })) {
            if (player.velocity.y != 0 || Player_TestCollision((Vector3){0,0.51f,0})) {
                player.position.z -= velXdt.z / steps;
            } else {
                player.position.y += 0.1f / steps;
                break;
            }
        }
    }

    World_LoadChunks();

    //Place Camera
    player.camera.position = player.position;
    player.camera.position.y += 1.5f;
    player.camera.position.x += 0.5f;
    player.camera.position.z += 0.5f;

    player.velocity.x -= player.velocity.x / 6.0f;
    player.velocity.z -=  player.velocity.z / 6.0f;
    
    Player_CheckInputs(); 
}

bool Player_TestCollision(Vector3 offset) {
    
    BoundingBox pB = player.collisionBox;
    pB.min = Vector3Add(Vector3Add(pB.min, player.position), offset);
    pB.max = Vector3Add(Vector3Add(pB.max, player.position), offset);
    
    for (int x = (int)(pB.min.x - 1); x < (int)(pB.max.x + 1); x++) {
        for (int z = (int)(pB.min.z - 1); z < (int)(pB.max.z + 1); z++) {
            for (int y = (int)(pB.min.y - 1); y < (int)(pB.max.y + 1); y++) {
                Vector3 blockPos = (Vector3) {x, y, z};

                Vector3 chunkPos = (Vector3) { floor(blockPos.x / CHUNK_SIZE_X), floor(blockPos.y / CHUNK_SIZE_Y), floor(blockPos.z / CHUNK_SIZE_Z) };
                Chunk* chunk = World_GetChunkAt(chunkPos);
                if (chunk == NULL || chunk->isMapGenerated == false) return true;

                int blockID = World_GetBlock(blockPos);
                Block blockDef = Block_GetDefinition(blockID);
                if (blockDef.colliderType != BlockColliderType_Solid) continue;
                
                BoundingBox blockB;
                blockB.min = (Vector3) {x + (blockDef.minBB.x / 16), y + (blockDef.minBB.y / 16), z + (blockDef.minBB.z / 16)};
                blockB.max = (Vector3) {x + (blockDef.maxBB.x / 16), y + (blockDef.maxBB.y / 16), z + (blockDef.maxBB.z / 16)};
                
                if (CheckCollisionBoxes(pB, blockB)) return true;
            }
        }
    }
    
    return false;
}


Vector3 Player_GetForwardVector(void) {
    float cx = cosf(Player_cameraAngle.x);
    float sx = sinf(Player_cameraAngle.x);
    
    float sy = sinf(Player_cameraAngle.y);
    float cy = cosf(Player_cameraAngle.y);
    
    return (Vector3) {cx * sy, cy, sx * sy};
}

Vector3 Player_GetChunkPosition(void) {
    return (Vector3) {(int)floor(player.position.x / CHUNK_SIZE_X), (int)floor(player.position.y / CHUNK_SIZE_Y), (int)floor(player.position.z / CHUNK_SIZE_Z)};
}