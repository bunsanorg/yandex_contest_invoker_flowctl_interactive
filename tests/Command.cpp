#define BOOST_TEST_MODULE Command
#include <boost/test/unit_test.hpp>

#include <yandex/contest/invoker/flowctl/interactive/Command.hpp>
#include <yandex/contest/invoker/flowctl/interactive/CommandIo.hpp>
#include <yandex/contest/invoker/flowctl/interactive/Error.hpp>

namespace ya = yandex::contest;
namespace yai = ya::invoker::flowctl::interactive;

BOOST_AUTO_TEST_SUITE(Command_)

BOOST_AUTO_TEST_SUITE(Io)

BOOST_AUTO_TEST_CASE(value)
{
    const auto enc = &yai::encodeCommandValue;
    const auto dec = &yai::decodeCommandValue;

    BOOST_CHECK_EQUAL(enc("hello world"), "hello%20world");
    BOOST_CHECK_EQUAL(dec("hello%20world"), "hello world");

    BOOST_CHECK_EQUAL(enc(" at the beginning"), "%20at%20the%20beginning");
    BOOST_CHECK_EQUAL(dec("%20at%20the%20beginning"), " at the beginning");

    BOOST_CHECK_EQUAL(enc("at the end "), "at%20the%20end%20");
    BOOST_CHECK_EQUAL(dec("at%20the%20end%20"), "at the end ");

    BOOST_CHECK_EQUAL(enc("double  space"), "double%20%20space");
    BOOST_CHECK_EQUAL(dec("double%20%20space"), "double  space");

    BOOST_CHECK_THROW(
        dec("hello world"),
        yai::DecodeCommandValueCharacterIsNotPermittedError
    );

    BOOST_CHECK_THROW(
        dec("hello%IsNotADigit"),
        yai::DecodeCommandValueDigitExpectedError
    );
}

BOOST_AUTO_TEST_CASE(command)
{
    using namespace yai;

    const auto enc = &encodeCommand;
    const auto dec = &decodeCommand;

    BOOST_CHECK_EQUAL(enc(makeCommand(REQUEST)), "REQUEST");
    BOOST_CHECK_EQUAL(dec("REQUEST"), makeCommand(REQUEST));

    BOOST_CHECK_EQUAL(
        enc(makeCommand(RESPONSE_MULTIPLIER, "10")),
        "RESPONSE_MULTIPLIER=10"
    );
    BOOST_CHECK_EQUAL(
        dec("RESPONSE_MULTIPLIER=10"),
        makeCommand(RESPONSE_MULTIPLIER, "10")
    );

    BOOST_CHECK_THROW(dec("RESPONSE_MULTIPLIER=10="), DecodeCommandError);
}

BOOST_AUTO_TEST_CASE(pack)
{
    using namespace yai;

    const auto enc = &encodeCommandPack;
    const auto dec = &decodeCommandPack;

    {
        const CommandPack pack = {
            makeCommand(REQUEST),
            makeCommand(LAST_RESPONSE),
            makeCommand(RESPONSE_MULTIPLIER, "10")
        };
        const std::string data =
            "REQUEST&"
            "LAST_RESPONSE&"
            "RESPONSE_MULTIPLIER=10;";

        BOOST_CHECK_EQUAL(enc(pack), data);
        BOOST_CHECK(dec(data) == pack);
    }
}

BOOST_AUTO_TEST_SUITE_END() // Io

BOOST_AUTO_TEST_SUITE_END() // Command_
