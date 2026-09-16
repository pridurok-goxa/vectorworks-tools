#include "user_prompts.h"

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

namespace vwlauncher {

namespace {

constexpr wchar_t kAppTitle[] = L"Vectorworks Launcher EN";

}  // namespace

std::optional<std::wstring> UserPrompts::PromptForVectorworksExecutable() {
    wchar_t filePath[MAX_PATH] = {};

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"Vectorworks executable\0Vectorworks*.exe\0Executable files\0*.exe\0All files\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Выберите исполняемый файл Vectorworks";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    wchar_t programFiles[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, SHGFP_TYPE_CURRENT, programFiles))) {
        ofn.lpstrInitialDir = programFiles;
    }

    if (!GetOpenFileNameW(&ofn)) {
        return std::nullopt;
    }

    return std::wstring(filePath);
}

bool UserPrompts::ConfirmLaunchNonVectorworksFile(const std::wstring& filePath) {
    const auto slash = filePath.find_last_of(L"\\/");
    const std::wstring fileName =
        (slash == std::wstring::npos) ? filePath : filePath.substr(slash + 1);

    const std::wstring message =
        L"Выбранный файл не похож на Vectorworks.\n\n" + fileName + L"\n\nЗапустить его?";

    const int result = MessageBoxW(
        nullptr,
        message.c_str(),
        kAppTitle,
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);

    return result == IDYES;
}

void UserPrompts::ShowNotFoundMessage() {
    MessageBoxW(
        nullptr,
        L"Файл запуска Vectorworks не найден автоматически.\n\n"
        L"Положите этот helper в папку установки Vectorworks "
        L"или выберите Vectorworks*.exe вручную.",
        kAppTitle,
        MB_OK | MB_ICONINFORMATION);
}

void UserPrompts::ShowCancelledMessage() {
    MessageBoxW(
        nullptr,
        L"Запуск Vectorworks отменён.",
        kAppTitle,
        MB_OK | MB_ICONINFORMATION);
}

void UserPrompts::ShowLaunchError(const std::wstring& message) {
    MessageBoxW(
        nullptr,
        message.c_str(),
        kAppTitle,
        MB_OK | MB_ICONERROR);
}

bool UserPrompts::ConfirmContinueDespiteLayoutFailure() {
    const int result = MessageBoxW(
        nullptr,
        L"Не удалось переключить раскладку на английскую (en-US).\n\n"
        L"Vectorworks может некорректно обрабатывать ввод с русской раскладкой.\n\n"
        L"Продолжить запуск Vectorworks?",
        kAppTitle,
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);

    return result == IDYES;
}

void UserPrompts::ShowLayoutUnavailableMessage() {
    MessageBoxW(
        nullptr,
        L"Английская раскладка (en-US) не найдена в Windows.\n\n"
        L"Один раз добавьте English в Параметры → Время и язык → Язык и регион, "
        L"затем снова запустите helper.",
        kAppTitle,
        MB_OK | MB_ICONWARNING);
}

}  // namespace vwlauncher
