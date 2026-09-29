#ifndef GNSS_IO_RINEX_OBS_LINE_READER_H
#define GNSS_IO_RINEX_OBS_LINE_READER_H

#include <fstream>
#include <iosfwd>
#include <optional>
#include <string>

namespace gnssio {

// Baris-per-baris reader dengan accessor fixed-width 1-based (kolom RINEX).
class LineReader {
public:
    explicit LineReader(const std::string& path);

    // false saat EOF. Melepas '\r' (file CRLF).
    bool nextLine();

    const std::string& line() const { return line_; }
    long lineNo() const { return lineNo_; }
    bool eof() const { return eof_; }

    // Kolom 1-based [start, start+len); di luar panjang baris = string kosong.
    static std::string field(const std::string& line, int start, int len);

    static std::optional<double> parseDouble(const std::string& text);
    static std::optional<int> parseInt(const std::string& text);

private:
    std::ifstream in_;
    std::string line_;
    long lineNo_ = 0;
    bool eof_ = false;
};

}  // namespace gnssio

#endif
