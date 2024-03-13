#pragma once

#include <string_view>

namespace FastTransport::Protocol {
void SetThreadName(std::string_view name);
} // namespace FastTransport::Protocol