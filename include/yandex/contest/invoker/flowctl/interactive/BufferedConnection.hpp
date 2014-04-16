#pragma once

#include <bunsan/asio/buffer_connection.hpp>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/signals2.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    class BufferedConnection: private boost::noncopyable
    {
    public:
        typedef boost::asio::posix::stream_descriptor Connection;

        typedef bunsan::asio::buffer_connection<
            Connection,
            Connection
        > Buffer;

        typedef boost::signals2::signal<void ()> StaticEventSignal;

        typedef boost::signals2::signal<
            void (boost::system::error_code, std::size_t)
        > ErrorSignal;

    public:
        BufferedConnection(
            Connection &interactorSource,
            Connection &interactorSink,
            Connection &solutionSource,
            Connection &solutionSink,
            const std::size_t outputLimitBytes):
                interactorToSolutionBuffer_(
                    interactorSource,
                    solutionSink,
                    boost::bind(
                        &BufferedConnection::handle_interactor_read,
                        this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                    ),
                    boost::bind(
                        &BufferedConnection::handle_solution_write,
                        this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                    )
                ),
                solutionToInteractorBuffer_(
                    solutionSource,
                    interactorSink,
                    boost::bind(
                        &BufferedConnection::handle_solution_read,
                        this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                    ),
                    boost::bind(
                        &BufferedConnection::handle_interactor_write,
                        this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                    )
                ),
                outputLimitBytes_(outputLimitBytes)
        {
            interactorToSolutionBuffer_.set_close_sink_on_eof(false);
            interactorToSolutionBuffer_.set_discard_on_sink_error(true);
        }

        void start()
        {
            interactorToSolutionBuffer_.start();
            solutionToInteractorBuffer_.start();
        }

        void closeInteractorToSolution()
        {
            interactorToSolutionBuffer_.close();
        }

        void closeSolutionToInteractor()
        {
            solutionToInteractorBuffer_.close();
        }

        void close()
        {
            closeInteractorToSolution();
            closeSolutionToInteractor();
        }

        void terminateInteractorToSolution()
        {
            interactorToSolutionBuffer_.terminate();
        }

        void terminateSolutionToInteractor()
        {
            solutionToInteractorBuffer_.terminate();
        }

        void terminate()
        {
            terminateInteractorToSolution();
            terminateSolutionToInteractor();
        }

        StaticEventSignal interactorEof;
        StaticEventSignal solutionEof;

        StaticEventSignal interactorOutputLimitExceeded;
        StaticEventSignal solutionOutputLimitExceeded;

        ErrorSignal interactorReadError;
        ErrorSignal interactorWriteError;
        ErrorSignal solutionReadError;
        ErrorSignal solutionWriteError;

    private:
        void handle_interactor_read(
            const boost::system::error_code &ec, const std::size_t size)
        {
            interactorOutputBytes_ += size;
            if (interactorOutputBytes_ > outputLimitBytes_)
            {
                interactorOutputLimitExceeded();
                terminate();
            }

            if (ec)
            {
                if (ec == boost::asio::error::eof)
                    interactorEof();
                else
                    interactorReadError(ec, size);
            }
        }

        void handle_interactor_write(
            const boost::system::error_code &ec, const std::size_t size)
        {
            if (ec)
            {
                interactorWriteError(ec, size);
            }
        }

        void handle_solution_read(
            const boost::system::error_code &ec, const std::size_t size)
        {
            solutionOutputBytes_ += size;
            if (solutionOutputBytes_ > outputLimitBytes_)
            {
                solutionOutputLimitExceeded();
                terminate();
            }

            if (ec)
            {
                if (ec == boost::asio::error::eof)
                    solutionEof();
                else
                    solutionReadError(ec, size);
            }
        }

        void handle_solution_write(
            const boost::system::error_code &ec, const std::size_t size)
        {
            if (ec)
            {
                solutionWriteError(ec, size);
            }
        }

    private:
        Buffer interactorToSolutionBuffer_;
        Buffer solutionToInteractorBuffer_;

        const std::size_t outputLimitBytes_;
        std::size_t interactorOutputBytes_ = 0;
        std::size_t solutionOutputBytes_ = 0;
    };
}}}}}
