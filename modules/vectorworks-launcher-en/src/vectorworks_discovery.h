#pragma once

#include <optional>
#include <string>

namespace vwlauncher {

// Поиск исполняемого файла Vectorworks рядом с helper и в типовых путях установки.
class VectorworksDiscovery {
public:
    static std::optional<std::wstring> FindVectorworksExecutable(const std::wstring& helperExecutablePath);
    static bool LooksLikeVectorworksExecutable(const std::wstring& filePath);
};

}  // namespace vwlauncher
