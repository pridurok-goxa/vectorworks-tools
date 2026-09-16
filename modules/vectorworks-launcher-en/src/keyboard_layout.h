#pragma once

#include <windows.h>

namespace vwlauncher {

// Переключение системной раскладки на английскую (en-US, KLID 00000409) через WinAPI.
class KeyboardLayoutHelper {
public:
    enum class LayoutSwitchResult {
        Success,
        FailedRetryable,
        EnglishNotAvailable,
    };

    static LayoutSwitchResult EnsureEnglishBeforeLaunch();
    static void TryReapplyEnglishToWindow(HWND windowHandle);
    static bool IsEnglishLayoutActive();
    static HKL GetOrLoadEnglishLayout();
};

}  // namespace vwlauncher
