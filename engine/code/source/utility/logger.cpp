#include "utility/logger.hpp"

void hm::log::ConsoleSink::Sink(const LogMessage& msg)
{
  std::println("{}", msg.payLoad);
}
void hm::log::FileSink::Sink(const LogMessage& msg)
{
  output_file << msg.payLoad;
}
