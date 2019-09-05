#include "platform/constants.hpp"
#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/file_reader.hpp"

#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <future>
#include <memory>
#include <regex>
#include <string>


using namespace std;

unique_ptr<ModelReader> Platform::GetReader(string const & file, string const & searchScope) const
{
  return make_unique<FileReader>(ReadPathForFile(file, searchScope),
                                 READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
}

bool Platform::GetFileSizeByName(string const & fileName, uint64_t & size) const
{
  try
  {
    return GetFileSizeByFullPath(ReadPathForFile(fileName), size);
  }
  catch (RootException const &)
  {
    return false;
  }
}

void Platform::GetFilesByRegExp(string const & directory, string const & regexp, FilesList & outFiles)
{
}

int Platform::PreCachingDepth() const
{
  return 3;
}

// static
Platform::EError Platform::MkDir(string const & dirName)
{
}

void Platform::SetupMeasurementSystem() const
{
  auto units = measurement_utils::Units::Metric;
  if (settings::Get(settings::kMeasurementUnits, units))
    return;
  bool const isMetric = true;
  units = isMetric ? measurement_utils::Units::Metric : measurement_utils::Units::Imperial;
  settings::Set(settings::kMeasurementUnits, units);
}

extern Platform & GetPlatform()
{
  static Platform platform;
  return platform;
}
