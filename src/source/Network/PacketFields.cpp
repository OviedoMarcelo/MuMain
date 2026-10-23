#include "Network/PacketFields.h"

#include "Data/Translation/MultiLanguage.h"

#include <vector>

namespace Network::PacketFields
{
namespace
{
constexpr int BitsPerByte = 8;
constexpr uint32_t ByteMask = 0xFFu;
constexpr int UInt32Bytes = 4;
} // namespace

std::wstring ReadString(const BYTE* data, int32_t offset, int32_t length)
{
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
    CMultiLanguage::ConvertFromUtf8(buffer.data(), reinterpret_cast<const char*>(data + offset), length);
    buffer[length] = L'\0';
    return std::wstring(buffer.data());
}

uint16_t ReadUInt16LittleEndian(const BYTE* data, int32_t offset)
{
    return static_cast<uint16_t>(data[offset] | (data[offset + 1] << BitsPerByte));
}

uint16_t ReadUInt16BigEndian(const BYTE* data, int32_t offset)
{
    return static_cast<uint16_t>((data[offset] << BitsPerByte) | data[offset + 1]);
}

uint32_t ReadUInt32LittleEndian(const BYTE* data, int32_t offset)
{
    uint32_t value = 0;
    for (int byte = UInt32Bytes - 1; byte >= 0; --byte)
    {
        value = (value << BitsPerByte) | data[offset + byte];
    }

    return value;
}

void WriteUInt32LittleEndian(BYTE* data, int32_t offset, uint32_t value)
{
    for (int byte = 0; byte < UInt32Bytes; ++byte)
    {
        data[offset + byte] = static_cast<BYTE>((value >> (byte * BitsPerByte)) & ByteMask);
    }
}
} // namespace Network::PacketFields
