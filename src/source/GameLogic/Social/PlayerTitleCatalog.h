#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <string>
#include <unordered_map>

// The titles which players show below their name, e.g. "Blood Castle Slayer".
//
// The server sends a PlayerTitle message when a player with a title comes into
// view, when a player changes its title, and after the chat command list was
// requested. An empty text removes the title. Servers which don't know titles
// never send it, so nobody shows one.
namespace GameLogic::Social
{
struct PlayerTitle
{
    std::wstring Text;
    BYTE Red = 0;
    BYTE Green = 0;
    BYTE Blue = 0;
};

class PlayerTitleCatalog
{
public:
    static PlayerTitleCatalog& Instance();

    // Forgets everything, e.g. when we connect to another server.
    void Reset();

    // Takes the PlayerTitle message. Returns false when the data isn't
    // plausible, so that the caller can ignore it.
    bool AddFromPacket(const BYTE* data, int32_t size);

    // Forgets the title of a player, e.g. when another player appears with its key.
    void Remove(int key);

    // The title of the player with the key, or nullptr if it shows none.
    const PlayerTitle* Find(int key) const;

private:
    std::unordered_map<int, PlayerTitle> m_titles;
};

inline PlayerTitleCatalog& PlayerTitles()
{
    return PlayerTitleCatalog::Instance();
}
} // namespace GameLogic::Social
