#pragma once

#include "coding/string_utf8_multilang.hpp"

#include "3party/icu/i18n/csdetect.h"
#include "3party/icu/i18n/csmatch.h"


#include <cld2/public/compact_lang_det.h>

#include <string>

class LanguageDetector {
  struct Result
  {
    std::string m_result;
    std::string m_name;
    int32_t m_confidence;
  };
public:
  LanguageDetector() {CHECK_EQUAL(m_status, U_ZERO_ERROR, ()); }
  LanguageDetector::Result DetectLanguage (std::string const & detectee)
  {
    m_detector.setText(detectee.c_str(), detectee.size());
    CharsetMatch const *  match = m_detector.detect(m_status);
    if (m_status != U_ZERO_ERROR)
      return {};

    return Result{match->getLanguage(), match->getName(), match->getConfidence()};
  }


private:
  UErrorCode m_status {U_ZERO_ERROR};
  CharsetDetector m_detector{m_status};
};


class LanguageDetector2 {

public:
  struct Result
  {
    int32_t m_language;
  };
  LanguageDetector2() {}
  LanguageDetector2::Result DetectLanguage (std::string const & detectee)
  {
    bool ok = true;
    int valid_prefix_bytes;
    auto language = CLD2::DetectLanguageCheckUTF8(detectee.c_str(), detectee.size(), true, &ok, &valid_prefix_bytes);

    if (!ok)
      return {};

    return Result{language};
  }


private:
  UErrorCode m_status {U_ZERO_ERROR};
  CharsetDetector m_detector{m_status};
};





