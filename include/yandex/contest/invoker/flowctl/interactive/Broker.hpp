#pragma once

#include <bunsan/stream_enum.hpp>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <string>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    class Broker: private boost::noncopyable
    {
    public:
        struct Options
        {
            std::string requestDelimiter;
            std::string responseDelimiter;

            int interactorBrokerFd = -1;
            int brokerInteractorFd = -1;
            int solutionBrokerFd = -1;
            int brokerSolutionFd = -1;

            bool emptyFirstRequest = false;
        };

        BUNSAN_INCLASS_STREAM_ENUM(Status,
        (
            OK
        ))

        explicit Broker(const Options &options);

        Status run();

    private:
        Options options_;
    };
}}}}}
