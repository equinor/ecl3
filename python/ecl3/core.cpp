#include <array>
#include <ciso646>
#include <fstream>
#include <sstream>
#include <string>
#include <string>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <ecl3/keyword.h>
#include <ecl3/summary.h>
#include <ecl3/io.hpp>

namespace py = pybind11;
using namespace py::literals;

namespace {

struct array {
    char keyword[9] = {};
    char type[5] = {};
    int count;
    py::list values;
};

struct stream : std::ifstream {
    stream(const std::string& path) :
        std::ifstream(path, std::ios::binary | std::ios::in)
    {
        if (!this->is_open()) {
            const auto msg = "could not open file '" + path + "'";
            throw std::invalid_argument(msg);
        }

        auto errors = std::ios::failbit | std::ios::badbit | std::ios::eofbit;
        this->exceptions(errors);
    }

    std::vector< array > keywords();

};

array getheader(std::ifstream& fs) {
    array a;
    char buffer[16];
    fs.read(buffer, sizeof(buffer));

    auto err = ecl3_array_header(buffer, a.keyword, a.type, &a.count);

    if (err) {
        auto msg = std::string("invalid argument to ecl3_array_header: ");
        msg.insert(msg.end(), buffer, buffer + sizeof(buffer));
        throw std::invalid_argument(msg);
    }

    return a;
}

template < typename T >
void extend(py::list& l, const char* src, int n, T tmp) {
    for (int i = 0; i < n; ++i) {
        std::memcpy(&tmp, src + (i * sizeof(T)), sizeof(T));
        l.append(tmp);
    }
}

void extend_char(py::list& l, const char* src, int n) {
    char tmp[8];
    for (int i = 0; i < n; ++i) {
        std::memcpy(tmp, src + (i * sizeof(tmp)), sizeof(tmp));
        l.append(py::str(tmp, sizeof(tmp)));
    }
}

void extend(py::list& l, const char* src, ecl3_typeids type, int count) {
    switch (type) {
        case ECL3_INTE:
            extend(l, src, count, std::int32_t(0));
            break;

        case ECL3_REAL:
            extend(l, src, count, float(0));
            break;

        case ECL3_DOUB:
            extend(l, src, count, double(0));
            break;

        case ECL3_CHAR:
            extend_char(l, src, count);
            break;

        default:
            throw std::invalid_argument("unknown type");
    }
}

std::vector< array > stream::keywords() {
    std::vector< array > kws;

    std::array< char, sizeof(std::int32_t) > head;
    std::array< char, sizeof(std::int32_t) > tail;

    auto buffer = std::vector< char >();

    std::int32_t ix;
    float fx;
    double dx;
    char str[9] = "        ";

    while (true) {
        try {
            this->read(head.data(), sizeof(head));
        } catch (std::ios::failure&) {
            if (this->eof()) return kws;
            /* some error is set - propagate exception */
        }
        auto kw = getheader(*this);
        this->read(tail.data(), sizeof(tail));
        if (head != tail) {
            std::int32_t h;
            std::int32_t t;
            ecl3_get_native(&h, head.data(), ECL3_INTE, 1);
            ecl3_get_native(&t, tail.data(), ECL3_INTE, 1);

            std::stringstream ss;
            ss << "array header: "
               << "head (" << h << ")"
               << " != tail (" << t << ")"
            ;

            throw std::runtime_error(ss.str());
        }

        int err;
        int type;
        int size;
        int blocksize;
        err = ecl3_typeid(kw.type, &type);
        if (err) {
            auto msg = std::string("unknown type: '");
            msg += kw.type;
            msg += "'";
            throw std::invalid_argument(msg);
        }
        ecl3_type_size(type, &size);
        ecl3_block_size(type, &blocksize);
        int remaining = kw.count;

        while (remaining > 0) {
            this->read(head.data(), sizeof(head));
            std::int32_t elems;
            ecl3_get_native(&elems, head.data(), ECL3_INTE, 1);

            buffer.resize(elems);
            this->read(buffer.data(), elems);

            this->read(tail.data(), sizeof(tail));
            if (head != tail) {
                std::int32_t h;
                std::int32_t t;
                ecl3_get_native(&h, head.data(), ECL3_INTE, 1);
                ecl3_get_native(&t, tail.data(), ECL3_INTE, 1);

                std::stringstream ss;
                ss << "array body: "
                   << "head (" << h << ")"
                   << " != tail (" << t << ")"
                ;

                throw std::runtime_error(ss.str());
            }

            int count;
            ecl3_array_body(
                buffer.data(),
                buffer.data(),
                type,
                remaining,
                blocksize,
                &count
            );
            remaining -= count;
            extend(kw.values, buffer.data(), ecl3_typeids(type), count);
        }

        kws.push_back(kw);
    }

    return kws;
}

py::list spec_keywords() {
    py::list xs;
    auto kw = ecl3_smspec_keywords();
    while (*kw)
        xs.append(py::str(*kw++));
    return xs;
}

bool is_void(const std::string& str) noexcept (true) {
    return str == ":+:+:+:+" or str == "        ";
}

bool is_void(std::int32_t i) noexcept (true) {
    return i < 0;
}

py::tuple columns(
    py::list keywords,
    py::list wgnames,
    py::list nums,
    py::list lgrs,
    py::list numlx,
    py::list numly,
    py::list numlz,
    const std::string& dtype_separator)
{
    /*
     * Figure out the fully qualified column names for a summary file.
     *
     * See the docs for ecl3_params_identifies for more details. In short, a
     * lot of data types are well or cell specific, and the keyword alone is
     * not enough to read anything meaningful out of the corresponding vector.
     * Determine these names by scanning through keywords, wgnames, nums etc.
     *
     * Sometimes, invalid or known void entries are used to signal that a
     * column is filled with garbage, most commonly :+:+:+:+, and these columns
     * are discarded.
     */
    assert(!keywords.empty());
    assert(!wgnames.empty());
    assert(!nums.empty());

    assert(keywords.size() == wgnames.size());
    assert(keywords.size() == nums.size());

    assert(lgrs.empty()  or keywords.size() == lgrs.size());
    assert(numlx.empty() or keywords.size() == numlx.size());
    assert(numly.empty() or keywords.size() == numly.size());
    assert(numlz.empty() or keywords.size() == numlz.size());

    static constexpr const auto WGNAMES = "WGNAMES ";
    static constexpr const auto NUMS    = "NUMS    ";
    static constexpr const auto LGRS    = "LGRS    ";
    static constexpr const auto NUMLX   = "NUMLX   ";
    static constexpr const auto NUMLY   = "NUMLY   ";
    static constexpr const auto NUMLZ   = "NUMLZ   ";

    auto names = std::vector< std::string >();
    auto pos = std::vector< int >();

    for (std::size_t i = 0; i < keywords.size(); ++i) {
        const auto kw = keywords[i].cast< std::string >();

        std::stringstream id;
        id << kw;
        if (ecl3_params_identifies(WGNAMES, kw.c_str())) {
            const auto wgname = wgnames[i].cast< std::string >();
            if (is_void(wgname)) continue;
            id << dtype_separator << wgname;
        }

        if (ecl3_params_identifies(NUMS, kw.c_str())) {
            const auto num = nums[i].cast< std::int32_t >();
            if (is_void(num)) continue;
            id << dtype_separator << num;
        }

        if (not lgrs.empty() and ecl3_params_identifies(LGRS, kw.c_str())) {
            const auto lgr = lgrs[i].cast< std::string >();
            if (is_void(lgr)) continue;
            id << dtype_separator << lgr;
        }

        if (not numlx.empty() and ecl3_params_identifies(NUMLX, kw.c_str())) {
            const auto nx = numlx[i].cast< std::int32_t >();
            if (is_void(nx)) continue;
            id << dtype_separator << nx;
        }

        if (not numly.empty() and ecl3_params_identifies(NUMLY, kw.c_str())) {
            const auto ny = numly[i].cast< std::int32_t >();
            if (is_void(ny)) continue;
            id << dtype_separator << ny;
        }

        if (not numlz.empty() and ecl3_params_identifies(NUMLZ, kw.c_str())) {
            const auto nz = numlz[i].cast< std::int32_t >();
            if (is_void(nz)) continue;
            id << dtype_separator << nz;
        }

        const auto name = id.str();
        // skip if this is a duplicate somehow
        if (std::find(names.begin(), names.end(), name) != names.end())
            continue;

        names.push_back(name);
        pos.push_back(int(i));
    }

    return py::make_tuple(names, pos);
}

template < unsigned long Len >
void expect(const std::string& expected, const std::array< char, Len >& str) {
    if (expected.size() != Len
        or not std::equal(str.begin(), str.end(), expected.begin())) {

        const auto stdstr = std::string(str.data(), str.size());
        const auto msg = "expected " + expected + ", was " + stdstr;
        throw std::runtime_error(msg);
    }
}

bool end_report_step(const ecl3::raw_array& kw) noexcept (true) {
    return std::equal(kw.keyword.begin(), kw.keyword.end(), "SEQHDR  ");
}

py::object readall(
    const std::string& fname,
    py::object alloc,
    int rowsize,
    const std::vector< int >& pos) {

    int rows = 0;
    std::int32_t report_step = 1;
    ecl3::stream_reader< std::ifstream > stream(fname);
    auto buffer = std::vector< unsigned char >(64 * rowsize);

    const auto& seqhdr = stream.next();
    if (seqhdr.empty()) {
        // No records at all, warrants an error for now
        const auto msg = "no initial SEQHDR found, file seems broken";
        throw std::runtime_error(msg);
    }

    expect("SEQHDR  ", seqhdr.keyword);
    expect("INTE", seqhdr.type);

    while (true) {
        if ((rows + 1) * rowsize >= buffer.size()) {
            buffer.resize(buffer.size() * 2);
        }

        const auto& ministep = stream.next();
        if (ministep.empty()) {
            // if this is empty, we're at an acceptable place for an eof
            // this won't happen after end_report_step is true, because it
            // already checks empty()
            break;
        }

        if (end_report_step(ministep)) {
            // read the next record, which now should not be empty (it should
            // be MINISTEP), then unget so the next iteration's MINISTEP gets
            // this record
            if (stream.next().empty()) {
                const auto msg = "unexpected end-of-file, expected MINISTEP";
                throw std::runtime_error(msg);
            }

            stream.unget();
            continue;
        }

        expect("MINISTEP", ministep.keyword);
        expect("INTE", ministep.type);

        auto* dst = buffer.data() + rows * rowsize;
        std::memcpy(dst += 0, &report_step, sizeof(report_step));
        std::memcpy(dst += 4, ministep.body.data(), sizeof(std::int32_t));

        // this invalidates all references to ministep
        const auto& params = stream.next();
        if (params.empty()) {
            const auto msg = "unexpected end-of-file, expected PARAMS";
            throw std::runtime_error(msg);
        }
        expect("PARAMS  ", params.keyword);
        const auto* src = params.body.data();
        // write_item
        for (auto p : pos) {
            static_assert(
                sizeof(float) == 4,
                "the pointer arithmetic relies on 4-byte float"
            );
            const auto src_off = p * sizeof(float);
            std::memcpy(dst += 4, src + src_off, sizeof(float));
        }

        ++rows;
    }

    py::buffer arr = alloc(rows);
    auto view = arr.request(true);
    if (view.size * view.itemsize != rowsize * rows) {
        std::stringstream msg;
        msg << "internal alloc function size error, was "
            << view.size * view.itemsize
            << " (" << view.size << " rows)"
            << ", expected "
            << rowsize * rows
            << " (" << rows << " rows)"
        ;
        throw std::invalid_argument(msg.str());
    }
    std::memcpy(view.ptr, buffer.data(), rows * rowsize);
    return arr;
}

}

PYBIND11_MODULE(core, m) {
    py::class_<stream>(m, "stream")
        .def(py::init<const std::string&>())
        .def("keywords", &stream::keywords)
    ;

    py::class_<array>(m, "array")
        .def("__repr__", [](const array& x) {
            std::stringstream ss;

            auto kw = std::string(x.keyword);
            auto type = std::string(x.type);

            ss << "{ " << kw << ", " << type << ": [ ";

            for (const auto& val : x.values) {
                ss << py::str(val).cast< std::string >() << " ";
            }

            ss << "] }";
            return ss.str();
        })
        .def_readonly("keyword", &array::keyword)
        .def_readonly("values", &array::values)
    ;

    m.def("spec_keywords", spec_keywords);
    m.def("unitsystem",  ecl3_unit_system_name);
    m.def("simulatorid", ecl3_simulatorid_name);
    m.def("columns", columns);
    m.def("readall", readall);
}
