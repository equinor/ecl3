#ifndef ECL3_IO_HPP
#define ECL3_IO_HPP

#include <array>
#include <string>
#include <vector>

#include <ecl3/keyword.h>

namespace ecl3 {

struct head_tail_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct header_error : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct invalid_type : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct raw_array {
    /*
     * These are really strings, but the ecl3 C API writes data through
     * pointers, and string.c_str and string.data return const char* up to
     * C++17. Since they're fixed size anyway this is ok, in particular on
     * systems without SSO, but it makes comparison more awkward.
     */
    std::array< char, 8 > keyword;
    std::array< char, 4 > type;
    int count = 0;
    std::vector< unsigned char > body;

    bool empty() const { return this->count == -1; }
};

/*
 * A wrapper-type for iostream like interfaces to stream arrays. Manages
 * buffers internally, and should be treated like a black-box readline() until
 * the returned array's empty method returns true. Because of this interface,
 * the reader works on streams and pipes, and can only read forward.
 *
 * Most challenges arise from it being really awkward to know ahead-of-time how
 * many arrays there are. The interface is rough, but is not meant to be used
 * by end-users - it's provided only for implementors convenience and is *not*
 * considered a part of the stable ecl3 interface. However, it's quite useful
 * for developing applications and function that loops through all arrays in a
 * file once.
 *
 * The Stream has to be API compatible, including some typedefs and enums, with
 * the C++ iostreams.
 *
 * Reading from an array when empty is true is undefined.
 *
 * Example
 * -------
 *  stream_reader< std::ifstream > fs(path);
 *  while (true) {
 *      const auto& array = fs.next();
 *      if (array.empty()) break;
 *  }
 */
template < typename Stream >
class stream_reader : Stream {
public:
    explicit stream_reader(const std::string& path);

    /*
     * Read the next array. This function updates the the array in-place, and
     * invalidates all pointers and references to previously-read arrays.
     */
    const raw_array& next();
    /*
     * Unget the previously-read record. When this is called, the file will
     * pretend to rewind as if the last array was not read, and return it next
     * time. Only one array can be unget'd.
     *
     * Using unget() can be used to emulate peek(), by calling next() and then
     * unget(). This is useful since the only way to determine if a report step
     * is over is checking if the next array is a SEQHDR or the next character
     * is end-of-file.
     *
     * If unget() is called before next(), behaviour is undefined.
     */
    void unget() noexcept (true);

private:
    raw_array last;

    void read_head();
    void read_body();

    bool ungetted = false;
};

template < typename Stream >
stream_reader< Stream >::stream_reader(const std::string& path) :
    Stream(path, Stream::binary | Stream::in)
{
    if (!this->is_open()) {
        const auto msg = "could not open file '" + path + "'";
        throw std::invalid_argument(msg);
    }

    auto errors = std::ios::failbit | std::ios::badbit | std::ios::eofbit;
    this->exceptions(errors);
}

namespace {

void check_headtail(std::array< char, sizeof(std::int32_t) > head,
                    std::array< char, sizeof(std::int32_t) > tail) {
    if (head == tail)
        return;

    std::int32_t h;
    std::int32_t t;
    ecl3_get_native(&h, head.data(), ECL3_INTE, 1);
    ecl3_get_native(&t, tail.data(), ECL3_INTE, 1);

    std::stringstream msg;
    msg << "head/tail mismatch: "
       << "head (" << h << ")"
       << " != "
       << "tail (" << t << ")"
    ;
    throw head_tail_error(msg.str());
};

}

template < typename Stream >
void stream_reader< Stream >::read_head() {
    std::array< char, sizeof(std::int32_t) > head;
    std::array< char, 16 > header;
    std::array< char, sizeof(std::int32_t) > tail;

    // TODO: -> unsigned char (with casts)

    try {
        this->read(head.data(), sizeof(head));
    } catch (typename Stream::failure&) {
        if (this->eof()) {
            this->last.count = -1;
            return;
        }

        /* some error is set - propagate exception */
        throw;
    }

    this->read(header.data(), sizeof(header));
    this->read(tail.data(), sizeof(tail));
    check_headtail(head, tail);
    const auto err = ecl3_array_header(
        header.data(),
        this->last.keyword.data(),
        this->last.type.data(),
        &this->last.count
    );

    if (err) {
        throw header_error("error parsing header");
    }
}

template < typename Stream >
void stream_reader< Stream >::read_body() {
    std::array< char, sizeof(std::int32_t) > head;
    std::array< char, sizeof(std::int32_t) > tail;

    int type;
    int size;
    int blocksize;
    auto err = ecl3_typeid(this->last.type.data(), &type);
    if (err) {
        std::stringstream ss;
        ss << "unknown type"
           << "'"
           << std::string(this->last.type.data(), this->last.type.size())
           << "'"
        ;
        throw std::invalid_argument(ss.str());
    }
    ecl3_type_size(type, &size);
    ecl3_block_size(type, &blocksize);

    auto buffer = std::vector< char >();
    int remaining = this->last.count;
    this->last.body.clear();
    while (remaining > 0) {
        this->read(head.data(), sizeof(head));
        std::int32_t elems;
        ecl3_get_native(&elems, head.data(), ECL3_INTE, 1);

        buffer.resize(elems);
        auto prev_size = this->last.body.size();
        this->read(buffer.data(), elems);

        this->read(tail.data(), sizeof(tail));
        check_headtail(head, tail);

        int count;
        this->last.body.resize(prev_size + elems);
        err = ecl3_array_body(
            this->last.body.data() + prev_size,
            buffer.data(),
            type,
            remaining,
            blocksize,
            &count
        );

        if (err) {
            throw std::runtime_error("error parsing array body");
        }

        remaining -= count;
    }

    if (remaining != 0)
        throw std::runtime_error("array not terminated correctly");
}

template < typename Stream >
const raw_array& stream_reader< Stream >::next() {
    if (this->ungetted) {
        this->ungetted = false;
        return this->last;
    }

    this->read_head();
    if (!this->last.empty())
        this->read_body();
    return this->last;
}

template < typename Stream >
void stream_reader< Stream >::unget() noexcept (true) {
    this->ungetted = true;
}

}

#endif // ECL3_IO_HPP
