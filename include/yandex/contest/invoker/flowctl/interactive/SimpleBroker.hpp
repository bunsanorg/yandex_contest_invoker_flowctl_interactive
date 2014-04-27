#pragma once

#include <bunsan/stream_enum.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

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
            int notifierFd = -1;
            int interactorSourceFd = -1;
            int interactorSinkFd = -1;
            int solutionSourceFd = -1;
            int solutionSinkFd = -1;

            std::size_t outputLimitBytes = 0;
            std::chrono::milliseconds terminationRealTimeLimit{0};

            boost::optional<boost::filesystem::path> dumpJudge;
            boost::optional<boost::filesystem::path> dumpSolution;
        };

        BUNSAN_INCLASS_STREAM_ENUM_INITIALIZED(Status,
        (
            (OK, 0),
            (FAILED, 1),

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
