/*
Copyright 2021-2022 Peter Repukat - FlatspotSoftware

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#endif

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "SteamTarget.h"

#include "OverlayLogSink.h"
#include "Settings.h"

#ifdef _WIN32

// default to high performance GPU
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;

LONG Win32FaultHandler(struct _EXCEPTION_POINTERS* ExInfo)

{
    std::string FaultTx = "";
    switch (ExInfo->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        FaultTx = "ACCESS VIOLATION";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        FaultTx = "DATATYPE MISALIGNMENT";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        FaultTx = "FLT DIVIDE BY ZERO";
        break;
    default:
        FaultTx = "(unknown)";
        break;
    }

    int wsFault = ExInfo->ExceptionRecord->ExceptionCode;
    PVOID CodeAdress = ExInfo->ExceptionRecord->ExceptionAddress;

    spdlog::error("*** A Program Fault occurred:");
    spdlog::error("*** Error code {:#x}: {}", wsFault, FaultTx);
    spdlog::error("*** Address: {:#x}", (int)CodeAdress);
    spdlog::error("*** Flags: {:#x}", ExInfo->ExceptionRecord->ExceptionFlags);

    MINIDUMP_EXCEPTION_INFORMATION M;
    HANDLE hDump_File;

    auto path = std::filesystem::temp_directory_path()
                    .parent_path()
                    .parent_path()
                    .parent_path();

    path /= "Roaming";
    path /= "GlosSI";
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);
    path /= "glossitarget.dmp";

    M.ThreadId = GetCurrentThreadId();
    M.ExceptionPointers = ExInfo;
    M.ClientPointers = 0;

    hDump_File = CreateFile(path.wstring().c_str(), GENERIC_WRITE, 0,
                            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                      hDump_File, MiniDumpNormal,
                      (ExInfo) ? &M : NULL, NULL, NULL);

    CloseHandle(hDump_File);

    /*if(want to continue)
    {
       ExInfo->ContextRecord->Eip++;
       return EXCEPTION_CONTINUE_EXECUTION;
    }
    */
    return EXCEPTION_EXECUTE_HANDLER;
}

#endif

#ifdef _WIN32
#ifdef CONSOLE
int main(int argc, char* argv[])
#else
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
#endif
#else
int main(int argc, char* argv[])
#endif
{
    const auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
#ifdef _WIN32
    auto path = std::filesystem::temp_directory_path()
                    .parent_path()
                    .parent_path()
                    .parent_path();

    path /= "Roaming";
    path /= "GlosSI";
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);
    path /= "glossitarget.log";
    const auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
#else
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("/tmp/glossitarget.log", true);
#endif
    file_sink->set_level(spdlog::level::trace);

    const auto overlay_sink = std::make_shared<spdlog::sinks::overlay_sink_mt>();
#ifdef _DEBUG
    overlay_sink->set_level(spdlog::level::debug);
#else
    overlay_sink->set_level(spdlog::level::info);
#endif

    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink, overlay_sink};
    auto logger = std::make_shared<spdlog::logger>("log", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(logger);
    SetUnhandledExceptionFilter(static_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(Win32FaultHandler));
    auto exit = 1;
    try {
#ifdef _WIN32
        std::string argsv = "";
        if (__argc > 1) {
            for (int i = 1; i < __argc; i++)
                argsv += i == 1 ? __argv[i] : std::string(" ") + __argv[i];
        }
        Settings::Parse(argsv);
        SteamTarget target(__argc, __argv);
#else
        std::string argsv = "";
        if (argc > 1) {
            for (int i = 1; i < argc; i++)
                argsv += i == 1 ? argv[i] : std::string(" ") + argv[i];
        }
        Settings::Parse(argsv);
        SteamTarget target(argc, argv);
#endif
        exit = target.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception occured: {}", e.what());
    }
    catch (...) {
        spdlog::error("Unknown exception occured");
    }
    spdlog::shutdown();
    return exit;
}