#pragma once

#include "fmt/color.h"
namespace hm
{
namespace log
{

// TODO make it editable from an interface
constexpr fmt::color ColorInfo {fmt::color::green};
constexpr fmt::color ColorError {fmt::color::red};
constexpr fmt::color ColorWarning {fmt::color::yellow};
template<typename... T>

static void Info(fmt::format_string<T...> fmt, T&&... args)
{
  fmt::print(fmt::fg(ColorInfo), "[Info] ");
  fmt::print(fmt, std::forward<T>(args)...);
  fmt::print("\n");
}
template<typename... T>

static void Error(fmt::format_string<T...> fmt, T&&... args)
{
  fmt::print(fmt::fg(ColorError), "[Error] ");
  fmt::print(fmt, std::forward<T>(args)...);
  fmt::print("\n");
}
template<typename... T>

static void Warning(fmt::format_string<T...> fmt, T&&... args)
{
  fmt::print(fmt::fg(ColorWarning), "[Warning] ");
  fmt::print(fmt, std::forward<T>(args)...);
  fmt::print("\n");
}

} // namespace log
} // namespace hm
