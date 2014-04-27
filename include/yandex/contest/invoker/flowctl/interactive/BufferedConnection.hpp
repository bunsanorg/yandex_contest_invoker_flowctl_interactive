#pragma once

#include <bunsan/asio/buffer_connection.hpp>
#include <bunsan/filesystem/fstream.hpp>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/signals2.hpp>

#include <memory>

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
            const std::size_t outputLimitBytes);

        void setDumpJudge(const boost::filesystem::path &path)
        {
            dumpJudge_ = path;
        }

        void setDumpSolution(const boost::filesystem::path &path)
        {
            dumpSolution_ = path;
        }

        void start();

        void closeInteractorToSolution();
        void closeSolutionToInteractor();
        void close();

        void terminateInteractorToSolution();
        void terminateSolutionToInteractor();
        void terminate();

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
            const boost::system::error_code &ec,
            const std::size_t size);

        void handle_interactor_write(
            const boost::system::error_code &ec,
            const std::size_t size);

        void handle_interactor_write_data(
            const char *const data,
            const std::size_t size);

        void handle_solution_read(
            const boost::system::error_code &ec,
            const std::size_t size);

        void handle_solution_read_data(
            const char *const data,
            const std::size_t size);

        void handle_solution_write(
            const boost::system::error_code &ec,
            const std::size_t size);


    private:
        Buffer interactorToSolutionBuffer_;
        Buffer solutionToInteractorBuffer_;

        const std::size_t outputLimitBytes_;
        std::size_t interactorOutputBytes_ = 0;
        std::size_t solutionOutputBytes_ = 0;

        boost::optional<boost::filesystem::path> dumpJudge_;
        boost::optional<boost::filesystem::path> dumpSolution_;
        std::unique_ptr<bunsan::filesystem::ofstream> dumpJudgeStream_;
        std::unique_ptr<bunsan::filesystem::ofstream> dumpSolutionStream_;
    };
}}}}}
