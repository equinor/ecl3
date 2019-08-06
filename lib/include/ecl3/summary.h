#ifndef ECL3_SUMMARY_H
#define ECL3_SUMMARY_H

#include <ecl3/common.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
 * The summary files are a set of snapshot of simulated values such as rates,
 * volume totals, and timestamps, and are organised in the following way:
 *
 * - A specification file (.SMSPEC) that describes the data layout
 * - A series of simulation steps in either a unified file (.UNSMRY) or
 *   separated by step (.Snnnn) where nnnn are consecutive numbers between 0000
 *   and 9999
 *
 * Briefly, the unified summary .UNSMRY is just a concatenated set of .Snnnn
 * files.
 *
 * In the summary file, data is recorded as report steps. In the non-unified
 * case, every .Snnnn file is a single report step. Any report step can have
 * one more timesteps, called ministeps. In documentation, these
 * report-ministep pairs will be denoted as report.mini, i.e. (1.2) describes
 * ministep 2 at report step 1. Report steps starts at 1, ministeps start at 0
 *
 * The specification is list of keywords with metadata describing how to
 * interpret the data in the summary files. It essentially describes a matrix -
 * consider a simulation with 2 wells, with summary for Well water cut (WWCT)
 * and Well oil production rate (WOPR):
 *
 *  Step | WWCT:W1 | WWCT:W2 | WOPR:W1 | WOPR:W2
 * ------+---------+---------+---------+--------
 *  1.0  | 0.2     | 0.4     | 1000.4  | 7231.8
 *  1.1  | 0.2     | 0.4     | 1020.1  | 4231.8
 *  2.0  | 0.3     | 0.3     | 1220.1  | 4231.7
 *  2.1  | 0.3     | 0.3     | 1220.1  | 2967.1
 *
 * The DIMENS keyword in the specification file specifies the paramter NLIST,
 * which are the number of columns in this matrix. For this example, NLIST = 4,
 * as step is derived from reportstep-ministep. The column headers (WWCT:W1) in
 * this example are constructed from the KEYWORDS and WGNAMES keywords in the
 * specification file, where WGNAME[n] corresponds to KEYWORD[n].
 *
 * In fact, most parameter in the specification file are index based. Consider
 * the three keywords KEYWORDS, WGNAMES, and UNITS in a specification file:
 *
 * KEYWORDS: [WWPR, WWPR, WOPR]
 * WGNAMES:  [W1, W2, W1]
 * UNITS:    [SM3/DAY, SM3/DAY, SM3/DAY]
 *
 * Formatted as a matrix:
 *
 * WWPR     | WWPR      | WOPR
 * W1       | W2        | W1
 * SM3/DAY  | SM3/DAY   | SM3/DAY
 *
 * ministep 1.0: [5.2, 1.3, 4.2]
 *
 * Means that the Well water production rate (WWPR) for the well W1 is 5.2
 * SM3/DAY at report step 1.0, i.e. the columns of the stacked keywords all
 * describe the same sample.
 *
 * Every report step starts with a SEQHDR keyword, followed by pairs of
 * MINISTEP-PARAMS keywords. The PARAMS should be NLIST long.
 */


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

/*
 *
 * The params_* functions are named as such because they deal with the
 * identifiers for vectors provided with the PARAMS keyword in the summary
 * files.
 *
 * Most keywords require additional data in order to uniquely identify what the
 * corresponding vector means. Well-related keywords (WOPR, WWCT etc) all
 * depend on a corresponding WGNAMES entry, whereas FIELD related keywords are
 * completely specified.
 *
 * This function implements the ruleset, and can be used to determine if a
 * vector depend on a data type to be fully specified.
 *
 * To make matters worse, summary specifications often contain columns whose
 * values are all garbage.  These are identified by a rubbish entry in any of
 * the additional specifiers. That means the otherwise valid keyword "WWCT    "
 * maybe contain all garbage, if the corresponding WGNAMES is ":+:+:+:+" or
 * blank.
 *
 * This functions returns non-zero a the keyword requires the additional data
 * type.
 *
 * When a vector is partially identified by the given id, the number of
 * identifiers required to uniquely identify the vector is returned. For
 * example, a WOPR entry is identified by WOPR + well-name, and
 * ecl3_params_identifies("WOPR    ", "WGNAMES ") return 1. Completions are
 * identified by well-name and NUMS, so both ("WGNAME ", "COFR    ") and
 * ("NUMS ", "COFR    ") return 2. This is to support iterating over possible
 * identifiers and terminate when the vector is fully specified.
 *
 * Notes
 * -----
 * This function currently implements what is expected from eclipse. Intersect
 * and petrel sometimes use the NAMES for varchar well names, rather than
 * WGNAMES. This function is (currently) not aware, and will say that only
 * WGNAMES will add to specification.
 *
 * Examples
 * --------
 * NLIST = 5
 *
 * KEYWORD | WGNAME
 * --------+---------
 * WWCT    | WELL1
 * WWCT    | WELL2
 * WWCT    | :+:+:+:+
 * WOPR    | WELL1
 * WOPR    | WELL1
 *
 * All PARAMS vector will have 5 values, but only 4 sensical.
 *
 * ecl3_params_identifies("WGNAMES ", "WWCT    ") -> true
 * ecl3_params_identifies("WGNAMES ", "GOPR    ") -> true
 * ecl3_params_identifies("NUMS    ", "GOPR    ") -> false
 * ecl3_params_identifies("WGNAMES ", "YEARS   ") -> false
 *
 * This function is intended as a predicate to check whether or not a sample
 * should be included in reports, computation or other data structures, or if
 * the vector is nonsensical.
 *
 * The function ecl3_params_partial_identifiers return a list of identifiers
 * that might make ecl3_params_iodentified_by return non-zero. For maximum
 * future compatibility for arbitrary input, consider this example:
 *
 * const char* keyword = "WOPR    ";
 * const char** id = ecl3_params_partial_identifiers();
 * while (*id) {
 *     if (ecl3_params_identifies(*id, keyword))
 *         printf("%s depends on %s\n", *id, keyword);
 *     ++id;
 * }
 */
ECL3_API
int ecl3_params_identifies(const char* id, const char* keyword);

/*
 * Obtain a list of the keywords that contribute to specialisation vectors, and
 * would make ecl3_params_identifies return non-zero.
 *
 * The list is terminated by a NULL.
 */
ECL3_API
const char** ecl3_params_partial_identifiers(void);

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

#endif //ECL3_SUMMARY_H
