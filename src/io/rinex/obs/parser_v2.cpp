#include "io/rinex/obs/parser_v2.h"

#include <algorithm>
#include <cmath>

#include "io/rinex/obs/header_parser.h"
#include "io/rinex/obs/obs_error.h"

namespace gnssio {
namespace {

// Epoch line v2 (rinex211.txt Tabel A2, terverifikasi dengan contoh A7):
//   1X | tahun I2 (2-3) | mm (5-6) | dd (8-9) | hh (11-12) | min (14-15) |
//   F11.7 detik (16-26) | 2X | flag I1 (29) | nsat I3 (30-32) |
//   12 x (A1,I2) sat list (33-68) | F12.9 clock (69-80)
// Field waktu boleh blank pada epoch event tanpa waktu signifikan.

RinexTime parseEpochTime(const std::string& line) {
    RinexTime t;
    // Epoch event bisa tanpa waktu signifikan (field blank) -> year tetap 0.
    const auto yy = LineReader::parseInt(LineReader::field(line, 2, 2));
    if (yy) {
        t.year = (*yy >= 80 && *yy <= 99) ? 1900 + *yy : 2000 + *yy;
    }
    t.month = LineReader::parseInt(LineReader::field(line, 5, 2)).value_or(0);
    t.day = LineReader::parseInt(LineReader::field(line, 8, 2)).value_or(0);
    t.hour = LineReader::parseInt(LineReader::field(line, 11, 2)).value_or(0);
    t.minute = LineReader::parseInt(LineReader::field(line, 14, 2)).value_or(0);
    t.second = LineReader::parseDouble(LineReader::field(line, 16, 11)).value_or(0.0);
    return t;
}

struct SatId {
    char system = 'G';
    int prn = 0;
};

SatId parseSatId(const std::string& line, int index) {
    const int start = 33 + index * 3;
    SatId id;
    const std::string sys = LineReader::field(line, start, 1);
    if (!sys.empty() && sys != " ") id.system = sys[0];
    id.prn = LineReader::parseInt(LineReader::field(line, start + 1, 2)).value_or(0);
    return id;
}

ObsValue readObsField(const std::string& line, int index) {
    const int start = 1 + index * 16;
    return ObsValue{
        LineReader::parseDouble(LineReader::field(line, start, 14)),
        LineReader::parseInt(LineReader::field(line, start + 14, 1)).value_or(0),
        LineReader::parseInt(LineReader::field(line, start + 15, 1)).value_or(0),
    };
}

int linesPerSatellite(int nObsTypes) {
    return (nObsTypes + 4) / 5;  // ceil(n/5)
}

}  // namespace

ObsHeader Rinex2ObsParser::parse(const std::string& path, const EpochCallback& onEpoch) {
    LineReader reader(path);
    const ObsHeader h = parseHeader(reader);
    const auto it = h.obsTypesPerSystem.find(' ');
    if (it == h.obsTypesPerSystem.end() || it->second.empty()) {
        throw RinexParseError("header v2 tanpa # / TYPES OF OBSERV", reader.lineNo());
    }
    const int nObs = static_cast<int>(it->second.size());

    while (reader.nextLine()) {
        const std::string line = reader.line();  // copy: reader.line() berubah saat baca record
        if (line.empty()) continue;

        ObsEpoch epoch;
        epoch.time = parseEpochTime(line);
        epoch.flag = LineReader::parseInt(LineReader::field(line, 29, 1)).value_or(0);
        const int nsat = LineReader::parseInt(LineReader::field(line, 30, 3)).value_or(0);
        if (epoch.flag > 6) {
            throw RinexParseError("epoch flag tidak valid: " + std::to_string(epoch.flag),
                                  reader.lineNo());
        }

        if (nsat <= 12) {
            epoch.receiverClock =
                LineReader::parseDouble(LineReader::field(line, 69, 12));
        }

        if (epoch.flag == 0) {
            for (int i = 0; i < nsat; ++i) {
                SatObs sat;
                const SatId id = parseSatId(line, i);
                sat.system = id.system;
                sat.prn = id.prn;
                const int nLines = linesPerSatellite(nObs);
                for (int l = 0; l < nLines; ++l) {
                    if (!reader.nextLine()) {
                        throw RinexParseError("file terpotong: record satelit hilang",
                                              reader.lineNo());
                    }
                    const int base = l * 5;
                    const int nHere = std::min(5, nObs - base);
                    for (int k = 0; k < nHere; ++k) {
                        sat.obs[it->second[base + k]] = readObsField(reader.line(), k);
                    }
                }
                epoch.sats.push_back(std::move(sat));
            }
        } else if (epoch.flag == 6) {
            // cycle slip: sat list di epoch line, slip dalam format observasi
            for (int i = 0; i < nsat; ++i) {
                (void)parseSatId(line, i);
                const int nLines = linesPerSatellite(nObs);
                for (int l = 0; l < nLines; ++l) {
                    if (!reader.nextLine()) {
                        throw RinexParseError("file terpotong: record cycle slip hilang",
                                              reader.lineNo());
                    }
                    epoch.specialRecords.push_back(reader.line());
                }
            }
        } else if (epoch.flag != 1) {
            // flag 2-5: nsat record khusus mengikuti (comment/header)
            epoch.specialRecords.reserve(nsat);
            for (int i = 0; i < nsat; ++i) {
                if (!reader.nextLine()) {
                    throw RinexParseError("file terpotong: record event hilang",
                                          reader.lineNo());
                }
                epoch.specialRecords.push_back(reader.line());
            }
        }

        if (onEpoch) onEpoch(epoch);
    }

    return h;
}

}  // namespace gnssio
