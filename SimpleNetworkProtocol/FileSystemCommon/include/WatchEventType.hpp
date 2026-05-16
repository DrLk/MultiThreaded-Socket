#pragma once

namespace FastTransport::FileSystem {

// Wire-protocol enum shared between the server (which produces these from
// inotify events) and the client (which receives them via the network as
// NotifyInvalEntry/NotifyInvalInode messages). Keep additions append-only.
enum class WatchEventType {
    Modified,
    Created,
    Deleted,
    MovedFrom,
    MovedTo,
};

} // namespace FastTransport::FileSystem
