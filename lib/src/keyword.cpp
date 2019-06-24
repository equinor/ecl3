#include <cctype>
#include <ciso646>
#include <cstdint>
#include <cstring>

#include <endianness/endianness.h>

#include <ecl3/f77.h>
#include <ecl3/keyword.h>

int ecl3_array_header(const void* source, char* kw, char* type, int* count) {
    const auto* src = reinterpret_cast< const char* >(source);

    std::memcpy(kw, src, 8);
    std::uint32_t tmp;
    ecl3_get_native(&tmp, src + 8, ECL3_INTE, 1);
    std::memcpy(type, src + 12, 4);
    *count = tmp;
    return ECL3_OK;
}

int ecl3_keyword_type(const char* str, int* type) {
    static_assert(
        sizeof(int) == sizeof(std::int32_t),
        "This function relies on 4-byte int. "
        "Implement a fallback for your architecture"
    );

    std::int32_t kw;
    std::memcpy(&kw, str, sizeof(kw));
    kw = be32toh(kw);
    /*
     * The enum values correspond to the 4 bytes that make up the ascii
     * representation of the type specifier. This means the enum now carries
     * both size information (by masking the lower 2 bytes of C0NN, implicitly
     * otherwise) and covers the total set of accepted types.
     *
     * This makes for a very fast & simple implementation, and makes the
     * knowledge easy to carry around
     */
    switch (kw) {
        case ECL3_INTE:
        case ECL3_REAL:
        case ECL3_DOUB:
        case ECL3_CHAR:
        case ECL3_MESS:
        case ECL3_LOGI:
        case ECL3_X231:

        case ECL3_C001:
        case ECL3_C002:
        case ECL3_C003:
        case ECL3_C004:
        case ECL3_C005:
        case ECL3_C006:
        case ECL3_C007:
        case ECL3_C008:
        case ECL3_C009:
        case ECL3_C010:
        case ECL3_C011:
        case ECL3_C012:
        case ECL3_C013:
        case ECL3_C014:
        case ECL3_C015:
        case ECL3_C016:
        case ECL3_C017:
        case ECL3_C018:
        case ECL3_C019:
        case ECL3_C020:
        case ECL3_C021:
        case ECL3_C022:
        case ECL3_C023:
        case ECL3_C024:
        case ECL3_C025:
        case ECL3_C026:
        case ECL3_C027:
        case ECL3_C028:
        case ECL3_C029:
        case ECL3_C030:
        case ECL3_C031:
        case ECL3_C032:
        case ECL3_C033:
        case ECL3_C034:
        case ECL3_C035:
        case ECL3_C036:
        case ECL3_C037:
        case ECL3_C038:
        case ECL3_C039:
        case ECL3_C040:
        case ECL3_C041:
        case ECL3_C042:
        case ECL3_C043:
        case ECL3_C044:
        case ECL3_C045:
        case ECL3_C046:
        case ECL3_C047:
        case ECL3_C048:
        case ECL3_C049:
        case ECL3_C050:
        case ECL3_C051:
        case ECL3_C052:
        case ECL3_C053:
        case ECL3_C054:
        case ECL3_C055:
        case ECL3_C056:
        case ECL3_C057:
        case ECL3_C058:
        case ECL3_C059:
        case ECL3_C060:
        case ECL3_C061:
        case ECL3_C062:
        case ECL3_C063:
        case ECL3_C064:
        case ECL3_C065:
        case ECL3_C066:
        case ECL3_C067:
        case ECL3_C068:
        case ECL3_C069:
        case ECL3_C070:
        case ECL3_C071:
        case ECL3_C072:
        case ECL3_C073:
        case ECL3_C074:
        case ECL3_C075:
        case ECL3_C076:
        case ECL3_C077:
        case ECL3_C078:
        case ECL3_C079:
        case ECL3_C080:
        case ECL3_C081:
        case ECL3_C082:
        case ECL3_C083:
        case ECL3_C084:
        case ECL3_C085:
        case ECL3_C086:
        case ECL3_C087:
        case ECL3_C088:
        case ECL3_C089:
        case ECL3_C090:
        case ECL3_C091:
        case ECL3_C092:
        case ECL3_C093:
        case ECL3_C094:
        case ECL3_C095:
        case ECL3_C096:
        case ECL3_C097:
        case ECL3_C098:
        case ECL3_C099:
            *type = kw;
            return ECL3_OK;

        default:
            return ECL3_INVALID_ARGS;
    }
}
