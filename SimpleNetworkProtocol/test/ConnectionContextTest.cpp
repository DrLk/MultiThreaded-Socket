#include "ConnectionContext.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <memory>

namespace FastTransport::Protocol {
TEST(ConnectionContextTest, ConnectionContextGetSet)
{
    ConnectionContext context;

    context.SetInt(Settings::MaxSpeed, 100);
    EXPECT_EQ(context.GetInt(Settings::MaxSpeed), 100);

    context.SetInt(Settings::MaxSpeed, 200);
    EXPECT_EQ(context.GetInt(Settings::MaxSpeed), 200);

    context.SetInt(Settings::MinSpeed, 1);
    EXPECT_EQ(context.GetInt(Settings::MinSpeed), 1);

    context.SetInt(Settings::MinSpeed, 50);
    EXPECT_EQ(context.GetInt(Settings::MinSpeed), 50);
}

TEST(ConnectionContextTest, ConnectionContextSubscribeUnsubscribe)
{
    auto context = std::make_shared<ConnectionContext>();

    class MockSubscriber : public ConnectionContext::Subscriber {
    public:
        explicit MockSubscriber(const std::shared_ptr<ConnectionContext>& context)
            : ConnectionContext::Subscriber(context)
        {
        }

        MockSubscriber(const MockSubscriber&) = delete;
        MockSubscriber(MockSubscriber&&) = delete;
        MockSubscriber& operator=(const MockSubscriber&) = delete;
        MockSubscriber& operator=(MockSubscriber&&) = delete;
        virtual ~MockSubscriber() = default;
        MOCK_METHOD(void, OnSettingsChanged, (const Settings, size_t));
    };

    MockSubscriber subscriber(context);
    EXPECT_CALL(subscriber, OnSettingsChanged(Settings::MaxSpeed, std::numeric_limits<size_t>::max()));
    EXPECT_CALL(subscriber, OnSettingsChanged(Settings::MinSpeed, std::numeric_limits<size_t>::min()));
    context->Subscribe(subscriber);

    EXPECT_CALL(subscriber, OnSettingsChanged(Settings::MinSpeed, 10));
    context->SetInt(Settings::MinSpeed, 10);
    EXPECT_EQ(context->GetInt(Settings::MinSpeed), 10);

    EXPECT_CALL(subscriber, OnSettingsChanged(Settings::MaxSpeed, 50));
    context->SetInt(Settings::MaxSpeed, 50);
    EXPECT_EQ(context->GetInt(Settings::MaxSpeed), 50);

    context->UnSubscribe(subscriber);
    context->UnSubscribe(subscriber);
    context->SetInt(Settings::MaxSpeed, 1000);
    EXPECT_EQ(context->GetInt(Settings::MaxSpeed), 1000);
}

} // namespace FastTransport::Protocol
