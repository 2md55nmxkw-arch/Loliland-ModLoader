#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <tlhelp32.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

static std::wstring kDefaultLauncher = L"C:\\Users\\devxr\\Desktop\\LoliLand.exe";
static std::wstring kDefaultTargetDir =
    L"C:\\Users\\devxr\\AppData\\Roaming\\.loliland\\game-resources\\clients\\hitech\\main\\mods";

static std::wstring exeDir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p(buf, n);
    auto pos = p.find_last_of(L"\\/");
    return pos == std::wstring::npos ? L"." : p.substr(0, pos);
}

static std::wstring envOrDefault(const wchar_t* name, const std::wstring& def) {
    wchar_t buf[1024];
    DWORD n = GetEnvironmentVariableW(name, buf, 1024);
    if (n == 0 || n >= 1024) return def;
    return std::wstring(buf, n);
}

static bool fileExists(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::vector<fs::path> listJars(const fs::path& dir) {
    std::vector<fs::path> out;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return out;
    for (auto& ent : fs::directory_iterator(dir, ec)) {
        if (ent.is_regular_file(ec)) {
            auto p = ent.path();
            if (p.extension() == ".jar" || p.extension() == ".JAR") out.push_back(p);
        }
    }
    return out;
}

static bool isGameRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"javaw.exe") == 0 ||
                _wcsicmp(pe.szExeFile, L"java.exe") == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

static bool isGameJvmAlive() { return isGameRunning(); }

static bool copyForce(const fs::path& src, const fs::path& dst) {
    std::error_code ec;
    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    if (ec) return false;
    SetFileAttributesW(dst.c_str(), FILE_ATTRIBUTE_HIDDEN);
    return true;
}

static std::atomic<bool> g_stop{false};

static void hammerThread(std::vector<fs::path> sources, fs::path target) {
    std::wcout << L"[loader] hammer thread started, " << sources.size()
               << L" mod(s)\n";
    while (!g_stop.load()) {
        for (auto& src : sources) {
            fs::path dst = target / src.filename();
            std::error_code ec;
            if (!fs::exists(dst, ec)) {
                copyForce(src, dst);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    for (auto& src : sources) {
        fs::path dst = target / src.filename();
        std::error_code ec;
        fs::remove(dst, ec);
    }
    std::wcout << L"[loader] hammer thread stopped, mods cleaned up\n";
}

static void waitForGameStart() {
    std::wcout << L"[loader] waiting for game JVM (javaw.exe) ...\n";
    while (!isGameJvmAlive()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::wcout << L"[loader] game JVM detected\n";
}

static void waitForGameEnd() {
    while (isGameJvmAlive()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::wcout << L"[loader] game JVM exited\n";
}

int wmain(int argc, wchar_t** argv) {
    std::wcout << L"====================================================\n";
    std::wcout << L"   LoliLoader - generic LoliLand mod injector\n";
    std::wcout << L"====================================================\n\n";

    fs::path here = exeDir();
    fs::path modsDir = here / L"mods";
    fs::path agentJar = here / L"ResolverAgent.jar";

    if (argc >= 2) modsDir = argv[1];

    std::wstring launcher = envOrDefault(L"LOLILOADER_LAUNCHER", kDefaultLauncher);
    std::wstring targetDirW = envOrDefault(L"LOLILOADER_TARGET", kDefaultTargetDir);
    fs::path targetDir = targetDirW;

    if (!fileExists(agentJar)) {
        std::wcerr << L"[loader] ERROR: ResolverAgent.jar not found next to "
                      L"LoliLoader.exe (expected at " << agentJar << L")\n";
        std::wcerr << L"          Build it via `gradlew build` and copy from "
                      L"build/libs/ResolverAgent-1.0.jar\n";
        std::wcerr << L"\nPress Enter to exit...";
        std::wcin.get();
        return 1;
    }
    if (!fileExists(launcher)) {
        std::wcerr << L"[loader] ERROR: launcher not found at " << launcher << L"\n";
        std::wcerr << L"          Set LOLILOADER_LAUNCHER env var to override.\n";
        std::wcerr << L"\nPress Enter to exit...";
        std::wcin.get();
        return 1;
    }

    std::error_code ec;
    fs::create_directories(modsDir, ec);
    auto jars = listJars(modsDir);
    std::wcout << L"[loader] mods folder: " << modsDir.wstring() << L"\n";
    std::wcout << L"[loader] target folder: " << targetDir.wstring() << L"\n";
    std::wcout << L"[loader] discovered " << jars.size() << L" mod jar(s):\n";
    for (auto& j : jars) std::wcout << L"   - " << j.filename().wstring() << L"\n";
    if (jars.empty()) {
        std::wcout << L"[loader] WARN: no .jar files in mods folder. Continuing "
                      L"anyway (agent will still run).\n";
    }

    std::wstring agentPath = agentJar.wstring();
    if (agentPath.find(L' ') != std::wstring::npos) {
        wchar_t up[MAX_PATH];
        DWORD n = GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            fs::path safe = fs::path(up) / L"ResolverAgent.jar";
            std::error_code copyEc;
            fs::copy_file(agentJar, safe,
                          fs::copy_options::overwrite_existing, copyEc);
            if (!copyEc) {
                agentPath = safe.wstring();
                std::wcout << L"[loader] copied agent to space-free path: "
                           << agentPath << L"\n";
            }
        }
    }

    std::wstring jto = L"-javaagent:" + agentPath;
    if (!SetEnvironmentVariableW(L"JAVA_TOOL_OPTIONS", jto.c_str())) {
        std::wcerr << L"[loader] WARN: SetEnvironmentVariable JAVA_TOOL_OPTIONS failed\n";
    } else {
        std::wcout << L"[loader] JAVA_TOOL_OPTIONS = " << jto << L"\n";
    }

    SHELLEXECUTEINFOW sei{}; sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = launcher.c_str();
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei)) {
        std::wcerr << L"[loader] ERROR: failed to launch " << launcher
                   << L" (GetLastError=" << GetLastError() << L")\n";
        std::wcerr << L"\nPress Enter to exit...";
        std::wcin.get();
        return 2;
    }
    std::wcout << L"[loader] launcher started (pid hidden)\n";

    waitForGameStart();

    std::thread hammer(hammerThread, jars, targetDir);

    waitForGameEnd();

    g_stop.store(true);
    if (hammer.joinable()) hammer.join();

    if (sei.hProcess) CloseHandle(sei.hProcess);
    std::wcout << L"[loader] all done. Press Enter to exit...";
    std::wcin.get();
    return 0;
}
