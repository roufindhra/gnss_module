#ifndef GNSS_IO_RINEX_OBS_HEADER_PARSER_H
#define GNSS_IO_RINEX_OBS_HEADER_PARSER_H

#include "io/rinex/obs/line_reader.h"
#include "io/rinex/obs/obs_types.h"

namespace gnssio {

// Membaca header sampai END OF HEADER (reader berhenti di baris record pertama).
// Throw RinexParseError bila line pertama bukan RINEX VERSION / TYPE atau EOF
// tercapai sebelum END OF HEADER.
ObsHeader parseHeader(LineReader& reader);

}  // namespace gnssio

#endif
