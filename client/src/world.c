/**
 * Copyright (c) 2021-2022 Sirvoid
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <raylib.h>
#include "chunkMeshGeneration.h"
#include "world.h"
#include "rlgl.h"
#include "raymath.h"
#include "worldgenerator.h"
#include "player.h"
#include "screens.h"

World world;
pthread_t chunkThread_id;

void World_Init(void) {
    world.mat = LoadMaterialDefault();
    world.loadChunks = false;
    world.drawDistance = 3;

    world.entities = MemAlloc(WORLD_MAX_ENTITIES * sizeof(Entity));
    for(int i = 0; i < WORLD_MAX_ENTITIES; i++) world.entities[i].type = 0; //type 0 = none

    WorldGenerator_Init(1);
}

void World_LoadSingleplayer(void) {

    Screen_Switch(SCREEN_GAME);

    world.chunks = MemAlloc(sizeof(Chunk));
    world.chunks->nextChunk = NULL;
    world.chunks->previousChunk = NULL;
    Chunk_Init(world.chunks, (Vector3){0,0,0});
    World_QueueChunk(world.chunks);

    world.loadChunks = true;

    pthread_create(&chunkThread_id, NULL, World_ReadChunksQueues, NULL);
}

pthread_mutex_t chunk_mutex;
void *World_ReadChunksQueues(void *state) {
    while(true) {
        
        
        if(world.loadChunks && world.generateChunksQueue != NULL) {
            Chunk_Generate(world.generateChunksQueue->chunk);

            pthread_mutex_lock(&chunk_mutex);
            world.generateChunksQueue = Chunk_PopFromQueue(world.generateChunksQueue);
            pthread_mutex_unlock(&chunk_mutex);
        }
        
        if(!world.loadChunks) return NULL;
    }
    return NULL;
}

void World_QueueChunk(Chunk *chunk) {
 
    if(!chunk->hasStartedGenerating) {
        pthread_mutex_lock(&chunk_mutex);
        world.generateChunksQueue = Chunk_AddToQueue(world.generateChunksQueue, chunk);
        pthread_mutex_unlock(&chunk_mutex);
    }
    chunk->hasStartedGenerating = true;

    if(chunk->isBuilding == false) {
        world.buildChunksQueue = Chunk_AddToQueue(world.buildChunksQueue, chunk);
        chunk->isBuilding = true;
    }
}

void World_AddChunk(Vector3 position) {

    Chunk *curChunk = world.chunks;
    if(curChunk == NULL) return;

    while(curChunk->nextChunk != NULL) {
        if(curChunk->position.x == position.x && curChunk->position.y == position.y && curChunk->position.z == position.z) { 
            return; //Already exists
        }
        curChunk = curChunk->nextChunk;
    }

    Chunk *newChunk = MemAlloc(sizeof(Chunk));
    newChunk->nextChunk = NULL;
    newChunk->previousChunk = curChunk;

    curChunk->nextChunk = newChunk;

    Chunk_Init(newChunk, position);
    World_QueueChunk(newChunk);
    Chunk_RefreshBorderingChunks(newChunk, true);
}

Chunk* World_GetChunkAt(Vector3 pos) {
    Chunk *chunk = world.chunks;
    while(chunk != NULL) {
        if(chunk->position.x == pos.x && chunk->position.y == pos.y && chunk->position.z == pos.z) {
            return chunk;
        }
        
        chunk = chunk->nextChunk;
    }
    
    return NULL;
}

void World_RemoveChunk(Vector3 position) {

    Chunk *curChunk = world.chunks;

    bool beenFound = false;
    while(curChunk != NULL) {
        if(curChunk->position.x == position.x && curChunk->position.y == position.y && curChunk->position.z == position.z) { 
            beenFound = true;
            break;
        }
        curChunk = curChunk->nextChunk;
    }

    if(!beenFound) return;

    Chunk* prevChunk = curChunk->previousChunk;
    Chunk *nextChunk = curChunk->nextChunk;

    if(curChunk == world.chunks) world.chunks = curChunk->nextChunk;
    if(prevChunk != NULL) prevChunk->nextChunk = nextChunk;
    if(nextChunk != NULL) nextChunk->previousChunk = prevChunk;
    
    Chunk_Unload(curChunk);
    MemFree(curChunk);
}

void World_LoadChunks() {

    if(!world.loadChunks) return;

    //Create chunks around
    Vector3 pos = (Vector3) {(int)floor(player.position.x / CHUNK_SIZE_X), (int)floor(player.position.y / CHUNK_SIZE_Y), (int)floor(player.position.z / CHUNK_SIZE_Z)};
    for(int y = world.drawDistance ; y >= -world.drawDistance; y--) {
        for(int x = -world.drawDistance ; x <= world.drawDistance; x++) {
            for(int z = -world.drawDistance ; z <= world.drawDistance; z++) {
                World_AddChunk((Vector3) {pos.x + x, pos.y + y, pos.z + z});
            }
        }
    }
    
    Chunk *chunk;
    for(int i = 0; i < 3; i++) {
        if(world.buildChunksQueue != NULL) {
            chunk = world.buildChunksQueue->chunk;
            if(chunk->isLightGenerated == true) {
                for(int j = 0; j < 6; j++) {
                    if(chunk->neighbours[j] != NULL) {
                        if(!chunk->neighbours[j]->isLightGenerated) {
                           break;
                        }
                    }
                    if(j == 5) {
                        Chunk_BuildMesh(chunk);
                        chunk->isBuilding = false;
                        world.buildChunksQueue = Chunk_PopFromQueue(world.buildChunksQueue);
                        break;
                    }
                }
            }
        }
    }
    
    //destroy far chunks
    chunk = world.chunks;
    float unloadDist = world.drawDistance * CHUNK_SIZE_X * 2.5f;
    while(chunk != NULL) {
        Chunk *nextChunk = chunk->nextChunk;

        if(Vector3Distance(chunk->blockPosition, player.position) > unloadDist) {
            if(chunk->isBuilt == true && chunk->isBuilding == false) {
                for(int j = 0; j < 6; j++) {
                    if(chunk->neighbours[j] != NULL) {
                        if(chunk->neighbours[j]->isBuilding == true) {
                            break;
                        }
                    }
                    if(j == 25) {
                        World_RemoveChunk(chunk->position);
                        break;
                    }
                }
                
            }
        }

        chunk = nextChunk;
    }
    
}

void World_Unload(void) {
    world.loadChunks = false;
    Chunk *curChunk = world.chunks;
    while(curChunk != NULL) {
        World_RemoveChunk(curChunk->position);
        curChunk = world.chunks;
    }
}

void World_ApplyTexture(Texture2D texture) {
    SetMaterialTexture(&world.mat, MATERIAL_MAP_DIFFUSE, texture);
}

void World_ApplyShader(Shader shader) {
    world.mat.shader = shader;
}

void World_Draw(Vector3 camPosition) {

    ChunkMesh_PrepareDrawing(world.mat);

    int amountChunks = 0;
    float frustumAngle = DEG2RAD * player.camera.fovy + 0.3f;
    Vector3 dirVec = Player_GetForwardVector();

    Chunk *chunk = world.chunks;
    while(chunk != NULL) {
        amountChunks++;
        chunk = chunk->nextChunk;
    }
    
    Vector3 chunkLocalCenter = (Vector3){CHUNK_SIZE_X / 2, CHUNK_SIZE_Y / 2, CHUNK_SIZE_Z / 2};

    //Create the sorted chunk list
    struct { Chunk *chunk; float dist; } *sortedChunks = MemAlloc(amountChunks * (sizeof(Chunk*) + sizeof(float)));

    int sortedLength = 0;
    chunk = world.chunks;
    while(chunk != NULL) {
        Vector3 centerChunk = Vector3Add(chunk->blockPosition, chunkLocalCenter);
        float distFromCam = Vector3Distance(centerChunk, camPosition);

        //Don't draw chunks behind the player
        Vector3 toChunkVec = Vector3Normalize(Vector3Subtract(centerChunk, camPosition));
       
        if(distFromCam > CHUNK_SIZE_X && Vector3Distance(toChunkVec, dirVec) > frustumAngle) {
            chunk = chunk->nextChunk;
            continue;
        }

        sortedChunks[sortedLength].dist = distFromCam;
        sortedChunks[sortedLength].chunk = chunk;
        sortedLength++;
        chunk = chunk->nextChunk;
    }
    
    //Sort chunks back to front
    for(int i = 1; i < sortedLength; i++) {
        int j = i;
        while(j > 0 && sortedChunks[j-1].dist <= sortedChunks[j].dist) {

            static struct { Chunk *chunk; float dist; } tempC;
            tempC.chunk = sortedChunks[j].chunk;
            tempC.dist = sortedChunks[j].dist;

            sortedChunks[j] = sortedChunks[j - 1];
            sortedChunks[j - 1].chunk = tempC.chunk;
            sortedChunks[j - 1].dist = tempC.dist;
            j = j - 1;
        }
    }
    
    ChunkMesh_PrepareDrawing(world.mat);

    //Draw sorted chunks
    for(int i = 0; i < sortedLength; i++) {
        Chunk *chunk = sortedChunks[i].chunk;

        Matrix matrix = (Matrix) { 1, 0, 0, chunk->blockPosition.x,
                                   0, 1, 0, chunk->blockPosition.y,
                                   0, 0, 1, chunk->blockPosition.z,
                                   0, 0, 0, 1 };
        
        ChunkMesh_Draw(chunk->mesh, world.mat, matrix);
        rlDisableBackfaceCulling();
        ChunkMesh_Draw(chunk->meshTransparent, world.mat, matrix);
        rlEnableBackfaceCulling();
    }

    ChunkMesh_FinishDrawing();

    MemFree(sortedChunks);

    //Draw entities
    for(int i = 0; i < WORLD_MAX_ENTITIES; i++) {
        if(world.entities[i].type == 0) continue;
        Entity_Draw(&world.entities[i]);
    }

}

int World_GetBlock(Vector3 blockPos) {
    
    //Get Chunk
    Vector3 chunkPos = (Vector3) { floor(blockPos.x / CHUNK_SIZE_X), floor(blockPos.y / CHUNK_SIZE_Y), floor(blockPos.z / CHUNK_SIZE_Z) };
    Chunk* chunk = World_GetChunkAt(chunkPos);
    
    if(chunk == NULL) return 0;
    
    //Get Block
    Vector3 blockPosInChunk = (Vector3) { 
                                floor(blockPos.x) - chunk->blockPosition.x,
                                floor(blockPos.y) - chunk->blockPosition.y, 
                                floor(blockPos.z) - chunk->blockPosition.z 
                               };

    return Chunk_GetBlock(chunk, blockPosInChunk);
}

void World_SetBlock(Vector3 blockPos, int blockID, bool immediate) {
    
    //Get Chunk
    Vector3 chunkPos = (Vector3) { floor(blockPos.x / CHUNK_SIZE_X), floor(blockPos.y / CHUNK_SIZE_Y), floor(blockPos.z / CHUNK_SIZE_Z) };
    Chunk* chunk = World_GetChunkAt(chunkPos);
    
    if(chunk == NULL) return;
    if(chunk->isLightGenerated == false) return;

    //Set Block
    Vector3 blockPosInChunk = (Vector3) { 
                                floor(blockPos.x) - chunkPos.x * CHUNK_SIZE_X, 
                                floor(blockPos.y) - chunkPos.y * CHUNK_SIZE_Y, 
                                floor(blockPos.z) - chunkPos.z * CHUNK_SIZE_Z 
                               };
    
    Chunk_SetBlock(chunk, blockPosInChunk, blockID);

    if(blockID == 0) {
        //Refresh mesh of neighbour chunks.
        Chunk_RefreshBorderingChunks(chunk, false);

        if(immediate == true) {
            Chunk_BuildMesh(chunk);
        } else {
            //Refresh current chunk.
            World_QueueChunk(chunk);
        }
    } else {

        if(immediate == true) {
            Chunk_BuildMesh(chunk);
        } else {
            //Refresh current chunk.
            World_QueueChunk(chunk);
        }

        //Refresh mesh of neighbour chunks.
        Chunk_RefreshBorderingChunks(chunk, false);
    }

}

/*-------------------------------------------------------------------------------------------------------*
*-------------------------------------------World Entities-----------------------------------------------*
*--------------------------------------------------------------------------------------------------------*/

void World_TeleportEntity(int ID, Vector3 position, Vector3 rotation) {
    Entity *entity = &world.entities[ID];
    entity->position = position;
    entity->rotation = (Vector3) { 0, rotation.y, 0 };
    
    for(int i = 0; i < entity->model.amountParts; i++) {
        if(entity->model.parts[i].type == PartType_Head) {
            entity->model.parts[i].rotation.x = rotation.x;
        }
    }
}

void World_AddEntity(int ID, int type, Vector3 position, Vector3 rotation) {
    world.entities[ID].type = type;
    world.entities[ID].position = position;
    world.entities[ID].rotation = rotation;
    
    EntityModel_Build(&world.entities[ID].model, entityModels[0]);
}

void World_RemoveEntity(int ID) {
    Entity_Remove(&world.entities[ID]);
}
