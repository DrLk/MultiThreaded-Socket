#include "pch.hpp"
#include "Counter.hpp"

#include <utility>

#include "Logger.hpp"

namespace FastTransport::Performance {

std::unordered_map<std::string, Counter&> Counter::Counters; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, fuchsia-statically-constructed-objects)

Counter::Counter(std::string_view name)
    : _counter(0)
    , _name(name)
{
    Counters.insert({ _name, *this });
}

Counter::~Counter()
{
    Counters.erase(_name);
}

void Counter::Count()
{
    _counter++;
}

void Counter::Print()
{
    for (auto& counter : Counters) {
        LOGGER() << "[" << counter.first << "]: " << counter.second._counter;
    }
}

} // namespace FastTransport::Performance
