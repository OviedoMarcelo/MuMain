#include "stdafx.h"

#include "World/MapInfra/ChristmasLorencia.h"

#include <array>
#include <atomic>
#include <cstdio>
#include <cwctype>

namespace World::ChristmasLorencia
{
namespace
{
std::atomic<bool> s_enabled{ false };

constexpr const wchar_t* ChristmasFolder = L"Data\\Navidad\\";

// The folders of Lorencia, after "Data\".
constexpr std::array<const wchar_t*, 2> LorenciaFolders = { L"world1\\", L"object1\\" };

std::wstring Normalize(const std::wstring& path)
{
    std::wstring result = path;
    for (auto& c : result)
    {
        c = (c == L'/') ? L'\\' : static_cast<wchar_t>(std::towlower(c));
    }

    return result;
}

bool FileExists(const std::wstring& path)
{
    FILE* fp = _wfopen(path.c_str(), L"rb");
    if (fp == nullptr)
    {
        return false;
    }

    fclose(fp);
    return true;
}

// The packed file which the client actually reads for a texture.
std::wstring PackedTexture(const std::wstring& path)
{
    const auto dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos)
    {
        return {};
    }

    const auto ext = Normalize(path.substr(dot + 1));
    if (ext == L"jpg")
    {
        return path.substr(0, dot) + L".OZJ";
    }

    if (ext == L"tga")
    {
        return path.substr(0, dot) + L".OZT";
    }

    return {};
}
} // namespace

void SetEnabled(bool enabled)
{
    s_enabled = enabled;
}

bool IsEnabled()
{
    return s_enabled;
}

std::wstring Resolve(const std::wstring& path)
{
    if (!s_enabled)
    {
        return path;
    }

    const auto normalized = Normalize(path);
    constexpr std::wstring_view dataPrefix = L"data\\";
    if (normalized.rfind(dataPrefix, 0) != 0)
    {
        return path;
    }

    const auto relative = normalized.substr(dataPrefix.size());
    bool isLorencia = false;
    for (const auto* folder : LorenciaFolders)
    {
        if (relative.rfind(folder, 0) == 0)
        {
            isLorencia = true;
            break;
        }
    }

    if (!isLorencia)
    {
        return path;
    }

    // Keeps the original spelling of the file name.
    const std::wstring candidate = ChristmasFolder + path.substr(dataPrefix.size());
    if (FileExists(candidate))
    {
        return candidate;
    }

    const auto packed = PackedTexture(candidate);
    if (!packed.empty() && FileExists(packed))
    {
        return candidate;
    }

    return path;
}
} // namespace World::ChristmasLorencia
