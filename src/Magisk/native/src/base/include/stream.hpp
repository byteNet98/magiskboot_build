#pragma once

#ifdef _WIN32
#  include <winsup/uio_compat.h>
#else
#include <sys/uio.h>
#endif
#include <cstdio>
#include <memory>

#include "../files.hpp"

#define ENABLE_IOV 0

struct out_stream {
    virtual bool write(const void *buf, size_t len) = 0;
#if ENABLE_IOV
    virtual ssize_t writev(const iovec *iov, int iovcnt);
#endif
    virtual ~out_stream() = default;
};

using out_strm_ptr = std::unique_ptr<out_stream>;

// Delegates all operations to base stream
class filter_out_stream : public out_stream {
public:
    filter_out_stream(out_strm_ptr &&base) : base(std::move(base)) {}
    bool write(const void *buf, size_t len) override;
protected:
    out_strm_ptr base;
};

// Buffered output stream, writing in chunks
class chunk_out_stream : public filter_out_stream {
public:
    chunk_out_stream(out_strm_ptr &&base, size_t buf_sz, size_t chunk_sz)
    : filter_out_stream(std::move(base)), chunk_sz(chunk_sz), data(buf_sz) {}

    chunk_out_stream(out_strm_ptr &&base, size_t buf_sz = 4096)
    : chunk_out_stream(std::move(base), buf_sz, buf_sz) {}

    bool write(const void *buf, size_t len) final;

protected:
    // Classes inheriting this class has to call finalize() in its destructor
    void finalize();
    virtual bool write_chunk(const void *buf, size_t len, bool final);

    size_t chunk_sz;

private:
    size_t buf_off = 0;
    heap_data data;
};

struct in_stream {
    virtual ssize_t read(void *buf, size_t len) = 0;
    ssize_t readFully(void *buf, size_t len);
#if ENABLE_IOV
    virtual ssize_t readv(const iovec *iov, int iovcnt);
#endif
    virtual ~in_stream() = default;
};

// A stream is something that is writable and readable
struct stream : public out_stream, public in_stream {};

using stream_ptr = std::unique_ptr<stream>;

// Byte stream that dynamically allocates memory
class byte_stream : public stream {
public:
    byte_stream(heap_data &data) : _data(data) {}

    ssize_t read(void *buf, size_t len) override;
    bool write(const void *buf, size_t len) override;

private:
    heap_data &_data;
    size_t _pos = 0;
    size_t _cap = 0;

    void resize(size_t new_sz, bool zero = false);
};

class rust_vec_stream : public stream {
public:
    rust_vec_stream(rust::Vec<uint8_t> &data) : _data(data) {}

    ssize_t read(void *buf, size_t len) override;
    bool write(const void *buf, size_t len) override;

private:
    rust::Vec<uint8_t> &_data;
    size_t _pos = 0;

    void ensure_size(size_t sz, bool zero = false);
};

class file_stream : public stream {
public:
    bool write(const void *buf, size_t len) final;
protected:
    virtual ssize_t do_write(const void *buf, size_t len) = 0;
};

// File stream but does not close the file descriptor at any time
class fd_stream : public file_stream {
public:
    fd_stream(int fd) : fd(fd) {}
    ssize_t read(void *buf, size_t len) override;
#if ENABLE_IOV
    ssize_t readv(const iovec *iov, int iovcnt) override;
    ssize_t writev(const iovec *iov, int iovcnt) override;
#endif
protected:
    ssize_t do_write(const void *buf, size_t len) override;
private:
    int fd;
};

/* ****************************************
 * Bridge between stream class and C stdio
 * ****************************************/

// stream_ptr -> sFILE
sFILE make_stream_fp(stream_ptr &&strm);

template <class T, class... Args>
sFILE make_stream_fp(Args &&... args) {
    return make_stream_fp(stream_ptr(new T(std::forward<Args>(args)...)));
}
