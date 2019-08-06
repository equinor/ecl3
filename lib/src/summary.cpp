#include <ciso646>
#include <cstring>
#include <string>

#include <ecl3/summary.h>

const char** ecl3_smspec_keywords(void) {
    static const char* kws[] = {
        "INTEHEAD",
        "RESTART ",
        "DIMENS  ",
        "KEYWORDS",
        "WGNAMES ",
        "NAMES   ",
        "NUMS    ",
        "LGRS    ",
        "NUMLX   ",
        "NUMLY   ",
        "NUMLZ   ",
        "LENGTHS ",
        "LENUNITS",
        "MEASRMNT",
        "UNITS   ",
        "STARTDAT",
        "LGRNAMES",
        "LGRVEC  ",
        "LGRTIMES",
        "RUNTIMEI",
        "RUNTIMED",
        "STEPRESN",
        "XCOORD  ",
        "YCOORD  ",
        "TIMESTMP",
        nullptr,
    };

    return kws;
}

const char* ecl3_unit_system_name(int sys) {
    switch (sys) {
        case ECL3_METRIC:   return "METRIC";
        case ECL3_FIELD:    return "FIELD";
        case ECL3_LAB:      return "LAB";
        case ECL3_PVTM:     return "PVT-M";

        default:            return nullptr;
    }
}

const char* ecl3_simulatorid_name(int id) {
    switch (id) {
        case ECL3_ECLIPSE100:           return "ECLIPSE 100";
        case ECL3_ECLIPSE300:           return "ECLIPSE 300";
        case ECL3_ECLIPSE300_THERMAL:   return "ECLIPSE 300 (thermal option)";
        case ECL3_INTERSECT:            return "INTERSECT";
        case ECL3_FRONTSIM:             return "FrontSim";

        default:                        return nullptr;
    }
}

const char** ecl3_params_partial_identifiers(void) {
    static const char* kws[] = {
        "WGNAMES ",
        "NUMS    ",
        "LGRS    ",
        "NUMLX   ",
        "NUMLY   ",
        "NUMLZ   ",
        nullptr,
    };

    return kws;
}

int ecl3_params_identifies(const char* type, const char* keyword) {
    static constexpr const auto WGNAMES = "WGNAMES ";
    static constexpr const auto NUMS    = "NUMS    ";
    static constexpr const auto LGRS    = "LGRS    ";
    static constexpr const auto NUMLX   = "NUMLX   ";
    static constexpr const auto NUMLY   = "NUMLY   ";
    static constexpr const auto NUMLZ   = "NUMLZ   ";

    const auto id = std::string(type, 8);

    switch (keyword[0]) {
        /* Aquifer data */
        case 'A':
            return id == NUMS ? 1 : 0;

        /* Block data */
        case 'B':
            return id == NUMS ? 1 : 0;

        /* Completion or connection data */
        case 'C':
            return (id == WGNAMES or id == NUMS) ? 2 : 0;

        /* Group data */
        case 'G':
            if (keyword[1] == 'M') return 0;
            return id == WGNAMES ? 1 : 0;

        /* Well data */
        case 'W':
            /* 
             * The {F,G,W}M mnemonics are reserved for other uses than
             * well/group, and are not parametrised
             */
            if (keyword[1] == 'M') return 0;

            // of course, WNEWTON is also a thing
            if (std::strncmp(keyword, "WNEWTON ", 8) == 0) return 0;

            return id == WGNAMES ? 1 : 0;

        case 'P':
            return id == WGNAMES ? 1 : 0;

        case 'R':
            return id == NUMS ? 1 : 0;

        case 'L':
        case 'N':
        case 'S':
            /* require more information - handle outside the switch */
            break;

        default:
            return 0;
    }

    if (keyword[0] == 'L') {
        switch (keyword[1]) {
            case 'B':
                return(id == LGRS
                    or id == NUMLX
                    or id == NUMLY
                    or id == NUMLZ
                    ) ? 4 : 0;
                ;

            case 'C':
                return(id == LGRS
                    or id == WGNAMES
                    or id == NUMLX
                    or id == NUMLY
                    or id == NUMLZ
                ) ? 4 : 0
                ;

            case 'W':
                return(id == LGRS
                    or id == WGNAMES
                ) ? 2 : 0
                ;
        }

        return 0;
    }

    const auto key = std::string(keyword, 8);

    if (key[0] == 'N') {
          if (key == "NEWTON  ") return 0;
          if (key == "NAIMFRAC") return 0;
          if (key == "NLINEARS") return 0;
          if (key == "NLINSMIN") return 0;
          if (key == "NLINSMAX") return 0;
          return id == WGNAMES ? 1 : 0;
    }

    if (key[0] == 'S') {
        if (key == "STEPTYPE") return 0;
        const auto prefix = key.substr(0, 4);
        if (prefix == "SGAS") return 0;
        if (prefix == "SOIL") return 0;
        if (prefix == "SWAT") return 0;

        return (id == WGNAMES or id == NUMS) ? 2 : 0;
    }

    return 0;
}
