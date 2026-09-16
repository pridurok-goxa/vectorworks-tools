#pragma once

#include <optional>
#include <string>

namespace vwlauncher {

// Диалоги и уведомления пользователя (MessageBoxW, GetOpenFileNameW).
class UserPrompts {
public:
    static std::optional<std::wstring> PromptForVectorworksExecutable();
    static bool ConfirmLaunchNonVectorworksFile(const std::wstring& filePath);
    static void ShowNotFoundMessage();
    static void ShowCancelledMessage();
    static void ShowLaunchError(const std::wstring& message);
    static bool ConfirmContinueDespiteLayoutFailure();
    static void ShowLayoutUnavailableMessage();
};

}  // namespace vwlauncher
