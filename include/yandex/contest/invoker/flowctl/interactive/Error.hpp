#pragma once

#include <yandex/contest/invoker/Error.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    struct Error: virtual invoker::Error {};

    struct CommandIoError: virtual Error
    {
        typedef boost::error_info<struct dataTag, std::string> data;
        typedef boost::error_info<struct characterTag, char> character;
    };
    struct EncodeCommandValueError: virtual CommandIoError {};
    struct DecodeCommandValueError: virtual CommandIoError {};
    struct DecodeCommandValueDigitExpectedError:
        virtual DecodeCommandValueError {};
    struct DecodeCommandValueCharacterIsNotPermittedError:
        virtual DecodeCommandValueError {};
    struct EncodeCommandError: virtual CommandIoError {};
    struct DecodeCommandError: virtual CommandIoError {};
    struct EncodeCommandPackError: virtual CommandIoError {};
    struct DecodeCommandPackError: virtual CommandIoError {};
}}}}}
