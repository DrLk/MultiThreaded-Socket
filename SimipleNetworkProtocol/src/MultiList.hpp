#pragma once

#include <list>

namespace FastTransport::Containers {
template <class T>
class MultiList {
public:
    MultiList() = default;

    MultiList(const MultiList& that) = delete;
    MultiList& operator=(const MultiList& that) = delete;
    MultiList(MultiList&& that) noexcept
        : _lists(std::move(that._lists))
    {
    }

    ~MultiList() = default;

    MultiList TryGenerate(size_t size)
    {
        MultiList list;

        while (!_lists.empty()) {
            auto& front = _lists.front();
            auto frontSize = front.size();
            list._lists.push_back(std::move(front));
            _lists.pop_front();

            if (frontSize >= size)
                break;

            size -= frontSize;
        }

        return list;
    }

    void push_back(T&& element)
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

    void splice(MultiList<T>&& that)
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

    MultiList& operator=(MultiList&& that) noexcept
    {
        _lists = std::move(that._lists);
        return *this;
    }

    [[nodiscard]] bool empty() const
    {
        return _lists.empty();
    }

    T& back()
    {
        return _lists.back().back();
    }

    void pop_back()
    {
        _lists.back().pop_back();

        if (_lists.back().empty()) {
            _lists.pop_back();
        }
    }

    [[nodiscard]] size_t size() const
    {
        size_t size = 0;
        for (const auto& it1 : _lists) {
            size += it1.size();
        }

        return size;
    }

    class Iterator {
        friend MultiList;

    public:
        explicit Iterator(MultiList<T>& container)
            : _container(container)
            , _it1(container._lists.begin())
        {
            if (_it1 != container._lists.end()) {
                _it2 = _it1->begin();
            }
        }

        // Iterator(MultiList<T>& container, const std::list<std::list<T>>::iterator& it1) : _container(container), _it1(it1)
        Iterator(MultiList<T>& container, typename std::list<std::list<T>>::iterator it1)
            : _container(container)
            , _it1(it1)
            , _it2()
        {
        }

        Iterator& operator++() // NOLINT(fuchsia-overloaded-operator)
        {
            _it2++;
            if (_it2 == _it1->end()) {
                _it1++;
                if (_it1 == _container._lists.end()) {
                    typename std::list<T>::iterator defaultIterator;
                    _it2 = defaultIterator;
                } else {
                    _it2 = _it1->begin();
                }
            }

            return *this;
        }

        Iterator operator++(int) // NOLINT(fuchsia-overloaded-operator)
        {
            Iterator retval = *this;
            ++(*this);
            return retval;
        }

        bool operator==(const Iterator& other) // NOLINT(fuchsia-overloaded-operator)
        {
            return _it1 == other._it1 && _it2 == other._it2;
        }

        bool operator!=(const Iterator& other) // NOLINT(fuchsia-overloaded-operator)
        {
            return !(*this == other);
        }

        T& operator*() // NOLINT(fuchsia-overloaded-operator)
        {
            return *_it2;
        }

    private:
        typename std::list<std::list<T>>::iterator _it1;
        typename std::list<T>::iterator _it2;

        MultiList<T>& _container;
    };

    Iterator begin()
    {
        // cosnt MultiList::Iterator& a = MultiList::Iterator(*this);
        return Iterator(*this);
    }

    Iterator end()
    {
        return Iterator(*this, _lists.end());
    }

    void erase(const Iterator& it)
    {
        it._it1->erase(it._it2);
        if (it._it1->empty()) {
            _lists.erase(it._it1);
        }
    }

private:
    std::list<std::list<T>> _lists;

    const int Size = 10;
};
} // namespace FastTransport::Containers
