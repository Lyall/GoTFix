#include "stdafx.h"
#include "helper.hpp"
#include <inipp/inipp.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <safetyhook.hpp>

HMODULE baseModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Logger and config setup
inipp::Ini<char> ini;
std::shared_ptr<spdlog::logger> logger;
string sFixName = "GoTFix";
string sFixVer = "0.9.0";
string sLogFile = "GoTFix.log";
string sConfigFile = "GoTFix.ini";
string sExeName;
filesystem::path sExePath;
filesystem::path sThisModulePath;
std::pair DesktopDimensions = { 0,0 };

// Ini Variables
bool bDisableLetterboxing;
bool bCompensateFOV;
bool bRemoveAspectLimit;
bool bSkipIntro;

// Aspect ratio + HUD stuff
float fPi = (float)3.141592653;
float fNativeAspect = (float)16 / 9;
float fAspectRatio;
float fAspectMultiplier;
float fHUDWidth;
float fHUDHeight;
float fDefaultHUDWidth = (float)1920;
float fDefaultHUDHeight = (float)1080;
float fHUDWidthOffset;
float fHUDHeightOffset;

// Variables
float fAspectRatioLimit = FLT_MAX;
float fLetterboxAspectRatio;
int iResX = 1920;
int iResY = 1080;

void Logging()
{
    // Get this module path
    WCHAR thisModulePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(thisModule, thisModulePath, MAX_PATH);
    sThisModulePath = thisModulePath;
    sThisModulePath = sThisModulePath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = { 0 };
    GetModuleFileNameW(baseModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // spdlog initialisation
    {
        try
        {
            logger = spdlog::basic_logger_st(sFixName.c_str(), sThisModulePath.string() + sLogFile, true);
            spdlog::set_default_logger(logger);

            spdlog::flush_on(spdlog::level::debug);
            spdlog::info("----------");
            spdlog::info("{} v{} loaded.", sFixName.c_str(), sFixVer.c_str());
            spdlog::info("----------");
            spdlog::info("Path to logfile: {}", sThisModulePath.string() + sLogFile);
            spdlog::info("----------");

            // Log module details
            spdlog::info("Module Name: {0:s}", sExeName.c_str());
            spdlog::info("Module Path: {0:s}", sExePath.string());
            spdlog::info("Module Address: 0x{0:x}", (uintptr_t)baseModule);
            spdlog::info("Module Timestamp: {0:d}", Memory::ModuleTimestamp(baseModule));
            spdlog::info("----------");
        }
        catch (const spdlog::spdlog_ex& ex)
        {
            AllocConsole();
            FILE* dummy;
            freopen_s(&dummy, "CONOUT$", "w", stdout);
            std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        }
    }
}

void ReadConfig()
{
    // Initialise config
    std::ifstream iniFile(sThisModulePath.string() + sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVer.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sThisModulePath.string().c_str() << std::endl;
    }
    else
    {
        spdlog::info("Path to config file: {}", sThisModulePath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Read ini file
    inipp::get_value(ini.sections["Remove Aspect Ratio Limit"], "Enabled", bRemoveAspectLimit);
    inipp::get_value(ini.sections["Disable Letterboxing"], "Enabled", bDisableLetterboxing);
    inipp::get_value(ini.sections["Disable Letterboxing"], "CompensateFOV", bCompensateFOV);

    // Log config parse
    spdlog::info("Config Parse: bRemoveAspectLimit: {}", bRemoveAspectLimit);
    spdlog::info("Config Parse: bDisableLetterboxing: {}", bDisableLetterboxing);
    spdlog::info("Config Parse: bCompensateFOV: {}", bCompensateFOV);
    spdlog::info("----------");

    // Grab desktop res
    DesktopDimensions = Util::GetPhysicalDesktopDimensions();
}

void AspectFOVFix()
{
    // Letterboxing in cutscenes
    uint8_t* LetterboxingScanResult = Memory::PatternScan(baseModule, "83 ?? ?? ?? ?? ?? 00 7C ?? 48 ?? ?? 48 ?? ?? FF 90 ?? ?? ?? ?? 84 ??") + 0x9;
    if (LetterboxingScanResult)
    {
        spdlog::info("Letterboxing: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)LetterboxingScanResult - (uintptr_t)baseModule);
        if (bDisableLetterboxing)
        {
            static SafetyHookMid  LetterboxingMidHook{};
            LetterboxingMidHook = safetyhook::create_mid(LetterboxingScanResult,
                [](SafetyHookContext& ctx)
                {
                    if (ctx.rdi + 0x75A)
                    {
                        if (*reinterpret_cast<BYTE*>(ctx.rdi + 0x75A) == 1 || *reinterpret_cast<BYTE*>(ctx.rdi + 0x75B) == 1)
                        {
                            *reinterpret_cast<BYTE*>(ctx.rdi + 0x75A) = 0; // Controller
                            *reinterpret_cast<BYTE*>(ctx.rdi + 0x75B) = 0; // Keyboard
                            spdlog::info("Letterboxing: Disabled letterboxing.");
                        }
                    }
                });
        }
    }
    else if (!LetterboxingScanResult)
    {
        spdlog::error("Letterboxing: Pattern scan failed.");
    }

    // Get current resolution and aspect ratio
    uint8_t* CurrentResolutionScanResult = Memory::PatternScan(baseModule, "89 ?? ?? ?? ?? ?? C1 ?? 03 89 ?? ?? ?? ?? ?? 41 ?? ?? C1 ?? 03");
    if (CurrentResolutionScanResult)
    {
        spdlog::info("Current Resolution: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CurrentResolutionScanResult - (uintptr_t)baseModule);

        static SafetyHookMid CurrentResolutionMidHook{};
        CurrentResolutionMidHook = safetyhook::create_mid(CurrentResolutionScanResult,
            [](SafetyHookContext& ctx)
            {
                iResX = (int)ctx.rdi;
                iResY = (int)ctx.r9;

                fAspectRatio = (float)iResX / iResY;
                fAspectMultiplier = fAspectRatio / fNativeAspect;
                fLetterboxAspectRatio = (float)iResX / ((409.00f * 0.001851851819f) * iResY); // Original cutscene aspect = 2.35:1 cinemascope a.k.a 1920x818 or 960x409 

                // Log aspect ratio stuff
                spdlog::info("----------");
                spdlog::info("Current Resolution: Resolution: {}x{}", iResX, iResY);
                spdlog::info("Current Resolution: fAspectRatio: {}", fAspectRatio);
                spdlog::info("Current Resolution: fAspectMultiplier: {}", fAspectMultiplier);
                spdlog::info("Current Resolution: fLetterboxAspectRatio: {}", fLetterboxAspectRatio);
            });
    }
    else if (!CurrentResolutionScanResult)
    {
        spdlog::error("Current Resolution: Pattern scan failed.");
    }

    // Compensate cutscene FOV
    uint8_t* CutsceneFOVScanResult = Memory::PatternScan(baseModule, "0F 28 ?? E8 ?? ?? ?? ?? F6 ?? ?? ?? ?? ?? 01");
    if (CutsceneFOVScanResult)
    {
        spdlog::info("Cutscene FOV: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)CutsceneFOVScanResult - (uintptr_t)baseModule);

        static SafetyHookMid CutsceneFOVMidHook{};
        CutsceneFOVMidHook = safetyhook::create_mid(CutsceneFOVScanResult,
            [](SafetyHookContext& ctx)
            {
                if (bCompensateFOV)
                {
                    float fovDegs = ctx.xmm6.f32[0] * (180.00f / fPi);
                    float newFovRads = (atanf(tanf(fovDegs * (fPi / 360)) / fLetterboxAspectRatio * fAspectRatio) * (360 / fPi)) * (fPi / 180.00f);
                    ctx.xmm6.f32[0] = (float)newFovRads;
                }
            });
    }
    else if (!CutsceneFOVScanResult)
    {
        spdlog::error("Cutscene FOV: Pattern scan failed.");
    }

    // Remove aspect ratio limit
    uint8_t* AspectRatioLimitScanResult = Memory::PatternScan(baseModule, "89 ?? ?? 84 ?? 74 ?? 84 ?? 74 ?? 48 ?? ?? ?? 48 ?? ?? ?? E8 ?? ?? ?? ??");
    if (AspectRatioLimitScanResult)
    {
        spdlog::info("Aspect Ratio Limit: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)AspectRatioLimitScanResult - (uintptr_t)baseModule);

        if (bRemoveAspectLimit)
        {
            static SafetyHookMid AspectRatioLimitMidHook{};
            AspectRatioLimitMidHook = safetyhook::create_mid(AspectRatioLimitScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.rax = *(uint32_t*)(&fAspectRatioLimit);
                });
        }
    }
    else if (!AspectRatioLimitScanResult)
    {
        spdlog::error("Aspect Ratio Limit: Pattern scan failed.");
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    AspectFOVFix();
    return true;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        thisModule = hModule;
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST); // set our Main thread priority higher than the games thread
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

