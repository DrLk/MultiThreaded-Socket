#pragma once

#include <condition_variable>
#include <mutex>

#include "MultiList.hpp"

namespace FastTransport::Containers {

using namespace std::chrono_literals;

template <class T>
class LockedList final : public FastTransport::Containers::MultiList<T> {
public:
    LockedList();
    LockedList(const LockedList& that) = delete;
    LockedList(LockedList&& that) noexcept;
    LockedList& operator=(const LockedList& that) = delete;
    LockedList& operator=(LockedList&& that) noexcept; // NOLINT(fuchsia-overloaded-operator)
    ~LockedList() override;

    template <class Predicate>
    bool Wait(std::unique_lock<std::mutex>& lock, Predicate&& predicate);
    void NotifyAll() noexcept;

    std::mutex _mutex; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes, misc-non-private-member-variables-in-classes)
private:
    std::condition_variable _condition {};
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
bool LockedList<T>::Wait(std::unique_lock<std::mutex>& lock, Predicate&& predicate)
{
    return _condition.wait_for(lock, 100ms, std::forward<Predicate>(predicate));
}

template <class T>
void LockedList<T>::NotifyAll() noexcept
{
    _condition.notify_all();
}
} // namespace FastTransport::Containers
