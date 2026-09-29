#include <cmath>
#include <cstdlib>
#include <iostream>

#include "io/rinex/obs/line_reader.h"

using namespace gnssio;

namespace {

template <typename T>
void expectEq(T got, T want, const char* what) {
    if (got != want) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        std::exit(EXIT_FAILURE);
    }
}

void expectNear(double got, double want, const char* what) {
    if (std::fabs(got - want) > 1e-9) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    // field(): kolom 1-based
    const std::string text = "0123456789ABCDEF";
    expectEq(LineReader::field(text, 1, 4), std::string("0123"), "field start=1");
    expectEq(LineReader::field(text, 11, 6), std::string("ABCDEF"), "field end of line");
    expectEq(LineReader::field(text, 12, 10), std::string("BCDEF"), "field clamped");
    expectEq(LineReader::field(text, 20, 3), std::string(""), "field past EOF");
    expectEq(LineReader::field(text, 0, 3), std::string(""), "field invalid start");

    // parseDouble: blank / tanpa digit -> nullopt
    expectEq(LineReader::parseDouble("").has_value(), false, "parseDouble empty");
    expectEq(LineReader::parseDouble("   ").has_value(), false, "parseDouble blank");
    expectNear(*LineReader::parseDouble("  12.50 "), 12.50, "parseDouble value");
    expectNear(*LineReader::parseDouble("-3.25"), -3.25, "parseDouble negative");

    // parseInt
    expectEq(LineReader::parseInt(" 42 ").has_value(), true, "parseInt has value");
    expectEq(*LineReader::parseInt(" 42 "), 42, "parseInt value");
    expectEq(LineReader::parseInt(" -- ").has_value(), false, "parseInt no digit");

    // nextLine(): trim \r, hitung nomor baris, deteksi EOF
    LineReader r("tests/data/rinex/obs/line_reader_sample.txt");
    expectEq(r.nextLine(), true, "line 1 present");
    expectEq(r.lineNo(), 1L, "line 1 number");
    expectEq(r.line(), std::string("alpha  12.50 beta"), "line 1 content");
    expectEq(r.nextLine(), true, "line 2 present");
    expectEq(LineReader::parseDouble(LineReader::field(r.line(), 8, 4)).has_value(), false,
             "line 2 text field not a number");
    expectEq(LineReader::parseInt(LineReader::field(r.line(), 1, 6)).has_value(), false,
             "line 2 word not an int");
    expectEq(r.nextLine(), true, "line 3 present");
    expectEq(r.nextLine(), false, "EOF detected");
    expectEq(r.eof(), true, "eof flag");

    return EXIT_SUCCESS;
}
