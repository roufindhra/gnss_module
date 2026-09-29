#ifndef GNSS_IO_RINEX_OBS_PARSER_V3_H
#define GNSS_IO_RINEX_OBS_PARSER_V3_H

#include "io/rinex/obs/parser.h"

namespace gnssio {

// Parser record observasi RINEX 3.x dan 4.x (layout record identik).
class Rinex34ObsParser : public IRinexObsParser {
public:
    ObsHeader parse(const std::string& path, const EpochCallback& onEpoch) override;
    const std::vector<std::string>& warnings() const override { return warnings_; }

private:
    std::vector<std::string> warnings_;
};

}  // namespace gnssio

#endif
