#include "io/rinex/obs/line_reader.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace gnssio {

LineReader::LineReader(const std::string& path) : in_(path) {}

bool LineReader::nextLine() {
    if (!std::getline(in_, line_)) {
        eof_ = true;
        return false;
    }
    ++lineNo_;
    while (!line_.empty() && (line_.back() == '\r' || line_.back() == '\n')) {
        line_.pop_back();
    }
    return true;
}

std::string LineReader::field(const std::string& line, int start, int len) {
    if (start < 1 || len <= 0) return {};
    const int idx = start - 1;
    if (idx >= static_cast<int>(line.size())) return {};
    const int end = std::min(idx + len, static_cast<int>(line.size()));
    return line.substr(idx, end - idx);
}

std::optional<double> LineReader::parseDouble(const std::string& text) {
    if (text.empty()) return std::nullopt;
    bool hasDigit = false;
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
    }
    if (!hasDigit) return std::nullopt;
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (end == text.c_str()) return std::nullopt;
    return value;
}

std::optional<int> LineReader::parseInt(const std::string& text) {
    if (text.empty()) return std::nullopt;
    bool hasDigit = false;
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
    }
    if (!hasDigit) return std::nullopt;
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str()) return std::nullopt;
    return static_cast<int>(value);
}

}  // namespace gnssio
