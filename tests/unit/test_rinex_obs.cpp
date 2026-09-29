#include <cmath>
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

void expectNear(double got, double want, const char* what) {
    if (std::fabs(got - want) > 1e-6) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        ++failures;
    }
}

std::vector<ObsEpoch> collect(const std::string& path, ObsHeader* headerOut) {
    std::vector<ObsEpoch> epochs;
    ObsHeader h = parseRinexObs(path, [&](const ObsEpoch& e) { epochs.push_back(e); });
    if (headerOut) *headerOut = h;
    return epochs;
}

const SatObs* findSat(const ObsEpoch& e, char system, int prn) {
    for (const auto& s : e.sats) {
        if (s.system == system && s.prn == prn) return &s;
    }
    return nullptr;
}

}  // namespace

int main() {
    // ---- v3.02 file asli (2 epoch, multi-konstelasi GREC)
    {
        ObsHeader h;
        const auto epochs = collect("tests/data/rinex/obs/v302_real.o", &h);
        expectEq(h.version.major, 3, "v302 major");
        expectEq(epochs.size(), size_t{2}, "v302 epoch count");

        const auto& e1 = epochs[0];
        expectEq(e1.time.year, 2023, "v302 year");
        expectEq(e1.time.month, 7, "v302 month");
        expectEq(e1.time.day, 4, "v302 day");
        expectEq(e1.time.hour, 7, "v302 hour");
        expectEq(e1.time.minute, 57, "v302 minute");
        expectEq(e1.time.second, 0.0, "v302 second");
        expectEq(e1.flag, 0, "v302 flag");
        expectEq(e1.receiverClock.has_value(), false, "v302 no clock");
        expectEq(e1.sats.size(), size_t{40}, "v302 epoch1 nsat");

        const SatObs* g05 = findSat(e1, 'G', 5);
        if (!g05) {
            std::cerr << "FAIL: v302 G05 tidak ditemukan\n";
            ++failures;
        } else {
            expectNear(*g05->obs.at("C1C").value, 24696854.312, "v302 G05 C1C");
            expectNear(*g05->obs.at("L1C").value, 129782847.071, "v302 G05 L1C");
            expectNear(*g05->obs.at("D1C").value, -578.210, "v302 G05 D1C");
            expectNear(*g05->obs.at("S1C").value, 36.250, "v302 G05 S1C");
            expectNear(*g05->obs.at("C2P").value, 0.0, "v302 G05 C2P");
            expectEq(g05->obs.at("C1C").lli, 0, "v302 G05 C1C lli");
            expectEq(g05->obs.at("C1C").ssi, 0, "v302 G05 C1C ssi");
            expectEq(g05->obs.size(), size_t{24}, "v302 G05 obs count");
        }

        const auto& e2 = epochs[1];
        expectEq(e2.time.minute, 57, "v302 epoch2 minute");
        expectEq(e2.time.second, 30.0, "v302 epoch2 second");
        expectEq(e2.sats.size(), size_t{39}, "v302 epoch2 nsat");
    }

    // ---- v4.02 sintetis: wrap 14 tipe, LLI/SSI non-blank, receiver clock
    {
        ObsHeader h;
        const auto epochs = collect("tests/data/rinex/obs/v402_basic.o", &h);
        expectEq(h.version.major, 4, "v402 major");
        expectEq(h.version.minor, 2, "v402 minor");
        expectEq(h.obsTypesPerSystem.at('G').size(), size_t{14}, "v402 G types");
        expectEq(h.obsTypesPerSystem.at('G').at(12), std::string("C1W"), "v402 G[12] continuation");
        expectEq(epochs.size(), size_t{1}, "v402 epoch count");

        const auto& e1 = epochs[0];
        expectEq(e1.sats.size(), size_t{2}, "v402 nsat");
        expectNear(*e1.receiverClock, -0.123456789012, "v402 rcv clock");

        const SatObs* g05 = findSat(e1, 'G', 5);
        if (!g05) {
            std::cerr << "FAIL: v402 G05 tidak ditemukan\n";
            ++failures;
        } else {
            expectEq(g05->obs.size(), size_t{14}, "v402 G05 obs count");
            expectNear(*g05->obs.at("C1C").value, 24696854.312, "v402 G05 C1C");
            expectEq(g05->obs.at("L1C").lli, 1, "v402 G05 L1C lli=1");
            expectEq(g05->obs.at("L1C").ssi, 9, "v402 G05 L1C ssi=9");
            expectNear(*g05->obs.at("L1C").value, 129782847.071, "v402 G05 L1C");
            expectEq(g05->obs.at("S5Q").ssi, 5, "v402 G05 S5Q ssi=5 (baris ke-3)");
            expectEq(g05->obs.at("C1W").value.has_value(), false, "v402 G05 C1W missing");
            expectNear(*g05->obs.at("L1W").value, 1000000.500, "v402 G05 L1W (baris ke-4)");
        }

        const SatObs* r01 = findSat(e1, 'R', 1);
        if (!r01) {
            std::cerr << "FAIL: v402 R01 tidak ditemukan\n";
            ++failures;
        } else {
            expectNear(*r01->obs.at("C1C").value, 20123456.789, "v402 R01 C1C");
            expectEq(r01->obs.at("C1C").ssi, 7, "v402 R01 C1C ssi=7");
            expectNear(*r01->obs.at("D1C").value, -1234.567, "v402 R01 D1C");
        }
    }

    // ---- v2.11 contoh resmi Annex A7: mixed G/E/R, semua varian event flag
    {
        ObsHeader h;
        const auto epochs = collect("tests/data/rinex/obs/v211_events.o", &h);
        expectEq(h.version.major, 2, "v211 major");
        expectEq(h.version.minor, 11, "v211 minor");
        expectEq(h.obsTypesPerSystem.at(' ').size(), size_t{5}, "v211 type count");
        expectEq(h.obsTypesPerSystem.at(' ').at(0), std::string("P1"), "v211 type[0]");
        expectEq(h.markerName, std::string("A 9080"), "v211 marker name");
        expectEq(epochs.size(), size_t{15}, "v211 epoch count");

        // epoch 1: 4 sat, clock receiver, missing value, LLI/SSI
        const auto& e1 = epochs[0];
        expectEq(e1.time.year, 2005, "v211 year 2-digit");
        expectEq(e1.time.month, 3, "v211 month");
        expectEq(e1.time.day, 24, "v211 day");
        expectEq(e1.time.hour, 13, "v211 hour");
        expectEq(e1.time.minute, 10, "v211 minute");
        expectEq(e1.time.second, 36.0, "v211 second");
        expectEq(e1.flag, 0, "v211 flag");
        expectNear(*e1.receiverClock, -0.123456789, "v211 rcv clock");
        expectEq(e1.sats.size(), size_t{4}, "v211 epoch1 nsat");

        const SatObs* g12 = findSat(e1, 'G', 12);
        if (!g12) {
            std::cerr << "FAIL: v211 G12 tidak ditemukan\n";
            ++failures;
        } else {
            expectNear(*g12->obs.at("P1").value, 23629347.915, "v211 G12 P1");
            expectNear(*g12->obs.at("L1").value, 0.300, "v211 G12 L1");
            expectEq(g12->obs.at("L1").ssi, 8, "v211 G12 L1 ssi");
            expectNear(*g12->obs.at("L2").value, -0.353, "v211 G12 L2");
            expectNear(*g12->obs.at("P2").value, 23629364.158, "v211 G12 P2");
            expectEq(g12->obs.at("L5").value.has_value(), false, "v211 G12 L5 missing");
        }
        const SatObs* e11 = findSat(e1, 'E', 11);
        if (!e11) {
            std::cerr << "FAIL: v211 E11 tidak ditemukan\n";
            ++failures;
        } else {
            expectEq(e11->obs.at("P1").value.has_value(), false, "v211 E11 P1 missing");
            expectNear(*e11->obs.at("L1").value, 0.324, "v211 E11 L1");
            expectNear(*e11->obs.at("L5").value, 0.178, "v211 E11 L5");
            expectEq(e11->obs.at("L5").ssi, 7, "v211 E11 L5 ssi");
        }

        // epoch 2 (line 24): flag 4, 4 record khusus
        expectEq(epochs[1].flag, 4, "v211 epoch2 flag");
        expectEq(epochs[1].specialRecords.size(), size_t{4}, "v211 epoch2 special");
        expectEq(epochs[1].sats.size(), size_t{0}, "v211 epoch2 no sats");

        // epoch 3 (line 29): 6 sat termasuk GLONASS
        expectEq(epochs[2].sats.size(), size_t{6}, "v211 epoch3 nsat");
        expectEq(findSat(epochs[2], 'R', 21) != nullptr, true, "v211 R21 ada");
        expectEq(findSat(epochs[2], 'R', 22) != nullptr, true, "v211 R22 ada");

        // epoch flag 3 (line 43): new site, 4 record khusus
        expectEq(epochs[5].flag, 3, "v211 flag3 epoch");
        expectEq(epochs[5].specialRecords.size(), size_t{4}, "v211 flag3 records");

        // event tanpa waktu signifikan (line 54): time = 0
        expectEq(epochs[8].time.year, 0, "v211 blank epoch year");
        expectEq(epochs[8].flag, 4, "v211 blank epoch flag");

        // flag 6 (line 63): cycle slip, 2 record
        expectEq(epochs[11].flag, 6, "v211 flag6 epoch");
        expectEq(epochs[11].specialRecords.size(), size_t{2}, "v211 flag6 slip lines");
    }

    if (failures > 0) {
        std::cerr << failures << " assertion gagal\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
