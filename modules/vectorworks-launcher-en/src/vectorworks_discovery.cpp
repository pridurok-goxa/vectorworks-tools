#include "vectorworks_discovery.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <regex>
#include <vector>
#include <windows.h>
#include <shlobj.h>

namespace vwlauncher {

namespace fs = std::filesystem;

namespace {

const std::wregex kYearInNameRegex(L"Vectorworks(\\d{4})\\.exe$", std::regex_constants::icase);

const wchar_t* kExcludedPatterns[] = {
    L"*Install Manager*.exe",
    L"*Installer*.exe",
    L"Uninstall.exe",
    L"vectorworks_error_handler.exe",
    L"VectorworksPackageManager.exe",
    L"PackageManager.exe",
    L"VWProxyMgr.exe",
    L"VWIMHelper.exe",
    L"ShouldInstall.exe",
    L"elevate.exe",
    L"curl.exe",
    L"vcredist*.exe",
    L"DXSETUP.exe",
    L"TeamViewerQS.exe",
    L"allplan_start.exe",
};

std::wstring ToLower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return value;
}

bool PathsEqual(const std::wstring& left, const std::wstring& right) {
    try {
        return ToLower(fs::weakly_canonical(left).wstring())
            == ToLower(fs::weakly_canonical(right).wstring());
    } catch (...) {
        return false;
    }
}

// Простое сопоставление с * и ? (без regex для предсказуемости).
bool MatchesWildcard(const std::wstring& input, const std::wstring& pattern) {
    size_t ip = 0;
    size_t pp = 0;
    size_t starPp = std::wstring::npos;
    size_t starIp = 0;

    while (ip < input.size()) {
        if (pp < pattern.size() && (pattern[pp] == L'?' || std::towlower(pattern[pp]) == std::towlower(input[ip]))) {
            ++ip;
            ++pp;
            continue;
        }

        if (pp < pattern.size() && pattern[pp] == L'*') {
            starPp = pp++;
            starIp = ip;
            continue;
        }

        if (starPp != std::wstring::npos) {
            pp = starPp + 1;
            ip = ++starIp;
            continue;
        }

        return false;
    }

    while (pp < pattern.size() && pattern[pp] == L'*') {
        ++pp;
    }

    return pp == pattern.size();
}

bool IsExcluded(const std::wstring& fileName) {
    for (const auto* pattern : kExcludedPatterns) {
        if (MatchesWildcard(fileName, pattern)) {
            return true;
        }
    }
    return false;
}

int GetYearFromFileName(const std::wstring& path) {
    const auto fileName = fs::path(path).filename().wstring();
    std::wsmatch match;
    if (std::regex_search(fileName, match, kYearInNameRegex) && match.size() > 1) {
        return std::stoi(match[1].str());
    }
    return 0;
}

FILETIME GetLastWriteUtc(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data)) {
        return {};
    }
    return data.ftLastWriteTime;
}

bool CompareFileTimeDesc(const FILETIME& left, const FILETIME& right) {
    return CompareFileTime(&left, &right) > 0;
}

std::optional<std::wstring> SelectBestCandidate(std::vector<std::wstring> candidates) {
    if (candidates.empty()) {
        return std::nullopt;
    }

    std::sort(candidates.begin(), candidates.end(), [](const std::wstring& a, const std::wstring& b) {
        const int yearA = GetYearFromFileName(a);
        const int yearB = GetYearFromFileName(b);
        if (yearA != yearB) {
            return yearA > yearB;
        }

        const FILETIME timeA = GetLastWriteUtc(a);
        const FILETIME timeB = GetLastWriteUtc(b);
        if (CompareFileTime(&timeA, &timeB) != 0) {
            return CompareFileTimeDesc(timeA, timeB);
        }

        return ToLower(fs::path(a).filename().wstring()) > ToLower(fs::path(b).filename().wstring());
    });

    return candidates.front();
}

void CollectVectorworksInDirectory(const std::wstring& directory, const std::wstring& helperFullPath,
                                   std::vector<std::wstring>& out) {
    std::error_code ec;
    if (!fs::is_directory(directory, ec)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (ec) {
            break;
  // доступ запрещён или IO-ошибка
        }
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto fileName = entry.path().filename().wstring();
        if (!MatchesWildcard(fileName, L"Vectorworks*.exe")) {
            continue;
        }

        const auto fullPath = entry.path().wstring();
        if (PathsEqual(fullPath, helperFullPath) || IsExcluded(fileName)) {
            continue;
        }

        out.push_back(fullPath);
    }
}

std::optional<std::wstring> FindInDirectory(const std::wstring& directory, const std::wstring& helperFullPath) {
    std::vector<std::wstring> candidates;
    CollectVectorworksInDirectory(directory, helperFullPath, candidates);
    return SelectBestCandidate(std::move(candidates));
}

void CollectInProgramFilesRoot(const std::wstring& programFilesRoot, const std::wstring& helperFullPath,
                               std::vector<std::wstring>& out) {
    std::error_code ec;
    if (!fs::is_directory(programFilesRoot, ec)) {
        return;
    }

    const wchar_t* dirPatterns[] = {L"Vectorworks *", L"Vectorworks * EN", L"VW*"};

    for (const auto* pattern : dirPatterns) {
        for (const auto& entry : fs::directory_iterator(programFilesRoot, ec)) {
            if (ec) {
                break;
            }
            if (!entry.is_directory()) {
                continue;
            }

            const auto dirName = entry.path().filename().wstring();
            if (!MatchesWildcard(dirName, pattern)) {
                continue;
            }

            CollectVectorworksInDirectory(entry.path().wstring(), helperFullPath, out);
        }
    }
}

}  // namespace

std::optional<std::wstring> VectorworksDiscovery::FindVectorworksExecutable(
    const std::wstring& helperExecutablePath) {
    std::error_code ec;
    const fs::path helperPath = fs::weakly_canonical(helperExecutablePath, ec);
    if (ec) {
        return std::nullopt;
    }

    const auto helperFullPath = helperPath.wstring();
    const auto helperDir = helperPath.parent_path().wstring();

    if (auto local = FindInDirectory(helperDir, helperFullPath)) {
        return local;
    }

    std::vector<std::wstring> candidates;

    wchar_t programFiles[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, SHGFP_TYPE_CURRENT, programFiles))) {
        CollectInProgramFilesRoot(programFiles, helperFullPath, candidates);
    }

    wchar_t programFilesX86[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, SHGFP_TYPE_CURRENT, programFilesX86))) {
        if (ToLower(programFilesX86) != ToLower(programFiles)) {
            CollectInProgramFilesRoot(programFilesX86, helperFullPath, candidates);
        }
    }

    return SelectBestCandidate(std::move(candidates));
}

bool VectorworksDiscovery::LooksLikeVectorworksExecutable(const std::wstring& filePath) {
    const auto fileName = fs::path(filePath).filename().wstring();
    if (fileName.size() < 14) {
        return false;
    }

    const std::wstring prefix = ToLower(fileName.substr(0, 11));
    const std::wstring suffix = ToLower(fileName.substr(fileName.size() - 4));

    if (prefix != L"vectorworks" || suffix != L".exe") {
        return false;
    }

    return !IsExcluded(fileName);
}

}  // namespace vwlauncher
