#pragma once

#include <condition_variable>

#include "MultiList.hpp"
#include "SpinLock.hpp"

namespace FastTransport::Containers {

using namespace std::chrono_literals;

template <class T>
class LockedList final : public FastTransport::Containers::MultiList<T> {

    using Mutex = FastTransport::Protocol::SpinLock;

public:
    LockedList();
    LockedList(const LockedList& that) = delete;
    LockedList(LockedList&& that) noexcept;
    LockedList& operator=(const LockedList& that) = delete;
    LockedList& operator=(LockedList&& that) noexcept; // NOLINT(fuchsia-overloaded-operator)
    ~LockedList() override;

    template <class Predicate>
    bool Wait(std::unique_lock<Mutex>& lock, std::stop_token stop, Predicate&& predicate);
    template <class Predicate>
    bool WaitFor(std::unique_lock<Mutex>& lock, std::stop_token stop, Predicate&& predicate);
    void NotifyAll() noexcept;

    Mutex _mutex; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
private:
    std::condition_variable_any _condition {};
};

template <class T>
LockedList<T>::LockedList() = default;

template <class T>
LockedList<T>::LockedList(LockedList&& that) noexcept
    : FastTransport::Containers::MultiList<T>(std::move(that))
{
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
LockedList<T>& LockedList<T>::operator=(LockedList&& that) noexcept
{
    FastTransport::Containers::MultiList<T>::operator=(std::move(that));
    return *this;
}

template <class T>
LockedList<T>::~LockedList() = default;

template <class T>
template <class Predicate>
bool LockedList<T>::Wait(std::unique_lock<Mutex>& lock, std::stop_token stop, Predicate&& predicate)
{
    return _condition.wait(lock, stop, std::forward<Predicate>(predicate));
}

template <class T>
template <class Predicate>
bool LockedList<T>::WaitFor(std::unique_lock<Mutex>& lock, std::stop_token stop, Predicate&& predicate)
{
    return _condition.wait_for(lock, stop, 50ms, std::forward<Predicate>(predicate));
}

template <class T>
void LockedList<T>::NotifyAll() noexcept
{
    _condition.notify_all();
}
} // namespace FastTransport::Containers
