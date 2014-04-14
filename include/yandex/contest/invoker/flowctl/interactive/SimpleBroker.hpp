#pragma once

#include <bunsan/stream_enum.hpp>

#include <boost/noncopyable.hpp>

#include <chrono>
#include <string>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    class SimpleBroker: private boost::noncopyable
    {
    public:
        struct Options
        {
            int interactorBrokerFd = -1;
            int brokerInteractorFd = -1;
            int solutionBrokerFd = -1;
            int brokerSolutionFd = -1;

            std::size_t outputLimitBytes = 0;
            std::chrono::milliseconds terminationRealTimeLimit{0};
        };

        BUNSAN_INCLASS_STREAM_ENUM_INITIALIZED(Status,
        (
            (OK, 0),

            (INTERACTOR_OUTPUT_LIMIT_EXCEEDED, 100),
            (SOLUTION_OUTPUT_LIMIT_EXCEEDED, 101),

            (INTERACTOR_TERMINATION_REAL_TIME_LIMIT_EXCEEDED, 102),
            (SOLUTION_TERMINATION_REAL_TIME_LIMIT_EXCEEDED, 103),

            (INTERACTOR_EXCESS_DATA, 104),
            (SOLUTION_EXCESS_DATA, 105)
        ))

        explicit SimpleBroker(const Options &options);

        Status run();

    private:
        Options options_;
    };
}}}}}