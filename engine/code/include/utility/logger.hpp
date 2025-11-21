#pragma once
#include <print>
#include <format>
#include <mutex>

namespace hm::log
{
enum class Level
{
  Info,
  Warning,
  Error
};
namespace color
{
constexpr const char* reset = "\033[0m";
constexpr const char* green = "\033[32m";
constexpr const char* red = "\033[31m";
constexpr const char* yellow = "\033[33m";
} // namespace color

template<typename... T>
static void Info(std::format_string<T...> fs, T&&... args)
{
  std::print("{}[Info] {}{}\n", color::green,
             std::format(fs, std::forward<T>(args)...), color::reset);
}

template<typename... T>
static void Error(std::format_string<T...> fs, T&&... args)
{
  std::print("{}[Error] {}{}\n", color::red,
             std::format(fs, std::forward<T>(args)...), color::reset);
}

template<typename... T>
static void Warning(std::format_string<T...> fs, T&&... args)
{
  std::print("{}[Warning] {}{}\n", color::yellow,
             std::format(fs, std::forward<T>(args)...), color::reset);
}

struct LogMessage
{
  std::string_view loggerName;
  std::string_view payLoad;
};
struct BaseSink
{
  virtual ~BaseSink() = default;
  virtual void Sink(const LogMessage& msg) = 0;
};
struct ConsoleSink : BaseSink
{
  void Sink(const LogMessage& msg) override;
};
struct FileSink : BaseSink
{
  explicit FileSink(const std::string& fileName) : output_file(fileName) {}
  void Sink(const LogMessage& msg) override;
  std::ofstream output_file;
};
struct Logger
{
  void Log(const std::string_view& msg)
  {
    for (const auto& sink : m_sinks)
    {
      sink->Sink({m_name, msg});
    }
  }
  template<typename... T>
  void Info(std::format_string<T...> fs, T&&... args)
  {
    Log(std::format(fs, std::forward<T>(args)...));
  }

  template<typename... T>
  void Error(std::format_string<T...> fs, T&&... args)
  {
    Log(std::format(fs, std::forward<T>(args)...));
  }

  template<typename... T>
  void Warning(std::format_string<T...> fs, T&&... args)
  {
    Log(std::format(fs, std::forward<T>(args)...));
  }

  std::vector<std::shared_ptr<BaseSink>> m_sinks;

 private:
  std::string m_name {"Logger"};
  Level m_level {Level::Info};
};

} // namespace hm::log
