#include "keyboard_layout.h"

#include <cstring>
#include <thread>
#include <vector>

namespace vwlauncher {

namespace {

constexpr wchar_t kEnglishUsLayoutId[] = L"00000409";
constexpr LANGID kEnglishUsLangId = 0x0409;
constexpr UINT kKlfActivate = 0x00000001;
constexpr UINT kWmInputLangChangeRequest = 0x0050;
constexpr UINT kInputKeyboard = 1;
constexpr UINT kKeyeventfKeyup = 0x0002;
constexpr int kHotkeyDelayMs = 80;
constexpr int kVerifyRetryDelayMs = 120;
constexpr int kPreLaunchRetryAttempts = 4;

constexpr WORD kVkLMenu = 0xA4;
constexpr WORD kVkLShift = 0xA0;
constexpr WORD kVkLControl = 0xA2;
constexpr WORD kVkLWin = 0x5B;
constexpr WORD kVkSpace = 0x20;

bool IsEnglishKlid(const wchar_t* klid) {
    if (klid == nullptr || klid[0] == L'\0') {
        return false;
    }
    return _wcsicmp(klid, kEnglishUsLayoutId) == 0;
}

bool IsEnglishHkl(HKL hkl) {
    if (hkl == nullptr) {
        return false;
    }
    const LANGID langId = LOWORD(reinterpret_cast<UINT_PTR>(hkl));
    return langId == kEnglishUsLangId;
}

void SendKey(WORD virtualKey, bool keyUp) {
    INPUT input = {};
    input.type = kInputKeyboard;
    input.ki.wVk = virtualKey;
    input.ki.dwFlags = keyUp ? kKeyeventfKeyup : 0;
    SendInput(1, &input, sizeof(INPUT));
}

void SimulateHotkey(WORD modifier, WORD key) {
    SendKey(modifier, false);
    SendKey(key, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    SendKey(key, true);
    SendKey(modifier, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(kHotkeyDelayMs));
}

void TryHotkeyFallback() {
    SimulateHotkey(kVkLMenu, kVkLShift);
    if (KeyboardLayoutHelper::IsEnglishLayoutActive()) {
        return;
    }

    SimulateHotkey(kVkLControl, kVkLShift);
    if (KeyboardLayoutHelper::IsEnglishLayoutActive()) {
        return;
    }

    SimulateHotkey(kVkLWin, kVkSpace);
}

bool GetActiveLayoutKlid(wchar_t* buffer, size_t bufferChars) {
    if (buffer == nullptr || bufferChars < 9) {
        return false;
    }
    return GetKeyboardLayoutNameW(buffer) != 0;
}

HKL GetActiveKeyboardLayoutHandle() {
    HWND foreground = GetForegroundWindow();
    if (foreground != nullptr) {
        const DWORD threadId = GetWindowThreadProcessId(foreground, nullptr);
        if (threadId != 0) {
            return GetKeyboardLayout(threadId);
        }
    }
    return GetKeyboardLayout(GetCurrentThreadId());
}

void TryActivateOnWindowThread(HWND windowHandle, HKL englishHkl) {
    if (englishHkl == nullptr) {
        return;
    }

    if (windowHandle == nullptr) {
        ActivateKeyboardLayout(englishHkl, kKlfActivate);
        return;
    }

    const DWORD targetThread = GetWindowThreadProcessId(windowHandle, nullptr);
    const DWORD currentThread = GetCurrentThreadId();
    bool attached = false;

    if (targetThread != 0 && targetThread != currentThread) {
        attached = AttachThreadInput(currentThread, targetThread, TRUE) != FALSE;
    }

    ActivateKeyboardLayout(englishHkl, kKlfActivate);

    if (attached) {
        AttachThreadInput(currentThread, targetThread, FALSE);
    }
}

void TryPostLayoutChangeRequest(HWND windowHandle, HKL englishHkl) {
    if (windowHandle == nullptr || englishHkl == nullptr) {
        return;
    }

    PostMessageW(
        windowHandle,
        kWmInputLangChangeRequest,
        0,
        reinterpret_cast<LPARAM>(englishHkl));
}

void ApplyEnglishLayers(HKL englishHkl, HWND targetWindow) {
    LoadKeyboardLayoutW(kEnglishUsLayoutId, kKlfActivate);
    TryActivateOnWindowThread(targetWindow, englishHkl);

    HWND foreground = GetForegroundWindow();
    TryPostLayoutChangeRequest(foreground, englishHkl);

    if (targetWindow != nullptr && targetWindow != foreground) {
        TryPostLayoutChangeRequest(targetWindow, englishHkl);
    }
}

}  // namespace

HKL KeyboardLayoutHelper::GetOrLoadEnglishLayout() {
    HKL hkl = LoadKeyboardLayoutW(kEnglishUsLayoutId, kKlfActivate);
    if (hkl != nullptr) {
        return hkl;
    }

    const int count = GetKeyboardLayoutList(0, nullptr);
    if (count <= 0) {
        return nullptr;
    }

    std::vector<HKL> layouts(static_cast<size_t>(count));
    if (GetKeyboardLayoutList(count, layouts.data()) != count) {
        return nullptr;
    }

    for (HKL layout : layouts) {
        if (IsEnglishHkl(layout)) {
            return layout;
        }
    }

    return nullptr;
}

bool KeyboardLayoutHelper::IsEnglishLayoutActive() {
    wchar_t klid[9] = {};
    if (GetActiveLayoutKlid(klid, 9) && IsEnglishKlid(klid)) {
        return true;
    }

    return IsEnglishHkl(GetActiveKeyboardLayoutHandle());
}

KeyboardLayoutHelper::LayoutSwitchResult KeyboardLayoutHelper::EnsureEnglishBeforeLaunch() {
    HKL englishHkl = GetOrLoadEnglishLayout();
    if (englishHkl == nullptr) {
        return LayoutSwitchResult::EnglishNotAvailable;
    }

    for (int attempt = 0; attempt < kPreLaunchRetryAttempts; ++attempt) {
        ApplyEnglishLayers(englishHkl, GetForegroundWindow());

        if (IsEnglishLayoutActive()) {
            return LayoutSwitchResult::Success;
        }

        if (attempt < kPreLaunchRetryAttempts - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kVerifyRetryDelayMs));
        }
    }

    TryHotkeyFallback();

    if (IsEnglishLayoutActive()) {
        return LayoutSwitchResult::Success;
    }

    return LayoutSwitchResult::FailedRetryable;
}

void KeyboardLayoutHelper::TryReapplyEnglishToWindow(HWND windowHandle) {
    if (windowHandle == nullptr) {
        return;
    }

    HKL englishHkl = GetOrLoadEnglishLayout();
    if (englishHkl == nullptr) {
        return;
    }

    ApplyEnglishLayers(englishHkl, windowHandle);

    if (IsEnglishLayoutActive()) {
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(kVerifyRetryDelayMs));
    ApplyEnglishLayers(englishHkl, windowHandle);

    if (IsEnglishLayoutActive()) {
        return;
    }

    TryHotkeyFallback();
}

}  // namespace vwlauncher
