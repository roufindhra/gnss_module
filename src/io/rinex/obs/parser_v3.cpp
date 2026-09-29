#include "io/rinex/obs/parser_v3.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include "io/rinex/obs/header_parser.h"
#include "io/rinex/obs/obs_error.h"

namespace gnssio {
namespace {

// Epoch line (rinex_4.02 Tabel A3 / contoh Tabel 18):
//   '>' (1) | I4 tahun (3-6) | mm (8-9) | dd (11-12) | hh (14-15) | min (17-18) |
//   F11.7 detik (20-30) | flag I1 (32) | nsat I3 (33-35) | clock bebas (36+)
struct EpochLine {
    RinexTime time;
    int flag = 0;
    int nsat = 0;
    std::optional<double> receiverClock;
};

EpochLine parseEpochLine(const std::string& line, long lineNo) {
    if (line.empty() || line[0] != '>') {
        throw RinexParseError("epoch line tidak diawali '>'", lineNo);
    }
    EpochLine e;
    const auto year = LineReader::parseInt(LineReader::field(line, 3, 4));
    if (!year) throw RinexParseError("tahun epoch tidak valid", lineNo);
    e.time.year = *year;
    e.time.month = LineReader::parseInt(LineReader::field(line, 8, 2)).value_or(0);
    e.time.day = LineReader::parseInt(LineReader::field(line, 11, 2)).value_or(0);
    e.time.hour = LineReader::parseInt(LineReader::field(line, 14, 2)).value_or(0);
    e.time.minute = LineReader::parseInt(LineReader::field(line, 17, 2)).value_or(0);
    e.time.second = LineReader::parseDouble(LineReader::field(line, 20, 11)).value_or(0.0);
    e.flag = LineReader::parseInt(LineReader::field(line, 32, 1)).value_or(0);
    e.nsat = LineReader::parseInt(LineReader::field(line, 33, 3)).value_or(0);

    std::istringstream rest(LineReader::field(line, 36, 128));
    std::string token;
    if (rest >> token) {
        e.receiverClock = LineReader::parseDouble(token);
    }
    return e;
}

bool isNewSatelliteLine(const std::string& line) {
    if (line.size() < 3) return false;
    // Sistem bebas (konstelasi baru bisa saja huruf lain); cukup cols 2-3 numerik.
    return LineReader::parseInt(LineReader::field(line, 2, 2)).has_value();
}

// Baris wrap mengulang ID satelit yang sama (Tabel 18); ID berbeda = satelit baru.
std::string satId(const std::string& line) {
    return LineReader::field(line, 1, 3);
}

// Group observasi 16 char (F14.3 + LLI + SSI), group ke-i mulai kolom 4.
ObsValue readObsField(const std::string& line, int index) {
    const int start = 4 + index * 16;
    return ObsValue{
        LineReader::parseDouble(LineReader::field(line, start, 14)),
        LineReader::parseInt(LineReader::field(line, start + 14, 1)).value_or(0),
        LineReader::parseInt(LineReader::field(line, start + 15, 1)).value_or(0),
    };
}

// Berapa group observasi yang muat di satu baris (minimal 1 agar loop selalu maju).
int obsPerLine(const std::string& line) {
    const int capacity = (static_cast<int>(line.size()) - 3) / 16;
    return std::max(1, capacity);
}

}  // namespace

ObsHeader Rinex34ObsParser::parse(const std::string& path, const EpochCallback& onEpoch) {
    LineReader reader(path);
    const ObsHeader h = parseHeader(reader);

    // Lookahead satu baris: baris satelit berikutnya yang sudah terlanjur dibaca
    // saat mendeteksi akhir record satelit sebelumnya.
    std::string pending;
    bool hasPending = false;
    auto nextRecordLine = [&]() -> std::string {
        if (hasPending) {
            hasPending = false;
            return std::exchange(pending, {});
        }
        if (!reader.nextLine()) {
            throw RinexParseError("file terpotong: record satelit hilang",
                                  reader.lineNo());
        }
        return reader.line();
    };
    auto stashIfNextRecord = [&](const std::string& line) {
        pending = line;
        hasPending = true;
    };

    while (true) {
        std::string line;
        if (hasPending) {
            line = std::exchange(pending, {});
            hasPending = false;
        } else if (!reader.nextLine()) {
            break;  // EOF bersih
        } else {
            line = reader.line();
        }
        if (line.empty()) continue;

        const EpochLine e = parseEpochLine(line, reader.lineNo());
        ObsEpoch epoch;
        epoch.time = e.time;
        epoch.flag = e.flag;
        epoch.receiverClock = e.receiverClock;

        if (e.flag == 0) {
            for (int i = 0; i < e.nsat; ++i) {
                std::string satLine = nextRecordLine();
                if (!isNewSatelliteLine(satLine)) {
                    throw RinexParseError("record satelit tidak valid", reader.lineNo());
                }
                SatObs sat;
                sat.system = satLine[0];
                sat.prn = LineReader::parseInt(LineReader::field(satLine, 2, 2)).value_or(0);

                auto it = h.obsTypesPerSystem.find(sat.system);
                if (it == h.obsTypesPerSystem.end()) {
                    warnings_.push_back("sistem satelit tak dikenal: " +
                                        std::string(1, sat.system));
                    // Lewati satelit, konsumsi barisnya sampai record berikutnya.
                    while (true) {
                        std::string l = nextRecordLine();
                        if (l[0] == '>' || satId(l) != satId(satLine)) {
                            stashIfNextRecord(l);
                            break;
                        }
                    }
                    continue;
                }

                const auto& types = it->second;
                const int nTypes = static_cast<int>(types.size());
                std::vector<ObsValue> values;
                values.reserve(nTypes);
                while (static_cast<int>(values.size()) < nTypes) {
                    const int remaining = nTypes - static_cast<int>(values.size());
                    const int nHere = std::min(remaining, obsPerLine(satLine));
                    for (int k = 0; k < nHere; ++k) {
                        values.push_back(readObsField(satLine, k));
                    }
                    if (static_cast<int>(values.size()) >= nTypes) break;
                    std::string cont = nextRecordLine();
                    if (cont[0] == '>' || satId(cont) != satId(satLine)) {
                        stashIfNextRecord(cont);
                        break;  // sisa tipe = missing
                    }
                    satLine = cont;
                }
                for (int k = 0; k < nTypes; ++k) {
                    sat.obs[types[k]] = k < static_cast<int>(values.size())
                                            ? values[k]
                                            : ObsValue{std::nullopt, 0, 0};
                }
                epoch.sats.push_back(std::move(sat));
            }
        } else if (e.flag != 1) {
            // flag 2-6: nsat record khusus mengikuti (comment/header/cycle slip)
            epoch.specialRecords.reserve(e.nsat);
            for (int i = 0; i < e.nsat; ++i) {
                epoch.specialRecords.push_back(nextRecordLine());
            }
        }

        if (onEpoch) onEpoch(epoch);
    }

    return h;
}

}  // namespace gnssio
