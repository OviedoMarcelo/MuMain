#pragma once

#include <string>

// The Christmas version of Lorencia of MU Cyber.
//
// While the server says so (server command 200, see ReceiveServerCommand), the
// files of Lorencia (Data\World1 and Data\Object1) are loaded from Data\Navidad
// instead, when they exist there. Files which aren't in Data\Navidad are loaded
// from their usual place, so the folder only needs what changes.
//
// The server sends it when the account logs in, before the character is
// selected, so that Lorencia is loaded with the right files. A change while
// playing applies the next time Lorencia is loaded.
namespace World::ChristmasLorencia
{
void SetEnabled(bool enabled);

bool IsEnabled();

// Returns the path of the Christmas file which replaces the specified one, or
// the specified path if it's not a file of Lorencia, the Christmas version is
// not enabled or Data\Navidad doesn't have it. For textures, the packed file
// (.OZJ or .OZT) counts, too.
std::wstring Resolve(const std::wstring& path);
} // namespace World::ChristmasLorencia
