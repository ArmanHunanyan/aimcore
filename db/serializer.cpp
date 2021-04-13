#include "serializer.h"
#include "dberror.h"
#include "pyutils.h"

#include <boost/endian/buffers.hpp> 

#include <unordered_map>
#include <algorithm>
#include <iostream>

namespace db {

void Buffer::putMem(void* ptr, SizeType len)
{
    uint8_t* cptr = (uint8_t*)ptr;
    auto emptyOffset = m_buf.size();
    m_buf.resize(emptyOffset + len);
    std::copy(cptr, cptr + len, &m_buf[emptyOffset]);
}

class PyPickle {
    struct data_t;

public:
    static boost::python::object dumps(boost::python::object obj, int protocol = -1);
    static boost::python::object loads(boost::python::object s);

private:
    static void initializeData();

    static data_t* data;
};

struct PyPickle::data_t {
    boost::python::object module;
    boost::python::object dumps;
    boost::python::object loads;
};

/// Data used for communicating with the Python `pickle' module.
PyPickle::data_t* PyPickle::data;

boost::python::object PyPickle::dumps(boost::python::object obj, int protocol)
{
    if (!data) initializeData();
    return (data->dumps)(obj, protocol);
}

boost::python::object PyPickle::loads(boost::python::object s)
{
    if (!data) initializeData();
    return (data->loads)(s);
}

void PyPickle::initializeData()
{
    data = new data_t;
    data->module = boost::python::object(boost::python::handle<>(PyImport_ImportModule("pickle")));
    data->dumps = data->module.attr("dumps");
    data->loads = data->module.attr("loads");
}

PyPickle pickle;

class SerializerV1
    : public Serializer
{
public:
    SerializerV1()
        : m_serializeProcsTable()
        , m_defaultSerializeProcs(serializeWTypeWSizePickle, serializeWTypePickle)
        , m_deserializeWTypeWSizeProcTable()
    {
        m_serializeProcsTable[Py_None->ob_type] = SerializeProcs(serializeNoneWTypeWSize, serializeNoneWType);
        m_serializeProcsTable[&PyLong_Type] = SerializeProcs(serializeLongWTypeWSize, serializeLongWType);
        m_serializeProcsTable[&PyBool_Type] = SerializeProcs(serializeBoolWTypeWSize, serializeBoolWType);
        m_serializeProcsTable[&PyFloat_Type] = SerializeProcs(serializeFloatWTypeWSize, serializeFloatWType);
        m_serializeProcsTable[&PyUnicode_Type] = SerializeProcs(serializeStrWTypeWSize, serializeStrWType);

        m_deserializeWTypeWSizeProcTable.resize((size_t)Type::MaxValue);
        m_deserializeWTypeWSizeProcTable[(size_t)Type::None] = deserializeNoneWTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int8] = deserializeInt8WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int16] = deserializeInt16WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int24] = deserializeInt24WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int32] = deserializeInt32WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int40] = deserializeInt40WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int48] = deserializeInt48WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int56] = deserializeInt56WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Int64] = deserializeInt64WTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::IntLarge] = deserializeIntLargeWTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::TrueType] = deserializeTrueWTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::FalseType] = deserializeFalseWTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Float] = deserializeFloatWTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Str] = deserializeStrWTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::UStr] = deserializeUStrWTypeWSize;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Pickle8S] = deserializeWTypeWSizePickle8S;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Pickle16S] = deserializeWTypeWSizePickle16S;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Pickle32S] = deserializeWTypeWSizePickle32S;
        m_deserializeWTypeWSizeProcTable[(size_t)Type::Pickle64S] = deserializeWTypeWSizePickle64S;

        m_deserializeWTypeProcTable.resize((size_t)Type::MaxValue);
        m_deserializeWTypeProcTable[(size_t)Type::None] = deserializeNoneWType;
        m_deserializeWTypeProcTable[(size_t)Type::Int8] = deserializeInt8WType;
        m_deserializeWTypeProcTable[(size_t)Type::Int16] = deserializeInt16WType;
        m_deserializeWTypeProcTable[(size_t)Type::Int24] = deserializeInt24WType;
        m_deserializeWTypeProcTable[(size_t)Type::Int32] = deserializeInt32WType;
        m_deserializeWTypeProcTable[(size_t)Type::Int40] = deserializeInt40WType;
        m_deserializeWTypeProcTable[(size_t)Type::Int48] = deserializeInt48WType;
        m_deserializeWTypeProcTable[(size_t)Type::Int56] = deserializeInt56WType;
        m_deserializeWTypeProcTable[(size_t)Type::Int64] = deserializeInt64WType;
        m_deserializeWTypeProcTable[(size_t)Type::IntLarge] = deserializeIntLargeWType;
        m_deserializeWTypeProcTable[(size_t)Type::TrueType] = deserializeTrueWType;
        m_deserializeWTypeProcTable[(size_t)Type::FalseType] = deserializeFalseWType;
        m_deserializeWTypeProcTable[(size_t)Type::Float] = deserializeFloatWType;
        m_deserializeWTypeProcTable[(size_t)Type::Str] = deserializeStrWType;
        m_deserializeWTypeProcTable[(size_t)Type::UStr] = deserializeUStrWType;
        m_deserializeWTypeProcTable[(size_t)Type::Pickle] = deserializeWTypePickle;
    }
    virtual ~SerializerV1() = default;

public:
    virtual void serializeWTypeWSize(Buffer& buffer, const boost::python::object& obj) const override
    {
        findSerializeProcs(obj).serializeWTypeWSizeProc(buffer, obj);
    }

    virtual boost::python::object deserializeWTypeWSize(const uint8_t*& data, const uint8_t* end) const override
    {
        assert(data != end);
        Type type = (Type)*data++;
        if (type >= Type::MaxValue || m_deserializeWTypeWSizeProcTable[(size_t)type] == DeserializeWTypeWSizeProc()) {
            throwCorruptedBufferError();
        }
        DeserializeWTypeWSizeProc proc = m_deserializeWTypeWSizeProcTable[(size_t)type];
        return proc(data, end);
    }

public:
    virtual void serializeWType(Buffer& buffer, const boost::python::object& obj) const override
    {
        findSerializeProcs(obj).serializeWTypeProc(buffer, obj);
    }

    virtual boost::python::object deserializeWType(const uint8_t* data, const uint8_t* end) const override
    {
        assert(data != end);
        Type type = (Type)*data++;
        if (type >= Type::MaxValue || m_deserializeWTypeProcTable[(size_t)type] == DeserializeWTypeProc()) {
            throwCorruptedBufferError();
        }
        DeserializeWTypeProc proc = m_deserializeWTypeProcTable[(size_t)type];
        if (proc == nullptr) {
            throwCorruptedBufferError(); // Invalid type
        }
        return proc(data, end);
    }

private:
    enum class Type
    {
        Invalid = 0,
        None,
        Int8,
        Int16,
        Int24,
        Int32,
        Int40,
        Int48,
        Int56,
        Int64,
        IntLarge,
        TrueType,
        FalseType,
        Float,
        Str,
        UStr,
        Pickle8S,
        Pickle = Pickle8S, // For WType only
        Pickle16S,
        Pickle32S,
        Pickle64S,
        MaxValue
    };

    // None
    static void serializeNoneWTypeWSize(Buffer& buffer, const boost::python::object& obj)
    {
        buffer.putByte((uint8_t)(Type::None));
    }

    static boost::python::object deserializeNoneWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return boost::python::object();
    }

    static void serializeNoneWType(Buffer& buffer, const boost::python::object& obj)
    {
        serializeNoneWTypeWSize(buffer, obj);
    }

    static boost::python::object deserializeNoneWType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeNoneWTypeWSize(data, end);
    }

    // Long
    struct PyBufferGuard
        : Py_buffer
    {
        ~PyBufferGuard()
        {
            PyBuffer_Release(this);
        }
    };

    static const boost::python::dict& toBytesKWArgs()
    {
        static boost::python::dict kw;
        static bool initialized = false;
        if (!initialized) {
            kw["signed"] = true;
            kw["byteorder"] = "big";
            initialized = true;
        }
        return kw;
    }

    static void serializeLongWTypeWSize(Buffer& buffer, const boost::python::object& obj)
    {
        serializeLongWTypeImpl(buffer, obj, true);
    }

    static void serializeLongWTypeImpl(Buffer& buffer, const boost::python::object& obj, bool wSize)
    {
        auto byteLen = (obj.attr("bit_length")() + 7 + 1) >> 3;
        if (byteLen == 0) {
            assert(obj == 0);
            byteLen = boost::python::object(1);
        }
        auto bytes = obj.attr("to_bytes")(*boost::python::make_tuple(byteLen), **toBytesKWArgs());

        PyBufferGuard view;
        PyObject_GetBuffer(bytes.ptr(), &view, PyBUF_SIMPLE);
        static Type typeTbl[] = {Type::MaxValue, Type::Int8, Type::Int16, Type::Int24, Type::Int32,
                                            Type::Int40, Type::Int48, Type::Int56, Type::Int64 };
        if (view.len <= 8) {
            buffer.putByte((uint8_t)typeTbl[view.len]);
        } else {
            buffer.putByte((uint8_t)Type::IntLarge);
            if (wSize) {
                uint32_t sz = static_cast<uint32_t>(view.len); // Not supporting integers with length greater than MAX_UINT bytes
                if (view.len > sz) {
                    throw db::Error("Value too latge");
                }
                boost::endian::big_uint32_buf_t size(sz);
                buffer.putMem(&size, sizeof(size));
            }
        }
        buffer.putMem(view.buf, view.len);
    }

    template <typename EndianBuff>
    static boost::python::object deserializeIntNWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        if ((end - data) < sizeof(EndianBuff)) {
            throwCorruptedBufferError();
        }
        EndianBuff buff;
        buff = *(EndianBuff*)data;
        data += sizeof(EndianBuff);
        return pyutils::createIntObject(buff.value());
    }

    static boost::python::object deserializeInt8WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSize<boost::endian::big_int8_buf_t>(data, end);
    }

    static boost::python::object deserializeInt16WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSize<boost::endian::big_int16_buf_t>(data, end);
    }

    template <typename EndianBuff, unsigned size>
    static boost::python::object deserializeIntNWTypeWSizeNoCompleteWord(const uint8_t*& data, const uint8_t* end)
    {
        if ((end - data) < size) {
            throwCorruptedBufferError();
        }
        EndianBuff buff;
        // Check most significant bit
        if (*data & 0x80) {
            // number is signed. Fill empty bytes with 0xFF
            buff = EndianBuff(-1);
        } else {
            buff = EndianBuff(0);
        }
        int8_t* pBuff = (int8_t*)&buff;
        memcpy(pBuff + sizeof(EndianBuff) - size, data, size);

        data += size;
        return pyutils::createIntObject(buff.value());
    }

    static boost::python::object deserializeInt24WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSizeNoCompleteWord<boost::endian::big_int32_buf_t, 3>(data, end);
    }

    static boost::python::object deserializeInt32WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSize<boost::endian::big_int32_buf_t>(data, end);
    }

    static boost::python::object deserializeInt40WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSizeNoCompleteWord<boost::endian::big_int64_buf_t, 5>(data, end);
    }

    static boost::python::object deserializeInt48WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSizeNoCompleteWord<boost::endian::big_int64_buf_t, 6>(data, end);
    }

    static boost::python::object deserializeInt56WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSizeNoCompleteWord<boost::endian::big_int64_buf_t, 7>(data, end);
    }

    static boost::python::object deserializeInt64WTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeIntNWTypeWSize<boost::endian::big_int64_buf_t>(data, end);
    }

    static boost::python::object deserializeIntLargeWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        if ((end - data) < 5) {
            throwCorruptedBufferError();
        }
        boost::endian::big_uint32_buf_t size = *(boost::endian::big_uint32_buf_t*)data;
        data += sizeof(boost::endian::big_uint32_buf_t);
        if ((end - data) < size.value()) {
            throwCorruptedBufferError();
        }
       
        boost::python::object memoryView(boost::python::handle<>(PyMemoryView_FromMemory((char*)data, size.value(), PyBUF_READ)));
        boost::python::object res(0);
        res = res.attr("from_bytes")(*boost::python::make_tuple(memoryView), **toBytesKWArgs());
        data += size.value();
        return res;
    }

    static void serializeLongWType(Buffer& buffer, const boost::python::object& obj)
    {
        serializeLongWTypeImpl(buffer, obj, false);
    }

    static boost::python::object deserializeIntLargeWType(const uint8_t* data, const uint8_t* end)
    {
        boost::python::object memoryView(boost::python::handle<>(PyMemoryView_FromMemory((char*)data, end - data, PyBUF_READ)));
        boost::python::object res(0);
        res = res.attr("from_bytes")(*boost::python::make_tuple(memoryView), **toBytesKWArgs());
        return res;
    }

    static boost::python::object deserializeInt8WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt8WTypeWSize(data, end);
    }

    static boost::python::object deserializeInt16WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt16WTypeWSize(data, end);
    }

    static boost::python::object deserializeInt24WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt24WTypeWSize(data, end);
    }

    static boost::python::object deserializeInt32WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt32WTypeWSize(data, end);
    }

    static boost::python::object deserializeInt40WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt40WTypeWSize(data, end);
    }

    static boost::python::object deserializeInt48WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt48WTypeWSize(data, end);
    }

    static boost::python::object deserializeInt56WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt56WTypeWSize(data, end);
    }

    static boost::python::object deserializeInt64WType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeInt64WTypeWSize(data, end);
    }

    // Bool
    static void serializeBoolWTypeWSize(Buffer& buffer, const boost::python::object& obj)
    {
        buffer.putByte((uint8_t)(obj ? Type::TrueType : Type::FalseType));
        // Type can hold all info. No need to use additional memory
    }

    static boost::python::object deserializeTrueWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return boost::python::object(true);
    }

    static boost::python::object deserializeFalseWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        return boost::python::object(false);
    }

    static void serializeBoolWType(Buffer& buffer, const boost::python::object& obj)
    {
        serializeBoolWTypeWSize(buffer, obj);
    }

    static boost::python::object deserializeTrueWType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeTrueWTypeWSize(data, end);
    }

    static boost::python::object deserializeFalseWType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeFalseWTypeWSize(data, end);
    }

    // Float
    static void serializeFloatWTypeWSize(Buffer& buffer, const boost::python::object& obj)
    {
        boost::python::extract<double> extract(obj);
        assert(extract.check());
        buffer.putByte((uint8_t)(Type::Float));
        boost::endian::big_float64_buf_t val(extract);
        buffer.putMem(&val, sizeof(val));
    }

    static boost::python::object deserializeFloatWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        if ((end - data) < sizeof(boost::endian::big_float64_buf_t)) {
            throwCorruptedBufferError();
        }
        boost::endian::big_float64_buf_t res = *(boost::endian::big_float64_buf_t*)(data);
        data += sizeof(boost::endian::big_float64_buf_t);
        return boost::python::object(res.value());
    }

    static void serializeFloatWType(Buffer& buffer, const boost::python::object& obj)
    {
        serializeFloatWTypeWSize(buffer, obj);
    }

    static boost::python::object deserializeFloatWType(const uint8_t* data, const uint8_t* end)
    {
        return deserializeFloatWTypeWSize(data, end);
    }

    // Str
    static void serializeStrWTypeWSize(Buffer& buffer, const boost::python::object& obj)
    {
        PyObject* ob = obj.ptr();
        PyUnicode_READY(ob);
        int kind = PyUnicode_KIND(ob);

        Py_ssize_t len;

        switch (kind) {
        case PyUnicode_1BYTE_KIND:
            Py_UCS1* repr;
            repr = PyUnicode_1BYTE_DATA(ob);
            len = PyUnicode_GET_LENGTH(ob);
            if (len == strlen((const char*)repr)) {
                buffer.putByte((uint8_t)(Type::Str));
                if (len != 0) buffer.putMem(repr, len * sizeof(Py_UCS1));
                buffer.putByte((uint8_t)0);
                break;
            } // string contains 0s. Use unicode representation
        case PyUnicode_WCHAR_KIND: // Deprecated
        case PyUnicode_2BYTE_KIND:
        case PyUnicode_4BYTE_KIND:
        {
            boost::python::object brepr = boost::python::str(obj).encode("'utf-8'");
            PyBufferGuard view;
            PyObject_GetBuffer(brepr.ptr(), &view, PyBUF_SIMPLE);
            buffer.putByte((uint8_t)(Type::UStr));
            boost::endian::big_uint32_buf_t size(static_cast<int32_t>(view.len));
            buffer.putMem(&size, sizeof(size));
            buffer.putMem(view.buf, view.len);
            break;
        }
        default:
            assert(false);
        }
    }

    static boost::python::object deserializeStrWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        std::vector<char> str;
        str.reserve(32);
        while ((data != end) && (*data != 0)) {
            str.push_back(*data++);
        }

        if (data == end) {
            if ((end - data) < sizeof(double)) {
                throwCorruptedBufferError();
            }
        }
        assert(*data == 0);
        str.push_back(*data++);

        return boost::python::object((const char*)&str[0]);
    }

    static boost::python::object deserializeUStrWTypeWSize(const uint8_t*& data, const uint8_t* end)
    {
        if ((end - data) < sizeof(boost::endian::big_uint32_buf_t)) {
            throwCorruptedBufferError();
        }
        boost::endian::big_uint32_buf_t size = *(boost::endian::big_uint32_buf_t*)data;
        data += sizeof(boost::endian::big_uint32_buf_t);
        boost::python::object res(
            boost::python::handle<>(
                PyUnicode_FromStringAndSize((char*)data, size.value())));
        if ((uint32_t)(end - data) < size.value()) {
            throwCorruptedBufferError();
        }
        data += size.value();
        return res;
    }

    static void serializeStrWType(Buffer& buffer, const boost::python::object& obj)
    {
        PyObject* ob = obj.ptr();
        PyUnicode_READY(ob);
        int kind = PyUnicode_KIND(ob);

        Py_ssize_t len;

        switch (kind) {
        case PyUnicode_1BYTE_KIND:
            Py_UCS1* repr;
            repr = PyUnicode_1BYTE_DATA(ob);
            len = PyUnicode_GET_LENGTH(ob);
            buffer.putByte((uint8_t)(Type::Str));
            if (len != 0) buffer.putMem(repr, len * sizeof(Py_UCS1));
            break;
        case PyUnicode_WCHAR_KIND: // Deprecated
        case PyUnicode_2BYTE_KIND:
        case PyUnicode_4BYTE_KIND:
        {
            boost::python::object brepr = boost::python::str(obj).encode("'utf-8'");
            PyBufferGuard view;
            PyObject_GetBuffer(brepr.ptr(), &view, PyBUF_SIMPLE);
            buffer.putByte((uint8_t)(Type::UStr));
            buffer.putMem(view.buf, view.len);
            break;
        }
        default:
            assert(false);
        }
    }

    static boost::python::object deserializeStrWType(const uint8_t* data, const uint8_t* end)
    {
        boost::python::object res(
            boost::python::handle<>(
                PyUnicode_FromStringAndSize((char*)data, end - data)));
        return res;
    }

    static boost::python::object deserializeUStrWType(const uint8_t* data, const uint8_t* end)
    {
        boost::python::object res(
            boost::python::handle<>(
                PyUnicode_FromStringAndSize((char*)data, end - data)));
        return res;
    }

    /// Default (use pickle proto 5)
    static const unsigned pickleVer = 5;
    static void serializeWTypeWSizePickle(Buffer& buffer, const boost::python::object& obj)
    {
        boost::python::object res = pickle.dumps(obj, pickleVer);
        Py_buffer view;
        PyObject_GetBuffer(res.ptr(), &view, PyBUF_SIMPLE);
        if (view.len < std::numeric_limits<uint8_t>::max()) {
            buffer.putByte((uint8_t)Type::Pickle8S);
            boost::endian::big_int8_buf_t size(static_cast<int8_t>(view.len));
            buffer.putMem(&size, sizeof(size));
            buffer.putMem(view.buf, view.len);
        } else if (view.len < std::numeric_limits<uint16_t>::max()) {
            buffer.putByte((uint8_t)Type::Pickle16S);
            boost::endian::big_int16_buf_t size(static_cast<int16_t>(view.len));
            buffer.putMem(&size, sizeof(size));
            buffer.putMem(view.buf, view.len);
        } else if (view.len < std::numeric_limits<uint32_t>::max()) {
            buffer.putByte((uint8_t)Type::Pickle32S);
            boost::endian::big_int32_buf_t size(static_cast<int32_t>(view.len));
            buffer.putMem(&size, sizeof(size));
            buffer.putMem(view.buf, view.len);
        } else {
            buffer.putByte((uint8_t)Type::Pickle64S);
            boost::endian::big_int64_buf_t size(view.len);
            buffer.putMem(&size, sizeof(size));
            buffer.putMem(view.buf, view.len);
        }
    }

    template <typename EndianBuffer>
    static boost::python::object deserializeWTypeWSizePickle(const uint8_t*& data, const uint8_t* end)
    {
        if ((end - data) < sizeof(EndianBuffer)) {
            throwCorruptedBufferError();
        }
        EndianBuffer size = *(EndianBuffer*)data;
        data += sizeof(EndianBuffer);

        boost::python::object memoryView(boost::python::handle<>(PyMemoryView_FromMemory((char*)data, size.value(), PyBUF_READ)));
        data += size.value();
        return pickle.loads(memoryView);
    }

    static boost::python::object deserializeWTypeWSizePickle8S(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeWTypeWSizePickle<boost::endian::big_uint8_buf_t>(data, end);
    }

    static boost::python::object deserializeWTypeWSizePickle16S(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeWTypeWSizePickle<boost::endian::big_uint16_buf_t>(data, end);
    }

    static boost::python::object deserializeWTypeWSizePickle32S(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeWTypeWSizePickle<boost::endian::big_uint32_buf_t>(data, end);
    }

    static boost::python::object deserializeWTypeWSizePickle64S(const uint8_t*& data, const uint8_t* end)
    {
        return deserializeWTypeWSizePickle<boost::endian::big_uint64_buf_t>(data, end);
    }

    static void serializeWTypePickle(Buffer& buffer, const boost::python::object& obj)
    {
        boost::python::object res = pickle.dumps(obj, pickleVer);
        Py_buffer view;
        PyObject_GetBuffer(res.ptr(), &view, PyBUF_SIMPLE);
        buffer.putByte((uint8_t)Type::Pickle);
        buffer.putMem(view.buf, view.len);
    }

    static boost::python::object deserializeWTypePickle(const uint8_t* data, const uint8_t* end)
    {
        boost::python::object memoryView(boost::python::handle<>(PyMemoryView_FromMemory((char*)data, end - data, PyBUF_READ)));
        return pickle.loads(memoryView);
    }

private:
    struct SerializeProcs
    {
        typedef decltype(&serializeNoneWTypeWSize) SerializeWTypeWSizeProc;
        typedef decltype(&serializeNoneWType) SerializeWTypeProc;
        
        SerializeWTypeWSizeProc serializeWTypeWSizeProc;
        SerializeWTypeProc serializeWTypeProc;

        SerializeProcs(SerializeWTypeWSizeProc serializeWTypeWSizeProc,
                       SerializeWTypeProc serializeWTypeProc)
            : serializeWTypeWSizeProc(serializeWTypeWSizeProc)
            , serializeWTypeProc(serializeWTypeProc)
        { }

        SerializeProcs() = default;
    };

    const SerializeProcs& findSerializeProcs(const boost::python::object& obj) const
    {
        auto pos = m_serializeProcsTable.find(obj.ptr()->ob_type);
        if (pos != m_serializeProcsTable.end()) {
            return (*pos).second;
        }
        return m_defaultSerializeProcs;
    }

private:
    std::unordered_map<PyTypeObject*, SerializeProcs> m_serializeProcsTable;
    SerializeProcs m_defaultSerializeProcs;

    typedef decltype(&deserializeNoneWTypeWSize) DeserializeWTypeWSizeProc;
    std::vector<DeserializeWTypeWSizeProc> m_deserializeWTypeWSizeProcTable;

    typedef decltype(&deserializeNoneWType) DeserializeWTypeProc;
    std::vector<DeserializeWTypeProc> m_deserializeWTypeProcTable;
};

std::unique_ptr<Serializer> Serializer::create(unsigned protocol)
{
    switch (protocol) {
    case 1:
        return std::unique_ptr<Serializer>(new SerializerV1());
    }
    return std::unique_ptr<Serializer>(nullptr);
}

void Serializer::throwCorruptedBufferError()
{
    throw db::Error("Corrupted database");
}

}
