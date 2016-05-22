#pragma once

#include <boost/version.hpp>
#if BOOST_VERSION < 106100
#define BOOST_NO_CXX11_VARIADIC_TEMPLATES
#endif

#include <bunsan/asio/buffer_connection.hpp>
#include <bunsan/filesystem/fstream.hpp>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/signals2.hpp>

#include <memory>

namespace yandex {
namespace contest {
namespace invoker {
namespace flowctl {
namespace interactive {

class BufferedConnection : private boost::noncopyable {
 public:
  using Connection = boost::asio::posix::stream_descriptor;
  using Buffer = bunsan::asio::buffer_connection<Connection, Connection>;
  using StaticEventSignal = boost::signals2::signal<void()>;
  using ErrorSignal =
      boost::signals2::signal<void(boost::system::error_code, std::size_t)>;

 public:
  BufferedConnection(Connection &interactorSource, Connection &interactorSink,
                     Connection &solutionSource, Connection &solutionSink,
                     const std::size_t outputLimitBytes);

  void setDumpJudge(const boost::filesystem::path &path) { dumpJudge_ = path; }

  void setDumpSolution(const boost::filesystem::path &path) {
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
  void handle_interactor_read(const boost::system::error_code &ec,
                              std::size_t size);

  void handle_interactor_write(const boost::system::error_code &ec,
                               std::size_t size);

  void handle_interactor_write_data(const char *data, std::size_t size);

  void handle_solution_read(const boost::system::error_code &ec,
                            std::size_t size);

  void handle_solution_read_data(const char *data, std::size_t size);

  void handle_solution_write(const boost::system::error_code &ec,
                             std::size_t size);

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

}  // namespace interactive
}  // namespace flowctl
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
