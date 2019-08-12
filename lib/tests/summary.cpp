#include <string>

#include <catch2/catch.hpp>

#include <ecl3/summary.h>

TEST_CASE("manual-listed exceptions don't require type") {
    /*
     * Test the known exceptions, i.e. names that *could* be recognised as
     * another class (group, well etc.) that otherwise should accept the
     * parameter, and verify that it is indeed an exception
     */
    CHECK(!ecl3_params_identifies("WGNAMES ", "GMCTP   "));
    CHECK(!ecl3_params_identifies("WGNAMES ", "GMCTG   "));
    CHECK(!ecl3_params_identifies("WGNAMES ", "GMCTW   "));
    CHECK(!ecl3_params_identifies("WGNAMES ", "GMCPL   "));

    CHECK(!ecl3_params_identifies("WGNAMES ", "WMCTL   "));
    CHECK(!ecl3_params_identifies("WGNAMES ", "WNEWTON "));

    CHECK(!ecl3_params_identifies("WGNAMES ", "NEWTON  "));
    CHECK(!ecl3_params_identifies("WGNAMES ", "NAIMFRAC"));
    CHECK(!ecl3_params_identifies("WGNAMES ", "NLINEARS"));
    CHECK(!ecl3_params_identifies("WGNAMES ", "NLINSMIN"));
    CHECK(!ecl3_params_identifies("WGNAMES ", "NLINSMAX"));

    CHECK(!ecl3_params_identifies("WGNAMES ", "STEPTYPE"));
    CHECK(!ecl3_params_identifies("WGNAMES ", "SOIL    "));
    CHECK(!ecl3_params_identifies("WGNAMES ", "SGAS    "));
    CHECK(!ecl3_params_identifies("WGNAMES ", "SWAT    "));
    CHECK(!ecl3_params_identifies("NUMS    ", "STEPTYPE"));
    CHECK(!ecl3_params_identifies("NUMS    ", "SOIL    "));
    CHECK(!ecl3_params_identifies("NUMS    ", "SGAS    "));
    CHECK(!ecl3_params_identifies("NUMS    ", "SWAT    "));
}

TEST_CASE("well types require WGNAMES and nothing else") {
    CHECK(ecl3_params_identifies("WGNAMES ", "WOPR    ") == 1);
    CHECK(ecl3_params_identifies("WGNAMES ", "WWCT    ") == 1);

    const auto** kw = ecl3_params_partial_identifiers();
    while (*kw) {
        const auto key = std::string(*kw++);
        if (key == "WGNAMES ") continue;
        INFO("key = " << key);
        CHECK(!ecl3_params_identifies(key.c_str(), "WOPR    "));
        CHECK(!ecl3_params_identifies(key.c_str(), "WWPR    "));
    }
}

TEST_CASE("group types require WGNAMES and  nothing else") {
    CHECK(ecl3_params_identifies("WGNAMES ", "GOPR    ") == 1);
    CHECK(ecl3_params_identifies("WGNAMES ", "GWCT    ") == 1);

    const auto** kw = ecl3_params_partial_identifiers();
    while (*kw) {
        const auto key = std::string(*kw++);
        if (key == "WGNAMES ") continue;
        INFO("key = " << key);
        CHECK(!ecl3_params_identifies(key.c_str(), "GOPR    "));
        CHECK(!ecl3_params_identifies(key.c_str(), "GWPR    "));
    }
}
