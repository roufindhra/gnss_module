#ifndef GNSS_IO_RINEX_OBS_TYPES_H
#define GNSS_IO_RINEX_OBS_TYPES_H

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace gnssio {

struct RinexVersion {
    int major = 0;
    int minor = 0;
};

// Waktu epoch apa adanya (tanpa konversi time system).
struct RinexTime {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    double second = 0.0;
};

struct ObsValue {
    std::optional<double> value;  // nullopt = field blank/missing
    int lli = 0;                  // v2/v3: bit0 slip, bit1 half-cycle (v2), bit2 AS/BOC
    int ssi = 0;                  // 1..9, 0 = unknown
};

struct SatObs {
    char system = ' ';  // G R E C J I S
    int prn = 0;
    std::map<std::string, ObsValue> obs;  // keyed by kode mentah (C1 / C1C)
};

struct ObsEpoch {
    RinexTime time;
    int flag = 0;  // 0=OK, 1..6 sesuai spek
    std::optional<double> receiverClock;
    std::vector<SatObs> sats;
    // flag 2-5/6: record khusus yang mengikuti epoch line (comment/header/cycle slip)
    std::vector<std::string> specialRecords;
};

struct ObsHeader {
    RinexVersion version;
    char satelliteSystem = ' ';  // kolom 41 line pertama (G/R/E/M/S, dst.)
    std::map<char, std::vector<std::string>> obsTypesPerSystem;
    // v2: satu daftar global, disimpan di bawah key ' ' (dari # / TYPES OF OBSERV)
    std::optional<double> markerX, markerY, markerZ;  // APPROX POSITION XYZ
    std::optional<double> interval;
    std::string markerName;
    std::optional<int> leapSeconds;
};

}  // namespace gnssio

#endif
