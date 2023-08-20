#include "ThreadName.hpp"

#include <cassert>
#include <string_view>

#ifdef WIN32
#include "windows.h"
#include <processthreadsapi.h>
#include <stdexcept>
#else
#include <pthread.h>
#endif

namespace FastTransport::Protocol {

#ifdef WIN32
std::wstring DecodeStringFrom(const std::string_view& utf8EncodedStr)
{
    int status = 1;
    std::wstring result;

    int a = std::ssize(utf8EncodedStr);
    size_t length = utf8EncodedStr.size();
    if (length) {
        int lengthSigned = length == -1 ? (int)-1 : (int)length;
        const int charsRequiredWithNull = MultiByteToWideChar(CP_UTF8, 0, utf8EncodedStr.data(), lengthSigned, NULL, 0);

        result = std::wstring(charsRequiredWithNull - (lengthSigned == -1 ? 1 : 0), '?');

        status = MultiByteToWideChar(CP_UTF8, 0, utf8EncodedStr.data(), lengthSigned, result.data(), charsRequiredWithNull);
        if (!status) {
            throw std::runtime_error("Failed to MultiByteToWideChar");
        }
    }

    return result;
}

#endif

void SetThreadName(const std::string_view& name)
{
    assert(name.size() < 16); // posix limitation
#ifdef WIN32
    SetThreadDescription(GetCurrentThread(), DecodeStringFrom(name).c_str());
#elif defined(__APPLE__)
    pthread_setname_np(name.data());
#elif defined(FREEBSD)
    pthread_set_name_np(pthread_self(), name.data());
#else

    pthread_setname_np(pthread_self(), name.data());

#endif
}
} // namespace FastTransport::Protocol
