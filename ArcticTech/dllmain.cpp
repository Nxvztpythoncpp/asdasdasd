#include <Windows.h>
#include <locale.h>
#include <DbgHelp.h>
#include <codecvt>
#include <format>
#include <locale>
#include <winternl.h>
#pragma comment(lib, "DbgHelp.lib")

#include "SDK/Interfaces.h"
#include "SDK/Hooks.h"
#include "Features/Visuals/SkinChanger.h"
#include "Features/Lua/Bridge/Bridge.h"

#include "panorama engine/panorama_engine.h"

void wait_for_modules()
{
    while (!GetModuleHandleA("serverbrowser.dll"))
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    while (!GetModuleHandleA("tier0.dll"))
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void Initialize(HMODULE hModule) {
    static bool initialized = false;

    wait_for_modules();

    auto current_process = GetCurrentProcess();
    auto priority_class = GetPriorityClass(current_process);

    if (priority_class != HIGH_PRIORITY_CLASS && priority_class != REALTIME_PRIORITY_CLASS)
        SetPriorityClass(current_process, HIGH_PRIORITY_CLASS);

    HWND csgo_window = FindWindowA(NULL, "Counter-Strike: Global Offensive - Direct3D 9");
    SetWindowTextW(csgo_window, L"Sunset");

    setlocale(LC_ALL, "ru_RI.UTF-8");

    Interfaces::Initialize();
    Hooks::Initialize();
    Lua->Setup();
    SkinChanger->FixViewModelSequence();

    initialized = true;

    if (initialized) {
        ctx.cheat_initialized = true;
        
       // std::this_thread::sleep_for(std::chrono::milliseconds(150));

        /*
            PEngine->Eval("GameInterfaceAPI.ConsoleCommand('echo message using panorama');"); // uwukson: works
        */
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Initialize, hModule, 0, 0);
    return TRUE;
}