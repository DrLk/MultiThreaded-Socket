#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "SpinLock.hpp"

namespace FastTransport::Protocol {

enum Settings {
    MaxSpeed = 0,
    MinSpeed = 1,
    Max = 2
};

class ConnectionContext {
public:
    using IntCallback = std::function<void(size_t)>;

    ConnectionContext();

    void SetInt(const Settings key, size_t value);
    size_t GetInt(const Settings key) const;

    class Subscriber {
    public:
        Subscriber(const std::shared_ptr<ConnectionContext>& context);
        ~Subscriber();

        virtual void OnSettingsChanged(const Settings key, size_t value) = 0;

    private:
        std::shared_ptr<ConnectionContext> _context;
    };

    void Subscribe(Subscriber& subscriber);
    void UnSubscribe(Subscriber& subscriber);

private:
    std::vector<std::atomic<size_t>> _intSettings;
    std::vector<std::reference_wrapper<Subscriber>> _intSubscribers;
    Thread::SpinLock _intSubscribersMutex;

    void NotifyIntSubscribers(const Settings key, size_t value);
};

} // namespace FastTransport::Protocol
