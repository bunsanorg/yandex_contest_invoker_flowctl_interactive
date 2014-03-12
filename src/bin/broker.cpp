#include <yandex/contest/invoker/flowctl/interactive/Broker.hpp>

#include <bunsan/runtime/demangle.hpp>

#include <boost/program_options.hpp>

#include <iostream>

int main(int argc, char *argv[])
{
    using namespace yandex::contest::invoker::flowctl::interactive;

    Broker::Options options;

    namespace po = boost::program_options;
    po::options_description desc("Usage");
    try
    {
        desc.add_options()
            (
                "request-delimiter",
                po::value<std::string>(
                    &options.requestDelimiter)->required(),
                "request delimiter"
            )
            (
                "response-delimiter",
                po::value<std::string>(
                    &options.responseDelimiter)->required(),
                "response delimiter"
            )
            (
                "interactor-to-broker",
                po::value<int>(
                    &options.interactorBrokerFd)->required(),
                "interactor > broker file descriptor"
            )
            (
                "broker-to-interactor",
                po::value<int>(
                    &options.brokerInteractorFd)->required(),
                "broker > interactor file descriptor"
            )
            (
                "solution-to-broker",
                po::value<int>(
                    &options.solutionBrokerFd)->required(),
                "solution > broker file descriptor"
            )
            (
                "broker-to-solution",
                po::value<int>(
                    &options.brokerSolutionFd)->required(),
                "broker > solution file descriptor"
            )
            (
                "empty-first-request",
                "interaction is started by solution"
            );

            po::variables_map vm;
            po::store(
                po::command_line_parser(argc, argv).
                options(desc).
                run(),
                vm
            );

            options.emptyFirstRequest = vm.count("empty-first-request");

            Broker broker(options);
            return static_cast<int>(broker.run());
    }
    catch (po::error &e)
    {
        std::cerr << e.what() << std::endl << desc << std::endl;
        return 200;
    }
    catch (std::exception &e)
    {
        std::cerr << "Program terminated due to exception of type \"" <<
                     bunsan::runtime::type_name(e) << "\"." << std::endl;
        std::cerr << "what() returns the following message:" << std::endl <<
                     e.what() << std::endl;
        return 1;
    }
}
