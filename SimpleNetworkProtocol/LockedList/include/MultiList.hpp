#pragma once

#include <list>

namespace FastTransport::Containers {
template <class T>
class MultiList {
public:
    class Iterator {
        friend MultiList;

    public:
        explicit Iterator(MultiList<T>* container);
        Iterator(MultiList<T>* container, const typename std::list<std::list<T>>::iterator& it1);

        Iterator& operator++(); // NOLINT(fuchsia-overloaded-operator)
        bool operator==(const Iterator& other) const; // NOLINT(fuchsia-overloaded-operator)
        bool operator!=(const Iterator& other) const; // NOLINT(fuchsia-overloaded-operator)
        T& operator*(); // NOLINT(fuchsia-overloaded-operator)

    private:
        typename std::list<std::list<T>>::iterator _it1;
        typename std::list<T>::iterator _it2;

        const MultiList<T>* _container;
    };

    MultiList() = default;
    virtual ~MultiList() = default;

    MultiList(const MultiList& that) = delete;
    MultiList(MultiList&& that) noexcept;
    MultiList& operator=(const MultiList& that) = delete;
    MultiList& operator=(MultiList&& that) noexcept;

    MultiList TryGenerate(std::size_t size);

    void push_back(T&& element);
    void splice(MultiList<T>&& that);
    [[nodiscard]] bool empty() const noexcept;
    T& back();
    void pop_back();
    [[nodiscard]] std::size_t size() const noexcept;

    Iterator begin();
    Iterator end();
    void erase(const Iterator& that);
    void swap(MultiList& that) noexcept;

private:
    std::list<std::list<T>> _lists;

    static constexpr int Size = 10;
};

template <class T>
MultiList<T>::Iterator::Iterator(MultiList<T>* container)
    : _container(container)
    , _it1(container->_lists.begin())
{
    if (_it1 != container->_lists.end()) {
        _it2 = _it1->begin();
    }
}

template <class T>
MultiList<T>::Iterator::Iterator(MultiList* container, const typename std::list<std::list<T>>::iterator& it1)
    : _container(container)
    , _it1(it1)
    , _it2()
{
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
typename MultiList<T>::Iterator& MultiList<T>::Iterator::operator++()
{
    _it2++;
    if (_it2 == _it1->end()) {
        _it1++;
        if (_it1 == _container->_lists.end()) {
            typename std::list<T>::iterator defaultIterator {};
            _it2 = defaultIterator;
        } else {
            _it2 = _it1->begin();
        }
    }

    return *this;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
bool MultiList<T>::Iterator::operator==(const Iterator& other) const
{
    return _it1 == other._it1 && _it2 == other._it2;
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
bool MultiList<T>::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

template <class T> // NOLINT(fuchsia-overloaded-operator)
T& MultiList<T>::Iterator::operator*()
{
    return *_it2;
}

template <class T>
MultiList<T>::MultiList(MultiList&& that) noexcept
    : _lists(std::move(that._lists))
{
}

template <class T>
MultiList<T>& MultiList<T>::operator=(MultiList&& that) noexcept
{
    _lists = std::move(that._lists);
    return *this;
}

template <class T>
MultiList<T> MultiList<T>::TryGenerate(std::size_t size)
{
    MultiList list;

    while (!_lists.empty() && size) {
        auto& front = _lists.front();
        auto frontSize = front.size();
        if (frontSize > size) {
            list._lists.emplace_back(0);

            auto it = front.begin();
            while (size--) {
                it++;
            }
            list._lists.back().splice(list._lists.back().end(), front, front.begin(), it);
            break;

        } else {

            list._lists.push_back(std::move(front));
            _lists.pop_front();

            size -= frontSize;
        }
    }

    return list;
}

template <class T>
void MultiList<T>::push_back(T&& element)
{
    if (_lists.empty()) {
        _lists.emplace_back(0);
    }

    auto& list = _lists.back();
    if (list.size() < Size) {
        list.push_back(std::move(element));
    } else {
        _lists.emplace_back(0);
        _lists.back().push_back(std::move(element));
    }
}

template <class T>
void MultiList<T>::splice(MultiList<T>&& that)
{
    if (!_lists.empty() && !that._lists.empty()) {
        auto& list = _lists.back();
        auto& thatList = that._lists.front();
        auto newListSize = list.size() + thatList.size();
        if (newListSize < Size) {
            list.splice(list.end(), std::move(thatList));
            that._lists.pop_front();
        }
    }
    _lists.splice(_lists.end(), std::move(that._lists));
}

template <class T>
[[nodiscard]] bool MultiList<T>::empty() const noexcept
{
    return _lists.empty();
}

template <class T>
T& MultiList<T>::back()
{
    return _lists.back().back();
}

template <class T>
void MultiList<T>::pop_back()
{
    _lists.back().pop_back();

    if (_lists.back().empty()) {
        _lists.pop_back();
    }
}

template <class T>
[[nodiscard]] std::size_t MultiList<T>::size() const noexcept
{
    std::size_t size = 0;
    for (const auto& it1 : _lists) {
        size += it1.size();
    }

    return size;
}

template <class T>
MultiList<T>::Iterator MultiList<T>::begin()
{
    return Iterator(this);
}

template <class T>
MultiList<T>::Iterator MultiList<T>::end()
{
    return Iterator(this, _lists.end());
}

template <class T>
void MultiList<T>::erase(const Iterator& that)
{
    that._it1->erase(that._it2);
    if (that._it1->empty()) {
        _lists.erase(that._it1);
    }
}

template <class T>
void MultiList<T>::swap(MultiList& that) noexcept
{
    _lists.swap(that._lists);
}

} // namespace FastTransport::Containers
