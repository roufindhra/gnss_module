#ifndef GNSS_IO_RINEX_OBS_ERROR_H
#define GNSS_IO_RINEX_OBS_ERROR_H

#include <stdexcept>
#include <string>

namespace gnssio {

class RinexParseError : public std::runtime_error {
public:
    RinexParseError(std::string message, long lineNo)
        : std::runtime_error("RINEX parse error at line " + std::to_string(lineNo) +
                             ": " + message),
          line(lineNo) {}

    long line;
};

}  // namespace gnssio

#endif
