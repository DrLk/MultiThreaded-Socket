#pragma once

#include "ByteStream.hpp"
#include "Stream.hpp"

namespace FastTransport::Protocol {

template <FastTransport::FileSystem::InputStream InputStream, FastTransport::FileSystem::OutputStream OutputStream>
class NetworkStream : public TaskQueue::Stream {
public:
    NetworkStream(InputStream& reader, OutputStream& writer)
        : TaskQueue::Stream()
        , _reader(reader)
        , _writer(writer)
    {
    }
    TaskQueue::Stream& write(const void* data, std::size_t size) override
    {
        _writer.get().write(data, size);
        return *this;
    }
    TaskQueue::Stream& read(void* data, std::size_t size) override
    {
        _reader.get().read(data, size);
        return *this;
    }

    TaskQueue::Stream& flush() override
    {
        _writer.get().flush();
        return *this;
    }

private:
    std::reference_wrapper<InputStream> _reader;
    std::reference_wrapper<OutputStream> _writer;
};

} // namespace FastTransport::Protocol
