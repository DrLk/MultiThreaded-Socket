#pragma once

#include <type_traits>

namespace FastTransport::Protocol {

template <class T>
concept trivial = std::is_trivial_v<T>;
} // namespace FastTransport::Protocol
