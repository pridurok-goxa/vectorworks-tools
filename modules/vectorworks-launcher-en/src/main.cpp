#include "keyboard_layout.h"
#include "user_prompts.h"
#include "vectorworks_discovery.h"

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>
#include <windows.h>

// ID иконки из resources/VectorworksLauncherEN.rc
#ifndef IDI_APPICON
#define IDI_APPICON 1
#endif

namespace vwlauncher {

namespace {

constexpr int kMainWindowPollTimeoutMs = 15000;
constexpr int kMainWindowPollIntervalMs = 200;

std::wstring GetHelperExecutablePath() {
    wchar_t modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return L"VectorworksLauncherEN.exe";
    }
    return std::wstring(modulePath, length);
}

bool EnsureEnglishLayoutBeforeLaunch() {
    switch (KeyboardLayoutHelper::EnsureEnglishBeforeLaunch()) {
        case KeyboardLayoutHelper::LayoutSwitchResult::Success:
            return true;

        case KeyboardLayoutHelper::LayoutSwitchResult::EnglishNotAvailable:
            UserPrompts::ShowLayoutUnavailableMessage();
            return false;

        case KeyboardLayoutHelper::LayoutSwitchResult::FailedRetryable:
            return UserPrompts::ConfirmContinueDespiteLayoutFailure();

        default:
            return false;
    }
}

std::optional<std::wstring> PromptManualVectorworksPath() {
    UserPrompts::ShowNotFoundMessage();

    const auto selected = UserPrompts::PromptForVectorworksExecutable();
    if (!selected.has_value()) {
        UserPrompts::ShowCancelledMessage();
        return std::nullopt;
    }

    if (!VectorworksDiscovery::LooksLikeVectorworksExecutable(*selected)
        && !UserPrompts::ConfirmLaunchNonVectorworksFile(*selected)) {
        UserPrompts::ShowCancelledMessage();
        return std::nullopt;
    }

    return selected;
}

struct MainWindowSearch {
    DWORD processId = 0;
    HWND mainWindow = nullptr;
};

BOOL CALLBACK EnumMainWindowProc(HWND hwnd, LPARAM lParam) {
    auto* search = reinterpret_cast<MainWindowSearch*>(lParam);

    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid != search->processId) {
        return TRUE;
    }

    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }

    if (GetWindow(hwnd, GW_OWNER) != nullptr) {
        return TRUE;
    }

    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if ((style & WS_VISIBLE) == 0) {
        return TRUE;
    }

    search->mainWindow = hwnd;
    return FALSE;
}

HWND FindMainWindowForProcess(DWORD processId) {
    MainWindowSearch search;
    search.processId = processId;
    EnumWindows(EnumMainWindowProc, reinterpret_cast<LPARAM>(&search));
    return search.mainWindow;
}

bool LaunchVectorworks(const std::wstring& executablePath, PROCESS_INFORMATION& processInfo) {
    const auto workingDirectory = std::filesystem::path(executablePath).parent_path().wstring();

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);

    // CreateProcessW требует изменяемую командную строку.
    std::wstring commandLine = L"\"" + executablePath + L"\"";

    const BOOL created = CreateProcessW(
        executablePath.c_str(),
        commandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &startupInfo,
        &processInfo);

    if (!created) {
        const DWORD error = GetLastError();
        wchar_t* messageBuffer = nullptr;
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&messageBuffer),
            0,
            nullptr);

        std::wstring message = L"Не удалось запустить Vectorworks:\n";
        if (messageBuffer != nullptr) {
            message += messageBuffer;
            LocalFree(messageBuffer);
        } else {
            message += L"код ошибки Windows " + std::to_wstring(error);
        }

        UserPrompts::ShowLaunchError(message);
        return false;
    }

    return true;
}

void TryApplyLayoutToVectorworksWindow(HANDLE processHandle, DWORD processId) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kMainWindowPollTimeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        DWORD exitCode = STILL_ACTIVE;
        if (!GetExitCodeProcess(processHandle, &exitCode) || exitCode != STILL_ACTIVE) {
            return;
        }

        HWND mainWindow = FindMainWindowForProcess(processId);
        if (mainWindow != nullptr) {
            // Повторный WM_INPUTLANGCHANGEREQUEST в окно Vectorworks после появления UI.
            KeyboardLayoutHelper::TryReapplyEnglishToWindow(mainWindow);
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(kMainWindowPollIntervalMs));
    }
}

}  // namespace

int RunLauncher() {
    const std::wstring helperPath = GetHelperExecutablePath();

    if (!EnsureEnglishLayoutBeforeLaunch()) {
        return 0;
    }

    std::optional<std::wstring> vectorworksPath =
        VectorworksDiscovery::FindVectorworksExecutable(helperPath);

    if (!vectorworksPath.has_value()) {
        vectorworksPath = PromptManualVectorworksPath();
    }

    if (!vectorworksPath.has_value() || vectorworksPath->empty()) {
        return 0;
    }

    PROCESS_INFORMATION processInfo = {};
    if (!LaunchVectorworks(*vectorworksPath, processInfo)) {
        return 1;
    }

    TryApplyLayoutToVectorworksWindow(processInfo.hProcess, processInfo.dwProcessId);

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return 0;
}

}  // namespace vwlauncher

// Точка входа Win32 GUI-приложения без консоли.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nShowCmd) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nShowCmd;

    return vwlauncher::RunLauncher();
}
