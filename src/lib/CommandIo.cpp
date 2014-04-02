#include <yandex/contest/invoker/flowctl/interactive/CommandIo.hpp>

#include <yandex/contest/invoker/flowctl/interactive/Error.hpp>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/io/detail/quoted_manip.hpp>
#include <boost/lexical_cast.hpp>

#include <sstream>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    std::ostream &operator<<(std::ostream &out, const Command &command)
    {
        out << command.first;
        if (!command.second.empty())
            out << '=' << boost::io::quoted(command.second);
        return out;
    }

    static bool isDigit(const char c)
    {
        return '0' <= c && c <= '9';
    }

    static bool isPermitted(const char c)
    {
        return
            isDigit(c) ||
            ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            c == '_';
    }

    std::string encodeCommandValue(const std::string &value)
    {
        try
        {
            std::ostringstream sout;
            for (const char c: value)
            {
                if (isPermitted(c))
                {
                    sout << c;
                }
                else
                {
                    sout << '%' <<
                            std::hex <<
                            std::setw(2) <<
                            std::setfill('0') <<
                            static_cast<unsigned>(
                                static_cast<unsigned char>(c)
                            );
                }
            }
            return sout.str();
        }
        catch (std::exception &)
        {
            BOOST_THROW_EXCEPTION(
                EncodeCommandValueError() <<
                bunsan::enable_nested_current());
        }
    }

    std::string decodeCommandValue(const std::string &data)
    {
        try
        {
            std::ostringstream sout;
            int complexPos = 0;
            unsigned buf = 0;
            for (const char c: data)
            {
                switch (complexPos)
                {
                case 0:
                    switch (c)
                    {
                    case '%':
                        complexPos = 2;
                        break;
                    default:
                        if (!isPermitted(c))
                            BOOST_THROW_EXCEPTION(
                                DecodeCommandValueCharacterIsNotPermittedError() <<
                                DecodeCommandValueCharacterIsNotPermittedError::character(c));
                        sout << c;
                    }
                    break;
                case 1:
                    if (!isDigit(c))
                        BOOST_THROW_EXCEPTION(
                            DecodeCommandValueDigitExpectedError() <<
                            DecodeCommandValueDigitExpectedError::character(c));
                    buf += c - '0';
                    sout << static_cast<char>(
                                static_cast<unsigned char>(buf));
                    complexPos = 0;
                    break;
                case 2:
                    if (!isDigit(c))
                        BOOST_THROW_EXCEPTION(
                            DecodeCommandValueDigitExpectedError() <<
                            DecodeCommandValueDigitExpectedError::character(c));
                    buf = 0x10 * (c - '0');
                    complexPos = 1;
                    break;
                }
            }
            if (complexPos)
                BOOST_THROW_EXCEPTION(
                    DecodeCommandValueError() <<
                    DecodeCommandValueError::data(data) <<
                    DecodeCommandValueError::message(
                        "Invalid escape sequence."));
            return sout.str();
        }
        catch (DecodeCommandValueError &)
        {
            throw;
        }
        catch (std::exception &)
        {
            BOOST_THROW_EXCEPTION(
                DecodeCommandValueError() <<
                DecodeCommandValueError::data(data) <<
                bunsan::enable_nested_current());
        }
    }

    std::string encodeCommand(const Command &command)
    {
        try
        {
            std::ostringstream sout;
            sout << command.first;
            if (!command.second.empty())
                sout << '=' << encodeCommandValue(command.second);
            return sout.str();
        }
        catch (std::exception &)
        {
            BOOST_THROW_EXCEPTION(
                EncodeCommandError() <<
                bunsan::enable_nested_current());
        }
    }

    Command decodeCommand(const std::string &data)
    {
        try
        {
            const std::size_t pos = data.find('=');
            Command command;
            command.first =
                boost::lexical_cast<CommandName>(
                    data.substr(0, pos));
            if (pos != std::string::npos)
                command.second =
                    decodeCommandValue(
                        data.substr(pos + 1));
            return command;
        }
        catch (std::exception &)
        {
            BOOST_THROW_EXCEPTION(
                DecodeCommandError() <<
                DecodeCommandError::data(data) <<
                bunsan::enable_nested_current());
        }
    }

    std::string encodeCommandPack(const CommandPack &pack)
    {
        try
        {
            std::ostringstream sout;
            bool first = true;
            for (const Command &command: pack)
            {
                if (!first)
                    sout << '&';
                first = false;

                sout << encodeCommand(command);
            }
            sout << ';';
            return sout.str();
        }
        catch (std::exception &)
        {
            BOOST_THROW_EXCEPTION(
                EncodeCommandPackError() <<
                bunsan::enable_nested_current());
        }
    }

    CommandPack decodeCommandPack(const std::string &data)
    {
        try
        {
            CommandPack pack;

            std::string data_ = data;
            std::vector<std::string> encodedCommands;
            boost::algorithm::split(
                encodedCommands,
                data_,
                boost::algorithm::is_any_of("&;"),
                boost::algorithm::token_compress_on
            );
            for (const std::string &encodedCommand: encodedCommands)
                if (!encodedCommand.empty())
                    pack.emplace(decodeCommand(encodedCommand));
            return pack;
        }
        catch (std::exception &)
        {
            BOOST_THROW_EXCEPTION(
                DecodeCommandPackError() <<
                DecodeCommandPackError::data(data) <<
                bunsan::enable_nested_current());
        }
    }
}}}}}
