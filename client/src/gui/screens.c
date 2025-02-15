/**
 * Copyright (c) 2021-2022 Sirvoid
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include <pthread.h>
#include <math.h>
#include "raylib.h"
#include "raygui.h"
#include "screens.h"
#include "chat.h"
#include "player.h"
#include "world.h"
#include "block.h"
#include "networkhandler.h"
#include "packet.h"
#include "client.h"
#include "clientws.h"
#include "worldgenerator.h"

Screen Screen_Current = SCREEN_LOGIN;
bool Screen_cursorEnabled = false;
bool Screen_showDebug = false;
int screenHeight;
int screenWidth;
bool *exitGame;
Color uiColBg;
int maxFPSChoice = 0;
const char* maxFPS = "60";

Texture2D mapTerrain;

void Screens_init(Texture2D terrain, bool *exit) {
    mapTerrain = terrain;
    exitGame = exit;

    //Set UI colors
    GuiSetStyle(BUTTON, BORDER_COLOR_NORMAL,    0xfffcfcff); 
    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,      0x00000000); 
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,      0xffffffff); 
    GuiSetStyle(BUTTON, BORDER_COLOR_FOCUSED,   0x010101ff); 
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,     0xfafafa00); 
    GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,     0x000000ff); 
    GuiSetStyle(BUTTON, BORDER_COLOR_PRESSED,   0xfcffffff); 
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,     0x00000000); 
    GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,     0xffffffff); 

    GuiSetStyle(SLIDER, BORDER_COLOR_NORMAL,    0xfffcfcff); 
    GuiSetStyle(SLIDER, BASE_COLOR_NORMAL,      0x00000000); 
    GuiSetStyle(SLIDER, TEXT_COLOR_NORMAL,      0xffffffff); 
    GuiSetStyle(SLIDER, BORDER_COLOR_FOCUSED,   0xf1f1f1ff); 
    GuiSetStyle(SLIDER, TEXT_COLOR_FOCUSED,     0xf1f1f1ff); 
    GuiSetStyle(SLIDER, BASE_COLOR_PRESSED,     0xfcffffff); 
    GuiSetStyle(SLIDER, BORDER_COLOR_PRESSED,   0xfcffffff); 
    GuiSetStyle(SLIDER, TEXT_COLOR_PRESSED,     0xffffffff); 
    GuiSetStyle(SLIDER, BORDER_WIDTH,           2); 
    
    GuiSetStyle(PROGRESSBAR, BORDER_COLOR_NORMAL,   0xfffdfdff); 
    GuiSetStyle(PROGRESSBAR, BORDER_COLOR_PRESSED,  0xfbf8f8ff); 
    GuiSetStyle(PROGRESSBAR, BASE_COLOR_PRESSED,    0xf8fbfbff); 
    
    GuiSetStyle(TEXTBOX, BORDER_COLOR_NORMAL,   0xf9f9f9ff); 
    GuiSetStyle(TEXTBOX, BASE_COLOR_NORMAL,     0xfbfbfbff); 
    GuiSetStyle(TEXTBOX, TEXT_COLOR_NORMAL,     0xfdf9f9ff); 
    GuiSetStyle(TEXTBOX, BASE_COLOR_FOCUSED,    0xc7effeff); 
    GuiSetStyle(TEXTBOX, BORDER_COLOR_PRESSED,  0x0392c7ff); 
    GuiSetStyle(TEXTBOX, TEXT_COLOR_PRESSED,    0x338bafff); 
}

void Screen_MakeGame(void) {

    //Draw debug infos
    if (Screen_showDebug) {
        const char* coordText = TextFormat("X: %i Y: %i Z: %i", (int)player.position.x, (int)player.position.y, (int)player.position.z);
        const char* debugText;

        if (Network_connectedToServer) {
            debugText = TextFormat("%2i FPS %2i PING", GetFPS(), Network_ping);
        } else {
            debugText = TextFormat("%2i FPS", GetFPS());
        }
    
        const char* versionText = "Midless Pre-Alpha 1.3 dev";
        DrawText(versionText, 9, 9, 20, BLACK);
        DrawText(versionText, 8, 8, 20, WHITE);

        DrawText(debugText, 9, 29, 20, BLACK);
        DrawText(coordText, 9, 49, 20, BLACK);
        DrawText(debugText, 8, 28, 20, WHITE);
        DrawText(coordText, 8, 48, 20, WHITE);
    }

    //Draw crosshair
    DrawRectangle(screenWidth / 2 - 8, screenHeight / 2 - 2, 16, 4, uiColBg);
    DrawRectangle(screenWidth / 2 - 2, screenHeight / 2 + 2,  4, 6, uiColBg);
    DrawRectangle(screenWidth / 2 - 2, screenHeight / 2 - 8,  4, 6, uiColBg);

    //Draw Block Selected
    Block blockDef = Block_GetDefinition(player.blockSelected);
    int texI = blockDef.textures[4];
    int texX = texI % 16 * 16;
    int texY = texI / 16 * 16;

    Rectangle texRec = (Rectangle) {
        texX + 16 - blockDef.maxBB.x, 
        texY + 16 - blockDef.maxBB.y, 
        (blockDef.maxBB.x - blockDef.minBB.x), 
        (blockDef.maxBB.y - blockDef.minBB.y)
    };

    Rectangle destRec = (Rectangle) { 
        screenWidth - 80 + (blockDef.minBB.x * 4), 
        16 + ((16 - blockDef.maxBB.y) * 4), 
        (blockDef.maxBB.x - blockDef.minBB.x) * 4, 
        (blockDef.maxBB.y - blockDef.minBB.y) * 4
    };

    DrawTexturePro(mapTerrain, texRec, destRec, (Vector2) {0, 0}, 0, WHITE);

    //Draw Chat
    Chat_Draw((Vector2){16, screenHeight - 52}, uiColBg);
}

void Screen_MakePause(void) {
    DrawRectangle(0, 0, screenWidth, screenHeight, uiColBg);

    int offsetY = screenHeight / 2 - 75;
    int offsetX = screenWidth / 2 - 100;

    int index = 0;

    //Continue Button
    if (GuiButton((Rectangle) {offsetX , offsetY + (index++ * 35), 200, 30 }, "Continue")) {
        Screen_Switch(SCREEN_GAME);
        DisableCursor();
        Screen_cursorEnabled = false;
    }

    //Options Button
    if (GuiButton((Rectangle) {offsetX, offsetY + (index++ * 35), 200, 30 }, "Options")) {
        Screen_Switch(SCREEN_OPTIONS);
    }

    //Main Menu Button
    if (GuiButton((Rectangle) {offsetX, offsetY + (index++ * 35), 200, 30 }, "Main Menu")) {
        if (Network_connectedToServer) {
            Network_Disconnect();
        } else {
            Screen_Switch(SCREEN_LOGIN);
             Screen_cursorEnabled = false;
            World_Unload();
        }
    }

    //Quit Button
    if (GuiButton((Rectangle) {offsetX, offsetY + (index++ * 35), 200, 30 }, "Quit")) {
        *exitGame = true;
    }
}

void Screen_MakeOptions(void) {
    DrawRectangle(0, 0, screenWidth, screenHeight, uiColBg);

    int offsetY = screenHeight / 2 - 75;
    int offsetX = screenWidth / 2 - 100;
    
    const char* fovTxt = TextFormat("Field of View: %i", (int)player.camera.fovy);

    //Field of view Button
    float newFovy = GuiSlider((Rectangle) {offsetX, offsetY, 200, 30 }, "", "", player.camera.fovy, 30.0f, 110.0f);
    Vector2 fovTxtSize = MeasureTextEx(GetFontDefault(), fovTxt, 10.0f, 1);
    DrawTextEx(GetFontDefault(), fovTxt, (Vector2){offsetX + 100 - fovTxtSize.x / 2 + 1, offsetY + 15 - fovTxtSize.y / 2 + 1}, 10.0f, 1, BLACK);
    DrawTextEx(GetFontDefault(), fovTxt, (Vector2){offsetX + 100 - fovTxtSize.x / 2, offsetY + 15 - fovTxtSize.y / 2}, 10.0f, 1, WHITE);

    if (newFovy != player.camera.fovy) {
        if (newFovy > player.camera.fovy) {
            player.camera.fovy = newFovy;
        } else {
            player.camera.fovy = newFovy;
        }
    }

    offsetY += 35;

    const char* drawDistanceTxt = TextFormat("Draw Distance: %i", world.drawDistance);

    //Draw distance Button
    int newDrawDistance = GuiSlider((Rectangle) {offsetX, offsetY, 200, 30 }, "", "", world.drawDistance, 2, 16);
    Vector2 drawDistanceTxtSize = MeasureTextEx(GetFontDefault(), drawDistanceTxt, 10.0f, 1);
    DrawTextEx(GetFontDefault(), drawDistanceTxt, (Vector2){offsetX + 100 - drawDistanceTxtSize.x / 2 + 1, offsetY + 15 - drawDistanceTxtSize.y / 2 + 1}, 10.0f, 1, BLACK);
    DrawTextEx(GetFontDefault(), drawDistanceTxt, (Vector2){offsetX + 100 - drawDistanceTxtSize.x / 2, offsetY + 15 - drawDistanceTxtSize.y / 2}, 10.0f, 1, WHITE);

    if (newDrawDistance != world.drawDistance) {
        if (newDrawDistance > world.drawDistance) {
            world.drawDistance = newDrawDistance;
            World_LoadChunks();
        } else {
            world.drawDistance = newDrawDistance;
            World_Reload();
        }

        if (Network_connectedToServer) {
            Network_Send(Packet_SetDrawDistance(world.drawDistance));
        }
    }

    offsetY += 35;

    //Draw Debug Button
    const char* debugStateTxt = "OFF";
    if (Screen_showDebug) debugStateTxt = "ON";
    const char* showDebugTxt = TextFormat("Show Debug: %s", debugStateTxt);
    if (GuiButton((Rectangle) {offsetX, offsetY, 200, 30 }, showDebugTxt)) {
        Screen_showDebug = !Screen_showDebug;
    }

    offsetY += 35;

    //Draw Max FPS
    const char* maxFPSTxt = TextFormat("Max FPS: %s", maxFPS);
    if (GuiButton((Rectangle) {offsetX, offsetY, 200, 30 }, maxFPSTxt)) {
        maxFPSChoice++;
        if (maxFPSChoice == 3) maxFPSChoice = 0;
        if (maxFPSChoice == 0) {
            maxFPS = "60";
            SetTargetFPS(60);
        } else if (maxFPSChoice == 1) {
            maxFPS = "120";
            SetTargetFPS(120);
        } else if (maxFPSChoice == 2) {
            maxFPS = "Unlimited";
            SetTargetFPS(0);
        }
    }

    offsetY += 35;

    //Back Button
    if (GuiButton((Rectangle) {offsetX, offsetY, 200, 30 }, "Back")) {
        Screen_Switch(SCREEN_PAUSE);
    }

}

void Screen_MakeJoining(void) {
    DrawRectangle(0, 0, screenWidth, screenHeight, BLACK);
    DrawText("Joining Server...", screenWidth / 2 - 80, screenHeight / 2 - 30, 20, WHITE);
}

char name_input[16] = "Player";
char ip_input[128] = "localhost";
char port_input[5] = "25565";

bool login_editMode = false;
bool ip_editMode = false;
bool port_editMode = false;

void Screen_MakeLogin(void) {
    if(IsCursorHidden()) EnableCursor();
    DrawRectangle(0, 0, screenWidth, screenHeight, BLACK);

    const char *title = "MIDLESS";
    int offsetY = screenHeight / 2;
    int offsetX = screenWidth / 2;

    DrawText(title, offsetX - (MeasureText(title, 80) / 2), offsetY - 100, 80, WHITE);

    //Name Input
    if (GuiTextBox((Rectangle) { offsetX - 80, offsetY - 15, 160, 30 }, name_input, 16, login_editMode)) {
        login_editMode = !login_editMode;
    }

    //IP Input
    if (GuiTextBox((Rectangle) { offsetX - 80, offsetY + 20, 116, 30 }, ip_input, 128, ip_editMode)) {
        ip_editMode = !ip_editMode;
    }

    //Port Input
    if (GuiTextBox((Rectangle) { offsetX + 40, offsetY + 20, 40, 30 }, port_input, 5, port_editMode)) {
        port_editMode = !port_editMode;
    }

    //Login button
    if (GuiButton((Rectangle) { offsetX - 80, offsetY + 55, 160, 30 }, "Login")) {
        DisableCursor();
        Screen_Switch(SCREEN_JOINING);
        Network_threadState = 0;
        Network_name = name_input;
        Network_ip = ip_input;
        Network_port = TextToInteger(port_input);

        char fullAddress[128] = "";
        strcat(fullAddress, ip_input);
        strcat(fullAddress, ":");
        strcat(fullAddress, port_input);

        Network_fullAddress = fullAddress;

        //Start Client on a new thread
        pthread_t clientThread_id;

        #if !defined(PLATFORM_WEB)
            pthread_create(&clientThread_id, NULL, Client_Init, (void*)&Network_threadState);
        #else
            pthread_create(&clientThread_id, NULL, ClientWS_Init, (void*)&Network_threadState);
        #endif
    }
    
    //Singleplayer Button
    if (GuiButton((Rectangle) { offsetX - 80, offsetY + 90, 160, 30 }, "Singleplayer")) {
        DisableCursor();
        Screen_Switch(SCREEN_LOADING);
    }

}

bool loadingNextFrame = false;
void Screen_MakeLoading(void) {
    DrawRectangle(0, 0, screenWidth, screenHeight, BLACK);
    DrawText("Loading World", screenWidth / 2 - 80, screenHeight / 2, 20, WHITE);
    if(loadingNextFrame) {
        World_LoadSingleplayer();
        loadingNextFrame = false;
    }
    loadingNextFrame = true;
}

void Screen_Make(void) {
    screenHeight = GetScreenHeight();
    screenWidth = GetScreenWidth();
    
    uiColBg = (Color){ 0, 0, 0, 80 };
    
    if (Screen_Current == SCREEN_GAME)
        Screen_MakeGame();
    else if (Screen_Current == SCREEN_PAUSE)
        Screen_MakePause();
    else if(Screen_Current == SCREEN_LOADING)
        Screen_MakeLoading();
    else if (Screen_Current == SCREEN_JOINING)
        Screen_MakeJoining();
    else if (Screen_Current == SCREEN_LOGIN)
        Screen_MakeLogin();
    else if (Screen_Current == SCREEN_OPTIONS)
        Screen_MakeOptions();
}

void Screen_Switch(Screen screen) {
    Screen_Current = screen;
}