#ifndef GNSS_IO_RINEX_OBS_PARSER_H
#define GNSS_IO_RINEX_OBS_PARSER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "io/rinex/obs/obs_types.h"

namespace gnssio {

using EpochCallback = std::function<void(const ObsEpoch&)>;

class IRinexObsParser {
public:
    virtual ~IRinexObsParser() = default;
    // Streaming: onEpoch dipanggil untuk tiap epoch; header dikembalikan.
    virtual ObsHeader parse(const std::string& path, const EpochCallback& onEpoch) = 0;
    // Non-fatal (mis. sistem satelit tak dikenal), terisi selama parse().
    virtual const std::vector<std::string>& warnings() const = 0;
};

// Deteksi versi dari line pertama file.
std::unique_ptr<IRinexObsParser> makeParser(const std::string& path);

// Convenience: pilih parser otomatis lalu parse.
ObsHeader parseRinexObs(const std::string& path, const EpochCallback& onEpoch);

}  // namespace gnssio

#endif
