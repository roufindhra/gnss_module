#include <cmath>
#include <cstdlib>
#include <iostream>

#include "io/rinex/obs/header_parser.h"
#include "io/rinex/obs/obs_error.h"

using namespace gnssio;

namespace {

template <typename T>
void expectEq(T got, T want, const char* what) {
    if (got != want) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        std::exit(EXIT_FAILURE);
    }
}

void expectNear(double got, double want, const char* what) {
    if (std::fabs(got - want) > 1e-6) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        std::exit(EXIT_FAILURE);
    }
}

void expectThrows(const char* path, const char* what) {
    try {
        LineReader r(path);
        parseHeader(r);
        std::cerr << "FAIL (no throw): " << what << "\n";
        std::exit(EXIT_FAILURE);
    } catch (const RinexParseError&) {
    }
}

}  // namespace

int main() {
    // ---- v2.11 header: versi, 12 tipe dengan continuation, posisi, interval
    {
        LineReader r("tests/data/rinex/obs/v211_header.o");
        const ObsHeader h = parseHeader(r);
        expectEq(h.version.major, 2, "v2 major");
        expectEq(h.version.minor, 11, "v2 minor");
        expectEq(h.satelliteSystem, 'M', "v2 system mixed");
        const auto& types = h.obsTypesPerSystem.at(' ');
        expectEq(types.size(), size_t{12}, "v2 obs type count");
        expectEq(types[0], std::string("C1"), "v2 type[0]");
        expectEq(types[8], std::string("S1"), "v2 type[8] last on line 1");
        expectEq(types[9], std::string("D2"), "v2 type[9] first on continuation");
        expectEq(types[11], std::string("C2"), "v2 type[11]");
        expectNear(*h.markerX, -1234567.8901, "v2 X");
        expectNear(*h.markerY, 4987654.3210, "v2 Y");
        expectNear(*h.markerZ, 1234567.8901, "v2 Z");
        expectNear(*h.interval, 15.0, "v2 interval");
        expectEq(*h.leapSeconds, 16, "v2 leap seconds");
        expectEq(h.markerName, std::string("TEST SITE"), "v2 marker name");
    }

    // ---- v3.02 header (file asli 1-KM 00.23O): multi-sistem, G 24 tipe kontinuasi
    {
        LineReader r("tests/data/rinex/obs/v302_header.o");
        const ObsHeader h = parseHeader(r);
        expectEq(h.version.major, 3, "v3 major");
        expectEq(h.version.minor, 2, "v3 minor");
        expectEq(h.obsTypesPerSystem.at('G').size(), size_t{24}, "v3 G type count");
        expectEq(h.obsTypesPerSystem.at('G').at(0), std::string("C1C"), "v3 G[0]");
        expectEq(h.obsTypesPerSystem.at('G').at(12), std::string("C2X"), "v3 G[12] last line 1");
        expectEq(h.obsTypesPerSystem.at('G').at(13), std::string("L2X"), "v3 G[13] continuation");
        expectEq(h.obsTypesPerSystem.at('G').at(23), std::string("S5Q"), "v3 G[23]");
        expectEq(h.obsTypesPerSystem.at('R').size(), size_t{12}, "v3 R type count");
        expectEq(h.obsTypesPerSystem.at('E').size(), size_t{12}, "v3 E type count");
        expectEq(h.obsTypesPerSystem.at('C').size(), size_t{24}, "v3 C type count");
        expectEq(h.obsTypesPerSystem.at('C').at(12), std::string("C7Q"), "v3 C[12] last line 1");
        expectEq(h.obsTypesPerSystem.at('C').at(13), std::string("L7Q"), "v3 C[13] continuation");
        expectNear(*h.markerX, -2199103.2300, "v3 X");
        expectNear(*h.interval, 30.0, "v3 interval");
    }

    // ---- error cases
    expectThrows("tests/data/rinex/obs/line_reader_sample.txt",
                 "bukan RINEX (line pertama salah)");

    return EXIT_SUCCESS;
}
