#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "ByteStream.hpp"
#include "FileTree.hpp"

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

OutputByteStream& operator<<(OutputByteStream& stream, const RequestTree& message);
InputByteStream& operator>>(InputByteStream& stream, RequestTree& message);

class Protocol {
public:
    Protocol(OutputByteStream& outStream, InputByteStream& inputStream)
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
        _inputStream >> type;
        RequestTree newRequest;
        _inputStream >> newRequest;

        _outStream << MessageType::ResponseTree;
        tree.Serialize(_outStream);

        _inputStream >> type;
        FastTransport::FileSystem::FileTree newTree;
        newTree.Deserialize(_inputStream.get());
        int i = 0;
        i++;
        return;

        while (true) {
            MessageType message;
            _inputStream >> message;

            switch (message) {
            case MessageType::RequestTree: {
                RequestTree request;
                _inputStream >> request;

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
    std::reference_wrapper<OutputByteStream> _outStream;
    std::reference_wrapper<InputByteStream> _inputStream;
};
