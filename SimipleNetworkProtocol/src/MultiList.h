#pragma once

#include <list>

namespace FastTransport::Containers
{
    template<class T>
    class MultiList
    {
    public:
        void push_back(T&& element)
        {
            if (_lists.empty())
                _lists.emplace_back();

            auto& list = _lists.back();
            if (list.size() < Size)
                list.push_back(std::move(element));
            else
            {
                _lists.emplace_back();
                _lists.back().push_back(std::move(element));
            }
        }

        void splice(MultiList<T>&& that)
        {
            if (!_lists.empty() && !that._lists.empty())
            {
                auto& list = _lists.back();
                auto& thatList = that._lists.front();
                auto newListSize = list.size() + thatList.size();
                if (newListSize < Size)
                {
                    list.splice(list.end(), std::move(thatList));
                    thatList.pop_front();
                }
            }
            _lists.splice(_lists.end(), std::move(that._lists));

            const std::list< std::list<T> >::iterator& it = _lists.begin();
        }

        class Iterator
        {
        public:

            explicit Iterator(MultiList<T>& container) : _container(container)
            {
                _it1 = container._lists.begin();
                _it2 = _it1->begin();
            }

            //Iterator(MultiList<T>& container, const std::list<std::list<T>>::iterator& it1) : _container(container), _it1(it1)
            Iterator(MultiList<T>& container, typename std::list<std::list<T>>::iterator it1) : _container(container), _it1(it1), _it2()
            {

            }

            Iterator& operator++() {
                _it2++;
                if (_it2 == _it1->end())
                {
                    _it1++;
                    if (_it1 == _container._lists.end())
                    {
                        typename std::list<T>::iterator defaultIterator;
                        _it2 = defaultIterator;
                    }
                    else
                        _it2 = _it1->begin();
                }

                return *this;
            }

            Iterator operator++(int)
            {
                Iterator retval = *this; ++(*this); 
                return retval;
            }

            bool operator==(Iterator other)
            {
                return _it1 == other._it1 && _it2 == other._it2;
            }

            bool operator!=(Iterator other)
            {
                return !(*this == other);
            }

            T& operator*()
            {
                return *_it2;
            }
        private:
            std::list<std::list<T>>::iterator _it1;
            std::list<T>::iterator _it2;

            MultiList<T>& _container;
        };

        Iterator begin()
        {
            //cosnt MultiList::Iterator& a = MultiList::Iterator(*this);
             return Iterator(*this);
        }

        Iterator end()
        {
            return Iterator(*this, _lists.end());
        }

    private:
        std::list<std::list<T>> _lists;

        const int Size = 10;

    };
}