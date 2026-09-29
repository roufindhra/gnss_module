#include <cstdlib>
#include <iostream>
#include <vector>

#include "io/rinex/obs/obs_error.h"
#include "io/rinex/obs/parser.h"

using namespace gnssio;

namespace {

int failures = 0;

template <typename T>
void expectEq(T got, T want, const char* what) {
    if (got != want) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        ++failures;
    }
}

std::vector<ObsEpoch> collect(const std::string& path, ObsHeader* headerOut,
                              std::vector<std::string>* warningsOut) {
    std::vector<ObsEpoch> epochs;
    auto parser = makeParser(path);
    ObsHeader h = parser->parse(path, [&](const ObsEpoch& e) { epochs.push_back(e); });
    if (headerOut) *headerOut = h;
    if (warningsOut) *warningsOut = parser->warnings();
    return epochs;
}

}  // namespace

int main() {
    // ---- file terpotong di tengah record: RinexParseError, bukan crash
    {
        try {
            collect("tests/data/rinex/obs/truncated.o", nullptr, nullptr);
            std::cerr << "FAIL (no throw): truncated.o\n";
            ++failures;
        } catch (const RinexParseError& e) {
            if (e.line < 25) {
                std::cerr << "FAIL: truncated line number got=" << e.line << "\n";
                ++failures;
            }
        }
    }

    // ---- epoch flag tidak valid (v2: >6) -> throw
    {
        try {
            collect("tests/data/rinex/obs/v211_badflag.o", nullptr, nullptr);
            std::cerr << "FAIL (no throw): v211_badflag.o\n";
            ++failures;
        } catch (const RinexParseError&) {
        }
    }

    // ---- sistem satelit tak dikenal: warning + satelit di-skip, epoch tetap utuh
    {
        ObsHeader h;
        std::vector<std::string> warnings;
        const auto epochs = collect("tests/data/rinex/obs/v302_unknown.o", &h, &warnings);
        expectEq(epochs.size(), size_t{2}, "unknown-sys epoch count");
        expectEq(epochs[0].sats.size(), size_t{39}, "unknown-sys epoch1 sats");
        expectEq(warnings.size(), size_t{1}, "unknown-sys warning count");
        if (!warnings.empty()) {
            expectEq(warnings[0].find('X') != std::string::npos, true,
                     "unknown-sys warning menyebut sistem X");
        }
    }

    if (failures > 0) {
        std::cerr << failures << " assertion gagal\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
