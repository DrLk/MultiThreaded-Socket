#include "ConnectionContext.hpp"

#include <limits>
#include <memory>
#include <mutex>

namespace FastTransport::Protocol {

ConnectionContext::ConnectionContext()
    : _intSettings(Settings::Max)
{
    _intSettings[Settings::MaxSpeed] = std::numeric_limits<size_t>::max();
    _intSettings[Settings::MinSpeed] = std::numeric_limits<size_t>::min();
}
void ConnectionContext::SetInt(const Settings key, size_t value)
{
    _intSettings[key].store(value, std::memory_order_relaxed);
    NotifyIntSubscribers(key, value);
}
size_t ConnectionContext::GetInt(const Settings key) const
{
    return _intSettings[key];
}

ConnectionContext::Subscriber::Subscriber(const std::shared_ptr<ConnectionContext>& context)
    : _context(context)
{
}

ConnectionContext::Subscriber::~Subscriber()
{
    _context->UnSubscribe(*this);
}

void ConnectionContext::Subscribe(Subscriber& subscriber)
{
    const std::scoped_lock lock(_intSubscribersMutex);
    _intSubscribers.emplace_back(subscriber);

    for (auto& setting : _intSettings) {
        subscriber.OnSettingsChanged(static_cast<Settings>(&setting - _intSettings.data()), setting);
    }
}

void ConnectionContext::UnSubscribe(Subscriber& subscriber)
{
    const std::scoped_lock lock(_intSubscribersMutex);
    std::erase_if(_intSubscribers, [&subscriber](const std::reference_wrapper<Subscriber>& value) {
        return &value.get() == &subscriber;
    });
}

void ConnectionContext::NotifyIntSubscribers(const Settings key, size_t value)
{
    const std::scoped_lock lock(_intSubscribersMutex);
    for (auto& subscriber : _intSubscribers) {
        subscriber.get().OnSettingsChanged(key, value);
    }
}
} // namespace FastTransport::Protocol
