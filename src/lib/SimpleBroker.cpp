#include <yandex/contest/invoker/flowctl/interactive/SimpleBroker.hpp>

#include <yandex/contest/invoker/flowctl/interactive/BufferedConnection.hpp>

#include <yandex/contest/detail/LogHelper.hpp>
#include <yandex/contest/invoker/Notifier.hpp>

#include <boost/asio/steady_timer.hpp>
#include <boost/io/detail/quoted_manip.hpp>
#include <boost/optional.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    SimpleBroker::SimpleBroker(const Options &options): options_(options) {}

    namespace
    {
        struct TerminationTimer
        {
            explicit TerminationTimer(boost::asio::io_service &ioService):
                timer(ioService) {}

            template <typename Duration, typename F>
            void async_wait(const Duration &duration, const F &f)
            {
                boost::call_once(
                    [this, duration, f]()
                    {
                        timer.expires_from_now(duration);
                        timer.async_wait(f);
                    },
                    flag
                );
            }

            void cancel()
            {
                boost::call_once([](){}, flag);
                timer.cancel();
            }

            boost::asio::steady_timer timer;
            boost::once_flag flag;
        };
    }

    SimpleBroker::Status SimpleBroker::run()
    {
        boost::asio::io_service ioService;

        STREAM_INFO << "Using " << options_.solutionSourceFd << " " <<
                       "as solution source file descriptor";
        boost::asio::posix::stream_descriptor solutionSource(
            ioService,
            options_.solutionSourceFd
        );

        STREAM_INFO << "Using " << options_.solutionSinkFd << " " <<
                       "as solution sink file descriptor";
        boost::asio::posix::stream_descriptor solutionSink(
            ioService,
            options_.solutionSinkFd
        );

        STREAM_INFO << "Using " << options_.interactorSourceFd << " " <<
                       "as interactor source file descriptor";
        boost::asio::posix::stream_descriptor interactorSource(
            ioService,
            options_.interactorSourceFd
        );

        STREAM_INFO << "Using " << options_.interactorSinkFd << " " <<
                       "as interactor sink file descriptor";
        boost::asio::posix::stream_descriptor interactorSink(
            ioService,
            options_.interactorSinkFd
        );

        BufferedConnection connection(
            interactorSource,
            interactorSink,
            solutionSource,
            solutionSink,
            options_.outputLimitBytes
        );

        if (options_.dumpJudge)
        {
            STREAM_INFO << "Dumping judge's output into " <<
                           *options_.dumpJudge;
            connection.setDumpJudge(*options_.dumpJudge);
        }

        if (options_.dumpSolution)
        {
            STREAM_INFO << "Dumping solution's output into " <<
                           *options_.dumpSolution;
            connection.setDumpSolution(*options_.dumpSolution);
        }

        STREAM_INFO << "Using " << options_.notifierFd << " " <<
                       "as notifier file descriptor";
        Notifier notifier(ioService, options_.notifierFd);

        TerminationTimer interactorTerminationTimer(ioService);
        TerminationTimer solutioninTerminationTimer(ioService);

        boost::mutex statusLock;
        boost::optional<Status> status;

        typedef boost::signals2::signal<
            void (const yandex::contest::invoker::process::Result &)
        > TerminationSignal;

        TerminationSignal interactorTermination;
        TerminationSignal solutionTermination;

        boost::optional<process::Result> interactorResult;
        boost::optional<process::Result> solutionResult;

        bool solutionExcessData = false;

        const auto result =
            [&](const Status status_)
            {
                boost::lock_guard<boost::mutex> lk(statusLock);
                if (!status)
                {
                    notifier.close();
                    status = status_;
                }
            };

        const auto waitForInteractorTermination =
            [&]()
            {
                STREAM_INFO << "Waiting for interactor's termination...";
                interactorTerminationTimer.async_wait(
                    options_.terminationRealTimeLimit,
                    [&](const boost::system::error_code &ec)
                    {
                        if (ec == boost::asio::error::operation_aborted)
                            return;
                        result(INTERACTOR_TERMINATION_REAL_TIME_LIMIT_EXCEEDED);
                    });
            };

        const auto waitForSolutionTermination =
            [&]()
            {
                STREAM_INFO << "Waiting for solution's termination";
                solutioninTerminationTimer.async_wait(
                    options_.terminationRealTimeLimit,
                    [&](const boost::system::error_code &ec)
                    {
                        if (ec == boost::asio::error::operation_aborted)
                            return;
                        result(SOLUTION_TERMINATION_REAL_TIME_LIMIT_EXCEEDED);
                    });
            };

        notifier.onError(
            [&](const Notifier::Error::Event &event)
            {
                if (event.errorCode ==
                    boost::asio::error::operation_aborted)
                {
                    STREAM_INFO << "Notification was canceled";
                    return;
                }

                STREAM_ERROR << "Notification failure: " <<
                                event.errorCode.message();
                result(FAILED);
            });

        notifier.onSpawn(
            [&](const Notifier::Spawn::Event &event)
            {
                STREAM_INFO << event.meta.name << ":" <<
                               event.meta.id << " has spawned";
            });

        notifier.onTermination(
            [&](const Notifier::Termination::Event &event)
            {
                STREAM_INFO << boost::io::quoted(event.meta.name) <<
                               " has terminated";
                if (event.meta.name == "interactor")
                    interactorTermination(event.result);
                else if (event.meta.name == "solution")
                    solutionTermination(event.result);
            });

        connection.interactorEof.connect(waitForInteractorTermination);
        connection.solutionEof.connect(waitForSolutionTermination);

        interactorTermination.connect(
            [&](const yandex::contest::invoker::process::Result &processResult)
            {
                BOOST_ASSERT(!interactorResult);
                interactorResult = processResult;
                interactorTerminationTimer.cancel();
                if (processResult)
                {
                    STREAM_INFO << "Interactor has terminated OK";
                    if (solutionExcessData)
                    {
                        result(SOLUTION_EXCESS_DATA);
                        return;
                    }
                    STREAM_INFO << "Closing solution's STDIN";
                    connection.closeInteractorToSolution();
                }
                else if (processResult.exitStatus)
                {
                    STREAM_INFO << "Interactor has terminated not OK, " <<
                                   "solution's STDIN left intact";
                }
                else
                {
                    STREAM_INFO << "Interactor has failed to exit";
                    result(FAILED);
                    return;
                }
                waitForSolutionTermination();

                if (solutionResult)
                    result(OK);
            });

        solutionTermination.connect(
            [&](const yandex::contest::invoker::process::Result &processResult)
            {
                BOOST_ASSERT(!solutionResult);
                solutionResult = processResult;
                solutioninTerminationTimer.cancel();

                if (interactorResult)
                    result(OK);
            });

        connection.interactorOutputLimitExceeded.connect([&]{
            result(INTERACTOR_OUTPUT_LIMIT_EXCEEDED);
        });

        connection.solutionOutputLimitExceeded.connect([&]{
            result(SOLUTION_OUTPUT_LIMIT_EXCEEDED);
        });

        connection.interactorReadError.connect(
            [&](const boost::system::error_code &ec,
                const std::size_t /*size*/)
            {
                STREAM_ERROR << "Interactor read failure: " << ec.message();
                result(FAILED);
            });

        connection.interactorWriteError.connect(
            [&](const boost::system::error_code &ec,
                const std::size_t /*size*/)
            {
                STREAM_ERROR << "Interactor write failure: " << ec.message();
                if (ec == boost::asio::error::broken_pipe)
                {
                    if (interactorResult)
                    {
                        if (*interactorResult)
                            result(SOLUTION_EXCESS_DATA);
                    }
                    else
                    {
                        solutionExcessData = true;
                        waitForInteractorTermination();
                    }
                }
                else
                {
                    result(FAILED);
                }
            });

        connection.solutionReadError.connect(
            [&](const boost::system::error_code &ec,
                const std::size_t /*size*/)
            {
                STREAM_ERROR << "Solution read failure: " << ec.message();
                result(FAILED);
            });

        connection.solutionWriteError.connect(
            [&](const boost::system::error_code &ec,
                const std::size_t /*size*/)
            {
                STREAM_INFO << "Solution write failure: " << ec.message();
            });

        STREAM_INFO << "Starting connection";
        connection.start();

        STREAM_INFO << "Starting notifier";
        notifier.start();

        STREAM_INFO << "Starting execution loop";
        ioService.run();
        STREAM_INFO << "Execution loop has finished";

        // The only possible way to stop execution loop
        // is to call result(), so status is set.
        BOOST_ASSERT(status);

        if (*status == OK)
        {
            BOOST_ASSERT(interactorResult);
            BOOST_ASSERT(solutionResult);
        }

        STREAM_INFO << "Completed with status = " << *status;

        return *status;
    }
}}}}}
