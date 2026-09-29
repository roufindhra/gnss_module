#include "io/rinex/obs/header_parser.h"

#include <cstdlib>
#include <cmath>

#include "io/rinex/obs/obs_error.h"

namespace gnssio {
namespace {

std::string label(const std::string& line) {
    std::string s = LineReader::field(line, 61, 20);
    size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) return {};
    size_t e = s.find_last_not_of(' ');
    return s.substr(b, e - b + 1);
}

std::string trimmed(const std::string& s) {
    size_t b = s.find_first_not_of(' ');
    if (b == std::string::npos) return {};
    size_t e = s.find_last_not_of(' ');
    return s.substr(b, e - b + 1);
}

// Ambil kode obs dari satu header line.
//   v2 "# / TYPES OF OBSERV": I6 + 9 x (4X,A2) -> col 11, 17, 23, ..., codeLen 2
//   v3/v4 "SYS / # / OBS TYPES": A1,2X,I3 + 13 x (1X,A3) -> col 8, 12, ..., codeLen 3
// Keduanya punya continuation line dengan label diulang, kode di posisi yang sama.
void appendObsTypes(const std::string& line, int firstCol, int colStep,
                    int maxPerLine, int codeLen, int& remaining,
                    std::vector<std::string>& out) {
    int col = firstCol;
    for (int i = 0; i < maxPerLine && remaining > 0; ++i, col += colStep) {
        std::string code = trimmed(LineReader::field(line, col, codeLen));
        if (code.empty()) break;
        out.push_back(code);
        --remaining;
    }
}

}  // namespace

ObsHeader parseHeader(LineReader& reader) {
    ObsHeader h;

    // Line pertama: RINEX VERSION / TYPE (wajib)
    if (!reader.nextLine()) {
        throw RinexParseError("file kosong", 0);
    }
    if (label(reader.line()) != "RINEX VERSION / TYPE") {
        throw RinexParseError("line pertama bukan RINEX VERSION / TYPE", reader.lineNo());
    }
    const std::string versionText = trimmed(LineReader::field(reader.line(), 1, 9));
    char* end = nullptr;
    const double version = std::strtod(versionText.c_str(), &end);
    if (end == versionText.c_str() || version < 2.0 || version > 4.99) {
        throw RinexParseError("versi RINEX tidak didukung: " + versionText,
                              reader.lineNo());
    }
    h.version.major = static_cast<int>(version);
    h.version.minor = static_cast<int>(std::lround((version - h.version.major) * 100.0));
    h.satelliteSystem = LineReader::field(reader.line(), 41, 1).empty()
                            ? ' '
                            : reader.line()[40];
    const char fileKind = LineReader::field(reader.line(), 21, 1).empty()
                              ? ' '
                              : reader.line()[20];
    if (fileKind != 'O') {
        throw RinexParseError("bukan file observasi (file type = " +
                                  std::string(1, fileKind) + ")",
                              reader.lineNo());
    }

    // State continuation untuk record obs types yang masih terbuka
    char currentSys = ' ';
    int remainingV2 = 0;
    int remainingV3 = 0;

    while (reader.nextLine()) {
        const std::string lab = label(reader.line());
        const std::string& line = reader.line();

        if (lab == "END OF HEADER") {
            return h;
        }

        if (lab.empty()) {
            continue;  // COMMENT / unknown tanpa label -> lewati
        }

        if (lab == "# / TYPES OF OBSERV") {
            if (remainingV2 > 0) {
                // Continuation line: label diulang di cols 61-80 (aturan spek v2).
                appendObsTypes(line, 11, 6, 9, 2, remainingV2,
                               h.obsTypesPerSystem[' ']);
                continue;
            }
            const int count = LineReader::parseInt(LineReader::field(line, 1, 6)).value_or(0);
            remainingV2 = count;
            h.obsTypesPerSystem[' '].clear();
            appendObsTypes(line, 11, 6, 9, 2, remainingV2, h.obsTypesPerSystem[' ']);
            continue;
        }

        if (lab == "SYS / # / OBS TYPES") {
            if (remainingV3 > 0) {
                // Continuation line: label diulang, tanpa system char & count.
                appendObsTypes(line, 8, 4, 13, 3, remainingV3,
                               h.obsTypesPerSystem[currentSys]);
                continue;
            }
            currentSys = line.empty() ? ' ' : line[0];
            const int count = LineReader::parseInt(LineReader::field(line, 4, 3)).value_or(0);
            remainingV3 = count;
            h.obsTypesPerSystem[currentSys].clear();
            appendObsTypes(line, 8, 4, 13, 3, remainingV3,
                           h.obsTypesPerSystem[currentSys]);
            continue;
        }

        if (lab == "APPROX POSITION XYZ") {
            h.markerX = LineReader::parseDouble(LineReader::field(line, 1, 14));
            h.markerY = LineReader::parseDouble(LineReader::field(line, 15, 14));
            h.markerZ = LineReader::parseDouble(LineReader::field(line, 29, 14));
            continue;
        }

        if (lab == "MARKER NAME") {
            h.markerName = trimmed(LineReader::field(line, 1, 60));
            continue;
        }

        if (lab == "INTERVAL") {
            h.interval = LineReader::parseDouble(LineReader::field(line, 1, 10));
            continue;
        }

        if (lab == "LEAP SECONDS") {
            h.leapSeconds = LineReader::parseInt(LineReader::field(line, 1, 6));
            continue;
        }

        // Record lain (PGM/RUN BY, ANT #, TIME OF FIRST OBS, WAVELENGTH FACT,
        // SYS / SCALE FACTOR, SIGNAL STRENGTH UNIT, GLONASS SLOT/FRQ#, dll.)
        // -> dilewati sesuai spek (unknown record wajib diabaikan).
    }

    throw RinexParseError("EOF sebelum END OF HEADER", reader.lineNo());
}

}  // namespace gnssio
