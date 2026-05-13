#pragma once

#include <functional>
#include <memory>

#include "FileTree.hpp"
#include "MainReadJob.hpp"
#include "MessageReader.hpp"

namespace FastTransport::TaskQueue {

class MergeIn : public MainReadJob {
    using MessageReader = Protocol::MessageReader;

public:
    static std::unique_ptr<MainReadJob> Create(FastTransport::FileSystem::FileTree& fileTree, MessageReader&& reader);

    MergeIn(FastTransport::FileSystem::FileTree& fileTree, MessageReader&& reader);

    void ExecuteMainRead(std::stop_token stop, ITaskScheduler& scheduler) override;

private:
    std::reference_wrapper<FastTransport::FileSystem::FileTree> _fileTree;
    MessageReader _reader;
};

} // namespace FastTransport::TaskQueue
