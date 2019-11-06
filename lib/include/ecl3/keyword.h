#ifndef ECL3_KEYWORD_H
#define ECL3_KEYWORD_H

#include <ecl3/common.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * @file keyword.h
 * The keyword module
 *
 * This module contains functions for working with the low-level concept of
 * *arrays* and *keywords*, and their peculiarities.
 *
 *     #include <ecl3/keyword.h>
 *
 * An array is a sequential structure of values, all of the same size, just
 * like arrays in C. However, when a Fortran program writes unformatted data to
 * file in a statement like this:
 *
 *     integer array(100)
 *     write(unit) array
 *
 * It also writes a head and tail immediately preceeding and following the
 * data. The head and tail are both 4-byte integers [1] that, in bytes, record
 * the size of the array. This detail allows seeking past arbitrary arrays as
 * units, from in both directions. What is actually found on disk after the
 * Fortran code would be:
 *
 *     | 400 | data ...... | 400 |
 *
 * As per the gnu fortran manual [1], the record byte marker is int32. We could
 * support 8-byte markers with either compile-time configuration or a run-time
 * switch, but as of now this is not implemented.
 *
 * [1] http://gcc.gnu.org/onlinedocs/gfortran/File-format-of-unformatted-sequential-files.html
 *
 * A *keyword* in ecl3 is the conceptual structure:
 *
 *     struct keyword {
 *         str  name;
 *         tag  type;
 *         int  len;
 *         byte data[];
 *     };
 *
 * Or, a more visual example, a tagged column vector:
 *
 *     +------------+
 *     | 'KEYWORDS' |
 *     | 'CHAR'     |
 *     | 5          |
 *     +------------+
 *     | 'TIME    ' |
 *     | 'FOPR    ' |
 *     | 'GOPR    ' |
 *     | 'GOPR    ' |
 *     | 'GOPR    ' |
 *     | 'GOPR    ' |
 *     +------------+
 *
 * The header and body of a keyword are written separately, which means they
 * both come with the Fortran block length metadata.
 *
 * Additionally, larger arrays are written in batches as several smaller arrays
 * in chunks of 1000 or 105 (for strings). These chunks are interspersed with
 * head and tail, but has no headers inbetween them.
 */

/**
 * Copy elements of type fmt from src to dst
 *
 * This is essentially a memcpy that's endian- and type aware, and translates
 * from on-disk representation of arrays to CPU-native representations. fmt
 * should be consistent with the type of dst, e.g. if fmt is ECL3_INTE, the
 * data in source should be int32, ECL3_DOUB should be double etc.
 *
 * **Returns**
 * \rst
 * ECL3_OK
 *    Success
 * ECL3_INVALID_ARGS
 *    fmt is unknown. Note that src and dst are not checked for NULL
 * ECL3_UNSUPPORTED
 *    fmt is a known and valid value, but is not yet supported
 * \endrst
 *
 * **Examples**
 *
 * Read an integer array from disk:
 *
 *     char head[4], tail[4];
 *     int32_t elems;
 *     int32_t* data;
 *     fread(head, sizeof(head), 1, fp);
 *     ecl3_get_native(&elems, head, ECL3_INTE, 1);
 *     elems /= sizeof(int32_t)
 *     data = malloc(head);
 *     fread(buffer, sizeof(int32_t), elems, fp);
 *     ecl3_get_native(data, buffer, ECL3_INTE, elems);
 *     fread(tail, sizeof(tail), 1, fp);
 */
ECL3_API
int ecl3_get_native(void* dst, const void* src, int fmt, size_t elems);

/**
 * Copy elements of type fmt from src to dst
 *
 * This is the host-to-disk inverse of ecl3_get_native
 *
 * @see ecl3_get_native
 */
ECL3_API
int ecl3_put_native(void* dst, const void* src, int fmt, size_t elems);


/**
 * Convert from in-file string representation to ecl3_typeids value
 *
 * This function maps the input string, as found in the file, to the
 * corresponding enum value in ecl3_typeids. The enum value is a lot more
 * practical to work with in C programs.
 *
 *
 * **Returns**
 *
 * ECL3_OK if str was any of the values in ecl3_typeids, and ECL3_INVALID_ARGS
 * otherwise.
 *
 * **Examples**
 *
 * Get the typeid for INTE:
 *
 *     int type;
 *     ecl3_typeid("INTE", &type);
 *     if (type == ECL3_INTE) puts("type was INTE");
 */
ECL3_API
int ecl3_typeid(const char* str, int* type);

/**
 * Get the size, in bytes, of a keyword type
 *
 * Takes a keyword type, as returned by ecl3_typeid, and yields the size
 * in bytes. Returns ECL3_OK on success, and ECL3_INVALID_ARGS if the type
 * argument is not an ecl3_typeid.
 *
 * This function is particularly useful for allocating buffer space for arrays,
 * without any branching.
 *
 * **Returns**
 *
 * \rst
 * ECL3_OK
 *    Success
 * ECL3_INVALID_ARGS
 *    fmt is unknown. Note that src and dst are not checked for NULL
 * ECL3_UNSUPPORTED
 *    fmt is a known and valid value, but is not yet supported
 * \endrst
 *
 * **Examples**
 *
 * Get the size of DOUB and alloc buffer:
 *
 *     int elems = 10;
 *     int elemsize;
 *     ecl3_type_size(ECL3_DOUB, &elemsize);
 *     void* buffer = malloc(elemsize * elems);
 */
ECL3_API
int ecl3_type_size(int type, int* size);

/**
 * String type name
 *
 * Get a static, null-terminated string representation of type. This is the
 * inverse function of ecl3_typeid, and in particular useful for printing.
 *
 * **Examples**
 *
 * type_name - typeid round trip:
 *
 *     int type;
 *     ecl3_typeid("INTE", &type)
 *     const char* name = ecl3_type_name(type);
 *     assert(strncmp("INTE", name, 4) == 0)
 */
ECL3_API
const char* ecl3_type_name(int type);

/**
 * Size of an array header
 *
 * The array header is the Fortran record:
 *
 *     STRUCTURE /KEYWORD/:
 *          CHARACTER (LEN=8) name
 *          INTEGER           len
 *          CHARACTER (LEN=4) type
 *
 * **Notes**
 *
 * This function is intended for a complete knowledge base, in particular for
 * allocating buffers and exposing it to scripting languages, but in most C and
 * C++ code it should be fine to use constant 16.
 */
ECL3_API
int ecl3_array_header_size();

/**
 * Parse keyword header
 *
 * The keyword header describes the array immediately following it: its name,
 * its type, and its size (in elements, not bytes!). This function parses the
 * keyword header.
 *
 * On disk, an array is typically laid out as such:
 *
 *     |head| KEYWORD COUNT TYPE |tail| |head| VALUE1 VALUE2 .. VALUEN |tail|
 *          + ------------------ +           + ----------------------- +
 *          | array header       |           | array body              |
 *
 * where |head| and |tail| are record length markers. This function is unaware
 * of the record markers, src should point to the start of the keyword header.
 *
 * This function faithfully outputs what's actually on disk. To obtain a more
 * practical representation for the array type, use the output type from this
 * function as input to ecl3_typeid.
 */
ECL3_API
int ecl3_array_header(const void* src, char* keyword, char* type, int* count);

#define ECL3_BLOCK_SIZE_NUMERIC 1000
#define ECL3_BLOCK_SIZE_STRING  105

/**
 * Get the specified block size for a given type
 *
 * @see ecl3_array_body for rationale and description
 */
ECL3_API
int ecl3_block_size(int type, int* size);

/**
 * Read chunks of array body items of type from src to dst
 *
 * Arrays are written to disk in chunks, i.e. large arrays are partitioned into
 * consecutive smaller arrays. To make matters worse, different data types are
 * blocked differently.
 *
 * @see ecl3_block_size to get the block size for a type.
 *
 * Consider a keyword [WOPR, INTE, 2800]. When written, it looks like this:
 *
 *     | HEADER | N0000 N0001 ... | N1000 N1001 ... | N2000 ... N2799 |
 *
 * Every | marks a Fortran head/tail.
 *
 * This function helps parse the bytes read from disk. It is designed to be
 * called multiple times on large arrays, until the entire keyword has been
 * read. It is expected that the caller updates dst/src/elems between
 * invocations, see the examples.
 *
 * @param dst output buffer
 * @param src input buffer, as read from disk
 * @param type from the keyword header, should one of enum ecl3_typeids
 * @param elems remaining elements in the array
 * @param chunk_size number of elements before the function pauses
 * @param count number of elements read
 *
 * **Returns**
 * \rst
 * ECL3_OK
 *      success
 * ECL3_INVALID_ARGS
 *      if the type is unknown
 * ECL3_UNSUPPORTED
 *      if the type is not yet supported
 * \endrst
 *
 * **Notes**
 *
 * The chunk_size value should typically be obtained by calling
 * ecl3_block_size. The manual specifies the size of these blocks dependent on
 * data types (the value output by ecl3_block_size), but this function impose
 * no such restriction. This is to enable recovery on broken-but-similar files
 * with weird blocking.
 *
 * **Examples**
 *
 * Read an arbitrary, blocked array from disk, and parse it:
 *
 *     type, remaining = parse_keyword_header();
 *     ecl3_block_size(type, &chunk_size);
 *     while (remaining > 0) {
 *         nbytes = read_f77_block_head()
 *         read(src, nbytes)
 *         ecl3_array_body(dst, src, type, remaining, block_size, &count);
 *         remaining -= count;
 *         dst += count;
 *     }
 */
ECL3_API
int ecl3_array_body(void* dst,
                    const void* src,
                    int type,
                    int elems,
                    int chunk_size,
                    int* count);

/*
 * The array data types in the manual. In the file format, these are specified
 * as 4-character strings, but it's useful to have a numerical representation
 * for C programs.
 *
 * Encode the type string itself (which occupies four bytes) as an int [1].
 * Users should not care about the numerical value, but it make some internal
 * operations easier. This also means the *length* of each value is contained
 * in the type tag itself.
 *
 * [1] This assumes 4-byte integers and will not work on other platforms.
 */
#define ECL3_MAKE_KWENUM(word) \
    ((int)((word)[0]) << 24 | \
     (int)((word)[1]) << 16 | \
     (int)((word)[2]) << 8  | \
     (int)((word)[3]))

enum ecl3_typeids {
    ECL3_INTE = ECL3_MAKE_KWENUM("INTE"),
    ECL3_REAL = ECL3_MAKE_KWENUM("REAL"),
    ECL3_DOUB = ECL3_MAKE_KWENUM("DOUB"),
    ECL3_CHAR = ECL3_MAKE_KWENUM("CHAR"),
    ECL3_MESS = ECL3_MAKE_KWENUM("MESS"),
    ECL3_LOGI = ECL3_MAKE_KWENUM("LOGI"),
    ECL3_X231 = ECL3_MAKE_KWENUM("X231"),

    ECL3_C001 = ECL3_MAKE_KWENUM("C001"),
    ECL3_C002 = ECL3_MAKE_KWENUM("C002"),
    ECL3_C003 = ECL3_MAKE_KWENUM("C003"),
    ECL3_C004 = ECL3_MAKE_KWENUM("C004"),
    ECL3_C005 = ECL3_MAKE_KWENUM("C005"),
    ECL3_C006 = ECL3_MAKE_KWENUM("C006"),
    ECL3_C007 = ECL3_MAKE_KWENUM("C007"),
    ECL3_C008 = ECL3_MAKE_KWENUM("C008"),
    ECL3_C009 = ECL3_MAKE_KWENUM("C009"),
    ECL3_C010 = ECL3_MAKE_KWENUM("C010"),
    ECL3_C011 = ECL3_MAKE_KWENUM("C011"),
    ECL3_C012 = ECL3_MAKE_KWENUM("C012"),
    ECL3_C013 = ECL3_MAKE_KWENUM("C013"),
    ECL3_C014 = ECL3_MAKE_KWENUM("C014"),
    ECL3_C015 = ECL3_MAKE_KWENUM("C015"),
    ECL3_C016 = ECL3_MAKE_KWENUM("C016"),
    ECL3_C017 = ECL3_MAKE_KWENUM("C017"),
    ECL3_C018 = ECL3_MAKE_KWENUM("C018"),
    ECL3_C019 = ECL3_MAKE_KWENUM("C019"),
    ECL3_C020 = ECL3_MAKE_KWENUM("C020"),
    ECL3_C021 = ECL3_MAKE_KWENUM("C021"),
    ECL3_C022 = ECL3_MAKE_KWENUM("C022"),
    ECL3_C023 = ECL3_MAKE_KWENUM("C023"),
    ECL3_C024 = ECL3_MAKE_KWENUM("C024"),
    ECL3_C025 = ECL3_MAKE_KWENUM("C025"),
    ECL3_C026 = ECL3_MAKE_KWENUM("C026"),
    ECL3_C027 = ECL3_MAKE_KWENUM("C027"),
    ECL3_C028 = ECL3_MAKE_KWENUM("C028"),
    ECL3_C029 = ECL3_MAKE_KWENUM("C029"),
    ECL3_C030 = ECL3_MAKE_KWENUM("C030"),
    ECL3_C031 = ECL3_MAKE_KWENUM("C031"),
    ECL3_C032 = ECL3_MAKE_KWENUM("C032"),
    ECL3_C033 = ECL3_MAKE_KWENUM("C033"),
    ECL3_C034 = ECL3_MAKE_KWENUM("C034"),
    ECL3_C035 = ECL3_MAKE_KWENUM("C035"),
    ECL3_C036 = ECL3_MAKE_KWENUM("C036"),
    ECL3_C037 = ECL3_MAKE_KWENUM("C037"),
    ECL3_C038 = ECL3_MAKE_KWENUM("C038"),
    ECL3_C039 = ECL3_MAKE_KWENUM("C039"),
    ECL3_C040 = ECL3_MAKE_KWENUM("C040"),
    ECL3_C041 = ECL3_MAKE_KWENUM("C041"),
    ECL3_C042 = ECL3_MAKE_KWENUM("C042"),
    ECL3_C043 = ECL3_MAKE_KWENUM("C043"),
    ECL3_C044 = ECL3_MAKE_KWENUM("C044"),
    ECL3_C045 = ECL3_MAKE_KWENUM("C045"),
    ECL3_C046 = ECL3_MAKE_KWENUM("C046"),
    ECL3_C047 = ECL3_MAKE_KWENUM("C047"),
    ECL3_C048 = ECL3_MAKE_KWENUM("C048"),
    ECL3_C049 = ECL3_MAKE_KWENUM("C049"),
    ECL3_C050 = ECL3_MAKE_KWENUM("C050"),
    ECL3_C051 = ECL3_MAKE_KWENUM("C051"),
    ECL3_C052 = ECL3_MAKE_KWENUM("C052"),
    ECL3_C053 = ECL3_MAKE_KWENUM("C053"),
    ECL3_C054 = ECL3_MAKE_KWENUM("C054"),
    ECL3_C055 = ECL3_MAKE_KWENUM("C055"),
    ECL3_C056 = ECL3_MAKE_KWENUM("C056"),
    ECL3_C057 = ECL3_MAKE_KWENUM("C057"),
    ECL3_C058 = ECL3_MAKE_KWENUM("C058"),
    ECL3_C059 = ECL3_MAKE_KWENUM("C059"),
    ECL3_C060 = ECL3_MAKE_KWENUM("C060"),
    ECL3_C061 = ECL3_MAKE_KWENUM("C061"),
    ECL3_C062 = ECL3_MAKE_KWENUM("C062"),
    ECL3_C063 = ECL3_MAKE_KWENUM("C063"),
    ECL3_C064 = ECL3_MAKE_KWENUM("C064"),
    ECL3_C065 = ECL3_MAKE_KWENUM("C065"),
    ECL3_C066 = ECL3_MAKE_KWENUM("C066"),
    ECL3_C067 = ECL3_MAKE_KWENUM("C067"),
    ECL3_C068 = ECL3_MAKE_KWENUM("C068"),
    ECL3_C069 = ECL3_MAKE_KWENUM("C069"),
    ECL3_C070 = ECL3_MAKE_KWENUM("C070"),
    ECL3_C071 = ECL3_MAKE_KWENUM("C071"),
    ECL3_C072 = ECL3_MAKE_KWENUM("C072"),
    ECL3_C073 = ECL3_MAKE_KWENUM("C073"),
    ECL3_C074 = ECL3_MAKE_KWENUM("C074"),
    ECL3_C075 = ECL3_MAKE_KWENUM("C075"),
    ECL3_C076 = ECL3_MAKE_KWENUM("C076"),
    ECL3_C077 = ECL3_MAKE_KWENUM("C077"),
    ECL3_C078 = ECL3_MAKE_KWENUM("C078"),
    ECL3_C079 = ECL3_MAKE_KWENUM("C079"),
    ECL3_C080 = ECL3_MAKE_KWENUM("C080"),
    ECL3_C081 = ECL3_MAKE_KWENUM("C081"),
    ECL3_C082 = ECL3_MAKE_KWENUM("C082"),
    ECL3_C083 = ECL3_MAKE_KWENUM("C083"),
    ECL3_C084 = ECL3_MAKE_KWENUM("C084"),
    ECL3_C085 = ECL3_MAKE_KWENUM("C085"),
    ECL3_C086 = ECL3_MAKE_KWENUM("C086"),
    ECL3_C087 = ECL3_MAKE_KWENUM("C087"),
    ECL3_C088 = ECL3_MAKE_KWENUM("C088"),
    ECL3_C089 = ECL3_MAKE_KWENUM("C089"),
    ECL3_C090 = ECL3_MAKE_KWENUM("C090"),
    ECL3_C091 = ECL3_MAKE_KWENUM("C091"),
    ECL3_C092 = ECL3_MAKE_KWENUM("C092"),
    ECL3_C093 = ECL3_MAKE_KWENUM("C093"),
    ECL3_C094 = ECL3_MAKE_KWENUM("C094"),
    ECL3_C095 = ECL3_MAKE_KWENUM("C095"),
    ECL3_C096 = ECL3_MAKE_KWENUM("C096"),
    ECL3_C097 = ECL3_MAKE_KWENUM("C097"),
    ECL3_C098 = ECL3_MAKE_KWENUM("C098"),
    ECL3_C099 = ECL3_MAKE_KWENUM("C099"),
};

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // ECL3_KEYWORD_H
