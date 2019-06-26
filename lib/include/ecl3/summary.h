#ifndef ECL3_SUMMARY_h
#define ECL3_SUMMARY_h

#include <ecl3/common.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
 * Obtain a list of the known keywords in the summary specification (.SMSPEC)
 * file.
 *
 * This centralises the known keywords. The intended use case is for users to
 * be able to figure out if all keywords in a file are known summary
 * specification keywords. All strings are NULL terminated.
 *
 * The list is terminated by a NULL.
 */
ECL3_API
const char** ecl3_smspec_keywords(void);

/*
 * The INTEHEAD (optional) keyword specifies the unit system and the simulation
 * program used to produce a summary. It is an array with two values:
 *
 * INTEHEAD = [ecl3_unit_systems, ecl3_simulator_identifiers]
 *
 * The functions ecl3_unit_system_name and ecl3_simulatorid_name map between an
 * identifier and the corresponding NULL-terminated string name.
 */
ECL3_API
const char* ecl3_unit_system_name(int);

ECL3_API
const char* ecl3_simulatorid_name(int);

enum ecl3_unit_systems {
    ECL3_METRIC = 1,
    ECL3_FIELD  = 2,
    ECL3_LAB    = 3,
    ECL3_PVTM   = 4,
};

enum ecl3_simulator_identifiers {
    ECL3_ECLIPSE100         = 100,
    ECL3_ECLIPSE300         = 300,
    ECL3_ECLIPSE300_THERMAL = 500,
    ECL3_INTERSECT          = 700,
    ECL3_FRONTSIM           = 800,
};

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //ECL3_SUMMARY_h
