#ifndef GNSS_IO_RINEX_OBS_PARSER_V2_H
#define GNSS_IO_RINEX_OBS_PARSER_V2_H

#include "io/rinex/obs/parser.h"

namespace gnssio {

// Parser record observasi RINEX 2.10/2.11.
class Rinex2ObsParser : public IRinexObsParser {
public:
    ObsHeader parse(const std::string& path, const EpochCallback& onEpoch) override;
    const std::vector<std::string>& warnings() const override { return warnings_; }

private:
    std::vector<std::string> warnings_;
};

}  // namespace gnssio

#endif
