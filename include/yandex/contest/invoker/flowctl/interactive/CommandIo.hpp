#pragma once

#include <yandex/contest/invoker/flowctl/interactive/Command.hpp>

namespace yandex {
namespace contest {
namespace invoker {
namespace flowctl {
namespace interactive {

std::ostream &operator<<(std::ostream &out, const Command &command);

std::string encodeCommandValue(const std::string &value);
std::string decodeCommandValue(const std::string &data);

std::string encodeCommand(const Command &command);
Command decodeCommand(const std::string &data);

std::string encodeCommandPack(const CommandPack &pack);
CommandPack decodeCommandPack(const std::string &data);

}  // namespace interactive
}  // namespace flowctl
}  // namespace invoker
}  // namespace contest
}  // namespace yandex
