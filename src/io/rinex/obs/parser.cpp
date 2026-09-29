#include "io/rinex/obs/parser.h"

#include <memory>

#include "io/rinex/obs/header_parser.h"
#include "io/rinex/obs/obs_error.h"
#include "io/rinex/obs/parser_v2.h"
#include "io/rinex/obs/parser_v3.h"

namespace gnssio {

std::unique_ptr<IRinexObsParser> makeParser(const std::string& path) {
    LineReader reader(path);
    if (!reader.nextLine()) {
        throw RinexParseError("file kosong", 0);
    }
    std::string lab = LineReader::field(reader.line(), 61, 20);
    const size_t b = lab.find_first_not_of(' ');
    if (b == std::string::npos || lab.substr(b) != "RINEX VERSION / TYPE") {
        throw RinexParseError("line pertama bukan RINEX VERSION / TYPE", 1);
    }
    const double version = LineReader::parseDouble(
                               LineReader::field(reader.line(), 1, 9))
                               .value_or(0.0);
    if (version >= 3.0 && version < 5.0) {
        return std::make_unique<Rinex34ObsParser>();
    }
    if (version >= 2.0 && version < 3.0) {
        return std::make_unique<Rinex2ObsParser>();
    }
    throw RinexParseError("versi RINEX tidak didukung", 1);
}

ObsHeader parseRinexObs(const std::string& path, const EpochCallback& onEpoch) {
    return makeParser(path)->parse(path, onEpoch);
}

}  // namespace gnssio
