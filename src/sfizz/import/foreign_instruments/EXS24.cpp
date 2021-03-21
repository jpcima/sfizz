#include "EXS24.h"


// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "EXS24.h"
#include <absl/strings/match.h>
#include <absl/strings/string_view.h>
#include <absl/memory/memory.h>
#include <algorithm>
#include <type_traits>
#include <locale>
#include <sstream>
#include <cstdio>

namespace sfz {

///
EXS24InstrumentFormat& EXS24InstrumentFormat::getInstance()
{
    static EXS24InstrumentFormat format;
    return format;
}

const char* EXS24InstrumentFormat::name() const noexcept
{
    return "EXS24 instrument";
}

bool EXS24InstrumentFormat::matchesFilePath(const fs::path& path) const
{
    const std::string ext = path.extension().u8string();
    return absl::EqualsIgnoreCase(ext, ".exs");
}

std::unique_ptr<InstrumentImporter> EXS24InstrumentFormat::createImporter() const
{
    return absl::make_unique<EXS24InstrumentImporter>();
}

///
std::string EXS24InstrumentImporter::convertToSfz(const fs::path& path) const
{
    std::ostringstream os;
    os.imbue(std::locale::classic());

    EXS24 exs;
    extractAllChunks(path, exs);

    return os.str();
}

const InstrumentFormat* EXS24InstrumentImporter::getFormat() const noexcept
{
    return &EXS24InstrumentFormat::getInstance();
}

///
struct FILE_deleter {
    void operator()(FILE* x) const noexcept { fclose(x); }
};
typedef std::unique_ptr<FILE, FILE_deleter> FILE_u;

///
static uint32_t convertU32(const void* data, bool little)
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    if (little)
        return (uint32_t(bytes[0]))       | (uint32_t(bytes[1]) << 8) |
               (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
    else
        return (uint32_t(bytes[0]) << 24) | (uint32_t(bytes[1]) << 16) |
               (uint32_t(bytes[2]) << 8)  | (uint32_t(bytes[3]));
}
static bool readU32(FILE* stream, uint32_t& number, bool little)
{
    uint8_t bytes[4] = {};
    size_t count = fread(reinterpret_cast<char*>(bytes), 4, 1, stream);
    number = convertU32(bytes, little);
    return count == 1;
}
static bool readS32(FILE* stream, int32_t& number, bool little)
{
    return readU32(stream, reinterpret_cast<uint32_t&>(number), little);
}
static bool readU8(FILE* stream, uint8_t& number)
{
    number = 0;
    return fread(&number, 1, 1, stream) == 1;
}
static bool readS8(FILE* stream, int8_t& number)
{
    return readU8(stream, reinterpret_cast<uint8_t&>(number));
}

///
bool EXS24InstrumentImporter::extractAllChunks(const fs::path& path, EXS24& exs) const
{
#if defined(_WIN32)
    FILE_u file_u { _wfopen(path.c_str(), L"rb") };
#else
    FILE_u file_u { fopen(path.c_str(), "rb") };
#endif
    FILE* file = file_u.get();

    using u_off_t = std::make_unsigned<off_t>::type;

    ///
    bool little {};
    uint32_t fileSize {};

    ///
    uint32_t headChunkMagic {};
    readU32(file, headChunkMagic, true);
    if (headChunkMagic == 0x00000101)
        little = true;
    else if (headChunkMagic == 0x01010000)
        little = false;
    else
        return false;

    ///
    if (fseek(file, 0, SEEK_END) != 0)
        return false;
    u_off_t fileEndOffset = ftell(file);
    if (fileEndOffset > 16 * 1024 * 1024)
        return false; // file too large

    ///
    fileSize = static_cast<uint32_t>(fileEndOffset);

    ///
    rewind(file);

    for (uint32_t chunkOffset = 0; chunkOffset < fileSize;) {
        auto seekToOffset = [file, chunkOffset](uint32_t offset) -> bool {
            return fseek(file, static_cast<off_t>(chunkOffset) + static_cast<off_t>(offset), SEEK_SET) == 0;
        };

        //
        if (!seekToOffset(0))
            return false;

        uint32_t chunkSignature {};
        if (!readU32(file, chunkSignature, little))
            break;

        if ((chunkSignature & 0xffff) != 0x0101)
            break;

        uint32_t chunkSize {};
        if (!readU32(file, chunkSize, little))
            break;
        chunkSize += 84;

        uint32_t chunkId {};
        if (!readU32(file, chunkId, little))
            break;

        char name[65] {};
        if (!seekToOffset(20))
            break;
        fread(name, 1, 64, file);

        //
        switch (chunkSignature >> 16) {
        case 0x0100: // zone
            {
                exs.zones.emplace_back();
                EXS24::Zone& zone = exs.zones.back();

                if (seekToOffset(84)) readU8(file, zone.flags);
                if (seekToOffset(85)) readU8(file, zone.rootNote);
                if (seekToOffset(86)) readS8(file, zone.fineTune);
                if (seekToOffset(87)) readS8(file, zone.pan);
                if (seekToOffset(88)) readS8(file, zone.volumeAdjust);
                if (seekToOffset(90)) readU8(file, zone.startNote);
                if (seekToOffset(91)) readU8(file, zone.endNote);
                if (seekToOffset(93)) readU8(file, zone.minVelocity);
                if (seekToOffset(94)) readU8(file, zone.maxVelocity);
                if (seekToOffset(96)) readS32(file, zone.sampleStart, little);
                if (seekToOffset(100)) readS32(file, zone.sampleEnd, little);
                if (seekToOffset(104)) readS32(file, zone.loopStart, little);
                if (seekToOffset(108)) readS32(file, zone.loopEnd, little);
                if (seekToOffset(117)) readU8(file, zone.loop);
                if (seekToOffset(172)) readS32(file, zone.group, little);
                if (seekToOffset(176)) readU32(file, zone.sample, little);
                memcpy(zone.name, name, 64);
            }
            break;
        case 0x0200: // group
            {
                exs.groups.emplace_back();
                EXS24::Group& group = exs.groups.back();

                if (seekToOffset(86)) readU8(file, group.polyphony);
                if (seekToOffset(157)) readU8(file, group.trigger);
                if (seekToOffset(158)) readU8(file, group.output);
                if (seekToOffset(164)) readS32(file, group.sequence, little);
                memcpy(group.name, name, 64);
            }
            break;
        case 0x0300: // sample
            {
                exs.samples.emplace_back();
                EXS24::Sample& sample = exs.samples.back();

                if (seekToOffset(88)) readS32(file, sample.length, little);
                if (seekToOffset(92)) readS32(file, sample.rate, little);
                if (seekToOffset(96)) readU8(file, sample.bitDepth);
                memcpy(sample.name, name, 64);
            }
            break;
        case 0x0400: // param
            {
            }
            break;
        }

        //
        chunkOffset = std::min(
            static_cast<uint64_t>(fileSize),
            static_cast<uint64_t>(chunkOffset) + static_cast<uint64_t>(chunkSize));
    }

    return true;
}

} // namespace sfz
