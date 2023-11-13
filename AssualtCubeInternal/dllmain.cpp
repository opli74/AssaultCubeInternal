// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#pragma comment(lib, "OpenGL32.lib")
#include <gl/GL.h>
#include <fstream>
#include <iostream>
#include "Hook.h"
#include <vector>

typedef BOOL(WINAPI* twglSwapBuffers)(HDC hDc);

twglSwapBuffers owglSwapBuffers;
twglSwapBuffers hwglSwapBuffers;

namespace gl
{
    void setupOrtho()
    {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glViewport(0, 0, viewport[2], viewport[3]);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, viewport[2], viewport[3], 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
    };
    void restoreGl()
    {
        glPopMatrix();
        glPopAttrib();
    }
    void drawRect(float x, float y, float w, float h, const GLubyte color[3])
    {
        glColor3ub(color[0], color[1], color[2]);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);

        glEnd();
    }
    void drawOutline(float x, float y, float w, float h, float lineWidth, const GLubyte color[3])
    {
        glLineWidth(lineWidth);
        glBegin(GL_LINE_STRIP);
        glColor3ub(color[0], color[1], color[2]);
        glVertex2f(x - 1.0f, y - 1.0f);
        glVertex2f(x + w + 1.0f, y - 1.0f);
        glVertex2f(x + w + 1.0f, y + h + 1.0f);
        glVertex2f(x - 1.0f, y + h + 1.0f);
        glVertex2f(x - 1.0f, y - 1.0f);
        glEnd();

    }
}

namespace rgb
{
    GLubyte purple[4] = { 125, 0, 255 };
};

void draw()
{
    HDC currentHDC = wglGetCurrentDC();

    gl::setupOrtho();
    gl::drawRect(150, 150, 50, 50, rgb::purple);
    gl::restoreGl();
}


struct Vector3
{
    float x, y, z;

    // Default constructor
    Vector3()
    {
        x = y = z = 0;
    }

    // Constructor with 3 arguments passed in X Y Z format
    Vector3(float xCoord, float yCoord, float zCoord)
    {
        x = xCoord;
        y = yCoord;
        z = zCoord;
    }
};

struct Weapon
{
    char pad_0000[0x14];
    int32_t* ammo;
};

struct playerEnt
{
    uintptr_t vTable; //0x0000
    Vector3 HeadPos; //0x0004
    char pad_0010[36]; //0x0010
    Vector3 Pos; //0x0034
    Vector3 ViewAngles; //0x0040
    char pad_004C[34]; //0x004C
    bool bCrouching; //0x006E
    char pad_006F[137]; //0x006F
    int32_t health; //0x00F8
    int32_t armour; //0x00FC
    char pad_0100[292]; //0x0100
    bool bAttacking; //0x0224
    char Name[16]; //0x0225
    char pad_0235[247]; //0x0235
    uint8_t Team; //0x032C
    char pad_032D[11]; //0x032D
    uint8_t State; //0x0338
    char pad_0339[59]; //0x0339
    Weapon* weaponInfo; //0x0374
    char pad_0378[282]; //0x0378
};


uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"ac_client.exe");

uintptr_t* localPlayerAddress = (uintptr_t*)(moduleBase + 0x10F4F4);
playerEnt* localPlayer = (playerEnt*)*localPlayerAddress;

struct ent
{
    playerEnt* entity[31];
};
uintptr_t* entitiesAddress = (uintptr_t*)(moduleBase + 0x10F4F8);
ent* entityList = (ent*)*entitiesAddress;



int* entityAmount = (int*)(moduleBase + 0x10F500);

bool running = true, bAmmo = false, bInvincible = false, bRecoil = false, bFast = false, bAimbot = false, bESP = false, bEjected = false;


//Hack moved into the hooked function
BOOL WINAPI hkwglSwapBuffers(HDC hDc)
{

    if (GetAsyncKeyState(VK_END) & 1)
    {
        bEjected = true;                                                       //Raise Eject Flag
        return owglSwapBuffers(hDc);                                     //Exit hook early
    }
    if (GetAsyncKeyState(VK_NUMPAD1) & 1)
    {
        bInvincible = !bInvincible;
    }
    if (GetAsyncKeyState(VK_NUMPAD2) & 1)
    {
        bAmmo = !bAmmo;
    }

    if (bInvincible)
    {
        localPlayer->health = 1337;
        localPlayer->armour = 1337;
    }
    else
    {
        localPlayer->health = 100;
        localPlayer->armour = 100;
    }

    if (bAmmo)
    {
        *localPlayer->weaponInfo->ammo = 1337;
    }

    for (int i = 0; i < *entityAmount; i++)
    {
        playerEnt* cEnt = entityList->entity[i];

        for (const char* c = cEnt->Name; *c != '\0'; c++)
        {
            std::cout << *c;
        }

        std::cout << "\n";
    }

    draw();

    return owglSwapBuffers(hDc);
}

uintptr_t WINAPI thread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;
    if (freopen_s(&f, "CONOUT$", "w", stdout) != 0 || f == nullptr) {
        std::cerr << "Failed to redirect stdout." << std::endl;
        // Handle the error, maybe return from your function or perform cleanup.
        return 0;
    }




    Hook SwapBuffersHook("wglSwapBuffers", "opengl32.dll", (BYTE*)hkwglSwapBuffers, (BYTE*)&owglSwapBuffers);

    SwapBuffersHook.Enable();
    MessageBox(NULL, L"  Injected!", L"", MB_OK);

    while (!bEjected) {
        //empty while loop to hold execution here until the player chooses to exit DLL
        continue;
    }

    //Disable Hook
    SwapBuffersHook.Disable();

    Sleep(10);
    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        auto  threadHandle = CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread, hModule, 0, nullptr));

    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}



