#pragma once

#include <bunsan/stream_enum.hpp>

#include <map>
#include <string>

#include <cstdint>

namespace yandex {
namespace contest {
namespace invoker {
namespace flowctl {
namespace interactive {

using UnderlyingType = std::uint64_t;

constexpr UnderlyingType CLASS_MASK = 0x100;

enum CommandClass : UnderlyingType {
  REQUEST_CLASS = 1 * CLASS_MASK,
  RESPONSE_CLASS = 2 * CLASS_MASK,
  VERDICT_CLASS = 4 * CLASS_MASK,
  RESPONSE_INFO_CLASS = 8 * CLASS_MASK,

  CONTROL_CLASS = REQUEST_CLASS | RESPONSE_CLASS,
  FLAG_CLASS = CONTROL_CLASS,
  PARAMETER_CLASS = VERDICT_CLASS | RESPONSE_INFO_CLASS,
};

BUNSAN_TYPED_STREAM_ENUM_INITIALIZED(
    CommandName, UnderlyingType,
    ((REQUEST, REQUEST_CLASS | 0x01), (EMPTY_REQUEST, REQUEST_CLASS | 0x02),
     (LAST_REQUEST, REQUEST_CLASS | 0x04),
     (EOF_REQUEST, EMPTY_REQUEST | LAST_REQUEST),

     (RESPONSE, RESPONSE_CLASS | 0x01), (LAST_RESPONSE, RESPONSE_CLASS | 0x02),
     (EMPTY_RESPONSE, RESPONSE_CLASS | 0x04),
     (EOF_RESPONSE, EMPTY_RESPONSE | LAST_RESPONSE),

     (CLOSE, EOF_REQUEST | EOF_RESPONSE),

     (OK_VERDICT, VERDICT_CLASS | 0x01),
     (OK_VERDICT_CUSTOM, VERDICT_CLASS | 0x02),
     (ERROR_VERDICT, VERDICT_CLASS | 0x04),
     (ERROR_VERDICT_CUSTOM, VERDICT_CLASS | 0x08),

     (RESPONSE_MULTIPLIER, RESPONSE_INFO_CLASS | 0x01)))

using CommandPack = std::map<CommandName, std::string>;
using Command = std::pair<CommandName, std::string>;

constexpr bool isRequestCommand(const CommandName name) {
  return name & REQUEST_CLASS;
}

constexpr bool isResponseCommand(const CommandName name) {
  return name & RESPONSE_CLASS;
}

constexpr bool isControlCommand(const CommandName name) {
  return name & CONTROL_CLASS;
}

constexpr bool isVerdictCommand(const CommandName name) {
  return name & VERDICT_CLASS;
}

constexpr bool isResponseInfoCommand(const CommandName name) {
  return name & RESPONSE_INFO_CLASS;
}

constexpr bool isFlag(const CommandName name) { return name & FLAG_CLASS; }

constexpr bool isParameter(const CommandName name) {
  return name & PARAMETER_CLASS;
}

inline Command makeCommand(const CommandName name) {
  return Command{name, std::string{}};
}

inline Command makeCommand(const CommandName name, const std::string &value) {
  return Command{name, value};
}

}  // namespace interactive
}  // namespace flowctl
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
