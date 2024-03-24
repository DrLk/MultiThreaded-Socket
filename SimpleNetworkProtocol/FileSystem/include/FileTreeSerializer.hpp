#pragma once

#include "ByteStream.hpp"

namespace FastTransport::FileSystem {

    template <InputStream Stream>
    void Deserialize(InputByteStream<Stream>& in)
    {
        _root.Deserialize(in, nullptr);
    }

    template <OutputStream Stream>
    void Serialize(OutputByteStream<Stream>& stream) const
    {
        //_root.Serialize(stream);
    }

} // namespace FastTransport::FileSystem
