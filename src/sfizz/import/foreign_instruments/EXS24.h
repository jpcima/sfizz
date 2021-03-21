// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "../ForeignInstrument.h"

namespace sfz {

class EXS24InstrumentFormat : public InstrumentFormat {
private:
    EXS24InstrumentFormat() noexcept = default;

public:
    static EXS24InstrumentFormat& getInstance();
    const char* name() const noexcept override;
    bool matchesFilePath(const fs::path& path) const override;
    std::unique_ptr<InstrumentImporter> createImporter() const override;
};

///
class EXS24InstrumentImporter : public InstrumentImporter {
public:
    std::string convertToSfz(const fs::path& path) const override;
    const InstrumentFormat* getFormat() const noexcept override;

private:
    struct EXS24 {
        struct Zone {
            uint8_t flags {};
            uint8_t rootNote {};
            int8_t fineTune {};
            int8_t pan {};
            int8_t volumeAdjust {};
            uint8_t startNote {};
            uint8_t endNote {};
            uint8_t minVelocity {};
            uint8_t maxVelocity {};
            int32_t sampleStart {};
            int32_t sampleEnd {};
            int32_t loopStart {};
            int32_t loopEnd {};
            uint8_t loop {};
            int32_t group {};
            uint32_t sample {};
            char name[65] {};
        };
        struct Group {
            uint8_t polyphony {};
            uint8_t trigger {};
            uint8_t output {};
            int32_t sequence {};
            char name[65] {};
        };
        struct Sample {
            int32_t length {};
            int32_t rate {};
            uint8_t bitDepth {};
            char name[65] {};
        };

        std::vector<Zone> zones;
        std::vector<Group> groups;
        std::vector<Sample> samples;
    };

    bool extractAllChunks(const fs::path& path, EXS24& exs) const;
};

} // namespace sfz
