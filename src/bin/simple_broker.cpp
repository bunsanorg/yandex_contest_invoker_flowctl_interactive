#include <yandex/contest/invoker/flowctl/interactive/SimpleBroker.hpp>

#include <bunsan/runtime/demangle.hpp>

#include <boost/program_options.hpp>

#include <iostream>

int main(int argc, char *argv[]) {
  using namespace yandex::contest::invoker::flowctl::interactive;

  SimpleBroker::Options options;
  std::uintmax_t terminationRealTimeLimitMillis;

  std::string dumpJudge, dumpSolution;

  namespace po = boost::program_options;
  po::options_description desc("Usage");
  try {
    desc.add_options()(
        "notifier", po::value<int>(&options.notifierFd)->required(),
        "notifier file descriptor"
    )(
        "interactor-source",
        po::value<int>(&options.interactorSourceFd)->required(),
        "interactor > broker file descriptor"
    )(
        "interactor-sink",
        po::value<int>(&options.interactorSinkFd)->required(),
        "broker > interactor file descriptor"
    )(
        "solution-source",
        po::value<int>(&options.solutionSourceFd)->required(),
        "solution > broker file descriptor"
    )(
        "solution-sink", po::value<int>(&options.solutionSinkFd)->required(),
        "broker > solution file descriptor"
    )(
        "output-limit-bytes",
        po::value<std::size_t>(&options.outputLimitBytes)->required(),
        "output limit in bytes"
    )(
        "termination-real-time-limit-millis",
        po::value<std::uintmax_t>(&terminationRealTimeLimitMillis)->required(),
        "termination real time limit in milliseconds"
    )(
        "dump-judge", po::value<std::string>(&dumpJudge),
        "dump judge->solution data"
    )(
        "dump-solution", po::value<std::string>(&dumpSolution),
        "dump solution->judge data"
    );

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    po::notify(vm);
    options.terminationRealTimeLimit =
        std::chrono::milliseconds(terminationRealTimeLimitMillis);
    if (vm.count("dump-judge")) options.dumpJudge = dumpJudge;
    if (vm.count("dump-solution")) options.dumpSolution = dumpSolution;

    SimpleBroker broker(options);
    return static_cast<int>(broker.run());
  } catch (po::error &e) {
    std::cerr << e.what() << std::endl
              << desc << std::endl;
    return 200;
  } catch (std::exception &e) {
    std::cerr << "Program terminated due to exception of type \""
              << bunsan::runtime::type_name(e) << "\"." << std::endl;
    std::cerr << "what() returns the following message:" << std::endl
              << e.what() << std::endl;
    return 1;
  }
}
