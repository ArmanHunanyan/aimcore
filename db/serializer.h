#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <boost/python.hpp>

#include <memory>
#include <vector>

namespace db {

class Buffer
{
public:
    typedef std::vector<uint8_t>::size_type SizeType;
    Buffer() = default;
    void putMem(void* ptr, SizeType len);
    void putByte(uint8_t byte)
    {
        m_buf.push_back(byte);
    }

    const uint8_t* ptr() const {
        return &m_buf[0];
    }
    SizeType size() const {
        return m_buf.size();
    }
    void clear() { m_buf.clear(); }

private:
    std::vector<uint8_t> m_buf;
};

class Serializer
{
public:
    virtual ~Serializer() = default;
    static std::unique_ptr<Serializer> create(unsigned protocol);

public:
    // serializeWTypeWSize will keep size in buffer if neccessary. deserializeWTypeWSize will read single
    // object and will move data pointer to next object's postion or end
    virtual void serializeWTypeWSize(Buffer& buffer, const boost::python::object& obj) const = 0;
    virtual boost::python::object deserializeWTypeWSize(const uint8_t*& data, const uint8_t* end) const = 0;
    // This overload is mainly for debugging and testing
    boost::python::object deserializeWTypeWSize(Buffer& buffer) const
    {
        const uint8_t *data = buffer.ptr(), *end = data + buffer.size();
        auto res = deserializeWTypeWSize(data, end);
        if (data != end) {
            throwCorruptedBufferError();
        }
        return res;
    }

public:
    // This serialization assumes that size of data will be kept separately. Deserialization
    // Assumes that end-data is a size of memory where single object is serialized using serializeWType
    virtual void serializeWType(Buffer& buffer, const boost::python::object& obj) const = 0;
    virtual boost::python::object deserializeWType(const uint8_t* data, const uint8_t* end) const = 0;
    // This overload is mainly for debugging and testing
    boost::python::object deserializeWType(Buffer& buffer) const
    {
        const uint8_t* data = buffer.ptr(), * end = data + buffer.size();
        auto res = deserializeWType(data, end);
        return res;
    }

protected:
    static void throwCorruptedBufferError();
};

} // namespace db

#endif;

