#include <yandex/contest/invoker/flowctl/interactive/BufferedConnection.hpp>

#include <yandex/contest/detail/LogHelper.hpp>

#include <boost/asio/detail/signal_init.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    BufferedConnection::BufferedConnection(
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
        boost::asio::detail::signal_init<SIGPIPE> sigpipe_init;

        interactorToSolutionBuffer_.set_close_sink_on_eof(false);
        interactorToSolutionBuffer_.set_discard_on_sink_error(true);

        interactorEof.connect([]{
            STREAM_INFO << "Interactor EOF";
        });

        solutionEof.connect([]{
            STREAM_INFO << "Solution EOF";
        });

        interactorOutputLimitExceeded.connect([]{
            STREAM_INFO << "Interactor output limit exceeded";
        });

        solutionOutputLimitExceeded.connect([]{
            STREAM_INFO << "Solution output limit exceeded";
        });

        interactorReadError.connect(
            [](const boost::system::error_code &ec, const std::size_t)
            {
                STREAM_INFO << "Interactor read error: " << ec.message();
            });

        interactorWriteError.connect(
            [](const boost::system::error_code &ec, const std::size_t)
            {
                STREAM_INFO << "Interactor write error: " << ec.message();
            });

        solutionReadError.connect(
            [](const boost::system::error_code &ec, const std::size_t)
            {
                STREAM_INFO << "Solution read error: " << ec.message();
            });

        solutionWriteError.connect(
            [](const boost::system::error_code &ec, const std::size_t)
            {
                STREAM_INFO << "Solution write error: " << ec.message();
            });
    }

    void BufferedConnection::start()
    {
        interactorToSolutionBuffer_.start();
        solutionToInteractorBuffer_.start();
    }

    void BufferedConnection::closeInteractorToSolution()
    {
        interactorToSolutionBuffer_.close();
    }

    void BufferedConnection::closeSolutionToInteractor()
    {
        solutionToInteractorBuffer_.close();
    }

    void BufferedConnection::close()
    {
        closeInteractorToSolution();
        closeSolutionToInteractor();
    }

    void BufferedConnection::terminateInteractorToSolution()
    {
        interactorToSolutionBuffer_.terminate();
    }

    void BufferedConnection::terminateSolutionToInteractor()
    {
        solutionToInteractorBuffer_.terminate();
    }

    void BufferedConnection::terminate()
    {
        terminateInteractorToSolution();
        terminateSolutionToInteractor();
    }

    void BufferedConnection::handle_interactor_read(
        const boost::system::error_code &ec,
        const std::size_t size)
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

    void BufferedConnection::handle_interactor_write(
        const boost::system::error_code &ec,
        const std::size_t size)
    {
        if (ec)
        {
            interactorWriteError(ec, size);
        }
    }

    void BufferedConnection::handle_solution_read(
        const boost::system::error_code &ec,
        const std::size_t size)
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

    void BufferedConnection::handle_solution_write(
        const boost::system::error_code &ec,
        const std::size_t size)
    {
        if (ec)
        {
            solutionWriteError(ec, size);
        }
    }
}}}}}
