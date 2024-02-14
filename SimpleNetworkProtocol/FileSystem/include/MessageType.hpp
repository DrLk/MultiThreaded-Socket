#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "ByteStream.hpp"
#include "FileTree.hpp"
#include "StreamConcept.hpp"

enum class MessageType {
    None = 0,
    RequestTree = 1,
    ResponseTree = 2,
    RequestFile = 3,
    ResponseFile = 4,
    RequestFileBytes = 5,
    ResponseFileBytes = 6,
    RequestCloseFile = 7,
    ResponseCloseFile = 8,
};

using FileID = std::uint64_t;

struct Request {
    MessageType type;
    std::vector<std::byte> bytes;
};

struct RequestTree {
    std::string path;
};

struct RequestFile {
    std::u8string path;
    std::uint64_t offset;
    std::uint64_t size;
};

struct ResponseFile {
    std::u8string path;
    FileID fileId;
};

struct RequestFileBytes {
    FileID fileId;
    std::uint64_t offset;
    std::uint64_t size;
};

struct ResponseFileBytes {
    FileID fileId;
    std::uint64_t offset;
    std::uint64_t size;
    std::vector<std::byte> bytes;
};

struct RequestCloseFile {
    FileID fileId;
};

struct ResponseCloseFile {
    FileID fileId;
};

FastTransport::FileSystem::OutputByteStream& operator<<(FastTransport::FileSystem::OutputByteStream& stream, const RequestTree& message);

template <FastTransport::FileSystem::InputStream Stream>
FastTransport::FileSystem::InputByteStream<Stream>& operator>>(FastTransport::FileSystem::InputByteStream<Stream>& stream, RequestTree& message)
{
    stream >> message.path;
    return stream;
}

class Protocol {
public:
    template <FastTransport::FileSystem::InputStream Stream>
    Protocol(FastTransport::FileSystem::OutputByteStream& outStream, FastTransport::FileSystem::InputByteStream<Stream>& inputStream)
        : _outStream(outStream)
        , _inputStream(inputStream)
    {
    }

    /*void SendBytes(std::span<std::byte> bytes)
    {
        std::copy(bytes.begin(), bytes.end(), std::ostream_iterator<std::byte, std::byte>(outStream));
    }

    std::vector<std::byte> RecvBytes(std::size_t size)
    {
        std::vector<std::byte> result(size);
        inStream.get().read(result.data(), result.size());
        return result;
    }*/

    void Run()
    {
        FastTransport::FileSystem::FileTree tree = FastTransport::FileSystem::FileTree::GetTestFileTree();

        RequestTree request {
            .path = "/mnt/test"
        };

        _outStream << MessageType::RequestTree;
        _outStream << request;

        MessageType type;
        _inputStream.get() >> type;
        RequestTree newRequest;
        _inputStream.get() >> newRequest;

        _outStream << MessageType::ResponseTree;
        tree.Serialize(_outStream);

        _inputStream.get() >> type;
        FastTransport::FileSystem::FileTree newTree;
        newTree.Deserialize(_inputStream.get());
        return;

        while (true) {
            MessageType message;
            _inputStream.get() >> message;

            switch (message) {
            case MessageType::RequestTree: {
                RequestTree request;
                _inputStream.get() >> request;

                _outStream << MessageType::ResponseTree;
                tree.Serialize(_outStream);
                break;
            }
            case MessageType::ResponseTree: {
                FastTransport::FileSystem::FileTree newTree;
                newTree.Deserialize(_inputStream.get());
                break;
            }
            case MessageType::RequestFile: {
                break;
            }
            case MessageType::ResponseFile: {
                break;
            }
            default:
                throw std::runtime_error("Unknown message type");
            }
        }
    }

private:
    std::reference_wrapper<FastTransport::FileSystem::OutputByteStream> _outStream;
    std::reference_wrapper<FastTransport::FileSystem::InputByteStream<std::basic_stringstream<std::byte>>> _inputStream;
};
