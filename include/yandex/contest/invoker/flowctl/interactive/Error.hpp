#pragma once

#include <yandex/contest/invoker/Error.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace flowctl {
namespace interactive {

struct Error : virtual invoker::Error {};

struct CommandIoError : virtual Error {
  using data = boost::error_info<struct dataTag, std::string>;
  using character = boost::error_info<struct characterTag, char>;
};
struct EncodeCommandValueError : virtual CommandIoError {};
struct DecodeCommandValueError : virtual CommandIoError {};
struct DecodeCommandValueDigitExpectedError : virtual DecodeCommandValueError {
};
struct DecodeCommandValueCharacterIsNotPermittedError
    : virtual DecodeCommandValueError {};
struct EncodeCommandError : virtual CommandIoError {};
struct DecodeCommandError : virtual CommandIoError {};
struct EncodeCommandPackError : virtual CommandIoError {};
struct DecodeCommandPackError : virtual CommandIoError {};

}  // namespace interactive
}  // namespace flowctl
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
