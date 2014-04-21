#include <yandex/contest/invoker/flowctl/interactive/SimpleBroker.hpp>

#include <yandex/contest/invoker/flowctl/interactive/BufferedConnection.hpp>

#include <yandex/contest/detail/LogHelper.hpp>
#include <yandex/contest/invoker/Notifier.hpp>

#include <boost/asio/steady_timer.hpp>
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

        bool interactorTerminated = false;
        bool solutionTerminated = false;

        bool solutionExcessData = false;

        const auto result =
            [&](const Status status_)
            {
                boost::lock_guard<boost::mutex> lk(statusLock);
                if (!status)
                {
                    ioService.stop();
                    status = status_;
                }
            };

        const auto waitForInteractorTermination =
            [&]()
            {
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
                STREAM_INFO << event.meta.name << " has terminated";
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
                BOOST_ASSERT(!interactorTerminated);
                interactorTerminated = true;
                interactorTerminationTimer.cancel();
                if (processResult)
                {
                    if (solutionExcessData)
                    {
                        result(SOLUTION_EXCESS_DATA);
                        return;
                    }
                    connection.closeInteractorToSolution();
                }
                waitForSolutionTermination();

                if (solutionTerminated)
                    result(OK);
            });

        solutionTermination.connect(
            [&](const yandex::contest::invoker::process::Result &/*processResult*/)
            {
                BOOST_ASSERT(!solutionTerminated);
                solutionTerminated = true;
                solutioninTerminationTimer.cancel();

                if (interactorTerminated)
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
                    solutionExcessData = true;
                    waitForInteractorTermination();
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

        if (status == OK)
        {
            BOOST_ASSERT(interactorTerminated);
            BOOST_ASSERT(solutionTerminated);
        }

        return *status;
    }
}}}}}
