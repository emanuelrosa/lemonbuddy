#pragma once

#include <boost/algorithm/string/replace.hpp>
#include <sstream>

#include "common.hpp"

LEMONBUDDY_NS

namespace string_util {
  /**
   * Check if haystack contains needle
   */
  inline auto contains(const string& haystack, const string& needle) {
    return haystack.find(needle) != string::npos;
  }

  /**
   * Convert string to uppercase
   */
  inline auto upper(const string& s) {
    string str(s);
    for (auto& c : str) c = toupper(c);
    return str;
  }

  /**
   * Convert string to lowercase
   */
  inline auto lower(const string& s) {
    string str(s);
    for (auto& c : str) c = tolower(c);
    return str;
  }

  /**
   * Test lower case equality
   */
  inline auto compare(const string& s1, const string& s2) {
    return lower(s1) == lower(s2);
  }

  /**
   * Replace first occurence of needle in haystack
   */
  inline auto replace(const string& haystack, string needle, string replacement) {
    string str(haystack);
    string::size_type pos;
    if (needle != replacement && (pos = str.find(needle)) != string::npos)
      str = str.replace(pos, needle.length(), replacement);
    return str;
  }

  /**
   * Replace all occurences of needle in haystack
   */
  inline auto replace_all(const string& haystack, string needle, string replacement) {
    string replaced;
    for (size_t i = 0; i < haystack.length(); i++) {
      if (haystack.compare(i, needle.length(), needle) == 0) {
        replaced += replacement;
        i += needle.length() - 1;
      } else {
        replaced += haystack[i];
      }
    }
    return replaced;
  }

  /**
   * Replace all consecutive occurrences of needle in haystack
   */
  inline auto squeeze(const string& haystack, char needle) {
    auto result = haystack;
    while (result.find({needle, needle}) != string::npos)
      result = replace_all(result, {needle, needle}, {needle});
    return result;
  }

  /**
   * Remove all occurrences of needle in haystack
   */
  inline auto strip(const string& haystack, char needle) {
    string str(haystack);
    string::size_type pos;
    while ((pos = str.find(needle)) != string::npos) str.erase(pos, 1);
    return str;
  }

  /**
   * Remove trailing newline
   */
  inline auto strip_trailing_newline(const string& haystack) {
    string str(haystack);
    if (str[str.length() - 1] == '\n')
      str.erase(str.length() - 1, 1);
    return str;
  }

  /**
   * Remove needle from the start of the string
   */
  inline auto ltrim(const string& haystack, char needle) {
    string str(haystack);
    while (str[0] == needle) str.erase(0, 1);
    return str;
  }

  /**
   * Remove needle from the end of the string
   */
  inline auto rtrim(const string& haystack, char needle) {
    string str(haystack);
    while (str[str.length() - 1] == needle) str.erase(str.length() - 1, 1);
    return str;
  }

  /**
   * Remove needle from the start and end of the string
   */
  inline auto trim(const string& haystack, char needle) {
    return rtrim(ltrim(haystack, needle), needle);
  }

  /**
   * Join all strings in vector into a single string separated by delim
   */
  inline auto join(vector<string> strs, string delim) {
    string str;
    for (auto& s : strs) str.append((str.empty() ? "" : delim) + s);
    return str;
  }

  /**
   * Explode string by delim into container
   */
  inline auto& split_into(string s, char delim, vector<string>& container) {
    string str;
    stringstream buffer(s);
    while (getline(buffer, str, delim)) container.emplace_back(str);
    return container;
  }

  /**
   * Explode string by delim
   */
  inline auto split(const string& s, char delim) {
    vector<string> vec;
    return split_into(s, delim, vec);
  }

  /**
   * Find the nth occurence of needle in haystack starting from pos
   */
  inline auto find_nth(string haystack, size_t pos, string needle, size_t nth) {
    size_t found_pos = haystack.find(needle, pos);
    if (1 == nth || string::npos == found_pos)
      return found_pos;
    return find_nth(haystack, found_pos + 1, needle, nth - 1);
  }

  /**
   * Get the resulting string from a ostream/
   *
   * Example usage:
   * @code cpp
   *   string_util::from_stream(std::stringstream() << ...);
   * @endcode
   */
  inline auto from_stream(const std::basic_ostream<char>& os) {
    return static_cast<const stringstream&>(os).str();
  }

  /**
   * Compute string hash
   */
  inline auto hash(string src) {
    return std::hash<string>()(src);
  }
}

LEMONBUDDY_NS_END
