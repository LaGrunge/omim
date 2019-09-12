#include "platform/http_user_agent.hpp"

#include <sstream>

namespace platform
{
HttpUserAgent::HttpUserAgent()
{
  m_appVersion = ExtractAppVersion();
}

std::string HttpUserAgent::Get() const
{
  std::stringstream ss;
  ss << "MAPS.ME/";
  ss << "Unknown/";
  ss << m_appVersion;
  return ss.str();
}
}  // platform
