#pragma once

#include "Core/Platform/WinCompat.h"

#include <cstdint>
#include <string>

// Reads and writes the fields of the messages of the server: numbers in the
// byte order of the packet definitions, and UTF-8 strings padded with zeros.
namespace Network::PacketFields
{
// A UTF-8 string of at most length bytes, which ends at the first zero.
std::wstring ReadString(const BYTE* data, int32_t offset, int32_t length);

uint16_t ReadUInt16LittleEndian(const BYTE* data, int32_t offset);
uint16_t ReadUInt16BigEndian(const BYTE* data, int32_t offset);
uint32_t ReadUInt32LittleEndian(const BYTE* data, int32_t offset);

void WriteUInt32LittleEndian(BYTE* data, int32_t offset, uint32_t value);
} // namespace Network::PacketFields
