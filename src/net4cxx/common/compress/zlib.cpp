//
// Created by yuwenyong on 17-9-15.
//

#include "net4cxx/common/compress/zlib.h"
#include "net4cxx/common/debugging/assert.h"

NS_BEGIN

#define ArrangeOutputBuffer(zst, buffer, length) { \
    size_t occupied; \
    if (buffer.empty()) { \
        buffer.resize(length); \
        occupied = 0; \
    } else { \
        occupied = (size_t)(zst.next_out - (Bytef *)buffer.data()); \
        if (occupied == length) { \
            length <<= 1; \
            buffer.resize(length); \
        } \
    } \
    zst.avail_out = (unsigned int)(length - occupied); \
    zst.next_out = (Bytef *)buffer.data() + occupied; \
}


#define ArrangeOutputBufferLimit(zst, buffer, length, maxLength) { \
    size_t occupied; \
    if (buffer.empty()) { \
        buffer.resize(length); \
        occupied = 0; \
    } else { \
        occupied = (size_t)(zst.next_out - (Bytef *)buffer.data()); \
        if (occupied == length) { \
            if (maxLength != 0) { \
                size_t newLength; \
                if (length == maxLength) { \
                    break; \
                } \
                if (length <= (maxLength >> 1)) { \
                    newLength = length << 1; \
                } else { \
                    newLength = maxLength; \
                } \
                buffer.resize(newLength); \
                length = newLength; \
            } else { \
                length <<= 1; \
                buffer.resize(length); \
            } \
        } \
    } \
    zst.avail_out = (unsigned int)(length - occupied); \
    zst.next_out = (Bytef *)buffer.data() + occupied; \
}


ByteArray Zlib::compress(const Byte *data, size_t len, int level) {
    z_stream zst;
    zst.zalloc = (alloc_func)NULL;
    zst.zfree = (free_func)Z_NULL;
    zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    ByteArray retVal;
    int err = deflateInit(&zst, level);
    if (err != Z_OK) {
        if (err == Z_MEM_ERROR) {
            NET4CXX_THROW_EXCEPTION(MemoryError, "Out of memory while compressing data");
        } else if (err == Z_STREAM_ERROR) {
            NET4CXX_THROW_EXCEPTION(ZlibError, "Bad compression level");
        } else {
            deflateEnd(&zst);
            handleError(zst, err, "while compressing data");
        }
    }
    zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBuffer(zst, retVal, oBufLen);
        err = deflate(&zst, Z_FINISH);
        if (err == Z_STREAM_ERROR) {
            deflateEnd(&zst);
            NET4CXX_THROW_EXCEPTION(ZlibError, "while compressing data");
        }
    } while (zst.avail_out == 0);
    NET4CXX_ASSERT(zst.avail_in == 0);
    NET4CXX_ASSERT(err == Z_STREAM_END);
    err = deflateEnd(&zst);
    if (err != Z_OK) {
        handleError(zst, err, "while finishing compression");
    }
    retVal.resize((size_t)(zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

std::string Zlib::compressToString(const Byte *data, size_t len, int level) {
    z_stream zst;
    zst.zalloc = (alloc_func)NULL;
    zst.zfree = (free_func)Z_NULL;
    zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    std::string retVal;
    int err = deflateInit(&zst, level);
    if (err != Z_OK) {
        if (err == Z_MEM_ERROR) {
            NET4CXX_THROW_EXCEPTION(MemoryError, "Out of memory while compressing data");
        } else if (err == Z_STREAM_ERROR) {
            NET4CXX_THROW_EXCEPTION(ZlibError, "Bad compression level");
        } else {
            deflateEnd(&zst);
            handleError(zst, err, "while compressing data");
        }
    }
    zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBuffer(zst, retVal, oBufLen);
        err = deflate(&zst, Z_FINISH);
        if (err == Z_STREAM_ERROR) {
            deflateEnd(&zst);
            NET4CXX_THROW_EXCEPTION(ZlibError, "while compressing data");
        }
    } while (zst.avail_out == 0);
    NET4CXX_ASSERT(zst.avail_in == 0);
    NET4CXX_ASSERT(err == Z_STREAM_END);
    err = deflateEnd(&zst);
    if (err != Z_OK) {
        handleError(zst, err, "while finishing compression");
    }
    retVal.resize((size_t)(zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

ByteArray Zlib::decompress(const Byte *data, size_t len, int wbits) {
    z_stream zst;
    zst.zalloc = (alloc_func)NULL;
    zst.zfree = (free_func)Z_NULL;
    zst.avail_in = 0;
    zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    ByteArray retVal;
    int err = inflateInit2(&zst, wbits);
    if (err != Z_OK) {
        if (err == Z_MEM_ERROR) {
            NET4CXX_THROW_EXCEPTION(MemoryError, "Out of memory while decompressing data");
        } else {
            inflateEnd(&zst);
            handleError(zst, err, "while preparing to decompress data");
        }
    }
    zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBuffer(zst, retVal, oBufLen);
        err = inflate(&zst, Z_FINISH);
        if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
            inflateEnd(&zst);
            if (err == Z_MEM_ERROR) {
                NET4CXX_THROW_EXCEPTION(MemoryError, "Out of memory while decompressing data");
            } else {
                handleError(zst, err, "while decompressing data");
            }
        }
    } while (zst.avail_out == 0);
    if (err != Z_STREAM_END) {
        inflateEnd(&zst);
        handleError(zst, err, "while decompressing data");
    }
    err = inflateEnd(&zst);
    if (err != Z_OK) {
        handleError(zst, err, "while finishing data decompression");
    }
    retVal.resize((size_t)(zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

std::string Zlib::decompressToString(const Byte *data, size_t len, int wbits) {
    z_stream zst;
    zst.zalloc = (alloc_func)NULL;
    zst.zfree = (free_func)Z_NULL;
    zst.avail_in = 0;
    zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    std::string retVal;
    int err = inflateInit2(&zst, wbits);
    if (err != Z_OK) {
        if (err == Z_MEM_ERROR) {
            NET4CXX_THROW_EXCEPTION(MemoryError, "Out of memory while decompressing data");
        } else {
            inflateEnd(&zst);
            handleError(zst, err, "while preparing to decompress data");
        }
    }
    zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBuffer(zst, retVal, oBufLen);
        err = inflate(&zst, Z_FINISH);
        if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
            inflateEnd(&zst);
            if (err == Z_MEM_ERROR) {
                NET4CXX_THROW_EXCEPTION(MemoryError, "Out of memory while decompressing data");
            } else {
                handleError(zst, err, "while decompressing data");
            }
        }
    } while (zst.avail_out == 0);
    if (err != Z_STREAM_END) {
        inflateEnd(&zst);
        handleError(zst, err, "while decompressing data");
    }
    err = inflateEnd(&zst);
    if (err != Z_OK) {
        handleError(zst, err, "while finishing data decompression");
    }
    retVal.resize((size_t)(zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

void Zlib::handleError(z_stream zst, int err, const char *msg) {
    const char *zmsg = Z_NULL;
    if (err == Z_VERSION_ERROR) {
        zmsg = "library version mismatch";
    }
    if (zmsg == Z_NULL) {
        zmsg = zst.msg;
    }
    if (zmsg == Z_NULL) {
        switch (err) {
            case Z_BUF_ERROR:
                zmsg = "incomplete or truncated stream";
                break;
            case Z_STREAM_ERROR:
                zmsg = "inconsistent stream state";
                break;
            case Z_DATA_ERROR:
                zmsg = "invalid input data";
                break;
            default:
                break;
        }
    }
    if (zmsg == Z_NULL) {
        NET4CXX_THROW_EXCEPTION(ZlibError, "Error %d %s", err, msg);
    } else {
        NET4CXX_THROW_EXCEPTION(ZlibError, "Error %d %s: %.200s", err, msg, zmsg);
    }
}

constexpr int Zlib::maxWBits;
constexpr int Zlib::deflated;
constexpr int Zlib::defMemLevel;
constexpr int Zlib::zBestSpeed;
constexpr int Zlib::zBestCompression;
constexpr int Zlib::zDefaultCompression;
constexpr int Zlib::zFiltered;
constexpr int Zlib::zHuffmanOnly;
constexpr int Zlib::zDefaultStrategy;

constexpr int Zlib::zFinish;
constexpr int Zlib::zNoFlush;
constexpr int Zlib::zSyncFlush;
constexpr int Zlib::zFullFlush;


CompressObj::CompressObj(int level, int method, int wbits, int memLevel, int strategy)
        : _inited(false)
        , _level(level)
        , _method(method)
        , _wbits(wbits)
        , _memLevel(memLevel)
        , _strategy(strategy) {
    _zst.zalloc = (alloc_func) NULL;
    _zst.zfree = (free_func) Z_NULL;
    _zst.next_in = NULL;
    _zst.avail_in = 0;
}

CompressObj::CompressObj(const CompressObj &rhs)
        : _inited(rhs._inited)
        , _level(rhs._level)
        , _method(rhs._method)
        , _wbits(rhs._wbits)
        , _memLevel(rhs._memLevel)
        , _strategy(rhs._strategy) {
    if (_inited) {
        int err = deflateCopy(&_zst, const_cast<z_stream *>(&rhs._zst));

        if (err != Z_OK) {
            _inited = false;
#ifdef NET4CXX_DEBUG
            if (err == Z_STREAM_ERROR) {
                NET4CXX_THROW_EXCEPTION(ValueError, "Inconsistent stream state");
            } else if (err == Z_MEM_ERROR) {
                NET4CXX_THROW_EXCEPTION(MemoryError, "Can't allocate memory for compression object");
            } else {
                Zlib::handleError(rhs._zst, err, "while copying compression object");
            }
#endif
        }
    } else {
        _zst.zalloc = (alloc_func) NULL;
        _zst.zfree = (free_func) Z_NULL;
        _zst.next_in = NULL;
        _zst.avail_in = 0;
    }
}

CompressObj::CompressObj(CompressObj &&rhs) noexcept
        : _inited(rhs._inited)
        , _level(rhs._level)
        , _method(rhs._method)
        , _wbits(rhs._wbits)
        , _memLevel(rhs._memLevel)
        , _strategy(rhs._strategy) {
    _zst = rhs._zst;
    if (rhs._inited) {
        rhs._inited = false;
        rhs._zst.zalloc = (alloc_func) NULL;
        rhs._zst.zfree = (free_func) Z_NULL;
        rhs._zst.next_in = NULL;
        rhs._zst.avail_in = 0;
    }
}

CompressObj& CompressObj::operator=(const CompressObj &rhs) {
    if (this == &rhs) {
        return *this;
    }
    clear();
    _inited = rhs._inited;
    _level = rhs._level;
    _method = rhs._method;
    _wbits = rhs._wbits;
    _memLevel = rhs._memLevel;
    _strategy = rhs._strategy;
    if (_inited) {
        int err = deflateCopy(&_zst, const_cast<z_stream *>(&rhs._zst));
        if (err != Z_OK) {
            _inited = false;
            if (err == Z_STREAM_ERROR) {
                NET4CXX_THROW_EXCEPTION(ValueError, "Inconsistent stream state");
            } else if (err == Z_MEM_ERROR) {
                NET4CXX_THROW_EXCEPTION(MemoryError, "Can't allocate memory for compression object");
            } else {
                Zlib::handleError(rhs._zst, err, "while copying compression object");
            }
        }
    }
    return *this;
}

CompressObj& CompressObj::operator=(CompressObj &&rhs) noexcept {
    clear();
    _inited = rhs._inited;
    _level = rhs._level;
    _method = rhs._method;
    _wbits = rhs._wbits;
    _memLevel = rhs._memLevel;
    _strategy = rhs._strategy;
    _zst = rhs._zst;
    if (rhs._inited) {
        rhs._inited = false;
        rhs._zst.zalloc = (alloc_func) NULL;
        rhs._zst.zfree = (free_func) Z_NULL;
        rhs._zst.next_in = NULL;
        rhs._zst.avail_in = 0;
    }
    return *this;
}

ByteArray CompressObj::compress(const Byte *data, size_t len) {
    initialize();
    _zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    int err;
    ByteArray retVal;
    _zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBuffer(_zst, retVal, oBufLen);
        err = deflate(&_zst, Z_NO_FLUSH);
        if (err == Z_STREAM_ERROR) {
            Zlib::handleError(_zst, err, "while compressing data");
        }
    } while (_zst.avail_out == 0);
    NET4CXX_ASSERT(_zst.avail_in == 0);
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

std::string CompressObj::compressToString(const Byte *data, size_t len) {
    initialize();
    _zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    int err;
    std::string retVal;
    _zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBuffer(_zst, retVal, oBufLen);
        err = deflate(&_zst, Z_NO_FLUSH);
        if (err == Z_STREAM_ERROR) {
            Zlib::handleError(_zst, err, "while compressing data");
        }
    } while (_zst.avail_out == 0);
    NET4CXX_ASSERT(_zst.avail_in == 0);
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

ByteArray CompressObj::flush(int flushMode) {
    initialize();
    size_t oBufLen = DEFAULTALLOC;
    int err;
    ByteArray retVal;
    if (flushMode == Z_NO_FLUSH) {
        return retVal;
    }
    _zst.avail_in = 0;
    do {
        ArrangeOutputBuffer(_zst, retVal, oBufLen)
        err = deflate(&_zst, flushMode);
        if (err == Z_STREAM_ERROR) {
            Zlib::handleError(_zst, err, "while flushing");
        }
    } while (_zst.avail_out == 0);
    NET4CXX_ASSERT(_zst.avail_in == 0);
    if (err == Z_STREAM_END && flushMode == Z_FINISH) {
        err = deflateEnd(&_zst);
        _inited = false;
        if (err != Z_OK) {
            Zlib::handleError(_zst, err, "from deflateEnd()");
        }
    } else if (err != Z_OK && err != Z_BUF_ERROR) {
        Zlib::handleError(_zst, err, "while flushing");
    }
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

std::string CompressObj::flushToString(int flushMode) {
    initialize();
    size_t oBufLen = DEFAULTALLOC;
    int err;
    std::string retVal;
    if (flushMode == Z_NO_FLUSH) {
        return retVal;
    }
    _zst.avail_in = 0;
    do {
        ArrangeOutputBuffer(_zst, retVal, oBufLen)
        err = deflate(&_zst, flushMode);
        if (err == Z_STREAM_ERROR) {
            Zlib::handleError(_zst, err, "while flushing");
        }
    } while (_zst.avail_out == 0);
    NET4CXX_ASSERT(_zst.avail_in == 0);
    if (err == Z_STREAM_END && flushMode == Z_FINISH) {
        err = deflateEnd(&_zst);
        _inited = false;
        if (err != Z_OK) {
            Zlib::handleError(_zst, err, "from deflateEnd()");
        }
    } else if (err != Z_OK && err != Z_BUF_ERROR) {
        Zlib::handleError(_zst, err, "while flushing");
    }
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

void CompressObj::initialize() {
    if (!_inited) {
        int err = deflateInit2(&_zst, _level, _method, _wbits, _memLevel, _strategy);
        if (err == Z_OK) {
            _inited = true;
        } else if (err == Z_MEM_ERROR) {
            NET4CXX_THROW_EXCEPTION(MemoryError, "Can't allocate memory for compression object");
        } else if (err == Z_STREAM_ERROR) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Invalid initialization option");
        } else {
            Zlib::handleError(_zst, err, "while creating compression object");
        }
    }
}

void CompressObj::clear() {
    if (_inited) {
        deflateEnd(&_zst);
        _zst.zalloc = (alloc_func) NULL;
        _zst.zfree = (free_func) Z_NULL;
        _zst.next_in = NULL;
        _zst.avail_in = 0;
        _inited = false;
    }
}

DecompressObj::DecompressObj(int wbits)
        : _inited(false)
        , _wbits(wbits) {
    _zst.zalloc = (alloc_func) NULL;
    _zst.zfree = (free_func) Z_NULL;
    _zst.next_in = NULL;
    _zst.avail_in = 0;
}

DecompressObj::DecompressObj(const DecompressObj &rhs)
        : _inited(rhs._inited)
        , _wbits(rhs._wbits)
        , _unusedData(rhs._unusedData)
        , _unconsumedTail(rhs._unconsumedTail) {
    if (_inited) {
        int err = inflateCopy(&_zst, const_cast<z_stream *>(&rhs._zst));
        if (err != Z_OK) {
            _inited = false;
#ifndef NDEBUG
            if (err == Z_STREAM_ERROR) {
                NET4CXX_THROW_EXCEPTION(ValueError, "Inconsistent stream state");
            } else if (err == Z_MEM_ERROR) {
                NET4CXX_THROW_EXCEPTION(MemoryError, "Can't allocate memory for decompression object");
            } else {
                Zlib::handleError(rhs._zst, err, "while copying decompression object");
            }
#endif
        }
    } else {
        _zst.zalloc = (alloc_func) NULL;
        _zst.zfree = (free_func) Z_NULL;
        _zst.next_in = NULL;
        _zst.avail_in = 0;
    }
}

DecompressObj::DecompressObj(DecompressObj &&rhs) noexcept
        : _inited(rhs._inited)
        , _wbits(rhs._wbits)
        , _unusedData(std::move(rhs._unusedData))
        , _unconsumedTail(std::move(rhs._unconsumedTail)) {
    _zst = rhs._zst;
    if (rhs._inited) {
        rhs._inited = false;
        rhs._zst.zalloc = (alloc_func) NULL;
        rhs._zst.zfree = (free_func) Z_NULL;
        rhs._zst.next_in = NULL;
        rhs._zst.avail_in = 0;
    }
}

DecompressObj& DecompressObj::operator=(const DecompressObj &rhs) {
    if (this == &rhs) {
        return *this;
    }
    clear();
    _inited = rhs._inited;
    _wbits = rhs._wbits;
    _unusedData = rhs._unusedData;
    _unconsumedTail = rhs._unconsumedTail;
    if (_inited) {
        int err = inflateCopy(&_zst, const_cast<z_stream *>(&rhs._zst));
        if (err != Z_OK) {
            _inited = false;
            if (err == Z_STREAM_ERROR) {
                NET4CXX_THROW_EXCEPTION(ValueError, "Inconsistent stream state");
            } else if (err == Z_MEM_ERROR) {
                NET4CXX_THROW_EXCEPTION(MemoryError, "Can't allocate memory for decompression object");
            } else {
                Zlib::handleError(rhs._zst, err, "while copying decompression object");
            }
        }
    }
    return *this;
}

DecompressObj& DecompressObj::operator=(DecompressObj &&rhs) noexcept {
    clear();
    _inited = rhs._inited;
    _wbits = rhs._wbits;
    _unusedData = std::move(rhs._unusedData);
    _unconsumedTail = std::move(rhs._unconsumedTail);
    _zst = rhs._zst;
    if (rhs._inited) {
        rhs._inited = false;
        rhs._zst.zalloc = (alloc_func) NULL;
        rhs._zst.zfree = (free_func) Z_NULL;
        rhs._zst.next_in = NULL;
        rhs._zst.avail_in = 0;
    }
    return *this;
}

ByteArray DecompressObj::decompress(const Byte *data, size_t len, size_t maxLength) {
    initialize();
    _zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    int err = Z_OK;
    ByteArray retVal;
    _zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBufferLimit(_zst, retVal, oBufLen, maxLength);
        err = inflate(&_zst, Z_SYNC_FLUSH);
        if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
            break;
        }
    } while (_zst.avail_out == 0);
    saveUnconsumedInput(data, len, err);
    if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
        Zlib::handleError(_zst, err, "while decompressing");
    }
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

std::string DecompressObj::decompressToString(const Byte *data, size_t len, size_t maxLength) {
    initialize();
    _zst.next_in = (Bytef *)data;
    size_t oBufLen = DEFAULTALLOC;
    int err = Z_OK;
    std::string retVal;
    _zst.avail_in = (unsigned int)len;
    do {
        ArrangeOutputBufferLimit(_zst, retVal, oBufLen, maxLength);
        err = inflate(&_zst, Z_SYNC_FLUSH);
        if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
            break;
        }
    } while (_zst.avail_out == 0);
    saveUnconsumedInput(data, len, err);
    if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
        Zlib::handleError(_zst, err, "while decompressing");
    }
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

ByteArray DecompressObj::flush() {
    initialize();
    _zst.next_in = _unconsumedTail.data();
    size_t oBufLen = DEFAULTALLOC;
    int err;
    ByteArray retVal;
    _zst.avail_in = (unsigned int)_unconsumedTail.size();
    do {
        ArrangeOutputBuffer(_zst, retVal, oBufLen);
        err = inflate(&_zst, Z_FINISH);
        if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
            break;
        }
    } while (_zst.avail_out == 0);
    saveUnconsumedInput(_unconsumedTail.data(), _unconsumedTail.size(), err);
    if (err == Z_STREAM_END) {
        err = inflateEnd(&_zst);
        _inited = false;
        if (err != Z_OK) {
            Zlib::handleError(_zst, err, "from inflateEnd()");
        }
    }
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

std::string DecompressObj::flushToString() {
    initialize();
    _zst.next_in = _unconsumedTail.data();
    size_t oBufLen = DEFAULTALLOC;
    int err;
    std::string retVal;
    _zst.avail_in = (unsigned int)_unconsumedTail.size();
    do {
        ArrangeOutputBuffer(_zst, retVal, oBufLen);
        err = inflate(&_zst, Z_FINISH);
        if (err != Z_OK && err != Z_BUF_ERROR && err != Z_STREAM_END) {
            break;
        }
    } while (_zst.avail_out == 0);
    saveUnconsumedInput(_unconsumedTail.data(), _unconsumedTail.size(), err);
    if (err == Z_STREAM_END) {
        err = inflateEnd(&_zst);
        _inited = false;
        if (err != Z_OK) {
            Zlib::handleError(_zst, err, "from inflateEnd()");
        }
    }
    retVal.resize((size_t)(_zst.next_out - (Bytef *)retVal.data()));
    return retVal;
}

void DecompressObj::saveUnconsumedInput(const Byte *data, size_t len, int err) {
    if (err == Z_STREAM_END) {
        if (_zst.avail_in > 0) {
            auto leftSize = (size_t)(data + len - _zst.next_in);
            _unusedData.insert(_unusedData.end(), _zst.next_in, _zst.next_in + leftSize);
            _zst.avail_in = 0;
        }
    }
    if (_zst.avail_in > 0 || !_unconsumedTail.empty()) {
        auto leftSize = size_t(data + len - _zst.next_in);
        _unconsumedTail.assign(_zst.next_in, _zst.next_in + leftSize);
    }
}

void DecompressObj::initialize() {
    if (!_inited) {
        int err = inflateInit2(&_zst, _wbits);
        if (err == Z_OK) {
            _inited = true;
        } else if (err == Z_STREAM_ERROR) {
            NET4CXX_THROW_EXCEPTION(ValueError, "Invalid initialization option");
        } else if (err == Z_MEM_ERROR) {
            NET4CXX_THROW_EXCEPTION(MemoryError, "Can't allocate memory for decompression object");
        } else {
            Zlib::handleError(_zst, err, "while creating decompression object");
        }
    }
}

void DecompressObj::clear() {
    if (_inited) {
        inflateEnd(&_zst);
        _zst.zalloc = (alloc_func) NULL;
        _zst.zfree = (free_func) Z_NULL;
        _zst.next_in = NULL;
        _zst.avail_in = 0;
        _inited = false;
    }
}

NS_END