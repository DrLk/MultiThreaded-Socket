#pragma once

namespace FastTransport
{
    namespace Protocol
    {
        class BufferOwner;

        class FreeableBuffer : public std::enable_shared_from_this<FreeableBuffer>
        {
        public:
            FreeableBuffer()
            {
            }
            FreeableBuffer(const std::shared_ptr<FreeableBuffer>& buffer) : _buffer(buffer)
            {
            }
            FreeableBuffer(std::shared_ptr<FreeableBuffer>&& buffer) noexcept : _buffer(std::move(buffer))
            {
            }
            virtual ~FreeableBuffer()
            {
            }
        protected:
            std::shared_ptr<FreeableBuffer> _buffer;
        };
    }
}
