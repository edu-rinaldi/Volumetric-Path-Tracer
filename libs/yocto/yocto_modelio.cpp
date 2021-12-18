//
// Implementation for Yocto/Ply.
//

//
// LICENSE:
//
// Copyright (c) 2016 -- 2021 Fabio Pellacini
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "yocto_modelio.h"

#include <charconv>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "ext/fast_float.h"
#include "yocto_color.h"

// -----------------------------------------------------------------------------
// USING DIRECTIVES
// -----------------------------------------------------------------------------
namespace yocto {

// using directives
using std::string_view;
using std::unordered_map;
using std::unordered_set;
using namespace std::string_literals;
using namespace std::string_view_literals;

}  // namespace yocto

// -----------------------------------------------------------------------------
// FILE IO
// -----------------------------------------------------------------------------
namespace yocto {

// Opens a file with a utf8 file name
static FILE* fopen_utf8(const char* filename, const char* mode) {
#ifdef _WIN32
  auto path8    = std::filesystem::u8path(filename);
  auto str_mode = string{mode};
  auto wmode    = std::wstring(str_mode.begin(), str_mode.end());
  return _wfopen(path8.c_str(), wmode.c_str());
#else
  return fopen(filename, mode);
#endif
}

// Load a text file
static bool load_text(const string& filename, string& str, string& error) {
  // https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
  auto fs = fopen_utf8(filename.c_str(), "rb");
  if (!fs) {
    error = filename + ": file not found";
    return false;
  }
  fseek(fs, 0, SEEK_END);
  auto length = ftell(fs);
  fseek(fs, 0, SEEK_SET);
  str.resize(length);
  if (fread(str.data(), 1, length, fs) != length) {
    fclose(fs);
    error = filename + ": read error";
    return false;
  }
  fclose(fs);
  return true;
}

// Save a text file
static bool save_text(
    const string& filename, const string& str, string& error) {
  auto fs = fopen_utf8(filename.c_str(), "wt");
  if (!fs) {
    error = filename + ": file not found";
    return false;
  }
  if (fprintf(fs, "%s", str.c_str()) < 0) {
    fclose(fs);
    error = filename + ": write error";
    return false;
  }
  fclose(fs);
  return true;
}

// Load a binary file
static bool load_binary(
    const string& filename, vector<byte>& data, string& error) {
  // https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
  auto fs = fopen_utf8(filename.c_str(), "rb");
  if (!fs) {
    error = filename + ": file not found";
    return false;
  }
  fseek(fs, 0, SEEK_END);
  auto length = ftell(fs);
  fseek(fs, 0, SEEK_SET);
  data.resize(length);
  if (fread(data.data(), 1, length, fs) != length) {
    fclose(fs);
    error = filename + ": read error";
    return false;
  }
  fclose(fs);
  return true;
}

// Save a binary file
static bool save_binary(
    const string& filename, const vector<byte>& data, string& error) {
  auto fs = fopen_utf8(filename.c_str(), "wb");
  if (!fs) {
    error = filename + ": file not found";
    return false;
  }
  if (fwrite(data.data(), 1, data.size(), fs) != data.size()) {
    fclose(fs);
    error = filename + ": write error";
    return false;
  }
  fclose(fs);
  return true;
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// PATH UTILITIES
// -----------------------------------------------------------------------------
namespace yocto {

// Make a path from a utf8 string
static std::filesystem::path make_path(const string& filename) {
  return std::filesystem::u8path(filename);
}

// Get directory name (not including /)
static string path_dirname(const string& filename) {
  return make_path(filename).parent_path().generic_u8string();
}

// Get filename without directory.
static string path_filename(const string& filename) {
  return make_path(filename).filename().u8string();
}

// Joins paths
static string path_join(const string& patha, const string& pathb) {
  return (make_path(patha) / make_path(pathb)).generic_u8string();
}

// Replaces extensions
static string replace_extension(const string& filename, const string& ext) {
  return make_path(filename).replace_extension(ext).u8string();
}

// Check if a file can be opened for reading.
static bool path_exists(const string& filename) {
  return exists(make_path(filename));
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// IMPLEMENTATION FOR UTILITIES
// -----------------------------------------------------------------------------
namespace yocto {

// Formats values to string
static void format_value(string& str, const string& value) { str += value; }
static void format_value(string& str, int8_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, int16_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, int32_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, int64_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, uint8_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, uint16_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, uint32_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, uint64_t value) {
  auto buffer = array<char, 64>{};
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
}
static void format_value(string& str, float value) {
  auto buffer = array<char, 256>{};
#ifdef _WIN32
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
#else
  auto len = snprintf(buffer.data(), buffer.size(), "%.9g", value);
  str.append(buffer.data(), buffer.data() + len);
#endif
}
static void format_value(string& str, double value) {
  auto buffer = array<char, 256>{};
#ifdef _WIN32
  auto result = std::to_chars(
      buffer.data(), buffer.data() + buffer.size(), value);
  str.append(buffer.data(), result.ptr);
#else
  auto len = snprintf(buffer.data(), buffer.size(), "%.17g", value);
  str.append(buffer.data(), buffer.data() + len);
#endif
}
static void format_value(string& str, const vec2f& value) {
  for (auto i = 0; i < 2; i++) {
    if (i != 0) str += " ";
    format_value(str, value[i]);
  }
}
static void format_value(string& str, const vec3f& value) {
  for (auto i = 0; i < 3; i++) {
    if (i != 0) str += " ";
    format_value(str, value[i]);
  }
}
static void format_value(string& str, const frame3f& value) {
  for (auto i = 0; i < 4; i++) {
    if (i != 0) str += " ";
    format_value(str, value[i]);
  }
}
static void format_value(string& str, const vec4f& value) {
  for (auto i = 0; i < 4; i++) {
    if (i != 0) str += " ";
    format_value(str, value[i]);
  }
}
static void format_value(string& str, const mat4f& value) {
  for (auto i = 0; i < 4; i++) {
    if (i != 0) str += " ";
    format_value(str, value[i]);
  }
}

// Foramt to file
static void format_values(string& str, string_view fmt) {
  auto pos = fmt.find("{}"sv);
  if (pos != string::npos) throw std::invalid_argument("bad format string");
  str += fmt;
}
template <typename Arg, typename... Args>
static void format_values(
    string& str, string_view fmt, const Arg& arg, const Args&... args) {
  auto pos = fmt.find("{}"sv);
  if (pos == string::npos) throw std::invalid_argument("bad format string");
  str += fmt.substr(0, pos);
  format_value(str, arg);
  format_values(str, fmt.substr(pos + 2), args...);
}

static bool is_newline(char c) { return c == '\r' || c == '\n'; }
static bool is_space(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
static void skip_whitespace(string_view& str) {
  while (!str.empty() && is_space(str.front())) str.remove_prefix(1);
}

static void remove_comment(
    string_view& str, char comment_char = '#', bool handle_quotes = false) {
  if (!handle_quotes) {
    while (!str.empty() && is_newline(str.back())) str.remove_suffix(1);
    auto cpy = str;
    while (!cpy.empty() && cpy.front() != comment_char) cpy.remove_prefix(1);
    str.remove_suffix(cpy.size());
  } else {
    while (!str.empty() && is_newline(str.back())) str.remove_suffix(1);
    auto cpy       = str;
    auto in_string = false;
    while (!cpy.empty()) {
      if (cpy.front() == '"') in_string = !in_string;
      if (cpy.front() == comment_char && !in_string) break;
      cpy.remove_prefix(1);
    }
    str.remove_suffix(cpy.size());
  }
}

// Read a line
static bool read_line(string_view& str, string_view& line) {
  if (str.empty()) return false;
  auto data = str.data();
  auto size = (size_t)0;
  while (!str.empty()) {
    if (str.front() == '\n') {
      str.remove_prefix(1);
      size++;
      break;
    } else {
      str.remove_prefix(1);
      size++;
    }
  }
  line = {data, size};
  return true;
}

// Parse values from a string
[[nodiscard]] static bool parse_value(string_view& str, string_view& value) {
  skip_whitespace(str);
  if (str.empty()) return false;
  if (str.front() != '"') {
    auto cpy = str;
    while (!cpy.empty() && !is_space(cpy.front())) cpy.remove_prefix(1);
    value = str;
    value.remove_suffix(cpy.size());
    str.remove_prefix(str.size() - cpy.size());
  } else {
    if (str.front() != '"') return false;
    str.remove_prefix(1);
    if (str.empty()) return false;
    auto cpy = str;
    while (!cpy.empty() && cpy.front() != '"') cpy.remove_prefix(1);
    if (cpy.empty()) return false;
    value = str;
    value.remove_suffix(cpy.size());
    str.remove_prefix(str.size() - cpy.size());
    str.remove_prefix(1);
  }
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, string& value) {
  auto valuev = string_view{};
  if (!parse_value(str, valuev)) return false;
  value = string{valuev};
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, int8_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, int16_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, int32_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, int64_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, uint8_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, uint16_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, uint32_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, uint64_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, float& value) {
  skip_whitespace(str);
  auto result = fast_float::from_chars(
      str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, double& value) {
  skip_whitespace(str);
  auto result = fast_float::from_chars(
      str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
#ifdef __APPLE__
[[nodiscard]] static bool parse_value(string_view& str, size_t& value) {
  skip_whitespace(str);
  auto result = std::from_chars(str.data(), str.data() + str.size(), value);
  if (result.ptr == str.data()) return false;
  str.remove_prefix(result.ptr - str.data());
  return true;
}
#endif
[[nodiscard]] static bool parse_value(string_view& str, bool& value) {
  auto valuei = 0;
  if (!parse_value(str, valuei)) return false;
  value = (bool)valuei;
  return true;
}

[[nodiscard]] static bool parse_value(string_view& str, vec2f& value) {
  for (auto i = 0; i < 2; i++)
    if (!parse_value(str, value[i])) return false;
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, vec3f& value) {
  for (auto i = 0; i < 3; i++)
    if (!parse_value(str, value[i])) return false;
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, vec4f& value) {
  for (auto i = 0; i < 4; i++)
    if (!parse_value(str, value[i])) return false;
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, mat3f& value) {
  for (auto i = 0; i < 3; i++)
    if (!parse_value(str, value[i])) return false;
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, mat4f& value) {
  for (auto i = 0; i < 4; i++)
    if (!parse_value(str, value[i])) return false;
  return true;
}
[[nodiscard]] static bool parse_value(string_view& str, frame3f& value) {
  for (auto i = 0; i < 4; i++)
    if (!parse_value(str, value[i])) return false;
  return true;
}

template <typename T>
[[nodiscard]] static bool read_value(string_view& str, T& value) {
  if (str.size() < sizeof(value)) return false;
  memcpy(&value, str.data(), sizeof(T));
  str.remove_prefix(sizeof(T));
  return true;
}

// Write data from a file
template <typename T>
static void write_value(vector<byte>& data, const T& value) {
  if constexpr (sizeof(T) == 1) {
    data.push_back(*(byte*)&value);
  } else {
    data.insert(data.end(), (byte*)(&value), (byte*)(&value) + sizeof(T));
  }
}

template <typename T>
static T swap_endian(T value) {
  // https://stackoverflow.com/questions/105252/how-do-i-convert-between-big-endian-and-little-endian-values-in-c
  static_assert(sizeof(char) == 1, "sizeof(char) == 1");
  union {
    T             value;
    unsigned char bytes[sizeof(T)];
  } source, dest;
  source.value = value;
  for (auto k = (size_t)0; k < sizeof(T); k++)
    dest.bytes[k] = source.bytes[sizeof(T) - k - 1];
  return dest.value;
}

template <typename T>
[[nodiscard]] static bool read_value(
    string_view& fs, T& value, bool big_endian) {
  if (!read_value(fs, value)) return false;
  if (big_endian) value = swap_endian(value);
  return true;
}

template <typename T>
static void write_value(vector<byte>& data, const T& value_, bool big_endian) {
  auto value = big_endian ? swap_endian(value_) : value_;
  return write_value(data, value);
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// IMPLEMENTATION FOR PLY LOADER AND WRITER
// -----------------------------------------------------------------------------
namespace yocto {

// Load ply
bool load_ply(const string& filename, ply_model& ply, string& error) {
  // ply type names
  static auto type_map = unordered_map<string, ply_type>{{"char", ply_type::i8},
      {"short", ply_type::i16}, {"int", ply_type::i32}, {"long", ply_type::i64},
      {"uchar", ply_type::u8}, {"ushort", ply_type::u16},
      {"uint", ply_type::u32}, {"ulong", ply_type::u64},
      {"float", ply_type::f32}, {"double", ply_type::f64},
      {"int8", ply_type::i8}, {"int16", ply_type::i16},
      {"int32", ply_type::i32}, {"int64", ply_type::i64},
      {"uint8", ply_type::u8}, {"uint16", ply_type::u16},
      {"uint32", ply_type::u32}, {"uint64", ply_type::u64},
      {"float32", ply_type::f32}, {"float64", ply_type::f64}};

  // load data
  auto data = vector<byte>{};
  if (!load_binary(filename, data, error)) return false;

  // parsing checks
  auto first_line = true;
  auto end_header = false;

  // read header ---------------------------------------------
  auto data_view   = string_view{(const char*)data.data(), data.size()};
  auto str         = string_view{};
  auto parse_error = [&filename, &error]() {
    error = filename + ": parse error";
    return false;
  };
  auto read_error = [&filename, &error]() {
    error = filename + ": parse error";
    return false;
  };
  while (read_line(data_view, str)) {
    // str
    remove_comment(str);
    skip_whitespace(str);
    if (str.empty()) continue;

    // get command
    auto cmd = ""s;
    if (!parse_value(str, cmd)) return parse_error();
    if (cmd.empty()) continue;

    // check magic number
    if (first_line) {
      if (cmd != "ply") return parse_error();
      first_line = false;
      continue;
    }

    // possible token values
    if (cmd == "ply") {
      if (!first_line) return parse_error();
    } else if (cmd == "format") {
      auto fmt = ""s;
      if (!parse_value(str, fmt)) return parse_error();
      if (fmt == "ascii") {
        ply.format = ply_format::ascii;
      } else if (fmt == "binary_little_endian") {
        ply.format = ply_format::binary_little_endian;
      } else if (fmt == "binary_big_endian") {
        ply.format = ply_format::binary_big_endian;
      } else {
        return parse_error();
      }
    } else if (cmd == "comment") {
      skip_whitespace(str);
      ply.comments.emplace_back(str);
    } else if (cmd == "obj_info") {
      skip_whitespace(str);
      // comment is the rest of the str
    } else if (cmd == "element") {
      auto& elem = ply.elements.emplace_back();
      if (!parse_value(str, elem.name)) return parse_error();
      if (!parse_value(str, elem.count)) return parse_error();
    } else if (cmd == "property") {
      if (ply.elements.empty()) return parse_error();
      auto& prop  = ply.elements.back().properties.emplace_back();
      auto  tname = ""s;
      if (!parse_value(str, tname)) return parse_error();
      if (tname == "list") {
        prop.is_list = true;
        if (!parse_value(str, tname)) return parse_error();
        auto itype = type_map.at(tname);
        if (itype != ply_type::u8) return parse_error();
        if (!parse_value(str, tname)) return parse_error();
        prop.type = type_map.at(tname);
      } else {
        prop.is_list = false;
        prop.type    = type_map.at(tname);
      }
      if (!parse_value(str, prop.name)) return parse_error();
    } else if (cmd == "end_header") {
      end_header = true;
      break;
    } else {
      return parse_error();
    }
  }

  // check exit
  if (!end_header) return parse_error();

  // allocate data ---------------------------------
  for (auto& element : ply.elements) {
    for (auto& property : element.properties) {
      auto count = property.is_list ? element.count * 3 : element.count;
      switch (property.type) {
        case ply_type::i8: property.data_i8.reserve(count); break;
        case ply_type::i16: property.data_i16.reserve(count); break;
        case ply_type::i32: property.data_i32.reserve(count); break;
        case ply_type::i64: property.data_i64.reserve(count); break;
        case ply_type::u8: property.data_u8.reserve(count); break;
        case ply_type::u16: property.data_u16.reserve(count); break;
        case ply_type::u32: property.data_u32.reserve(count); break;
        case ply_type::u64: property.data_u64.reserve(count); break;
        case ply_type::f32: property.data_f32.reserve(count); break;
        case ply_type::f64: property.data_f64.reserve(count); break;
      }
      if (property.is_list) property.ldata_u8.reserve(element.count);
    }
  }

  // read data -------------------------------------
  if (ply.format == ply_format::ascii) {
    for (auto& elem : ply.elements) {
      for (auto idx = 0; idx < elem.count; idx++) {
        if (!read_line(data_view, str)) return read_error();
        for (auto& prop : elem.properties) {
          if (prop.is_list)
            if (!parse_value(str, prop.ldata_u8.emplace_back()))
              return parse_error();
          auto vcount = prop.is_list ? prop.ldata_u8.back() : 1;
          for (auto i = 0; i < vcount; i++) {
            switch (prop.type) {
              case ply_type::i8:
                if (!parse_value(str, prop.data_i8.emplace_back()))
                  return parse_error();
                break;
              case ply_type::i16:
                if (!parse_value(str, prop.data_i16.emplace_back()))
                  return parse_error();
                break;
              case ply_type::i32:
                if (!parse_value(str, prop.data_i32.emplace_back()))
                  return parse_error();
                break;
              case ply_type::i64:
                if (!parse_value(str, prop.data_i64.emplace_back()))
                  return parse_error();
                break;
              case ply_type::u8:
                if (!parse_value(str, prop.data_u8.emplace_back()))
                  return parse_error();
                break;
              case ply_type::u16:
                if (!parse_value(str, prop.data_u16.emplace_back()))
                  return parse_error();
                break;
              case ply_type::u32:
                if (!parse_value(str, prop.data_u32.emplace_back()))
                  return parse_error();
                break;
              case ply_type::u64:
                if (!parse_value(str, prop.data_u64.emplace_back()))
                  return parse_error();
                break;
              case ply_type::f32:
                if (!parse_value(str, prop.data_f32.emplace_back()))
                  return parse_error();
                break;
              case ply_type::f64:
                if (!parse_value(str, prop.data_f64.emplace_back()))
                  return parse_error();
                break;
            }
          }
        }
      }
    }
  } else {
    auto big_endian = ply.format == ply_format::binary_big_endian;
    for (auto& elem : ply.elements) {
      for (auto idx = 0; idx < elem.count; idx++) {
        for (auto& prop : elem.properties) {
          if (prop.is_list) {
            if (!read_value(
                    data_view, prop.ldata_u8.emplace_back(), big_endian))
              return read_error();
          }
          auto vcount = prop.is_list ? prop.ldata_u8.back() : 1;
          for (auto i = 0; i < vcount; i++) {
            switch (prop.type) {
              case ply_type::i8:
                if (!read_value(
                        data_view, prop.data_i8.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::i16:
                if (!read_value(
                        data_view, prop.data_i16.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::i32:
                if (!read_value(
                        data_view, prop.data_i32.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::i64:
                if (!read_value(
                        data_view, prop.data_i64.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::u8:
                if (!read_value(
                        data_view, prop.data_u8.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::u16:
                if (!read_value(
                        data_view, prop.data_u16.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::u32:
                if (!read_value(
                        data_view, prop.data_u32.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::u64:
                if (!read_value(
                        data_view, prop.data_u64.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::f32:
                if (!read_value(
                        data_view, prop.data_f32.emplace_back(), big_endian))
                  return read_error();
                break;
              case ply_type::f64:
                if (!read_value(
                        data_view, prop.data_f64.emplace_back(), big_endian))
                  return read_error();
                break;
            }
          }
        }
      }
    }
  }

  // done
  return true;
}

// save ply
bool save_ply(const string& filename, const ply_model& ply, string& error) {
  // ply type names
  static auto type_map = unordered_map<ply_type, string>{{ply_type::i8, "char"},
      {ply_type::i16, "short"}, {ply_type::i32, "int"}, {ply_type::i64, "uint"},
      {ply_type::u8, "uchar"}, {ply_type::u16, "ushort"},
      {ply_type::u32, "uint"}, {ply_type::u64, "ulong"},
      {ply_type::f32, "float"}, {ply_type::f64, "double"}};
  static auto format_map = unordered_map<ply_format, string>{
      {ply_format::ascii, "ascii"},
      {ply_format::binary_little_endian, "binary_little_endian"},
      {ply_format::binary_big_endian, "binary_big_endian"}};

  // buffer
  auto header = string{};

  // header
  format_values(header, "ply\n");
  format_values(header, "format {} 1.0\n", format_map.at(ply.format));
  format_values(header, "comment Written by Yocto/GL\n");
  format_values(header, "comment https://github.com/xelatihy/yocto-gl\n");
  for (auto& comment : ply.comments)
    format_values(header, "comment {}\n", comment);
  for (auto& elem : ply.elements) {
    format_values(header, "element {} {}\n", elem.name, (uint64_t)elem.count);
    for (auto& prop : elem.properties) {
      if (prop.is_list) {
        format_values(header, "property list uchar {} {}\n",
            type_map[prop.type], prop.name);
      } else {
        format_values(
            header, "property {} {}\n", type_map[prop.type], prop.name);
      }
    }
  }

  format_values(header, "end_header\n");

  // properties
  if (ply.format == ply_format::ascii) {
    // buffer
    auto buffer = header;

    for (auto& elem : ply.elements) {
      auto cur = vector<size_t>(elem.properties.size(), 0);
      for (auto idx = 0; idx < elem.count; idx++) {
        for (auto& prop : elem.properties) {
          if (prop.is_list)
            format_values(buffer, "{} ", (int)prop.ldata_u8[idx]);
          auto vcount = prop.is_list ? prop.ldata_u8[idx] : 1;
          for (auto i = 0; i < vcount; i++) {
            switch (prop.type) {
              case ply_type::i8:
                format_values(buffer, "{} ", prop.data_i8[cur[idx]++]);
                break;
              case ply_type::i16:
                format_values(buffer, "{} ", prop.data_i16[cur[idx]++]);
                break;
              case ply_type::i32:
                format_values(buffer, "{} ", prop.data_i32[cur[idx]++]);
                break;
              case ply_type::i64:
                format_values(buffer, "{} ", prop.data_i64[cur[idx]++]);
                break;
              case ply_type::u8:
                format_values(buffer, "{} ", prop.data_u8[cur[idx]++]);
                break;
              case ply_type::u16:
                format_values(buffer, "{} ", prop.data_u16[cur[idx]++]);
                break;
              case ply_type::u32:
                format_values(buffer, "{} ", prop.data_u32[cur[idx]++]);
                break;
              case ply_type::u64:
                format_values(buffer, "{} ", prop.data_u64[cur[idx]++]);
                break;
              case ply_type::f32:
                format_values(buffer, "{} ", prop.data_f32[cur[idx]++]);
                break;
              case ply_type::f64:
                format_values(buffer, "{} ", prop.data_f64[cur[idx]++]);
                break;
            }
          }
          format_values(buffer, "\n");
        }
      }
    }

    // save file
    if (!save_text(filename, buffer, error)) return false;
  } else {
    // buffer
    auto buffer = vector<byte>{
        (const byte*)header.data(), (const byte*)header.data() + header.size()};

    auto big_endian = ply.format == ply_format::binary_big_endian;
    for (auto& elem : ply.elements) {
      auto cur = vector<size_t>(elem.properties.size(), 0);
      for (auto idx = 0; idx < elem.count; idx++) {
        for (auto pidx = 0; pidx < elem.properties.size(); pidx++) {
          auto& prop = elem.properties[pidx];
          if (prop.is_list) write_value(buffer, prop.ldata_u8[idx], big_endian);
          auto vcount = prop.is_list ? prop.ldata_u8[idx] : 1;
          for (auto i = 0; i < vcount; i++) {
            switch (prop.type) {
              case ply_type::i8:
                write_value(buffer, prop.data_i8[cur[pidx]++], big_endian);
                break;
              case ply_type::i16:
                write_value(buffer, prop.data_i16[cur[pidx]++], big_endian);
                break;
              case ply_type::i32:
                write_value(buffer, prop.data_i32[cur[pidx]++], big_endian);
                break;
              case ply_type::i64:
                write_value(buffer, prop.data_i64[cur[pidx]++], big_endian);
                break;
              case ply_type::u8:
                write_value(buffer, prop.data_u8[cur[pidx]++], big_endian);
                break;
              case ply_type::u16:
                write_value(buffer, prop.data_u16[cur[pidx]++], big_endian);
                break;
              case ply_type::u32:
                write_value(buffer, prop.data_u32[cur[pidx]++], big_endian);
                break;
              case ply_type::u64:
                write_value(buffer, prop.data_u64[cur[pidx]++], big_endian);
                break;
              case ply_type::f32:
                write_value(buffer, prop.data_f32[cur[pidx]++], big_endian);
                break;
              case ply_type::f64:
                write_value(buffer, prop.data_f64[cur[pidx]++], big_endian);
                break;
            }
          }
        }
      }
    }

    // save file
    if (!save_binary(filename, buffer, error)) return false;
  }

  // done
  return true;
}

// Get ply properties
bool has_property(
    const ply_model& ply, const string& element, const string& property) {
  for (auto& elem : ply.elements) {
    if (elem.name != element) continue;
    for (auto& prop : elem.properties) {
      if (prop.name == property) return true;
    }
  }
  return false;
}
ply_property& get_property(
    ply_model& ply, const string& element, const string& property) {
  for (auto& elem : ply.elements) {
    if (elem.name != element) continue;
    for (auto& prop : elem.properties) {
      if (prop.name == property) return prop;
    }
  }
  throw std::runtime_error("property not found");
}
const ply_property& get_property(
    const ply_model& ply, const string& element, const string& property) {
  for (auto& elem : ply.elements) {
    if (elem.name != element) continue;
    for (auto& prop : elem.properties) {
      if (prop.name == property) return prop;
    }
  }
  throw std::runtime_error("property not found");
}
template <typename T, typename T1>
static bool convert_property(const vector<T1>& prop, vector<T>& values) {
  values = vector<T>(prop.size());
  for (auto i = (size_t)0; i < prop.size(); i++) values[i] = (T)prop[i];
  return true;
}
template <typename T>
static bool convert_property(const ply_property& prop, vector<T>& values) {
  switch (prop.type) {
    case ply_type::i8: return convert_property(prop.data_i8, values);
    case ply_type::i16: return convert_property(prop.data_i16, values);
    case ply_type::i32: return convert_property(prop.data_i32, values);
    case ply_type::i64: return convert_property(prop.data_i64, values);
    case ply_type::u8: return convert_property(prop.data_u8, values);
    case ply_type::u16: return convert_property(prop.data_u16, values);
    case ply_type::u32: return convert_property(prop.data_u32, values);
    case ply_type::u64: return convert_property(prop.data_u64, values);
    case ply_type::f32: return convert_property(prop.data_f32, values);
    case ply_type::f64: return convert_property(prop.data_f64, values);
  }
  // return here to silence warnings
  throw std::runtime_error{"should not have gotten here"};
  return false;
}
bool get_value(const ply_model& ply, const string& element,
    const string& property, vector<float>& values) {
  values.clear();
  if (!has_property(ply, element, property)) return false;
  auto& prop = get_property(ply, element, property);
  if (prop.is_list) return false;
  if (!convert_property(prop, values)) return false;
  return true;
}
bool get_values(const ply_model& ply, const string& element,
    const array<string, 2>& properties, vector<vec2f>& values) {
  values.clear();
  auto x = vector<float>{}, y = vector<float>{};
  if (!get_value(ply, element, properties[0], x)) return false;
  if (!get_value(ply, element, properties[1], y)) return false;
  values = vector<vec2f>(x.size());
  for (auto i = (size_t)0; i < values.size(); i++) values[i] = {x[i], y[i]};
  return true;
}
bool get_values(const ply_model& ply, const string& element,
    const array<string, 3>& properties, vector<vec3f>& values) {
  values.clear();
  auto x = vector<float>{}, y = vector<float>{}, z = vector<float>{};
  if (!get_value(ply, element, properties[0], x)) return false;
  if (!get_value(ply, element, properties[1], y)) return false;
  if (!get_value(ply, element, properties[2], z)) return false;
  values = vector<vec3f>(x.size());
  for (auto i = (size_t)0; i < values.size(); i++)
    values[i] = {x[i], y[i], z[i]};
  return true;
}
bool get_values(const ply_model& ply, const string& element,
    const array<string, 4>& properties, vector<vec4f>& values) {
  values.clear();
  auto x = vector<float>{}, y = vector<float>{}, z = vector<float>{},
       w = vector<float>{};
  if (!get_value(ply, element, properties[0], x)) return false;
  if (!get_value(ply, element, properties[1], y)) return false;
  if (!get_value(ply, element, properties[2], z)) return false;
  if (!get_value(ply, element, properties[3], w)) return false;
  values = vector<vec4f>(x.size());
  for (auto i = (size_t)0; i < values.size(); i++)
    values[i] = {x[i], y[i], z[i], w[i]};
  return true;
}
bool get_values(const ply_model& ply, const string& element,
    const array<string, 12>& properties, vector<frame3f>& values) {
  values.clear();
  auto coords = array<vector<float>, 12>{};
  for (auto idx = 0; idx < 12; idx++)
    if (!get_value(ply, element, properties[idx], coords[idx])) return false;
  values = vector<frame3f>(coords[0].size());
  for (auto i = (size_t)0; i < values.size(); i++) {
    for (auto c = 0; c < 12; c++) (&values[i].x.x)[c] = coords[c][i];
  }
  return true;
}
bool get_lists(const ply_model& ply, const string& element,
    const string& property, vector<vector<int>>& lists) {
  lists.clear();
  if (!has_property(ply, element, property)) return false;
  auto& prop = get_property(ply, element, property);
  if (!prop.is_list) return false;
  auto& sizes  = prop.ldata_u8;
  auto  values = vector<int>{};
  if (!convert_property(prop, values)) return false;
  lists    = vector<vector<int>>(sizes.size());
  auto cur = (size_t)0;
  for (auto i = (size_t)0; i < lists.size(); i++) {
    lists[i].resize(sizes[i]);
    for (auto c = 0; c < sizes[i]; c++) {
      lists[i][c] = values[cur++];
    }
  }
  return true;
}
bool get_list_sizes(const ply_model& ply, const string& element,
    const string& property, vector<byte>& sizes) {
  if (!has_property(ply, element, property)) return {};
  auto& prop = get_property(ply, element, property);
  if (!prop.is_list) return {};
  sizes = prop.ldata_u8;
  return true;
}
bool get_list_values(const ply_model& ply, const string& element,
    const string& property, vector<int>& values) {
  if (!has_property(ply, element, property)) return {};
  auto& prop = get_property(ply, element, property);
  if (!prop.is_list) return {};
  return convert_property<int>(prop, values);
}

static vector<vec2f> flip_ply_texcoord(const vector<vec2f>& texcoords) {
  auto flipped = texcoords;
  for (auto& uv : flipped) uv.y = 1 - uv.y;
  return flipped;
}

// Get ply properties for meshes
bool get_positions(const ply_model& ply, vector<vec3f>& positions) {
  return get_values(ply, "vertex", {"x", "y", "z"}, positions);
}
bool get_normals(const ply_model& ply, vector<vec3f>& normals) {
  return get_values(ply, "vertex", {"nx", "ny", "nz"}, normals);
}
bool get_texcoords(const ply_model& ply, vector<vec2f>& texcoords, bool flipv) {
  if (has_property(ply, "vertex", "u")) {
    if (!get_values(ply, "vertex", {"u", "v"}, texcoords)) return false;
  } else {
    if (!get_values(ply, "vertex", {"s", "t"}, texcoords)) return false;
  }
  if (flipv) {
    for (auto& uv : texcoords) uv.y = 1 - uv.y;
  }
  return true;
}
bool get_colors(const ply_model& ply, vector<vec3f>& colors) {
  return get_values(ply, "vertex", {"red", "green", "blue"}, colors);
}
bool get_colors(const ply_model& ply, vector<vec4f>& colors) {
  if (has_property(ply, "vertex", "alpha")) {
    return get_values(ply, "vertex", {"red", "green", "blue", "alpha"}, colors);
  } else {
    auto colors3 = vector<vec3f>{};
    if (!get_values(ply, "vertex", {"red", "green", "blue"}, colors3))
      return false;
    colors.resize(colors3.size());
    for (auto i = 0; i < colors.size(); i++)
      colors[i] = {colors3[i].x, colors3[i].y, colors3[i].z, 1};
    return true;
  }
}
bool get_radius(const ply_model& ply, vector<float>& radius) {
  return get_value(ply, "vertex", "radius", radius);
}
bool get_faces(const ply_model& ply, vector<vector<int>>& faces) {
  return get_lists(ply, "face", "vertex_indices", faces);
}
bool get_triangles(const ply_model& ply, vector<vec3i>& triangles) {
  triangles.clear();
  auto indices = vector<int>{};
  auto sizes   = vector<uint8_t>{};
  if (!get_list_values(ply, "face", "vertex_indices", indices)) return false;
  if (!get_list_sizes(ply, "face", "vertex_indices", sizes)) return false;
  triangles = vector<vec3i>{};
  triangles.reserve(sizes.size());
  auto cur = 0;
  for (auto size : sizes) {
    for (auto c = 2; c < size; c++) {
      triangles.push_back(
          {indices[cur + 0], indices[cur + c - 1], indices[cur + c]});
    }
    cur += size;
  }
  return true;
}
bool get_quads(const ply_model& ply, vector<vec4i>& quads) {
  quads.clear();
  auto indices = vector<int>{};
  auto sizes   = vector<uint8_t>{};
  if (!get_list_values(ply, "face", "vertex_indices", indices)) return false;
  if (!get_list_sizes(ply, "face", "vertex_indices", sizes)) return false;
  quads = vector<vec4i>{};
  quads.reserve(sizes.size());
  auto cur = 0;
  for (auto size : sizes) {
    if (size == 4) {
      quads.push_back({indices[cur + 0], indices[cur + 1], indices[cur + 2],
          indices[cur + 3]});
    } else {
      for (auto c = 2; c < size; c++) {
        quads.push_back({indices[cur + 0], indices[cur + c - 1],
            indices[cur + c], indices[cur + c]});
      }
    }
    cur += size;
  }
  return true;
}
bool get_faces(
    const ply_model& ply, vector<vec3i>& triangles, vector<vec4i>& quads) {
  if (has_quads(ply)) {
    return get_quads(ply, quads);
  } else {
    return get_triangles(ply, triangles);
  }
}
bool get_lines(const ply_model& ply, vector<vec2i>& lines) {
  auto indices = vector<int>{};
  auto sizes   = vector<uint8_t>{};
  if (!get_list_values(ply, "line", "vertex_indices", indices)) return false;
  if (!get_list_sizes(ply, "line", "vertex_indices", sizes)) return false;
  lines = vector<vec2i>{};
  lines.reserve(sizes.size());
  auto cur = 0;
  for (auto size : sizes) {
    for (auto c = 1; c < size; c++) {
      lines.push_back({indices[cur + c - 1], indices[cur + c]});
    }
    cur += size;
  }
  return true;
}
bool get_points(const ply_model& ply, vector<int>& values) {
  return get_list_values(ply, "point", "vertex_indices", values);
}
bool has_quads(const ply_model& ply) {
  auto sizes = vector<uint8_t>{};
  if (!get_list_sizes(ply, "face", "vertex_indices", sizes)) return false;
  for (auto size : sizes)
    if (size == 4) return true;
  return false;
}

// Add ply properties
static ply_element& add_element(
    ply_model& ply, const string& element_name, size_t count) {
  for (auto& elem : ply.elements) {
    if (elem.name == element_name) return elem;
  }
  auto& elem = ply.elements.emplace_back();
  elem.name  = element_name;
  elem.count = count;
  return elem;
}
static ply_property& add_property(ply_model& ply, const string& element_name,
    const string& property_name, size_t count, ply_type type, bool is_list) {
  add_element(ply, element_name, count);
  for (auto& elem : ply.elements) {
    if (elem.name != element_name) continue;
    for (auto& prop : elem.properties) {
      if (prop.name == property_name) return prop;
    }
    auto& prop   = elem.properties.emplace_back();
    prop.name    = property_name;
    prop.type    = type;
    prop.is_list = is_list;
    return prop;
  }
  throw std::invalid_argument{"should not have gotten here"};
}
template <typename T>
static vector<T> make_vector(const T* value, size_t count, int stride) {
  auto ret = vector<T>(count);
  for (auto idx = (size_t)0; idx < count; idx++) ret[idx] = value[idx * stride];
  return ret;
}

static bool add_values(ply_model& ply, const float* values, size_t count,
    const string& element, const string* properties, int nprops) {
  if (values == nullptr) return false;
  for (auto p = 0; p < nprops; p++) {
    add_property(ply, element, properties[p], count, ply_type::f32, false);
    auto& prop = get_property(ply, element, properties[p]);
    prop.data_f32.resize(count);
    for (auto i = 0; i < count; i++) prop.data_f32[i] = values[p + i * nprops];
  }
  return true;
}

static bool add_values(ply_model& ply, const int* values, size_t count,
    const string& element, const string* properties, int nprops) {
  if (values == nullptr) return false;
  for (auto p = 0; p < nprops; p++) {
    add_property(ply, element, properties[p], count, ply_type::i32, false);
    auto& prop = get_property(ply, element, properties[p]);
    prop.data_i32.resize(count);
    for (auto i = 0; i < count; i++) prop.data_i32[i] = values[p + i * nprops];
  }
  return true;
}

bool add_value(ply_model& ply, const string& element, const string& property,
    const vector<float>& values) {
  if (values.empty()) return false;
  auto properties = vector{property};
  return add_values(
      ply, values.data(), values.size(), element, properties.data(), 1);
}
bool add_values(ply_model& ply, const string& element,
    const array<string, 2>& properties, const vector<vec2f>& values) {
  if (values.empty()) return false;
  return add_values(
      ply, &values.front().x, values.size(), element, properties.data(), 2);
}
bool add_values(ply_model& ply, const string& element,
    const array<string, 3>& properties, const vector<vec3f>& values) {
  if (values.empty()) return false;
  return add_values(
      ply, &values.front().x, values.size(), element, properties.data(), 3);
}
bool add_values(ply_model& ply, const string& element,
    const array<string, 4>& properties, const vector<vec4f>& values) {
  if (values.empty()) return false;
  return add_values(
      ply, &values.front().x, values.size(), element, properties.data(), 4);
}
bool add_values(ply_model& ply, const string& element,
    const array<string, 12>& properties, const vector<frame3f>& values) {
  if (values.empty()) return false;
  return add_values(ply, &values.front().x.x, values.size(), element,
      properties.data(), (int)properties.size());
}

bool add_value(ply_model& ply, const string& element, const string& property,
    const vector<int>& values) {
  if (values.empty()) return false;
  auto properties = vector{property};
  return add_values(
      ply, values.data(), values.size(), element, properties.data(), 1);
}
bool add_values(ply_model& ply, const string& element,
    const array<string, 2>& properties, const vector<vec2i>& values) {
  if (values.empty()) return false;
  return add_values(
      ply, &values.front().x, values.size(), element, properties.data(), 2);
}
bool add_values(ply_model& ply, const string& element,
    const array<string, 3>& properties, const vector<vec3i>& values) {
  if (values.empty()) return false;
  return add_values(
      ply, &values.front().x, values.size(), element, properties.data(), 3);
}
bool add_values(ply_model& ply, const string& element,
    const array<string, 4>& properties, const vector<vec4i>& values) {
  if (values.empty()) return false;
  return add_values(
      ply, &values.front().x, values.size(), element, properties.data(), 4);
}

bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vector<int>>& values) {
  if (values.empty()) return false;
  add_property(ply, element, property, values.size(), ply_type::i32, true);
  auto& prop = get_property(ply, element, property);
  prop.data_i32.reserve(values.size() * 4);
  prop.ldata_u8.reserve(values.size());
  for (auto& value : values) {
    prop.data_i32.insert(prop.data_i32.end(), value.begin(), value.end());
    prop.ldata_u8.push_back((uint8_t)value.size());
  }
  return true;
}
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<byte>& sizes, const vector<int>& values) {
  if (values.empty()) return false;
  add_property(ply, element, property, sizes.size(), ply_type::i32, true);
  auto& prop    = get_property(ply, element, property);
  prop.data_i32 = values;
  prop.ldata_u8 = sizes;
  return true;
}
bool add_lists(ply_model& ply, const int* values, size_t count, int size,
    const string& element, const string& property) {
  if (values == nullptr) return false;
  add_property(ply, element, property, count, ply_type::i32, true);
  auto& prop = get_property(ply, element, property);
  prop.data_i32.assign(values, values + count * size);
  prop.ldata_u8.assign(count, (byte)size);
  return true;
}
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<int>& values) {
  if (values.empty()) return false;
  return add_lists(ply, values.data(), values.size(), 1, element, property);
}
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vec2i>& values) {
  if (values.empty()) return false;
  return add_lists(ply, &values.front().x, values.size(), 2, element, property);
}
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vec3i>& values) {
  if (values.empty()) return false;
  return add_lists(ply, &values.front().x, values.size(), 3, element, property);
}
bool add_lists(ply_model& ply, const string& element, const string& property,
    const vector<vec4i>& values) {
  if (values.empty()) return false;
  return add_lists(ply, &values.front().x, values.size(), 4, element, property);
}

// Add ply properties for meshes
bool add_positions(ply_model& ply, const vector<vec3f>& values) {
  return add_values(ply, "vertex", {"x", "y", "z"}, values);
}
bool add_normals(ply_model& ply, const vector<vec3f>& values) {
  return add_values(ply, "vertex", {"nx", "ny", "nz"}, values);
}
bool add_texcoords(ply_model& ply, const vector<vec2f>& values, bool flipv) {
  return add_values(
      ply, "vertex", {"u", "v"}, flipv ? flip_ply_texcoord(values) : values);
}
bool add_colors(ply_model& ply, const vector<vec3f>& values) {
  return add_values(ply, "vertex", {"red", "green", "blue"}, values);
}
bool add_colors(ply_model& ply, const vector<vec4f>& values) {
  return add_values(ply, "vertex", {"red", "green", "blue", "alpha"}, values);
}
bool add_radius(ply_model& ply, const vector<float>& values) {
  return add_value(ply, "vertex", "radius", values);
}
bool add_faces(ply_model& ply, const vector<vector<int>>& values) {
  return add_lists(ply, "face", "vertex_indices", values);
}
bool add_faces(ply_model& ply, const vector<vec3i>& triangles,
    const vector<vec4i>& quads) {
  if (triangles.empty() && quads.empty()) return false;
  if (quads.empty()) {
    return add_lists(ply, "face", "vertex_indices", triangles);
  } else if (triangles.empty() &&
             std::all_of(quads.begin(), quads.end(),
                 [](const vec4i& q) { return q.z != q.w; })) {
    return add_lists(ply, "face", "vertex_indices", quads);
  } else {
    auto sizes   = vector<uint8_t>();
    auto indices = vector<int>{};
    sizes.reserve(triangles.size() + quads.size());
    indices.reserve(triangles.size() * 3 + quads.size() * 4);
    for (auto& t : triangles) {
      sizes.push_back(3);
      indices.push_back(t.x);
      indices.push_back(t.y);
      indices.push_back(t.z);
    }
    for (auto& q : quads) {
      sizes.push_back(q.z == q.w ? 3 : 4);
      indices.push_back(q.x);
      indices.push_back(q.y);
      indices.push_back(q.z);
      if (q.z != q.w) indices.push_back(q.w);
    }
    return add_lists(ply, "face", "vertex_indices", sizes, indices);
  }
}
bool add_triangles(ply_model& ply, const vector<vec3i>& values) {
  return add_faces(ply, values, {});
}
bool add_quads(ply_model& ply, const vector<vec4i>& values) {
  return add_faces(ply, {}, values);
}
bool add_lines(ply_model& ply, const vector<vec2i>& values) {
  return add_lists(ply, "line", "vertex_indices", values);
}
bool add_points(ply_model& ply, const vector<int>& values) {
  return add_lists(ply, "point", "vertex_indices", values);
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// IMPLEMENTATION FOR OBJ LOADER AND WRITER
// -----------------------------------------------------------------------------
namespace yocto {

[[nodiscard]] static bool parse_value(string_view& str, obj_vertex& value) {
  value = obj_vertex{0, 0, 0};
  if (!parse_value(str, value.position)) return false;
  if (!str.empty() && str.front() == '/') {
    str.remove_prefix(1);
    if (!str.empty() && str.front() == '/') {
      str.remove_prefix(1);
      if (!parse_value(str, value.normal)) return false;
    } else {
      if (!parse_value(str, value.texcoord)) return false;
      if (!str.empty() && str.front() == '/') {
        str.remove_prefix(1);
        if (!parse_value(str, value.normal)) return false;
      }
    }
  }
  return true;
}

// Input for OBJ textures
[[nodiscard]] static bool parse_value(string_view& str, obj_texture& info) {
  // initialize
  info = obj_texture();

  // get tokens
  auto tokens = vector<string>();
  skip_whitespace(str);
  while (!str.empty()) {
    auto token = ""s;
    if (!parse_value(str, token)) return false;
    tokens.push_back(token);
    skip_whitespace(str);
  }
  if (tokens.empty()) return false;

  // texture name
  info.path = tokens.back();
  for (auto& c : info.path)
    if (c == '\\') c = '/';

  // texture params
  auto last = string();
  for (auto i = 0; i < tokens.size() - 1; i++) {
    if (tokens[i] == "-bm") info.scale = (float)atof(tokens[i + 1].c_str());
    if (tokens[i] == "-clamp") info.clamp = true;
  }

  return true;
}

// Read obj
static bool load_mtl(const string& filename, obj_model& obj, string& error) {
  // texture map
  auto texture_map = unordered_map<string, int>{};
  auto texture_id  = 0;
  for (auto& texture : obj.textures) texture_map[texture.path] = texture_id++;
  auto parse_texture = [&texture_map, &obj](string_view& str, int& texture_id) {
    auto texture_path = obj_texture{};
    if (!parse_value(str, texture_path)) return false;
    auto texture_it = texture_map.find(texture_path.path);
    if (texture_it == texture_map.end()) {
      auto& texture             = obj.textures.emplace_back();
      texture.path              = texture_path.path;
      texture_id                = (int)obj.textures.size() - 1;
      texture_map[texture.path] = texture_id;
    } else {
      texture_id = texture_it->second;
    }
    return true;
  };

  // load data
  auto data = string{};
  if (!load_text(filename, data, error)) return false;

  // init parsing
  obj.materials.emplace_back();

  // read the file str by str
  auto data_view   = string_view{data.data(), data.size()};
  auto str         = string_view{};
  auto parse_error = [&filename, &error]() {
    error = filename + ": parse error";
    return false;
  };
  while (read_line(data_view, str)) {
    // str
    remove_comment(str);
    skip_whitespace(str);
    if (str.empty()) continue;

    // get command
    auto cmd = ""s;
    if (!parse_value(str, cmd)) return parse_error();
    if (cmd.empty()) continue;

    // grab material
    auto& material = obj.materials.back();

    // possible token values
    if (cmd == "newmtl") {
      auto& material = obj.materials.emplace_back();
      if (!parse_value(str, material.name)) return parse_error();
    } else if (cmd == "illum") {
      if (!parse_value(str, material.illum)) return parse_error();
    } else if (cmd == "Ke") {
      if (!parse_value(str, material.emission)) return parse_error();
    } else if (cmd == "Ka") {
      if (!parse_value(str, material.ambient)) return parse_error();
    } else if (cmd == "Kd") {
      if (!parse_value(str, material.diffuse)) return parse_error();
    } else if (cmd == "Ks") {
      if (!parse_value(str, material.specular)) return parse_error();
    } else if (cmd == "Kt") {
      if (!parse_value(str, material.transmission)) return parse_error();
    } else if (cmd == "Tf") {
      if (!parse_value(str, material.transmission)) return parse_error();
      material.transmission = max(1 - material.transmission, 0.0f);
      if (max(material.transmission) < 0.001) material.transmission = {0, 0, 0};
    } else if (cmd == "Tr") {
      if (!parse_value(str, material.opacity)) return parse_error();
      material.opacity = 1 - material.opacity;
    } else if (cmd == "Ns") {
      if (!parse_value(str, material.exponent)) return parse_error();
    } else if (cmd == "d") {
      if (!parse_value(str, material.opacity)) return parse_error();
    } else if (cmd == "map_Ke") {
      if (!parse_texture(str, material.emission_tex)) return parse_error();
    } else if (cmd == "map_Ka") {
      if (!parse_texture(str, material.ambient_tex)) return parse_error();
    } else if (cmd == "map_Kd") {
      if (!parse_texture(str, material.diffuse_tex)) return parse_error();
    } else if (cmd == "map_Ks") {
      if (!parse_texture(str, material.specular_tex)) return parse_error();
    } else if (cmd == "map_Tr") {
      if (!parse_texture(str, material.transmission_tex)) return parse_error();
    } else if (cmd == "map_d" || cmd == "map_Tr") {
      if (!parse_texture(str, material.opacity_tex)) return parse_error();
    } else if (cmd == "map_bump" || cmd == "bump") {
      if (!parse_texture(str, material.bump_tex)) return parse_error();
    } else if (cmd == "map_disp" || cmd == "disp") {
      if (!parse_texture(str, material.displacement_tex)) return parse_error();
    } else if (cmd == "map_norm" || cmd == "norm") {
      if (!parse_texture(str, material.normal_tex)) return parse_error();
    } else {
      continue;
    }
  }

  // remove placeholder material
  obj.materials.erase(obj.materials.begin());

  // done
  return true;
}

// Read obj
static bool load_obx(const string& filename, obj_model& obj, string& error) {
  // texture map
  auto texture_map = unordered_map<string, int>{};
  auto texture_id  = 0;
  for (auto& texture : obj.textures) texture_map[texture.path] = texture_id++;
  auto parse_texture = [&texture_map, &obj](string_view& str, int& texture_id) {
    auto texture_path = obj_texture{};
    if (!parse_value(str, texture_path)) return false;
    auto texture_it = texture_map.find(texture_path.path);
    if (texture_it == texture_map.end()) {
      auto& texture             = obj.textures.emplace_back();
      texture.path              = texture_path.path;
      texture_id                = (int)obj.textures.size() - 1;
      texture_map[texture.path] = texture_id;
    } else {
      texture_id = texture_it->second;
    }
    return true;
  };

  // load data
  auto data = string{};
  if (!load_text(filename, data, error)) return false;

  // init parsing
  obj.cameras.emplace_back();
  obj.environments.emplace_back();

  // read the file str by str
  auto data_view   = string_view{data.data(), data.size()};
  auto str         = string_view{};
  auto parse_error = [&filename, &error]() {
    error = filename + ": parse error";
    return false;
  };
  while (read_line(data_view, str)) {
    // str
    remove_comment(str);
    skip_whitespace(str);
    if (str.empty()) continue;

    // get command
    auto cmd = ""s;
    if (!parse_value(str, cmd)) return parse_error();
    if (cmd.empty()) continue;

    // grab elements
    auto& camera      = obj.cameras.back();
    auto& environment = obj.environments.back();

    // read values
    if (cmd == "newCam") {
      auto& camera = obj.cameras.emplace_back();
      if (!parse_value(str, camera.name)) return parse_error();
    } else if (cmd == "Co") {
      if (!parse_value(str, camera.ortho)) return parse_error();
    } else if (cmd == "Ca") {
      if (!parse_value(str, camera.aspect)) return parse_error();
    } else if (cmd == "Cl") {
      if (!parse_value(str, camera.lens)) return parse_error();
    } else if (cmd == "Cs") {
      if (!parse_value(str, camera.film)) return parse_error();
    } else if (cmd == "Cf") {
      if (!parse_value(str, camera.focus)) return parse_error();
    } else if (cmd == "Cp") {
      if (!parse_value(str, camera.aperture)) return parse_error();
    } else if (cmd == "Cx") {
      if (!parse_value(str, camera.frame)) return parse_error();
    } else if (cmd == "Ct") {
      auto lookat = mat3f{};
      if (!parse_value(str, lookat)) return parse_error();
      camera.frame = lookat_frame(lookat.x, lookat.y, lookat.z);
      if (camera.focus == 0) camera.focus = length(lookat.y - lookat.x);
    } else if (cmd == "newEnv") {
      auto& environment = obj.environments.emplace_back();
      if (!parse_value(str, environment.name)) return parse_error();
    } else if (cmd == "Ee") {
      if (!parse_value(str, environment.emission)) return parse_error();
    } else if (cmd == "map_Ee") {
      if (!parse_texture(str, environment.emission_tex)) return parse_error();
    } else if (cmd == "Ex") {
      if (!parse_value(str, environment.frame)) return parse_error();
    } else if (cmd == "Et") {
      auto lookat = mat3f{};
      if (!parse_value(str, lookat)) return parse_error();
      environment.frame = lookat_frame(lookat.x, lookat.y, lookat.z, true);
    } else {
      // unused
    }
  }

  // remove placeholders
  obj.cameras.erase(obj.cameras.begin());
  obj.environments.erase(obj.environments.begin());

  // done
  return true;
}

// Read obj
bool load_obj(const string& filename, obj_model& obj, string& error,
    bool face_varying, bool split_materials) {
  // load data
  auto data = string{};
  if (!load_text(filename, data, error)) return false;

  // parsing state
  auto opositions   = vector<vec3f>{};
  auto onormals     = vector<vec3f>{};
  auto otexcoords   = vector<vec2f>{};
  auto oname        = ""s;
  auto gname        = ""s;
  auto mtllibs      = vector<string>{};
  auto material_map = unordered_map<string, int>{};
  int  cur_material = -1;
  auto cur_shape    = (obj_shape*)nullptr;
  auto cur_shapes   = unordered_map<int, int>{};

  // initialize obj
  obj       = {};
  cur_shape = &obj.shapes.emplace_back();
  if (split_materials) {
    cur_shapes = {{cur_material, (int)obj.shapes.size() - 1}};
  }

  // read the file str by str
  auto data_view   = string_view{data.data(), data.size()};
  auto str         = string_view{};
  auto parse_error = [&filename, &error]() {
    error = filename + ": parse error";
    return false;
  };
  auto dependent_error = [&filename, &error]() {
    error = filename + ": error in " + error;
    return false;
  };
  while (read_line(data_view, str)) {
    // str
    remove_comment(str);
    skip_whitespace(str);
    if (str.empty()) continue;

    // get command
    auto cmd = ""s;
    if (!parse_value(str, cmd)) return parse_error();
    if (cmd.empty()) continue;

    // possible token values
    if (cmd == "v") {
      if (!parse_value(str, opositions.emplace_back())) return parse_error();
    } else if (cmd == "vn") {
      if (!parse_value(str, onormals.emplace_back())) return parse_error();
    } else if (cmd == "vt") {
      if (!parse_value(str, otexcoords.emplace_back())) return parse_error();
    } else if (cmd == "f" || cmd == "l" || cmd == "p") {
      // elemnet type
      auto etype = (cmd == "f")   ? obj_etype::face
                   : (cmd == "l") ? obj_etype::line
                                  : obj_etype::point;
      // grab shape and add element
      auto& shape   = *cur_shape;
      auto& element = shape.elements.emplace_back();
      if (cur_material < 0) {
        auto& material              = obj.materials.emplace_back();
        material.name               = "__default__";
        material.diffuse            = {0.8f, 0.8f, 0.8f};
        cur_material                = 0;
        material_map[material.name] = 0;
      }
      element.material = cur_material;
      element.etype    = etype;
      // parse vertices
      skip_whitespace(str);
      while (!str.empty()) {
        auto vert = obj_vertex{};
        if (!parse_value(str, vert)) return parse_error();
        if (vert.position == 0) break;
        if (vert.position < 0)
          vert.position = (int)opositions.size() + vert.position + 1;
        if (vert.texcoord < 0)
          vert.texcoord = (int)otexcoords.size() + vert.texcoord + 1;
        if (vert.normal < 0)
          vert.normal = (int)onormals.size() + vert.normal + 1;
        shape.vertices.push_back(vert);
        element.size += 1;
        skip_whitespace(str);
      }
    } else if (cmd == "o" || cmd == "g") {
      skip_whitespace(str);
      auto& name = cmd == "o" ? oname : gname;
      if (str.empty()) {
        name = "";
      } else {
        if (!parse_value(str, name)) return parse_error();
      }
      if (split_materials) {
        cur_shape       = &obj.shapes.emplace_back();
        cur_shapes      = {{cur_material, (int)obj.shapes.size() - 1}};
        cur_shape->name = oname + gname;
      } else {
        if (!cur_shape->vertices.empty()) {
          cur_shape = &obj.shapes.emplace_back();
        }
        cur_shape->name = oname + gname;
      }
    } else if (cmd == "usemtl") {
      auto mname = string{};
      if (!parse_value(str, mname)) return parse_error();
      auto material_it = material_map.find(mname);
      if (material_it == material_map.end()) return parse_error();
      if (split_materials && cur_material != material_it->second) {
        cur_material  = material_it->second;
        auto shape_it = cur_shapes.find(cur_material);
        if (shape_it == cur_shapes.end()) {
          cur_shape                = &obj.shapes.emplace_back();
          cur_shapes[cur_material] = (int)obj.shapes.size() - 1;
          cur_shape->name          = oname + gname;
        } else {
          cur_shape = &obj.shapes.at(shape_it->second);
        }
      } else {
        cur_material = material_it->second;
      }
    } else if (cmd == "mtllib") {
      auto mtllib = ""s;
      if (!parse_value(str, mtllib)) return parse_error();
      if (std::find(mtllibs.begin(), mtllibs.end(), mtllib) == mtllibs.end()) {
        mtllibs.push_back(mtllib);
        if (!load_mtl(path_join(path_dirname(filename), mtllib), obj, error))
          return dependent_error();
        auto material_id = 0;
        for (auto& material : obj.materials)
          material_map[material.name] = material_id++;
      }
    } else {
      // unused
    }
  }

  // remove empty shapes if splitting by materials
  if (split_materials) {
    obj.shapes.erase(
        std::remove_if(obj.shapes.begin(), obj.shapes.end(),
            [](const obj_shape& shape) { return shape.elements.empty(); }),
        obj.shapes.end());
  }

  // convert vertex data
  if (face_varying) {
    auto ipositions = vector<int>{};
    auto inormals   = vector<int>{};
    auto itexcoords = vector<int>{};
    for (auto& shape : obj.shapes) {
      ipositions.assign(opositions.size() + 1, 0);
      inormals.assign(onormals.size() + 1, 0);
      itexcoords.assign(otexcoords.size() + 1, 0);
      for (auto& vertex : shape.vertices) {
        if (vertex.position != 0 && ipositions[vertex.position] == 0) {
          shape.positions.push_back(opositions[vertex.position - 1]);
          ipositions[vertex.position] = (int)shape.positions.size();
        }
        if (vertex.normal != 0 && inormals[vertex.normal] == 0) {
          shape.normals.push_back(onormals[vertex.normal - 1]);
          inormals[vertex.normal] = (int)shape.normals.size();
        }
        if (vertex.texcoord != 0 && itexcoords[vertex.texcoord] == 0) {
          shape.texcoords.push_back(otexcoords[vertex.texcoord - 1]);
          itexcoords[vertex.texcoord] = (int)shape.texcoords.size();
        }
        vertex.position = ipositions[vertex.position];
        vertex.normal   = inormals[vertex.normal];
        vertex.texcoord = itexcoords[vertex.texcoord];
      }
    }
  } else {
    auto vertex_map = unordered_map<obj_vertex, obj_vertex>{};
    for (auto& shape : obj.shapes) {
      vertex_map.clear();
      for (auto& vertex : shape.vertices) {
        auto vertex_it = vertex_map.find(vertex);
        if (vertex_it == vertex_map.end()) {
          auto new_vertex = vertex;
          auto index      = (int)vertex_map.size();
          if (vertex.position > 0) {
            shape.positions.push_back(opositions[vertex.position - 1]);
            new_vertex.position = index + 1;
          }
          if (vertex.normal > 0) {
            shape.normals.push_back(onormals[vertex.normal - 1]);
            new_vertex.normal = index + 1;
          }
          if (vertex.texcoord > 0) {
            shape.texcoords.push_back(otexcoords[vertex.texcoord - 1]);
            new_vertex.texcoord = index + 1;
          }
          vertex_map[vertex] = new_vertex;
          vertex             = new_vertex;
        } else {
          vertex = vertex_it->second;
        }
      }
    }
  }

  // load extensions
  auto extfilename = replace_extension(filename, ".obx");
  if (path_exists(extfilename)) {
    if (!load_obx(extfilename, obj, error)) return dependent_error();
  }

  // done
  return true;
}

// Read obj
bool load_obj(const string& filename, obj_shape& shape, string& error,
    bool face_varying) {
  // load data
  auto data = string{};
  if (!load_text(filename, data, error)) return false;

  // parsing state
  auto material_map = unordered_map<string, int>{};
  int  cur_material = -1;

  // initialize obj
  shape = {};

  // read the file str by str
  auto data_view   = string_view{data.data(), data.size()};
  auto str         = string_view{};
  auto parse_error = [&filename, &error]() {
    error = filename + ": parse error";
    return false;
  };
  while (read_line(data_view, str)) {
    // str
    remove_comment(str);
    skip_whitespace(str);
    if (str.empty()) continue;

    // get command
    auto cmd = ""s;
    if (!parse_value(str, cmd)) return parse_error();
    if (cmd.empty()) continue;

    // possible token values
    if (cmd == "v") {
      if (!parse_value(str, shape.positions.emplace_back()))
        return parse_error();
    } else if (cmd == "vn") {
      if (!parse_value(str, shape.normals.emplace_back())) return parse_error();
    } else if (cmd == "vt") {
      if (!parse_value(str, shape.texcoords.emplace_back()))
        return parse_error();
    } else if (cmd == "f" || cmd == "l" || cmd == "p") {
      // elemnet type
      auto etype = (cmd == "f")   ? obj_etype::face
                   : (cmd == "l") ? obj_etype::line
                                  : obj_etype::point;
      // grab shape and add element
      auto& element    = shape.elements.emplace_back();
      element.material = cur_material;
      element.etype    = etype;
      // parse vertices
      skip_whitespace(str);
      while (!str.empty()) {
        auto vert = obj_vertex{};
        if (!parse_value(str, vert)) return parse_error();
        if (vert.position == 0) break;
        if (vert.position < 0)
          vert.position = (int)shape.positions.size() + vert.position + 1;
        if (vert.texcoord < 0)
          vert.texcoord = (int)shape.texcoords.size() + vert.texcoord + 1;
        if (vert.normal < 0)
          vert.normal = (int)shape.normals.size() + vert.normal + 1;
        shape.vertices.push_back(vert);
        element.size += 1;
        skip_whitespace(str);
      }
    } else if (cmd == "usemtl") {
      auto mname = string{};
      if (!parse_value(str, mname)) return parse_error();
      auto material_it = material_map.find(mname);
      if (material_it == material_map.end()) {
        cur_material        = (int)material_map.size();
        material_map[mname] = cur_material;
      } else {
        cur_material = material_it->second;
      }
    } else {
      // unused
    }
  }

  // convert vertex data
  if (!face_varying) {
    auto opositions = vector<vec3f>{};
    auto onormals   = vector<vec3f>{};
    auto otexcoords = vector<vec2f>{};
    shape.positions.swap(opositions);
    shape.normals.swap(onormals);
    shape.texcoords.swap(otexcoords);
    auto vertex_map = unordered_map<obj_vertex, obj_vertex>{};
    for (auto& vertex : shape.vertices) {
      auto vertex_it = vertex_map.find(vertex);
      if (vertex_it == vertex_map.end()) {
        auto new_vertex = vertex;
        auto index      = (int)vertex_map.size();
        if (vertex.position > 0) {
          shape.positions.push_back(opositions[vertex.position - 1]);
          new_vertex.position = index + 1;
        }
        if (vertex.normal > 0) {
          shape.normals.push_back(onormals[vertex.normal - 1]);
          new_vertex.normal = index + 1;
        }
        if (vertex.texcoord > 0) {
          shape.texcoords.push_back(otexcoords[vertex.texcoord - 1]);
          new_vertex.texcoord = index + 1;
        }
        vertex_map[vertex] = new_vertex;
        vertex             = new_vertex;
      } else {
        vertex = vertex_it->second;
      }
    }
  }

  // done
  return true;
}

// Format values
[[maybe_unused]] static void format_value(
    string& str, const obj_texture& value) {
  str += value.path.empty() ? "" : value.path;
}
static void format_value(string& str, const obj_vertex& value) {
  format_value(str, value.position);
  if (value.texcoord != 0) {
    str += "/";
    format_value(str, value.texcoord);
    if (value.normal != 0) {
      str += "/";
      format_value(str, value.normal);
    }
  } else if (value.normal != 0) {
    str += "//";
    format_value(str, value.normal);
  }
}

// Save obj
static bool save_mtl(
    const string& filename, const obj_model& obj, string& error) {
  // buffer
  auto buffer = string{};

  // save comments
  format_values(buffer, "#\n");
  format_values(buffer, "# Written by Yocto/GL\n");
  format_values(buffer, "# https://github.com/xelatihy/yocto-gl\n");
  format_values(buffer, "#\n\n");
  for (auto& comment : obj.comments) {
    format_values(buffer, "# {}\n", comment);
  }
  format_values(buffer, "\n");

  // write material
  for (auto& material : obj.materials) {
    format_values(buffer, "newmtl {}\n", material.name);
    format_values(buffer, "illum {}\n", material.illum);
    if (material.emission != vec3f{0, 0, 0})
      format_values(buffer, "Ke {}\n", material.emission);
    if (material.ambient != vec3f{0, 0, 0})
      format_values(buffer, "Ka {}\n", material.ambient);
    format_values(buffer, "Kd {}\n", material.diffuse);
    format_values(buffer, "Ks {}\n", material.specular);
    if (material.reflection != vec3f{0, 0, 0})
      format_values(buffer, "Kr {}\n", material.reflection);
    if (material.transmission != vec3f{0, 0, 0})
      format_values(buffer, "Kt {}\n", material.transmission);
    format_values(buffer, "Ns {}\n", (int)material.exponent);
    if (material.opacity != 1)
      format_values(buffer, "d {}\n", material.opacity);
    if (material.emission_tex >= 0)
      format_values(
          buffer, "map_Ke {}\n", obj.textures[material.emission_tex].path);
    if (material.diffuse_tex >= 0)
      format_values(
          buffer, "map_Kd {}\n", obj.textures[material.diffuse_tex].path);
    if (material.specular_tex >= 0)
      format_values(
          buffer, "map_Ks {}\n", obj.textures[material.specular_tex].path);
    if (material.transmission_tex >= 0)
      format_values(
          buffer, "map_Kt {}\n", obj.textures[material.transmission_tex].path);
    if (material.reflection_tex >= 0)
      format_values(
          buffer, "map_Kr {}\n", obj.textures[material.reflection_tex].path);
    if (material.exponent_tex >= 0)
      format_values(
          buffer, "map_Ns {}\n", obj.textures[material.exponent_tex].path);
    if (material.opacity_tex >= 0)
      format_values(
          buffer, "map_d {}\n", obj.textures[material.opacity_tex].path);
    if (material.bump_tex >= 0)
      format_values(
          buffer, "map_bump {}\n", obj.textures[material.bump_tex].path);
    if (material.displacement_tex >= 0)
      format_values(buffer, "map_disp {}\n",
          obj.textures[material.displacement_tex].path);
    if (material.normal_tex >= 0)
      format_values(
          buffer, "map_norm {}\n", obj.textures[material.normal_tex].path);
    format_values(buffer, "\n");
  }

  // save file
  if (!save_text(filename, buffer, error)) return false;

  // done
  return true;
}

// Save obj
static bool save_obx(
    const string& filename, const obj_model& obj, string& error) {
  // buffer
  auto buffer = string{};

  // save comments
  format_values(buffer, "#\n");
  format_values(buffer, "# Written by Yocto/GL\n");
  format_values(buffer, "# https://github.com/xelatihy/yocto-gl\n");
  format_values(buffer, "#\n\n");
  for (auto& comment : obj.comments) {
    format_values(buffer, "# {}\n", comment);
  }
  format_values(buffer, "\n");

  // cameras
  for (auto& camera : obj.cameras) {
    format_values(buffer, "newCam {}\n", camera.name);
    format_values(buffer, "  Co {}\n", camera.ortho);
    format_values(buffer, "  Ca {}\n", camera.aspect);
    format_values(buffer, "  Cl {}\n", camera.lens);
    format_values(buffer, "  Cs {}\n", camera.film);
    format_values(buffer, "  Cf {}\n", camera.focus);
    format_values(buffer, "  Cp {}\n", camera.aperture);
    format_values(buffer, "  Cx {}\n", camera.frame);
  }

  // environments
  for (auto& environment : obj.environments) {
    format_values(buffer, "newEnv {}\n", environment.name);
    format_values(buffer, "  Ee {}\n", environment.emission);
    if (environment.emission_tex >= 0) {
      format_values(
          buffer, "  map_Ee {}\n", obj.textures[environment.emission_tex].path);
    }
    format_values(buffer, "  Ex {}\n", environment.frame);
  }

  // save file
  if (!save_text(filename, buffer, error)) return false;

  // done
  return true;
}

// Save obj
bool save_obj(const string& filename, const obj_model& obj, string& error) {
  // buffer
  auto buffer = string{};

  // save comments
  format_values(buffer, "#\n");
  format_values(buffer, "# Written by Yocto/GL\n");
  format_values(buffer, "# https://github.com/xelatihy/yocto-gl\n");
  format_values(buffer, "#\n\n");
  for (auto& comment : obj.comments) {
    format_values(buffer, "# {}\n", comment);
  }
  format_values(buffer, "\n");

  // save material library
  if (!obj.materials.empty()) {
    format_values(buffer, "mtllib {}\n\n",
        replace_extension(path_filename(filename), ".mtl"));
  }

  // save objects
  auto vert_size = obj_vertex{0, 0, 0};
  for (auto& shape : obj.shapes) {
    format_values(buffer, "o {}\n", shape.name);
    for (auto& p : shape.positions) format_values(buffer, "v {}\n", p);
    for (auto& n : shape.normals) format_values(buffer, "vn {}\n", n);
    for (auto& t : shape.texcoords) format_values(buffer, "vt {}\n", t);
    auto cur_material = -1, cur_vertex = 0;
    for (auto& element : shape.elements) {
      if (!obj.materials.empty() && cur_material != element.material) {
        format_values(
            buffer, "usemtl {}\n", obj.materials[element.material].name);
        cur_material = element.material;
      }
      if (element.etype == obj_etype::face) {
        format_values(buffer, "{}", "f");
      } else if (element.etype == obj_etype::line) {
        format_values(buffer, "{}", "l");
      } else if (element.etype == obj_etype::point) {
        format_values(buffer, "{}", "p");
      }
      for (auto c = 0; c < element.size; c++) {
        auto vert = shape.vertices[cur_vertex++];
        if (vert.position != 0) vert.position += vert_size.position;
        if (vert.normal != 0) vert.normal += vert_size.normal;
        if (vert.texcoord != 0) vert.texcoord += vert_size.texcoord;
        format_values(buffer, " {}", vert);
      }
      format_values(buffer, "\n");
    }
    format_values(buffer, "\n");
    vert_size.position += (int)shape.positions.size();
    vert_size.normal += (int)shape.normals.size();
    vert_size.texcoord += (int)shape.texcoords.size();
  }

  // save file
  if (!save_text(filename, buffer, error)) return false;

  auto dependent_error = [&filename, &error]() {
    error = filename + ": error in " + error;
    return false;
  };

  // save mtl
  if (!obj.materials.empty()) {
    if (!save_mtl(replace_extension(filename, ".mtl"), obj, error))
      return dependent_error();
  }

  // save obx
  if (!obj.cameras.empty() || !obj.environments.empty()) {
    if (!save_obx(replace_extension(filename, ".obx"), obj, error))
      return dependent_error();
  }

  // done
  return true;
}

// Save obj
bool save_obj(const string& filename, const obj_shape& shape, string& error) {
  // buffer
  auto buffer = string{};

  // save comments
  format_values(buffer, "#\n");
  format_values(buffer, "# Written by Yocto/GL\n");
  format_values(buffer, "# https://github.com/xelatihy/yocto-gl\n");
  format_values(buffer, "#\n\n");
  format_values(buffer, "\n");

  // save objects
  format_values(buffer, "o {}\n", shape.name);
  for (auto& p : shape.positions) format_values(buffer, "v {}\n", p);
  for (auto& n : shape.normals) format_values(buffer, "vn {}\n", n);
  for (auto& t : shape.texcoords) format_values(buffer, "vt {}\n", t);
  auto cur_material = -1, cur_vertex = 0;
  for (auto& element : shape.elements) {
    if (cur_material != element.material) {
      format_values(
          buffer, "usemtl {}\n", "material" + std::to_string(element.material));
      cur_material = element.material;
    }
    if (element.etype == obj_etype::face) {
      format_values(buffer, "{}", "f");
    } else if (element.etype == obj_etype::line) {
      format_values(buffer, "{}", "l");
    } else if (element.etype == obj_etype::point) {
      format_values(buffer, "{}", "p");
    }
    for (auto c = 0; c < element.size; c++) {
      auto& vert = shape.vertices[cur_vertex++];
      format_values(buffer, " {}", vert);
    }
    format_values(buffer, "\n");
  }

  // save file
  if (!save_text(filename, buffer, error)) return false;

  // done
  return true;
}

// Get obj shape.
void get_positions(const obj_shape& shape, vector<vec3f>& positions) {
  positions = shape.positions;
}
void get_normals(const obj_shape& shape, vector<vec3f>& normals) {
  normals = shape.normals;
}
void get_texcoords(
    const obj_shape& shape, vector<vec2f>& texcoords, bool flipv) {
  texcoords = shape.texcoords;
  if (flipv) {
    for (auto& texcoord : texcoords) texcoord.y = 1 - texcoord.y;
  }
}
void get_faces(const obj_shape& shape, vector<vec3i>& triangles,
    vector<vec4i>& quads, vector<int>& materials) {
  if (has_quads(shape)) {
    get_quads(shape, quads, materials);
  } else {
    get_triangles(shape, triangles, materials);
  }
}
void get_triangles(
    const obj_shape& shape, vector<vec3i>& triangles, vector<int>& materials) {
  triangles.clear();
  materials.clear();
  triangles.reserve(shape.elements.size());
  materials.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::face) continue;
    for (auto c = 2; c < element.size; c++) {
      triangles.push_back({shape.vertices[cur + 0].position - 1,
          shape.vertices[cur + c - 1].position - 1,
          shape.vertices[cur + c].position - 1});
      materials.push_back(element.material);
    }
    cur += element.size;
  }
}
void get_quads(
    const obj_shape& shape, vector<vec4i>& quads, vector<int>& materials) {
  quads.clear();
  materials.clear();
  quads.reserve(shape.elements.size());
  materials.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::face) continue;
    if (element.size == 4) {
      quads.push_back({shape.vertices[cur + 0].position - 1,
          shape.vertices[cur + 1].position - 1,
          shape.vertices[cur + 2].position - 1,
          shape.vertices[cur + 3].position - 1});
      materials.push_back(element.material);
    } else {
      for (auto c = 2; c < element.size; c++) {
        quads.push_back({shape.vertices[cur + 0].position - 1,
            shape.vertices[cur + c - 1].position - 1,
            shape.vertices[cur + c].position - 1,
            shape.vertices[cur + c].position - 1});
        materials.push_back(element.material);
      }
    }
    cur += element.size;
  }
}
void get_lines(
    const obj_shape& shape, vector<vec2i>& lines, vector<int>& materials) {
  lines.clear();
  materials.clear();
  lines.reserve(shape.elements.size());
  materials.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::line) continue;
    for (auto c = 1; c < element.size; c++) {
      lines.push_back({shape.vertices[cur + c - 1].position - 1,
          shape.vertices[cur + c].position - 1});
      materials.push_back(element.material);
    }
    cur += element.size;
  }
}
void get_points(
    const obj_shape& shape, vector<int>& points, vector<int>& materials) {
  points.clear();
  materials.clear();
  points.reserve(shape.elements.size());
  materials.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::point) continue;
    for (auto c = 0; c < element.size; c++) {
      points.push_back({shape.vertices[cur + 0].position - 1});
      materials.push_back(element.material);
    }
    cur += element.size;
  }
}
void get_fvquads(const obj_shape& shape, vector<vec4i>& quadspos,
    vector<vec4i>& quadsnorm, vector<vec4i>& quadstexcoord,
    vector<int>& materials) {
  quadspos.clear();
  quadsnorm.clear();
  quadstexcoord.clear();
  materials.clear();
  quadspos.reserve(shape.elements.size());
  quadsnorm.reserve(shape.elements.size());
  quadstexcoord.reserve(shape.elements.size());
  materials.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::face) continue;
    if (element.size == 4) {
      if (shape.vertices[0].position != 0)
        quadspos.push_back({shape.vertices[cur + 0].position - 1,
            shape.vertices[cur + 1].position - 1,
            shape.vertices[cur + 2].position - 1,
            shape.vertices[cur + 3].position - 1});
      if (shape.vertices[0].normal != 0)
        quadsnorm.push_back({shape.vertices[cur + 0].normal - 1,
            shape.vertices[cur + 1].normal - 1,
            shape.vertices[cur + 2].normal - 1,
            shape.vertices[cur + 3].normal - 1});
      if (shape.vertices[0].texcoord != 0)
        quadstexcoord.push_back({shape.vertices[cur + 0].texcoord - 1,
            shape.vertices[cur + 1].texcoord - 1,
            shape.vertices[cur + 2].texcoord - 1,
            shape.vertices[cur + 3].texcoord - 1});
      materials.push_back(element.material);
    } else {
      for (auto c = 2; c < element.size; c++) {
        if (shape.vertices[0].position != 0)
          quadspos.push_back({shape.vertices[cur + 0].position - 1,
              shape.vertices[cur + c - 1].position - 1,
              shape.vertices[cur + c].position - 1,
              shape.vertices[cur + c].position - 1});
        if (shape.vertices[0].normal != 0)
          quadsnorm.push_back({shape.vertices[cur + 0].normal - 1,
              shape.vertices[cur + c - 1].normal - 1,
              shape.vertices[cur + c].normal - 1,
              shape.vertices[cur + c].normal - 1});
        if (shape.vertices[0].texcoord != 0)
          quadstexcoord.push_back({shape.vertices[cur + 0].texcoord - 1,
              shape.vertices[cur + c - 1].texcoord - 1,
              shape.vertices[cur + c].texcoord - 1,
              shape.vertices[cur + c].texcoord - 1});
        materials.push_back(element.material);
      }
    }
    cur += element.size;
  }
}
void get_faces(const obj_shape& shape, int material, vector<vec3i>& triangles,
    vector<vec4i>& quads) {
  if (has_quads(shape)) {
    get_quads(shape, material, quads);
  } else {
    get_triangles(shape, material, triangles);
  }
}
void get_triangles(
    const obj_shape& shape, int material, vector<vec3i>& triangles) {
  triangles.clear();
  if (shape.elements.empty()) return;
  triangles.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::face) continue;
    if (element.material != material) continue;
    for (auto c = 2; c < element.size; c++) {
      triangles.push_back({shape.vertices[cur + 0].position - 1,
          shape.vertices[cur + c - 1].position - 1,
          shape.vertices[cur + c].position - 1});
    }
    cur += element.size;
  }
}
void get_quads(const obj_shape& shape, int material, vector<vec4i>& quads) {
  quads.clear();
  if (shape.elements.empty()) return;
  quads.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::face) continue;
    if (element.material != material) continue;
    if (element.size == 4) {
      quads.push_back({shape.vertices[cur + 0].position - 1,
          shape.vertices[cur + 1].position - 1,
          shape.vertices[cur + 2].position - 1,
          shape.vertices[cur + 3].position - 1});
    } else {
      for (auto c = 2; c < element.size; c++) {
        quads.push_back({shape.vertices[cur + 0].position - 1,
            shape.vertices[cur + c - 1].position - 1,
            shape.vertices[cur + c].position - 1,
            shape.vertices[cur + c].position - 1});
      }
    }
    cur += element.size;
  }
}
void get_lines(const obj_shape& shape, int material, vector<vec2i>& lines) {
  lines.clear();
  if (shape.elements.empty()) return;
  lines.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::line) continue;
    if (element.material != material) continue;
    for (auto c = 1; c < element.size; c++) {
      lines.push_back({shape.vertices[cur + c - 1].position - 1,
          shape.vertices[cur + c].position - 1});
    }
    cur += element.size;
  }
}
void get_points(const obj_shape& shape, int material, vector<int>& points) {
  points.clear();
  if (shape.elements.empty()) return;
  points.reserve(shape.elements.size());
  auto cur = 0;
  for (auto& element : shape.elements) {
    if (element.etype != obj_etype::point) continue;
    if (element.material != material) continue;
    for (auto c = 0; c < element.size; c++) {
      points.push_back({shape.vertices[cur + 0].position - 1});
    }
    cur += element.size;
  }
}

bool has_quads(const obj_shape& shape) {
  for (auto& element : shape.elements)
    if (element.etype == obj_etype::face && element.size == 4) return true;
  return false;
}

vector<int> get_materials(const obj_shape& shape) {
  auto materials    = vector<int>{};
  auto material_set = unordered_set<int>{};
  for (auto& element : shape.elements) {
    if (material_set.find(element.material) == material_set.end()) {
      material_set.insert(element.material);
      materials.push_back(element.material);
    }
  }
  return materials;
}

// Add obj shape
void add_positions(obj_shape& shape, const vector<vec3f>& positions) {
  shape.positions.insert(
      shape.positions.end(), positions.begin(), positions.end());
}
void add_normals(obj_shape& shape, const vector<vec3f>& normals) {
  shape.normals.insert(shape.normals.end(), normals.begin(), normals.end());
}
void add_texcoords(
    obj_shape& shape, const vector<vec2f>& texcoords, bool flipv) {
  shape.texcoords.insert(
      shape.texcoords.end(), texcoords.begin(), texcoords.end());
  if (flipv) {
    for (auto idx = shape.texcoords.size() - texcoords.size();
         idx < shape.texcoords.size(); idx++)
      shape.texcoords[idx].y = 1 - shape.texcoords[idx].y;
  }
}
void add_triangles(obj_shape& shape, const vector<vec3i>& triangles,
    int material, bool has_normals, bool has_texcoord) {
  for (auto idx = 0; idx < triangles.size(); idx++) {
    auto& triangle = triangles[idx];
    for (auto c = 0; c < 3; c++) {
      shape.vertices.push_back({
          triangle[c] + 1,
          !has_texcoord ? 0 : triangle[c] + 1,
          !has_normals ? 0 : triangle[c] + 1,
      });
    }
    shape.elements.push_back({3, obj_etype::face, material});
  }
}
void add_quads(obj_shape& shape, const vector<vec4i>& quads, int material,
    bool has_normals, bool has_texcoord) {
  for (auto idx = 0; idx < quads.size(); idx++) {
    auto& quad = quads[idx];
    auto  nv   = quad.z == quad.w ? 3 : 4;
    for (auto c = 0; c < nv; c++) {
      shape.vertices.push_back({
          quad[c] + 1,
          !has_texcoord ? 0 : quad[c] + 1,
          !has_normals ? 0 : quad[c] + 1,
      });
    }
    shape.elements.push_back({(uint16_t)nv, obj_etype::face, material});
  }
}
void add_lines(obj_shape& shape, const vector<vec2i>& lines, int material,
    bool has_normals, bool has_texcoord) {
  for (auto idx = 0; idx < lines.size(); idx++) {
    auto& line = lines[idx];
    for (auto c = 0; c < 2; c++) {
      shape.vertices.push_back({
          line[c] + 1,
          !has_texcoord ? 0 : line[c] + 1,
          !has_normals ? 0 : line[c] + 1,
      });
    }
    shape.elements.push_back({2, obj_etype::line, material});
  }
}
void add_points(obj_shape& shape, const vector<int>& points, int material,
    bool has_normals, bool has_texcoord) {
  for (auto idx = 0; idx < points.size(); idx++) {
    auto& point = points[idx];
    shape.vertices.push_back({
        point + 1,
        !has_texcoord ? 0 : point + 1,
        !has_normals ? 0 : point + 1,
    });
    shape.elements.push_back({1, obj_etype::point, material});
  }
}
void add_fvquads(obj_shape& shape, const vector<vec4i>& quadspos,
    const vector<vec4i>& quadsnorm, const vector<vec4i>& quadstexcoord,
    int material) {
  for (auto idx = 0; idx < quadspos.size(); idx++) {
    auto nv = quadspos[idx].z == quadspos[idx].w ? 3 : 4;
    for (auto c = 0; c < nv; c++) {
      shape.vertices.push_back({
          quadspos.empty() ? 0 : quadspos[idx][c] + 1,
          quadstexcoord.empty() ? 0 : quadstexcoord[idx][c] + 1,
          quadsnorm.empty() ? 0 : quadsnorm[idx][c] + 1,
      });
    }
    shape.elements.push_back({(uint16_t)nv, obj_etype::face, material});
  }
}
void add_quads(obj_shape& shape, const vector<vec4i>& quads,
    const vector<int>& materials, bool has_normals, bool has_texcoord) {
  for (auto idx = 0; idx < quads.size(); idx++) {
    auto& quad = quads[idx];
    auto  nv   = quad.z == quad.w ? 3 : 4;
    for (auto c = 0; c < nv; c++) {
      shape.vertices.push_back({
          quad[c] + 1,
          !has_texcoord ? 0 : quad[c] + 1,
          !has_normals ? 0 : quad[c] + 1,
      });
    }
    shape.elements.push_back({(uint16_t)nv, obj_etype::face, materials[idx]});
  }
}
void add_lines(obj_shape& shape, const vector<vec2i>& lines,
    const vector<int>& materials, bool has_normals, bool has_texcoord) {
  for (auto idx = 0; idx < lines.size(); idx++) {
    auto& line = lines[idx];
    for (auto c = 0; c < 2; c++) {
      shape.vertices.push_back({
          line[c] + 1,
          !has_texcoord ? 0 : line[c] + 1,
          !has_normals ? 0 : line[c] + 1,
      });
    }
    shape.elements.push_back({2, obj_etype::line, materials[idx]});
  }
}
void add_points(obj_shape& shape, const vector<int>& points,
    const vector<int>& materials, bool has_normals, bool has_texcoord) {
  for (auto idx = 0; idx < points.size(); idx++) {
    auto& point = points[idx];
    shape.vertices.push_back({
        point + 1,
        !has_texcoord ? 0 : point + 1,
        !has_normals ? 0 : point + 1,
    });
    shape.elements.push_back({1, obj_etype::point, materials[idx]});
  }
}
void add_fvquads(obj_shape& shape, const vector<vec4i>& quadspos,
    const vector<vec4i>& quadsnorm, const vector<vec4i>& quadstexcoord,
    const vector<int>& materials) {
  for (auto idx = 0; idx < quadspos.size(); idx++) {
    auto nv = quadspos[idx].z == quadspos[idx].w ? 3 : 4;
    for (auto c = 0; c < nv; c++) {
      shape.vertices.push_back({
          quadspos.empty() ? 0 : quadspos[idx][c] + 1,
          quadstexcoord.empty() ? 0 : quadstexcoord[idx][c] + 1,
          quadsnorm.empty() ? 0 : quadsnorm[idx][c] + 1,
      });
    }
    shape.elements.push_back({(uint16_t)nv, obj_etype::face, materials[idx]});
  }
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// HELPER FOR StL
// -----------------------------------------------------------------------------
namespace std {

// Hash functor for vector for use with hash_map
template <>
struct hash<yocto::vec3f> {
  size_t operator()(const yocto::vec3f& v) const {
    const std::hash<float> hasher = std::hash<float>();
    auto                   h      = (size_t)0;
    h ^= hasher(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= hasher(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= hasher(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
  }
};

}  // namespace std

// -----------------------------------------------------------------------------
// STL PARSING
// -----------------------------------------------------------------------------
namespace yocto {

// Load/save stl
bool load_stl(const string& filename, stl_model& stl, string& error,
    bool unique_vertices) {
  stl.shapes.clear();

  // load data
  auto data = vector<byte>{};
  if (!load_binary(filename, data, error)) return false;

  // parse
  auto data_view  = string_view{(const char*)data.data(), data.size()};
  auto read_error = [&filename, &error]() {
    error = filename + ": read error";
    return false;
  };

  // assume it is binary and read hader
  auto header = array<char, 80>{};
  if (!read_value(data_view, header)) return read_error();

  // check if binary
  auto binary = header[0] != 's' || header[1] != 'o' || header[2] != 'l' ||
                header[3] != 'i' || header[4] != 'd';

  // check size in case the binary had a bad header
  if (!binary) {
    auto ntriangles = (uint32_t)0;
    if (!read_value(data_view, ntriangles)) return read_error();
    auto length = data.size();
    auto size   = 80 + 4 + (4 * 12 + 2) * (size_t)ntriangles;
    binary      = length == size;
  }

  // switch on type
  if (binary) {
    // load data
    auto data = vector<byte>{};
    if (!load_binary(filename, data, error)) return false;
    auto data_view = string_view{(const char*)data.data(), data.size()};

    // skip header
    auto header = array<char, 80>{};
    if (!read_value(data_view, header)) return read_error();

    // read shapes until the end
    auto ntriangles = (uint32_t)0;
    while (!data_view.empty()) {
      // append shape
      auto& shape = stl.shapes.emplace_back();

      // read num triangles
      if (!read_value(data_view, ntriangles)) return read_error();

      // resize buffers
      shape.fnormals.resize(ntriangles);
      shape.triangles.resize(ntriangles);
      shape.positions.resize(ntriangles * 3);

      // read all data
      for (auto triangle_id = 0; triangle_id < (int)ntriangles; triangle_id++) {
        // read triangle data
        if (!read_value(data_view, shape.fnormals[triangle_id]))
          return read_error();
        if (!read_value(data_view, shape.positions[triangle_id * 3 + 0]))
          return read_error();
        if (!read_value(data_view, shape.positions[triangle_id * 3 + 1]))
          return read_error();
        if (!read_value(data_view, shape.positions[triangle_id * 3 + 2]))
          return read_error();
        shape.triangles[triangle_id] = {
            triangle_id * 3 + 0, triangle_id * 3 + 1, triangle_id * 3 + 2};
        // read unused attrobute count
        auto attribute_count = (uint16_t)0;
        if (!read_value(data_view, attribute_count)) return read_error();
      }
    }

    // check if read at least one
    if (stl.shapes.empty()) return read_error();
  } else {
    // load data
    auto data = string{};
    if (!load_text(filename, data, error)) return false;

    // parse state
    auto in_solid = false, in_facet = false, in_loop = false;

    // read all lines
    auto data_view   = string_view{data.data(), data.size()};
    auto str         = string_view{};
    auto parse_error = [&filename, &error]() {
      error = filename + ": parse error";
      return false;
    };
    while (read_line(data_view, str)) {
      // str
      remove_comment(str);
      skip_whitespace(str);
      if (str.empty()) continue;

      // get command
      auto cmd = ""s;
      if (!parse_value(str, cmd)) return parse_error();
      if (cmd.empty()) continue;

      // switch over command
      if (cmd == "solid") {
        if (in_solid) return parse_error();
        in_solid = true;
        stl.shapes.emplace_back();
      } else if (cmd == "endsolid") {
        if (!in_solid) return parse_error();
        in_solid = false;
      } else if (cmd == "facet") {
        if (!in_solid || in_facet) return parse_error();
        in_facet = true;
        // next command
        if (!parse_value(str, cmd)) return parse_error();
        if (cmd != "normal") return parse_error();
        // vertex normal
        if (!parse_value(str, stl.shapes.back().fnormals.emplace_back()))
          return parse_error();
      } else if (cmd == "endfacet") {
        if (!in_solid || !in_facet || in_loop) return parse_error();
        in_facet = false;
        // check that it was a triangle
        auto last_pos = (int)stl.shapes.back().positions.size() - 3;
        if (stl.shapes.back().triangles.empty() && last_pos != 0)
          return parse_error();
        if (!stl.shapes.back().triangles.empty() &&
            last_pos != stl.shapes.back().triangles.back().z + 1)
          return parse_error();
        // add triangle
        stl.shapes.back().triangles.push_back(
            {last_pos + 0, last_pos + 1, last_pos + 2});
      } else if (cmd == "outer") {
        if (!in_solid || !in_facet || in_loop) return parse_error();
        in_loop = true;
        // next command
        if (!parse_value(str, cmd)) return parse_error();
        if (cmd != "loop") return parse_error();
        return parse_error();
      } else if (cmd == "endloop") {
        if (!in_solid || !in_facet || !in_loop) return parse_error();
        in_loop = false;
      } else if (cmd == "vertex") {
        // vertex position
        if (!parse_value(str, stl.shapes.back().positions.emplace_back()))
          return parse_error();
      } else {
        return parse_error();
      }
    }
  }

  // make unique vertices
  if (unique_vertices) {
    for (auto& shape : stl.shapes) {
      auto vertex_map       = unordered_map<vec3f, int>{};
      auto unique_positions = vector<vec3f>{};
      for (auto& triangle : shape.triangles) {
        for (auto& vertex_id : triangle) {
          auto vertex_it = vertex_map.find(shape.positions[vertex_id]);
          if (vertex_it == vertex_map.end()) {
            auto new_vertex_id = (int)unique_positions.size();
            unique_positions.push_back(shape.positions[vertex_id]);
            vertex_map.insert(
                vertex_it, {unique_positions.back(), new_vertex_id});
            vertex_id = new_vertex_id;
          } else {
            vertex_id = vertex_it->second;
          }
        }
      }
      std::swap(unique_positions, shape.positions);
    }
  }

  // done
  return true;
}

bool save_stl(
    const string& filename, const stl_model& stl, string& error, bool ascii) {
  // helper
  auto triangle_normal = [](const vec3f& p0, const vec3f& p1, const vec3f& p2) {
    return normalize(cross(p1 - p0, p2 - p0));
  };

  // switch on format
  if (!ascii) {
    // buffer
    auto buffer = vector<byte>{};

    // header
    auto header = array<char, 80>{0};
    snprintf(header.data(), header.size(), "Binary STL - Written by Yocto/GL");
    write_value(buffer, header);

    // write shapes
    for (auto& shape : stl.shapes) {
      auto ntriangles = (uint32_t)shape.triangles.size();
      write_value(buffer, ntriangles);
      for (auto triangle_idx = 0; triangle_idx < shape.triangles.size();
           triangle_idx++) {
        auto& triangle = shape.triangles[triangle_idx];
        auto  fnormal  = !shape.fnormals.empty()
                             ? shape.fnormals[triangle_idx]
                             : triangle_normal(shape.positions[triangle.x],
                                 shape.positions[triangle.y],
                                 shape.positions[triangle.z]);
        write_value(buffer, fnormal);
        write_value(buffer, shape.positions[triangle.x]);
        write_value(buffer, shape.positions[triangle.y]);
        write_value(buffer, shape.positions[triangle.z]);
        auto attribute_count = (uint16_t)0;
        write_value(buffer, attribute_count);
      }
    }

    // save file
    if (!save_binary(filename, buffer, error)) return false;
  } else {
    // buffer
    auto buffer = string{};

    for (auto& shape : stl.shapes) {
      format_values(buffer, "solid \n");
      for (auto triangle_idx = 0; triangle_idx < shape.triangles.size();
           triangle_idx++) {
        auto& triangle = shape.triangles[triangle_idx];
        auto  fnormal  = !shape.fnormals.empty()
                             ? shape.fnormals[triangle_idx]
                             : triangle_normal(shape.positions[triangle.x],
                                 shape.positions[triangle.y],
                                 shape.positions[triangle.z]);
        format_values(buffer, "facet normal {}\n", fnormal);
        format_values(buffer, "outer loop\n");
        format_values(buffer, "vertex {}\n", shape.positions[triangle.x]);
        format_values(buffer, "vertex {}\n", shape.positions[triangle.y]);
        format_values(buffer, "vertex {}\n", shape.positions[triangle.z]);
        format_values(buffer, "endloop\n");
        format_values(buffer, "endfacet\n");
      }
      format_values(buffer, "endsolid \n");
    }

    // save file
    if (!save_text(filename, buffer, error)) return false;
  }

  // done
  return true;
}

// Get/set data
bool get_triangles(const stl_model& stl, int shape_id, vector<vec3i>& triangles,
    vector<vec3f>& positions, vector<vec3f>& fnormals) {
  if (shape_id < 0 || shape_id >= stl.shapes.size()) return false;
  auto& shape = stl.shapes.at(shape_id);
  triangles   = shape.triangles;
  positions   = shape.positions;
  fnormals    = shape.fnormals;
  return true;
}
void add_triangles(stl_model& stl, const vector<vec3i>& triangles,
    const vector<vec3f>& positions, const vector<vec3f>& fnormals) {
  auto& shape     = stl.shapes.emplace_back();
  shape.triangles = triangles;
  shape.positions = positions;
  shape.fnormals  = fnormals;
}

}  // namespace yocto

// -----------------------------------------------------------------------------
// PBRT PARSING
// -----------------------------------------------------------------------------
namespace yocto {

// Pbrt type
enum struct pbrt_type {
  // clang-format off
  real, integer, boolean, string, point, normal, vector, texture, color,
  point2, vector2, spectrum
  // clang-format on
};

// Pbrt value
struct pbrt_value {
  string        name     = "";
  pbrt_type     type     = pbrt_type::real;
  int           value1i  = 0;
  float         value1f  = 0;
  vec2f         value2f  = {0, 0};
  vec3f         value3f  = {0, 0, 0};
  bool          value1b  = false;
  string        value1s  = "";
  vector<float> vector1f = {};
  vector<vec2f> vector2f = {};
  vector<vec3f> vector3f = {};
  vector<int>   vector1i = {};
};

// Pbrt command
struct pbrt_command {
  string             name   = "";
  string             type   = "";
  vector<pbrt_value> values = {};
  frame3f            frame  = identity3x4f;
  frame3f            frend  = identity3x4f;
};

// get pbrt value
static bool get_pbrt_value(const pbrt_value& pbrt, string& val) {
  if (pbrt.type == pbrt_type::string || pbrt.type == pbrt_type::texture) {
    val = pbrt.value1s;
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, bool& val) {
  if (pbrt.type == pbrt_type::boolean) {
    val = pbrt.value1b;
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, int& val) {
  if (pbrt.type == pbrt_type::integer) {
    val = pbrt.value1i;
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, float& val) {
  if (pbrt.type == pbrt_type::real) {
    val = pbrt.value1f;
    return true;
  } else {
    return false;
  }
}
[[maybe_unused]] static bool get_pbrt_value(
    const pbrt_value& pbrt, vec2f& val) {
  if (pbrt.type == pbrt_type::point2 || pbrt.type == pbrt_type::vector2) {
    val = pbrt.value2f;
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, vec3f& val) {
  if (pbrt.type == pbrt_type::point || pbrt.type == pbrt_type::vector ||
      pbrt.type == pbrt_type::normal || pbrt.type == pbrt_type::color) {
    val = pbrt.value3f;
    return true;
  } else if (pbrt.type == pbrt_type::real) {
    val = vec3f{pbrt.value1f, pbrt.value1f, pbrt.value1f};
    return true;
  } else {
    return false;
  }
}
[[maybe_unused]] static bool get_pbrt_value(
    const pbrt_value& pbrt, vector<float>& val) {
  if (pbrt.type == pbrt_type::real) {
    if (!pbrt.vector1f.empty()) {
      val = pbrt.vector1f;
    } else {
      val = {pbrt.value1f};
    }
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, vector<vec2f>& val) {
  if (pbrt.type == pbrt_type::point2 || pbrt.type == pbrt_type::vector2) {
    if (!pbrt.vector2f.empty()) {
      val = pbrt.vector2f;
    } else {
      val = {pbrt.value2f};
    }
    return true;
  } else if (pbrt.type == pbrt_type::real) {
    if (pbrt.vector1f.empty() || (pbrt.vector1f.size() % 2) != 0) return false;
    val.resize(pbrt.vector1f.size() / 2);
    for (auto i = 0; i < val.size(); i++)
      val[i] = {pbrt.vector1f[i * 2 + 0], pbrt.vector1f[i * 2 + 1]};
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, vector<vec3f>& val) {
  if (pbrt.type == pbrt_type::point || pbrt.type == pbrt_type::vector ||
      pbrt.type == pbrt_type::normal || pbrt.type == pbrt_type::color) {
    if (!pbrt.vector3f.empty()) {
      val = pbrt.vector3f;
    } else {
      val = {pbrt.value3f};
    }
    return true;
  } else if (pbrt.type == pbrt_type::real) {
    if (pbrt.vector1f.empty() || (pbrt.vector1f.size() % 3) != 0) return false;
    val.resize(pbrt.vector1f.size() / 3);
    for (auto i = 0; i < val.size(); i++)
      val[i] = {pbrt.vector1f[i * 3 + 0], pbrt.vector1f[i * 3 + 1],
          pbrt.vector1f[i * 3 + 2]};
    return true;
  } else {
    return false;
  }
}

[[maybe_unused]] static bool get_pbrt_value(
    const pbrt_value& pbrt, vector<int>& val) {
  if (pbrt.type == pbrt_type::integer) {
    if (!pbrt.vector1i.empty()) {
      val = pbrt.vector1i;
    } else {
      val = {pbrt.vector1i};
    }
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, vector<vec3i>& val) {
  if (pbrt.type == pbrt_type::integer) {
    if (pbrt.vector1i.empty() || (pbrt.vector1i.size() % 3) != 0) return false;
    val.resize(pbrt.vector1i.size() / 3);
    for (auto i = 0; i < val.size(); i++)
      val[i] = {pbrt.vector1i[i * 3 + 0], pbrt.vector1i[i * 3 + 1],
          pbrt.vector1i[i * 3 + 2]};
    return true;
  } else {
    return false;
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, pair<float, string>& val) {
  if (pbrt.type == pbrt_type::string || pbrt.type == pbrt_type::texture) {
    val.first = 0;
    return get_pbrt_value(pbrt, val.second);
  } else {
    val.second = "";
    return get_pbrt_value(pbrt, val.first);
  }
}
static bool get_pbrt_value(const pbrt_value& pbrt, pair<vec3f, string>& val) {
  if (pbrt.type == pbrt_type::string || pbrt.type == pbrt_type::texture) {
    val.first = {0, 0, 0};
    return get_pbrt_value(pbrt, val.second);
  } else {
    val.second = "";
    return get_pbrt_value(pbrt, val.first);
  }
}
template <typename T>
static bool get_pbrt_value(
    const vector<pbrt_value>& pbrt, const string& name, T& val) {
  for (auto& p : pbrt) {
    if (p.name == name) {
      return get_pbrt_value(p, val);
    }
  }
  return true;
}

// pbrt value construction
static pbrt_value make_pbrt_value(
    const string& name, const string& val, pbrt_type type = pbrt_type::string) {
  auto pbrt    = pbrt_value{};
  pbrt.name    = name;
  pbrt.type    = type;
  pbrt.value1s = val;
  return pbrt;
}
static pbrt_value make_pbrt_value(
    const string& name, bool val, pbrt_type type = pbrt_type::boolean) {
  auto pbrt    = pbrt_value{};
  pbrt.name    = name;
  pbrt.type    = type;
  pbrt.value1b = val;
  return pbrt;
}
static pbrt_value make_pbrt_value(
    const string& name, int val, pbrt_type type = pbrt_type::integer) {
  auto pbrt    = pbrt_value{};
  pbrt.name    = name;
  pbrt.type    = type;
  pbrt.value1i = val;
  return pbrt;
}
static pbrt_value make_pbrt_value(
    const string& name, float val, pbrt_type type = pbrt_type::real) {
  auto pbrt    = pbrt_value{};
  pbrt.name    = name;
  pbrt.type    = type;
  pbrt.value1f = val;
  return pbrt;
}
[[maybe_unused]] static pbrt_value make_pbrt_value(
    const string& name, const vec2f& val, pbrt_type type = pbrt_type::point2) {
  auto pbrt    = pbrt_value{};
  pbrt.name    = name;
  pbrt.type    = type;
  pbrt.value2f = val;
  return pbrt;
}
static pbrt_value make_pbrt_value(
    const string& name, const vec3f& val, pbrt_type type = pbrt_type::color) {
  auto pbrt    = pbrt_value{};
  pbrt.name    = name;
  pbrt.type    = type;
  pbrt.value3f = val;
  return pbrt;
}
static pbrt_value make_pbrt_value(const string& name, const vector<vec2f>& val,
    pbrt_type type = pbrt_type::point2) {
  auto pbrt     = pbrt_value{};
  pbrt.name     = name;
  pbrt.type     = type;
  pbrt.vector2f = val;
  return pbrt;
}
static pbrt_value make_pbrt_value(const string& name, const vector<vec3f>& val,
    pbrt_type type = pbrt_type::point) {
  auto pbrt     = pbrt_value{};
  pbrt.name     = name;
  pbrt.type     = type;
  pbrt.vector3f = val;
  return pbrt;
}
static pbrt_value make_pbrt_value(const string& name, const vector<vec3i>& val,
    pbrt_type type = pbrt_type::integer) {
  auto pbrt     = pbrt_value{};
  pbrt.name     = name;
  pbrt.type     = type;
  pbrt.vector1i = {&val.front().x, &val.front().x + val.size() * 3};
  return pbrt;
}

[[maybe_unused]] static void remove_pbrt_comment(
    string_view& str, char comment_char = '#') {
  while (!str.empty() && is_newline(str.back())) str.remove_suffix(1);
  auto cpy       = str;
  auto in_string = false;
  while (!cpy.empty()) {
    if (cpy.front() == '"') in_string = !in_string;
    if (cpy.front() == comment_char && !in_string) break;
    cpy.remove_prefix(1);
  }
  str.remove_suffix(cpy.size());
}

// Read a pbrt command from file
static bool read_pbrt_cmdline(string_view& str, string& cmd) {
  cmd.clear();
  auto found = false;
  auto copy  = str;
  auto line  = string_view{};
  while (read_line(str, line)) {
    // line
    remove_comment(line, '#', true);
    skip_whitespace(line);
    if (line.empty()) continue;

    // check if command
    auto is_cmd = line[0] >= 'A' && line[0] <= 'Z';
    if (is_cmd) {
      if (found) {
        str = copy;
        return true;
      } else {
        found = true;
      }
    } else if (!found) {
      return false;
    }
    cmd += line;
    cmd += " ";
    copy = str;
  }
  return found;
}

// parse a quoted string
[[nodiscard]] static bool parse_command(string_view& str, string& value) {
  skip_whitespace(str);
  if (!isalpha((int)str.front())) return false;
  auto pos = str.find_first_not_of(
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  if (pos == string_view::npos) {
    value.assign(str);
    str.remove_prefix(str.size());
  } else {
    value.assign(str.substr(0, pos));
    str.remove_prefix(pos + 1);
  }
  return true;
}

// parse pbrt value with optional parens
template <typename T>
[[nodiscard]] static bool parse_param(string_view& str, T& value) {
  skip_whitespace(str);
  auto parens = !str.empty() && str.front() == '[';
  if (parens) str.remove_prefix(1);
  if (!parse_value(str, value)) return false;
  if (!str.data()) return false;
  if (parens) {
    skip_whitespace(str);
    if (!str.empty() && str.front() == '[') return false;
    str.remove_prefix(1);
  }
  return true;
}

// parse a quoted string
[[nodiscard]] static bool parse_nametype(
    string_view& str_, string& name, string& type) {
  auto value = ""s;
  if (!parse_value(str_, value)) return false;
  if (str_.empty()) return false;
  auto str  = string_view{value};
  auto pos1 = str.find(' ');
  if (pos1 == string_view::npos) return false;
  type = string(str.substr(0, pos1));
  str.remove_prefix(pos1);
  auto pos2 = str.find_first_not_of(' ');
  if (pos2 == string_view::npos) return false;
  str.remove_prefix(pos2);
  name = string(str);
  return true;
}

static pair<vec3f, vec3f> get_etak(const string& name) {
  static const unordered_map<string, pair<vec3f, vec3f>> metal_ior_table = {
      {"a-C", {{2.9440999183f, 2.2271502925f, 1.9681668794f},
                  {0.8874329109f, 0.7993216383f, 0.8152862927f}}},
      {"Ag", {{0.1552646489f, 0.1167232965f, 0.1383806959f},
                 {4.8283433224f, 3.1222459278f, 2.1469504455f}}},
      {"Al", {{1.6574599595f, 0.8803689579f, 0.5212287346f},
                 {9.2238691996f, 6.2695232477f, 4.8370012281f}}},
      {"AlAs", {{3.6051023902f, 3.2329365777f, 2.2175611545f},
                   {0.0006670247f, -0.0004999400f, 0.0074261204f}}},
      {"AlSb", {{-0.0485225705f, 4.1427547893f, 4.6697691348f},
                   {-0.0363741915f, 0.0937665154f, 1.3007390124f}}},
      {"Au", {{0.1431189557f, 0.3749570432f, 1.4424785571f},
                 {3.9831604247f, 2.3857207478f, 1.6032152899f}}},
      {"Be", {{4.1850592788f, 3.1850604423f, 2.7840913457f},
                 {3.8354398268f, 3.0101260162f, 2.8690088743f}}},
      {"Cr", {{4.3696828663f, 2.9167024892f, 1.6547005413f},
                 {5.2064337956f, 4.2313645277f, 3.7549467933f}}},
      {"CsI", {{2.1449030413f, 1.7023164587f, 1.6624194173f},
                  {0.0000000000f, 0.0000000000f, 0.0000000000f}}},
      {"Cu", {{0.2004376970f, 0.9240334304f, 1.1022119527f},
                 {3.9129485033f, 2.4528477015f, 2.1421879552f}}},
      {"Cu2O", {{3.5492833755f, 2.9520622449f, 2.7369202137f},
                   {0.1132179294f, 0.1946659670f, 0.6001681264f}}},
      {"CuO", {{3.2453822204f, 2.4496293965f, 2.1974114493f},
                  {0.5202739621f, 0.5707372756f, 0.7172250613f}}},
      {"d-C", {{2.7112524747f, 2.3185812849f, 2.2288565009f},
                  {0.0000000000f, 0.0000000000f, 0.0000000000f}}},
      {"Hg", {{2.3989314904f, 1.4400254917f, 0.9095512090f},
                 {6.3276269444f, 4.3719414152f, 3.4217899270f}}},
      {"HgTe", {{4.7795267752f, 3.2309984581f, 2.6600252401f},
                   {1.6319827058f, 1.5808189339f, 1.7295753852f}}},
      {"Ir", {{3.0864098394f, 2.0821938440f, 1.6178866805f},
                 {5.5921510077f, 4.0671757150f, 3.2672611269f}}},
      {"K", {{0.0640493070f, 0.0464100621f, 0.0381842017f},
                {2.1042155920f, 1.3489364357f, 0.9132113889f}}},
      {"Li", {{0.2657871942f, 0.1956102432f, 0.2209198538f},
                 {3.5401743407f, 2.3111306542f, 1.6685930000f}}},
      {"MgO", {{2.0895885542f, 1.6507224525f, 1.5948759692f},
                  {0.0000000000f, -0.0000000000f, 0.0000000000f}}},
      {"Mo", {{4.4837010280f, 3.5254578255f, 2.7760769438f},
                 {4.1111307988f, 3.4208716252f, 3.1506031404f}}},
      {"Na", {{0.0602665320f, 0.0561412435f, 0.0619909494f},
                 {3.1792906496f, 2.1124800781f, 1.5790940266f}}},
      {"Nb", {{3.4201353595f, 2.7901921379f, 2.3955856658f},
                 {3.4413817900f, 2.7376437930f, 2.5799132708f}}},
      {"Ni", {{2.3672753521f, 1.6633583302f, 1.4670554172f},
                 {4.4988329911f, 3.0501643957f, 2.3454274399f}}},
      {"Rh", {{2.5857954933f, 1.8601866068f, 1.5544279524f},
                 {6.7822927110f, 4.7029501026f, 3.9760892461f}}},
      {"Se-e", {{5.7242724833f, 4.1653992967f, 4.0816099264f},
                   {0.8713747439f, 1.1052845009f, 1.5647788766f}}},
      {"Se", {{4.0592611085f, 2.8426947380f, 2.8207582835f},
                 {0.7543791750f, 0.6385150558f, 0.5215872029f}}},
      {"SiC", {{3.1723450205f, 2.5259677964f, 2.4793623897f},
                  {0.0000007284f, -0.0000006859f, 0.0000100150f}}},
      {"SnTe", {{4.5251865890f, 1.9811525984f, 1.2816819226f},
                   {0.0000000000f, 0.0000000000f, 0.0000000000f}}},
      {"Ta", {{2.0625846607f, 2.3930915569f, 2.6280684948f},
                 {2.4080467973f, 1.7413705864f, 1.9470377016f}}},
      {"Te-e", {{7.5090397678f, 4.2964603080f, 2.3698732430f},
                   {5.5842076830f, 4.9476231084f, 3.9975145063f}}},
      {"Te", {{7.3908396088f, 4.4821028985f, 2.6370708478f},
                 {3.2561412892f, 3.5273908133f, 3.2921683116f}}},
      {"ThF4", {{1.8307187117f, 1.4422274283f, 1.3876488528f},
                   {0.0000000000f, 0.0000000000f, 0.0000000000f}}},
      {"TiC", {{3.7004673762f, 2.8374356509f, 2.5823030278f},
                  {3.2656905818f, 2.3515586388f, 2.1727857800f}}},
      {"TiN", {{1.6484691607f, 1.1504482522f, 1.3797795097f},
                  {3.3684596226f, 1.9434888540f, 1.1020123347f}}},
      {"TiO2-e", {{3.1065574823f, 2.5131551146f, 2.5823844157f},
                     {0.0000289537f, -0.0000251484f, 0.0001775555f}}},
      {"TiO2", {{3.4566203131f, 2.8017076558f, 2.9051485020f},
                   {0.0001026662f, -0.0000897534f, 0.0006356902f}}},
      {"VC", {{3.6575665991f, 2.7527298065f, 2.5326814570f},
                 {3.0683516659f, 2.1986687713f, 1.9631816252f}}},
      {"VN", {{2.8656011588f, 2.1191817791f, 1.9400767149f},
                 {3.0323264950f, 2.0561075580f, 1.6162930914f}}},
      {"V", {{4.2775126218f, 3.5131538236f, 2.7611257461f},
                {3.4911844504f, 2.8893580874f, 3.1116965117f}}},
      {"W", {{4.3707029924f, 3.3002972445f, 2.9982666528f},
                {3.5006778591f, 2.6048652781f, 2.2731930614f}}},
  };
  return metal_ior_table.at(name);
}

// Pbrt measure subsurface parameters (sigma_prime_s, sigma_a in mm^-1)
// from pbrt code at pbrt/code/medium.cpp
[[maybe_unused]] static pair<vec3f, vec3f> get_subsurface(const string& name) {
  static const unordered_map<string, pair<vec3f, vec3f>> params = {
      // From "A Practical Model for Subsurface Light Transport"
      // Jensen, Marschner, Levoy, Hanrahan
      // Proc SIGGRAPH 2001
      {"Apple", {{2.29f, 2.39f, 1.97f}, {0.0030f, 0.0034f, 0.046f}}},
      {"Chicken1", {{0.15f, 0.21f, 0.38f}, {0.015f, 0.077f, 0.19f}}},
      {"Chicken2", {{0.19f, 0.25f, 0.32f}, {0.018f, 0.088f, 0.20f}}},
      {"Cream", {{7.38f, 5.47f, 3.15f}, {0.0002f, 0.0028f, 0.0163f}}},
      {"Ketchup", {{0.18f, 0.07f, 0.03f}, {0.061f, 0.97f, 1.45f}}},
      {"Marble", {{2.19f, 2.62f, 3.00f}, {0.0021f, 0.0041f, 0.0071f}}},
      {"Potato", {{0.68f, 0.70f, 0.55f}, {0.0024f, 0.0090f, 0.12f}}},
      {"Skimmilk", {{0.70f, 1.22f, 1.90f}, {0.0014f, 0.0025f, 0.0142f}}},
      {"Skin1", {{0.74f, 0.88f, 1.01f}, {0.032f, 0.17f, 0.48f}}},
      {"Skin2", {{1.09f, 1.59f, 1.79f}, {0.013f, 0.070f, 0.145f}}},
      {"Spectralon", {{11.6f, 20.4f, 14.9f}, {0.00f, 0.00f, 0.00f}}},
      {"Wholemilk", {{2.55f, 3.21f, 3.77f}, {0.0011f, 0.0024f, 0.014f}}},
      // From "Acquiring Scattering Properties of Participating Media by
      // Dilution",
      // Narasimhan, Gupta, Donner, Ramamoorthi, Nayar, Jensen
      // Proc SIGGRAPH 2006
      {"Lowfat Milk",
          {{0.89187f, 1.5136f, 2.532f}, {0.002875f, 0.00575f, 0.0115f}}},
      {"Reduced Milk",
          {{2.4858f, 3.1669f, 4.5214f}, {0.0025556f, 0.0051111f, 0.012778f}}},
      {"Regular Milk",
          {{4.5513f, 5.8294f, 7.136f}, {0.0015333f, 0.0046f, 0.019933f}}},
      {"Espresso",
          {{0.72378f, 0.84557f, 1.0247f}, {4.7984f, 6.5751f, 8.8493f}}},
      {"Mint Mocha Coffee",
          {{0.31602f, 0.38538f, 0.48131f}, {3.772f, 5.8228f, 7.82f}}},
      {"Lowfat Soy Milk", {{0.30576f, 0.34233f, 0.61664f},
                              {0.0014375f, 0.0071875f, 0.035937f}}},
      {"Regular Soy Milk",
          {{0.59223f, 0.73866f, 1.4693f}, {0.0019167f, 0.0095833f, 0.065167f}}},
      {"Lowfat Chocolate Milk",
          {{0.64925f, 0.83916f, 1.1057f}, {0.0115f, 0.0368f, 0.1564f}}},
      {"Regular Chocolate Milk",
          {{1.4585f, 2.1289f, 2.9527f}, {0.010063f, 0.043125f, 0.14375f}}},
      {"Coke",
          {{8.9053e-05f, 8.372e-05f, 0.0f}, {0.10014f, 0.16503f, 0.2468f}}},
      {"Pepsi",
          {{6.1697e-05f, 4.2564e-05f, 0.0f}, {0.091641f, 0.14158f, 0.20729f}}},
      {"Sprite", {{6.0306e-06f, 6.4139e-06f, 6.5504e-06f},
                     {0.001886f, 0.0018308f, 0.0020025f}}},
      {"Gatorade", {{0.0024574f, 0.003007f, 0.0037325f},
                       {0.024794f, 0.019289f, 0.008878f}}},
      {"Chardonnay", {{1.7982e-05f, 1.3758e-05f, 1.2023e-05f},
                         {0.010782f, 0.011855f, 0.023997f}}},
      {"White Zinfandel", {{1.7501e-05f, 1.9069e-05f, 1.288e-05f},
                              {0.012072f, 0.016184f, 0.019843f}}},
      {"Merlot", {{2.1129e-05f, 0.0f, 0.0f}, {0.11632f, 0.25191f, 0.29434f}}},
      {"Budweiser Beer", {{2.4356e-05f, 2.4079e-05f, 1.0564e-05f},
                             {0.011492f, 0.024911f, 0.057786f}}},
      {"Coors Light Beer",
          {{5.0922e-05f, 4.301e-05f, 0.0f}, {0.006164f, 0.013984f, 0.034983f}}},
      {"Clorox", {{0.0024035f, 0.0031373f, 0.003991f},
                     {0.0033542f, 0.014892f, 0.026297f}}},
      {"Apple Juice", {{0.00013612f, 0.00015836f, 0.000227f},
                          {0.012957f, 0.023741f, 0.052184f}}},
      {"Cranberry Juice", {{0.00010402f, 0.00011646f, 7.8139e-05f},
                              {0.039437f, 0.094223f, 0.12426f}}},
      {"Grape Juice",
          {{5.382e-05f, 0.0f, 0.0f}, {0.10404f, 0.23958f, 0.29325f}}},
      {"Ruby Grapefruit Juice",
          {{0.011002f, 0.010927f, 0.011036f}, {0.085867f, 0.18314f, 0.25262f}}},
      {"White Grapefruit Juice",
          {{0.22826f, 0.23998f, 0.32748f}, {0.0138f, 0.018831f, 0.056781f}}},
      {"Shampoo", {{0.0007176f, 0.0008303f, 0.0009016f},
                      {0.014107f, 0.045693f, 0.061717f}}},
      {"Strawberry Shampoo", {{0.00015671f, 0.00015947f, 1.518e-05f},
                                 {0.01449f, 0.05796f, 0.075823f}}},
      {"Head & Shoulders Shampoo",
          {{0.023805f, 0.028804f, 0.034306f}, {0.084621f, 0.15688f, 0.20365f}}},
      {"Lemon Tea Powder",
          {{0.040224f, 0.045264f, 0.051081f}, {2.4288f, 4.5757f, 7.2127f}}},
      {"Orange Powder", {{0.00015617f, 0.00017482f, 0.0001762f},
                            {0.001449f, 0.003441f, 0.007863f}}},
      {"Pink Lemonade Powder", {{0.00012103f, 0.00013073f, 0.00012528f},
                                   {0.001165f, 0.002366f, 0.003195f}}},
      {"Cappuccino Powder",
          {{1.8436f, 2.5851f, 2.1662f}, {35.844f, 49.547f, 61.084f}}},
      {"Salt Powder",
          {{0.027333f, 0.032451f, 0.031979f}, {0.28415f, 0.3257f, 0.34148f}}},
      {"Sugar Powder", {{0.00022272f, 0.00025513f, 0.000271f},
                           {0.012638f, 0.031051f, 0.050124f}}},
      {"Suisse Mocha Powder",
          {{2.7979f, 3.5452f, 4.3365f}, {17.502f, 27.004f, 35.433f}}},
      {"Pacific Ocean Surface Water", {{0.0001764f, 0.00032095f, 0.00019617f},
                                          {0.031845f, 0.031324f, 0.030147f}}},
  };
  return params.at(name);
}

template <typename T, typename V>
[[nodiscard]] static bool parse_pvalues(string_view& str, T& value, V& values) {
  values.clear();
  skip_whitespace(str);
  if (str.empty()) return false;
  if (str.front() == '[') {
    str.remove_prefix(1);
    skip_whitespace(str);
    if (str.empty()) return false;
    while (!str.empty()) {
      auto& val = values.empty() ? value : values.emplace_back();
      if (!parse_value(str, val)) return false;
      if (!str.data()) return false;
      skip_whitespace(str);
      if (str.empty()) break;
      if (str.front() == ']') break;
      if (values.empty()) values.push_back(value);
    }
    if (str.empty()) return false;
    if (str.front() != ']') return false;
    str.remove_prefix(1);
  } else {
    if (!parse_value(str, value)) return false;
  }
  return true;
}

[[nodiscard]] static bool parse_params(
    string_view& str, vector<pbrt_value>& values) {
  auto starts_with = [](string_view value, string_view prefix) {
    if (prefix.size() > value.size()) return false;
    return value.rfind(prefix, 0) == 0;
  };
  auto ends_with = [](string_view value, string_view postfix) {
    if (postfix.size() > value.size()) return false;
    return std::equal(postfix.rbegin(), postfix.rend(), value.rbegin());
  };

  values.clear();
  skip_whitespace(str);
  while (!str.empty()) {
    auto& value = values.emplace_back();
    auto  type  = ""s;
    if (!parse_nametype(str, value.name, type)) return false;
    skip_whitespace(str);
    if (str.empty()) return false;
    if (type == "float") {
      value.type = pbrt_type::real;
      if (!parse_pvalues(str, value.value1f, value.vector1f)) return false;
    } else if (type == "integer") {
      value.type = pbrt_type::integer;
      if (!parse_pvalues(str, value.value1i, value.vector1i)) return false;
    } else if (type == "string") {
      auto vector1s = vector<string>{};
      value.type    = pbrt_type::string;
      if (!parse_pvalues(str, value.value1s, vector1s)) return false;
      if (!vector1s.empty()) return false;
    } else if (type == "bool") {
      auto value1s  = ""s;
      auto vector1s = vector<string>{};
      value.type    = pbrt_type::boolean;
      if (!parse_pvalues(str, value1s, vector1s)) return false;
      if (!vector1s.empty()) return false;
      value.value1b = value1s == "true";
    } else if (type == "texture") {
      auto vector1s = vector<string>{};
      value.type    = pbrt_type::texture;
      if (!parse_pvalues(str, value.value1s, vector1s)) return false;
      if (!vector1s.empty()) return false;
    } else if (type == "point" || type == "point3") {
      value.type = pbrt_type::point;
      if (!parse_pvalues(str, value.value3f, value.vector3f)) return false;
    } else if (type == "normal" || type == "normal3") {
      value.type = pbrt_type::normal;
      if (!parse_pvalues(str, value.value3f, value.vector3f)) return false;
    } else if (type == "vector" || type == "vector3") {
      value.type = pbrt_type::vector;
      if (!parse_pvalues(str, value.value3f, value.vector3f)) return false;
    } else if (type == "point2") {
      value.type = pbrt_type::point2;
      if (!parse_pvalues(str, value.value2f, value.vector2f)) return false;
    } else if (type == "vector2") {
      value.type = pbrt_type::vector2;
      if (!parse_pvalues(str, value.value2f, value.vector2f)) return false;
    } else if (type == "blackbody") {
      value.type = pbrt_type::color;
      // auto blackbody = zero2f;
      // auto vec tor2f  = vector<vec2f>{};
      // parse_pvalues(str, blackbody, vector2f);
      // if (!vector2f.empty()) return false;
      // value.value3f = blackbody_to_rgb(blackbody.x) * blackbody.y;
      auto blackbody = 0.0f;
      auto vector1f  = vector<float>{};
      if (!parse_pvalues(str, blackbody, vector1f)) return false;
      if (vector1f.size() < 2) {
        value.value3f = blackbody_to_rgb(blackbody);
      } else {
        value.value3f = blackbody_to_rgb(vector1f[0]) * vector1f[1];
      }
    } else if (type == "color" || type == "rgb") {
      value.type = pbrt_type::color;
      if (!parse_pvalues(str, value.value3f, value.vector3f)) return false;
    } else if (type == "xyz") {
      value.type = pbrt_type::color;
      if (!parse_pvalues(str, value.value3f, value.vector3f)) return false;
      // xyz conversion
      return false;
    } else if (type == "spectrum") {
      auto is_string = false;
      auto str1      = str;
      skip_whitespace(str1);
      if (!str1.empty() && str1.front() == '"') {
        is_string = true;
      } else if (!str1.empty() && str1.front() == '[') {
        str1.remove_prefix(1);
        skip_whitespace(str1);
        if (!str1.empty() && str1.front() == '"') is_string = true;
      }
      if (is_string) {
        value.type     = pbrt_type::color;
        auto filename  = ""s;
        auto filenames = vector<string>{};
        skip_whitespace(str);
        auto has_parens = str.front() == '[';
        if (has_parens) str.remove_prefix(1);
        if (!parse_value(str, filename)) return false;
        if (has_parens) {
          skip_whitespace(str);
          if (str.front() != ']') return false;
          str.remove_prefix(1);
        }
        if (str.empty()) return false;
        auto filenamep = path_filename(filename);
        auto name      = string_view{filenamep};
        if (ends_with(name, ".spd")) {
          name.remove_suffix(4);
          if (name == "SHPS") {
            value.value3f = {1, 1, 1};
          } else if (ends_with(name, ".eta")) {
            name.remove_suffix(4);
            auto eta      = get_etak(string{name}).first;
            value.value3f = {eta.x, eta.y, eta.z};
          } else if (ends_with(name, ".k")) {
            name.remove_suffix(2);
            auto k        = get_etak(string{name}).second;
            value.value3f = {k.x, k.y, k.z};
          } else {
            return false;
          }
        } else if (starts_with(name, "metal-")) {
          name.remove_prefix(6);
          if (ends_with(name, "-eta")) {
            name.remove_suffix(4);
            auto eta      = get_etak(string{name}).first;
            value.value3f = {eta.x, eta.y, eta.z};
          } else if (ends_with(name, "-k")) {
            name.remove_suffix(2);
            auto k        = get_etak(string{name}).second;
            value.value3f = {k.x, k.y, k.z};
          } else {
            return false;
          }
        } else if (starts_with(name, "glass-")) {
          value.value3f = {1.5, 1.5, 1.5};
        } else {
          return false;
        }
      } else {
        value.type = pbrt_type::spectrum;
        if (!parse_pvalues(str, value.value1f, value.vector1f)) return false;
      }
    } else {
      return false;
    }
    skip_whitespace(str);
  }
  return true;
}

// Other pbrt elements
struct pbrt_film {
  // film approximation
  string filename   = "";
  vec2i  resolution = {0, 0};
};

// Pbrt area light
struct pbrt_arealight {
  // arealight parameters
  string name     = "";
  vec3f  emission = {0, 0, 0};
};

// Pbrt medium. Not parsed at the moment.
struct pbrt_medium {
  // medium parameters
  string name = "";
};

// convert pbrt films
[[nodiscard]] static bool convert_film(pbrt_film& film,
    const pbrt_command& command, const string& filename, bool verbose = false) {
  if (command.type == "image") {
    film.resolution = {512, 512};
    get_pbrt_value(command.values, "xresolution", film.resolution.x);
    get_pbrt_value(command.values, "yresolution", film.resolution.y);
    film.filename = "out.png"s;
    get_pbrt_value(command.values, "filename", film.filename);
  } else if (command.type == "rgb") {
    film.resolution = {512, 512};
    get_pbrt_value(command.values, "xresolution", film.resolution.x);
    get_pbrt_value(command.values, "yresolution", film.resolution.y);
    film.filename = "out.png"s;
    get_pbrt_value(command.values, "filename", film.filename);
  } else {
    return false;
  }
  return true;
}

// convert pbrt elements
[[nodiscard]] static bool convert_camera(pbrt_camera& pcamera,
    const pbrt_command& command, const vec2i& resolution,
    const string& filename, bool verbose = false) {
  pcamera.frame      = command.frame;
  pcamera.frend      = command.frend;
  pcamera.frame      = inverse((frame3f)pcamera.frame);
  pcamera.frame.z    = -pcamera.frame.z;
  pcamera.resolution = resolution;
  auto film_aspect =
      (resolution == zero2i) ? 1 : (float)resolution.x / (float)resolution.y;
  if (command.type == "perspective") {
    auto fov = 90.0f;
    get_pbrt_value(command.values, "fov", fov);
    // auto lensradius = if(!get_pbrt_value(values, "lensradius", 0.0f);
    pcamera.aspect = film_aspect;
    if (pcamera.aspect >= 1) {
      pcamera.lens = (0.036f / pcamera.aspect) / (2 * tan(radians(fov) / 2));
    } else {
      pcamera.lens = (0.036f * pcamera.aspect) / (2 * tan(radians(fov) / 2));
    }
    get_pbrt_value(command.values, "frameaspectratio", pcamera.aspect);
    pcamera.focus = 10.0f;
    get_pbrt_value(command.values, "focaldistance", pcamera.focus);
  } else if (command.type == "realistic") {
    auto lensfile = ""s;
    get_pbrt_value(command.values, "lensfile", lensfile);
    lensfile         = lensfile.substr(0, lensfile.size() - 4);
    lensfile         = lensfile.substr(lensfile.find('.') + 1);
    lensfile         = lensfile.substr(0, lensfile.size() - 2);
    auto lens        = max((float)std::atof(lensfile.c_str()), 35.0f) * 0.001f;
    pcamera.lens     = 2 * atan(0.036f / (2 * lens));
    pcamera.aperture = 0.0f;
    get_pbrt_value(command.values, "aperturediameter", pcamera.aperture);
    pcamera.focus = 10.0f;
    get_pbrt_value(command.values, "focusdistance", pcamera.focus);
    pcamera.aspect = film_aspect;
  } else {
    return false;
  }
  return true;
}

// convert pbrt textures
[[nodiscard]] static bool convert_texture(pbrt_texture& ptexture,
    const pbrt_command&                                 command,
    unordered_map<string, pbrt_texture>& texture_map, const string& filename,
    bool verbose = false) {
  auto make_filename = [&texture_map](const string& name) {
    if (name.empty()) return ""s;
    auto pos = texture_map.find(name);
    if (pos == texture_map.end()) return ""s;
    return pos->second.filename;
  };

  ptexture.name = command.name;
  if (command.type == "imagemap") {
    ptexture.filename = "";
    get_pbrt_value(command.values, "filename", ptexture.filename);
  } else if (command.type == "constant") {
    ptexture.constant = vec3f{1, 1, 1};
    get_pbrt_value(command.values, "value", ptexture.constant);
  } else if (command.type == "bilerp") {
    ptexture.constant = {1, 0, 0};
  } else if (command.type == "checkerboard") {
    // auto tex1     = if(!get_pbrt_value(command.values, "tex1",
    // pair{vec3f{1,1,1},
    // ""s}); auto tex2     = if(!get_pbrt_value(command.values, "tex2",
    //  pair{vec3f{0}, ""s}); auto rgb1     = tex1.second == "" ?
    //  tex1.first :
    // vec3f{0.4f, 0.4f, 0.4f}; auto rgb2     = tex1.second == "" ? tex2.first
    // : vec3f{0.6f, 0.6f, 0.6f}; auto params   = proc_image_params{};
    // params.type = proc_image_params::type_t::checker; params.color0 =
    // {rgb1.x, rgb1.y, rgb1.z, 1}; params.color1 = {rgb2.x, rgb2.y, rgb2.z,
    // 1}; params.scale = 2; make_proc_image(texture.hdr, params);
    // float_to_byte(texture.ldr, texture.hdr); texture.hdr = {};
    ptexture.constant = {0.5, 0.5, 0.5};
  } else if (command.type == "dots") {
    ptexture.constant = {0.5, 0.5, 0.5};
  } else if (command.type == "fbm") {
    ptexture.constant = {0.5, 0.5, 0.5};
  } else if (command.type == "marble") {
    ptexture.constant = {0.5, 0.5, 0.5};
  } else if (command.type == "mix") {
    auto tex1 = pair{vec3f{0, 0, 0}, ""s}, tex2 = pair{vec3f{1, 1, 1}, ""s};
    get_pbrt_value(command.values, "tex1", tex1);
    get_pbrt_value(command.values, "tex2", tex2);
    if (!make_filename(tex1.second).empty()) {
      ptexture.filename = make_filename(tex1.second);
    } else if (!make_filename(tex2.second).empty()) {
      ptexture.filename = make_filename(tex2.second);
    } else {
      ptexture.constant = {1, 0, 0};
    }
  } else if (command.type == "scale") {
    auto tex1 = pair{vec3f{1, 1, 1}, ""s}, tex2 = pair{vec3f{1, 1, 1}, ""s};
    get_pbrt_value(command.values, "tex1", tex2);
    get_pbrt_value(command.values, "tex2", tex1);
    if (!make_filename(tex1.second).empty()) {
      ptexture.filename = make_filename(tex1.second);
    } else if (!make_filename(tex2.second).empty()) {
      ptexture.filename = make_filename(tex2.second);
    } else {
      ptexture.constant = {1, 0, 0};
    }
  } else if (command.type == "uv") {
    ptexture.constant = {1, 0, 0};
  } else if (command.type == "windy") {
    ptexture.constant = {1, 0, 0};
  } else if (command.type == "wrinkled") {
    ptexture.constant = {1, 0, 0};
  } else {
    return false;
  }
  return true;
}

// convert pbrt materials
[[nodiscard]] static bool convert_material(pbrt_material& pmaterial,
    const pbrt_command& command, unordered_map<string, int>& texture_map,
    const unordered_map<string, pbrt_material>& named_materials,
    const unordered_map<string, pbrt_texture>&  named_textures,
    const string& filename, bool verbose = false) {
  // helpers
  auto get_texture_id = [&texture_map](const string& path) {
    if (path.empty()) return -1;
    auto texture_it = texture_map.find(path);
    if (texture_it == texture_map.end()) {
      auto texture_id   = (int)texture_map.size();
      texture_map[path] = texture_id;
      return texture_id;
    } else {
      return texture_it->second;
    }
  };
  auto get_texture = [&](const vector<pbrt_value>& values, const string& name,
                         vec3f& color, int& texture_id, const vec3f& def) {
    auto textured = pair{def, ""s};
    get_pbrt_value(values, name, textured);
    if (textured.second.empty()) {
      color      = textured.first;
      texture_id = -1;
    } else {
      auto& texture = named_textures.at(textured.second);
      if (texture.filename.empty()) {
        color      = texture.constant;
        texture_id = -1;
      } else {
        color      = {1, 1, 1};
        texture_id = get_texture_id(texture.filename);
      }
    }
  };
  auto get_scalar = [&](const vector<pbrt_value>& values, const string& name,
                        float& scalar, float def) {
    auto textured = pair{vec3f{def, def, def}, ""s};
    get_pbrt_value(values, name, textured);
    if (textured.second.empty()) {
      scalar = mean(textured.first);
    } else {
      auto& texture = named_textures.at(textured.second);
      if (texture.filename.empty()) {
        scalar = mean(texture.constant);
      } else {
        scalar = def;
      }
    }
  };
  auto get_color = [&](const vector<pbrt_value>& values, const string& name,
                       vec3f& color, const vec3f& def) {
    auto textured = pair{def, ""s};
    get_pbrt_value(values, name, textured);
    if (textured.second.empty()) {
      color = textured.first;
    } else {
      auto& texture = named_textures.at(textured.second);
      if (texture.filename.empty()) {
        color = texture.constant;
      } else {
        color = def;
      }
    }
  };

  auto get_roughness = [&](const vector<pbrt_value>& values, float& roughness,
                           float def = 0.1) {
    auto roughness_ = pair{vec3f{def, def, def}, ""s};
    get_pbrt_value(values, "roughness", roughness_);
    auto uroughness = roughness_, vroughness = roughness_;
    auto remaproughness = true;
    get_pbrt_value(values, "uroughness", uroughness);
    get_pbrt_value(values, "vroughness", vroughness);
    get_pbrt_value(values, "remaproughness", remaproughness);

    roughness = 0;
    if (uroughness.first == vec3f{0, 0, 0} ||
        vroughness.first == vec3f{0, 0, 0})
      return;
    roughness = mean(vec2f{mean(uroughness.first), mean(vroughness.first)});
    // from pbrt code
    if (remaproughness) {
      roughness = max(roughness, 1e-3f);
      auto x    = log(roughness);
      roughness = 1.62142f + 0.819955f * x + 0.1734f * x * x +
                  0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
    }
    roughness = sqrt(roughness);
  };

  auto eta_to_reflectivity = [](const vec3f&  eta,
                                 const vec3f& etak = {0, 0, 0}) -> vec3f {
    return ((eta - 1) * (eta - 1) + etak * etak) /
           ((eta + 1) * (eta + 1) + etak * etak);
  };

  pmaterial.name = command.name;
  if (command.type == "uber") {
    auto diffuse = vec3f{0, 0, 0}, specular = vec3f{0, 0, 0},
         transmission = vec3f{0, 0, 0};
    auto diffuse_map = -1, specular_map = -1, transmission_map = -1;
    get_texture(
        command.values, "Kd", diffuse, diffuse_map, vec3f{0.25, 0.25, 0.25});
    get_texture(
        command.values, "Ks", specular, specular_map, vec3f{0.25, 0.25, 0.25});
    get_texture(
        command.values, "Kt", transmission, transmission_map, vec3f{0, 0, 0});
    if (max(transmission) > 0.1) {
      pmaterial.type      = pbrt_mtype::thinglass;
      pmaterial.color     = transmission;
      pmaterial.color_tex = transmission_map;
    } else if (max(specular) > 0.1) {
      pmaterial.type      = pbrt_mtype::plastic;
      pmaterial.color     = diffuse;
      pmaterial.color_tex = diffuse_map;
    } else {
      pmaterial.type      = pbrt_mtype::plastic;
      pmaterial.color     = diffuse;
      pmaterial.color_tex = diffuse_map;
    }
    get_scalar(command.values, "opacity", pmaterial.opacity, 1);
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    get_roughness(command.values, pmaterial.roughness, 0.1f);
  } else if (command.type == "plastic") {
    pmaterial.type = pbrt_mtype::plastic;
    get_texture(command.values, "Kd", pmaterial.color, pmaterial.color_tex,
        vec3f{0.25, 0.25, 0.25});
    // get_scalar(command.values, "Ks", pmaterial.specular, 0.25))
    //   return parse_error();
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    pmaterial.roughness = 0.1f;
    get_roughness(command.values, pmaterial.roughness, 0.1f);
  } else if (command.type == "coateddiffuse") {
    pmaterial.type = pbrt_mtype::plastic;
    get_texture(command.values, "reflectance", pmaterial.color,
        pmaterial.color_tex, vec3f{0.25, 0.25, 0.25});
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    pmaterial.roughness = 0.1f;
    get_roughness(command.values, pmaterial.roughness, 0.1f);
  } else if (command.type == "translucent") {
    // not well supported yet
    pmaterial.type = pbrt_mtype::matte;
    get_texture(command.values, "Kd", pmaterial.color, pmaterial.color_tex,
        vec3f{0.25, 0.25, 0.25});
    // get_scalar(command.values, "Ks", pmaterial.specular, 0.25))
    //   return parse_error();
    // get_scalar(command.values, "eta", pmaterial.ior, 1.5))
    //   return parse_error();
    // get_roughness(command.values, pmaterial.roughness, 0.1))
    //   return parse_error();
  } else if (command.type == "diffusetransmission") {
    // not well supported yet
    pmaterial.type = pbrt_mtype::matte;
    get_texture(command.values, "reflectance", pmaterial.color,
        pmaterial.color_tex, vec3f{0.25f, 0.25f, 0.25f});
    // get_texture(command.values, "transmittance", pmaterial.color,
    //         pmaterial.color_tex, vec3f{0.25, 0.25, 0.25}))
    //   return parse_error();
  } else if (command.type == "matte") {
    pmaterial.type = pbrt_mtype::matte;
    get_texture(command.values, "Kd", pmaterial.color, pmaterial.color_tex,
        vec3f{0.5, 0.5, 0.5});
  } else if (command.type == "diffuse") {
    pmaterial.type = pbrt_mtype::matte;
    get_texture(command.values, "reflectance", pmaterial.color,
        pmaterial.color_tex, vec3f{0.5f, 0.5f, 0.5f});
  } else if (command.type == "mirror") {
    pmaterial.type = pbrt_mtype::metal;
    get_texture(command.values, "Kr", pmaterial.color, pmaterial.color_tex,
        vec3f{0.9f, 0.9f, 0.9f});
    pmaterial.roughness = 0;
  } else if (command.type == "metal") {
    pmaterial.type = pbrt_mtype::metal;
    // get_texture(
    //     values, "Kr", material->specular, material->specular_tex,
    //     vec3f{1,1,1});
    auto eta = vec3f{0, 0, 0}, etak = vec3f{0, 0, 0};
    get_color(command.values, "eta", eta,
        vec3f{0.2004376970f, 0.9240334304f, 1.1022119527f});
    get_color(command.values, "k", etak,
        vec3f{3.9129485033f, 2.4528477015f, 2.1421879552f});
    pmaterial.color     = eta_to_reflectivity(eta, etak);
    pmaterial.roughness = 0.01f;
    get_roughness(command.values, pmaterial.roughness, 0.01f);
  } else if (command.type == "conductor") {
    pmaterial.type = pbrt_mtype::metal;
    auto eta = vec3f{0, 0, 0}, etak = vec3f{0, 0, 0};
    get_color(command.values, "eta", eta,
        vec3f{0.2004376970f, 0.9240334304f, 1.1022119527f});
    get_color(command.values, "k", etak,
        vec3f{3.9129485033f, 2.4528477015f, 2.1421879552f});
    pmaterial.color     = eta_to_reflectivity(eta, etak);
    pmaterial.roughness = 0.01f;
    get_roughness(command.values, pmaterial.roughness, 0.01f);
  } else if (command.type == "coatedconductor") {
    pmaterial.type = pbrt_mtype::metal;
    auto eta = vec3f{0, 0, 0}, etak = vec3f{0, 0, 0};
    get_color(command.values, "conductor.eta", eta,
        vec3f{0.2004376970f, 0.9240334304f, 1.1022119527f});
    get_color(command.values, "conductor.k", etak,
        vec3f{3.9129485033f, 2.4528477015f, 2.1421879552f});
    pmaterial.color     = eta_to_reflectivity(eta, etak);
    pmaterial.roughness = 0.01f;
    get_roughness(command.values, pmaterial.roughness, 0.01f);
  } else if (command.type == "substrate") {
    // not well supported
    pmaterial.type = pbrt_mtype::plastic;
    get_texture(command.values, "Kd", pmaterial.color, pmaterial.color_tex,
        vec3f{0.5f, 0.5f, 0.5f});
    auto specular = 0.0f;
    get_scalar(command.values, "Ks", specular, 0.5f);
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    pmaterial.roughness = 0.1f;
    get_roughness(command.values, pmaterial.roughness, 0.1f);
  } else if (command.type == "glass") {
    pmaterial.type = pbrt_mtype::glass;
    get_texture(command.values, "Kt", pmaterial.color, pmaterial.color_tex,
        vec3f{1, 1, 1});
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    pmaterial.roughness = 0;
    get_roughness(command.values, pmaterial.roughness, 0.0f);
  } else if (command.type == "dielectric") {
    pmaterial.type  = pbrt_mtype::glass;
    pmaterial.color = {1, 1, 1};
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    pmaterial.roughness = 0;
    get_roughness(command.values, pmaterial.roughness, 0.0f);
  } else if (command.type == "thindielectric") {
    pmaterial.type  = pbrt_mtype::thinglass;
    pmaterial.color = {1, 1, 1};
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    pmaterial.roughness = 0;
    get_roughness(command.values, pmaterial.roughness, 0.0f);
  } else if (command.type == "hair") {
    pmaterial.type = pbrt_mtype::matte;
    get_texture(command.values, "color", pmaterial.color, pmaterial.color_tex,
        vec3f{0, 0, 0});
    pmaterial.roughness = 1;
    if (verbose) printf("hair material not properly supported\n");
  } else if (command.type == "disney") {
    pmaterial.type = pbrt_mtype::matte;
    get_texture(command.values, "color", pmaterial.color, pmaterial.color_tex,
        vec3f{0.5f, 0.5f, 0.5f});
    pmaterial.roughness = 1;
    if (verbose) printf("disney material not properly supported\n");
  } else if (command.type == "kdsubsurface") {
    pmaterial.type = pbrt_mtype::plastic;
    get_texture(command.values, "Kd", pmaterial.color, pmaterial.color_tex,
        vec3f{0.5f, 0.5f, 0.5f});
    // get_scalar(command.values, "Kr", pmaterial.specular, 1))
    //   return parse_error();
    get_scalar(command.values, "eta", pmaterial.ior, 1.5f);
    pmaterial.roughness = 0;
    get_roughness(command.values, pmaterial.roughness, 0);
    if (verbose) printf("kdsubsurface material not properly supported\n");
  } else if (command.type == "subsurface") {
    pmaterial.type = pbrt_mtype::subsurface;
    // get_scalar(command.values, "Kr", pmaterial.specular, 1))
    //   return parse_error();
    // get_scalar(command.values, "Kt", pmaterial.transmission, 1))
    //   return parse_error();
    pmaterial.color = {1, 1, 1};
    get_scalar(command.values, "eta", pmaterial.ior, 1.5);
    pmaterial.roughness = 0;
    get_roughness(command.values, pmaterial.roughness, 0);
    auto scale = 1.0f;
    get_pbrt_value(command.values, "scale", scale);
    pmaterial.volscale = 1 / scale;
    auto sigma_a = vec3f{0, 0, 0}, sigma_s = vec3f{0, 0, 0};
    auto sigma_a_tex = -1, sigma_s_tex = -1;
    get_texture(command.values, "sigma_a", sigma_a, sigma_a_tex,
        vec3f{0.011f, .0024f, .014f});
    get_texture(command.values, "sigma_prime_s", sigma_s, sigma_s_tex,
        vec3f{2.55f, 3.12f, 3.77f});
    pmaterial.volmeanfreepath = 1 / (sigma_a + sigma_s);
    pmaterial.volscatter      = sigma_s / (sigma_a + sigma_s);
    if (verbose) printf("subsurface material not properly supported\n");
  } else if (command.type == "mix") {
    auto namedmaterial1 = ""s, namedmaterial2 = ""s;
    get_pbrt_value(command.values, "namedmaterial1", namedmaterial1);
    get_pbrt_value(command.values, "namedmaterial2", namedmaterial2);
    auto matname = (!namedmaterial1.empty()) ? namedmaterial1 : namedmaterial2;
    auto matit   = named_materials.find(matname);
    if (matit == named_materials.end()) return false;
    auto saved_name = pmaterial.name;
    pmaterial       = matit->second;
    pmaterial.name  = saved_name;
    if (verbose) printf("mix material not properly supported\n");
  } else if (command.type == "fourier") {
    auto bsdffile = ""s;
    get_pbrt_value(command.values, "bsdffile", bsdffile);
    if (bsdffile.rfind('/') != string::npos)
      bsdffile = bsdffile.substr(bsdffile.rfind('/') + 1);
    if (bsdffile == "paint.bsdf") {
      pmaterial.type      = pbrt_mtype::plastic;
      pmaterial.color     = {0.6f, 0.6f, 0.6f};
      pmaterial.ior       = 1.5f;
      pmaterial.roughness = 0.2f;
    } else if (bsdffile == "ceramic.bsdf") {
      pmaterial.type      = pbrt_mtype::plastic;
      pmaterial.color     = {0.6f, 0.6f, 0.6f};
      pmaterial.ior       = 1.5f;
      pmaterial.roughness = 0.25f;
    } else if (bsdffile == "leather.bsdf") {
      pmaterial.type      = pbrt_mtype::plastic;
      pmaterial.color     = {0.6f, 0.57f, 0.48f};
      pmaterial.ior       = 1.5f;
      pmaterial.roughness = 0.3f;
    } else if (bsdffile == "coated_copper.bsdf") {
      pmaterial.type      = pbrt_mtype::metal;
      auto eta            = vec3f{0.2004376970f, 0.9240334304f, 1.1022119527f};
      auto etak           = vec3f{3.9129485033f, 2.4528477015f, 2.1421879552f};
      pmaterial.color     = eta_to_reflectivity(eta, etak);
      pmaterial.roughness = 0.01f;
    } else if (bsdffile == "roughglass_alpha_0.2.bsdf") {
      pmaterial.type      = pbrt_mtype::glass;
      pmaterial.color     = {1, 1, 1};
      pmaterial.ior       = 1.5f;
      pmaterial.roughness = 0.2f;
    } else if (bsdffile == "roughgold_alpha_0.2.bsdf") {
      pmaterial.type      = pbrt_mtype::metal;
      auto eta            = vec3f{0.1431189557f, 0.3749570432f, 1.4424785571f};
      auto etak           = vec3f{3.9831604247f, 2.3857207478f, 1.6032152899f};
      pmaterial.color     = eta_to_reflectivity(eta, etak);
      pmaterial.roughness = 0.2f;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

// Make a triangle shape from a quad grid
template <typename PositionFunc, typename NormalFunc>
static void make_shape(vector<vec3i>& triangles, vector<vec3f>& positions,
    vector<vec3f>& normals, vector<vec2f>& texcoords, const vec2i& steps,
    const PositionFunc& position_func, const NormalFunc& normal_func) {
  auto vid = [steps](int i, int j) { return j * (steps.x + 1) + i; };
  auto tid = [steps](int i, int j, int c) { return (j * steps.x + i) * 2 + c; };
  positions.resize((steps.x + 1) * (steps.y + 1));
  normals.resize((steps.x + 1) * (steps.y + 1));
  texcoords.resize((steps.x + 1) * (steps.y + 1));
  for (auto j = 0; j < steps.y + 1; j++) {
    for (auto i = 0; i < steps.x + 1; i++) {
      auto uv              = vec2f{i / (float)steps.x, j / (float)steps.y};
      positions[vid(i, j)] = position_func(uv);
      normals[vid(i, j)]   = normal_func(uv);
      texcoords[vid(i, j)] = uv;
    }
  }
  triangles.resize(steps.x * steps.y * 2);
  for (auto j = 0; j < steps.y; j++) {
    for (auto i = 0; i < steps.x; i++) {
      triangles[tid(i, j, 0)] = {vid(i, j), vid(i + 1, j), vid(i + 1, j + 1)};
      triangles[tid(i, j, 1)] = {vid(i, j), vid(i + 1, j + 1), vid(i, j + 1)};
    }
  }
}

// pbrt sphere
static void make_sphere(vector<vec3i>& triangles, vector<vec3f>& positions,
    vector<vec3f>& normals, vector<vec2f>& texcoords, const vec2i& steps,
    float radius) {
  make_shape(
      triangles, positions, normals, texcoords, steps,
      [radius](const vec2f& uv) {
        auto pt = vec2f{2 * pif * uv.x, pif * (1 - uv.y)};
        return radius *
               vec3f{cos(pt.x) * sin(pt.y), sin(pt.x) * sin(pt.y), cos(pt.y)};
      },
      [](const vec2f& uv) {
        auto pt = vec2f{2 * pif * uv.x, pif * (1 - uv.y)};
        return vec3f{cos(pt.x) * sin(pt.y), sin(pt.x) * sin(pt.y), cos(pt.y)};
      });
}
static void make_disk(vector<vec3i>& triangles, vector<vec3f>& positions,
    vector<vec3f>& normals, vector<vec2f>& texcoords, const vec2i& steps,
    float radius) {
  make_shape(
      triangles, positions, normals, texcoords, steps,
      [radius](const vec2f& uv) {
        auto a = 2 * pif * uv.x;
        return radius * (1 - uv.y) * vec3f{cos(a), sin(a), 0};
      },
      [](const vec2f& uv) {
        return vec3f{0, 0, 1};
      });
}
static void make_quad(vector<vec3i>& triangles, vector<vec3f>& positions,
    vector<vec3f>& normals, vector<vec2f>& texcoords, const vec2i& steps,
    float radius) {
  make_shape(
      triangles, positions, normals, texcoords, steps,
      [radius](const vec2f& uv) {
        return vec3f{(uv.x - 0.5f) * radius, (uv.y - 0.5f) * radius, 0};
      },
      [](const vec2f& uv) {
        return vec3f{0, 0, 1};
      });
}

// Convert pbrt shapes
[[nodiscard]] static bool convert_shape(pbrt_shape& pshape,
    const pbrt_command& command, string& alphamap,
    const unordered_map<string, pbrt_texture>& named_textures,
    const string& ply_dirname, bool ply_meshes, const string& filename,
    bool verbose = false) {
  // helpers
  auto get_alpha = [&](const vector<pbrt_value>& values, const string& name,
                       string& filename) -> bool {
    auto def      = 1.0f;
    auto textured = pair{def, ""s};
    get_pbrt_value(values, name, textured);
    if (textured.second.empty()) {
      filename = "";
    } else {
      filename = named_textures.at(textured.second).filename;
    }
    return true;
  };

  pshape.frame = command.frame;
  pshape.frend = command.frend;
  if (command.type == "trianglemesh") {
    pshape.positions = {};
    pshape.normals   = {};
    pshape.texcoords = {};
    pshape.triangles = {};
    get_pbrt_value(command.values, "P", pshape.positions);
    get_pbrt_value(command.values, "N", pshape.normals);
    get_pbrt_value(command.values, "uv", pshape.texcoords);
    for (auto& uv : pshape.texcoords) uv.y = (1 - uv.y);
    get_pbrt_value(command.values, "indices", pshape.triangles);
  } else if (command.type == "loopsubdiv") {
    pshape.positions = {};
    pshape.triangles = {};
    get_pbrt_value(command.values, "P", pshape.positions);
    get_pbrt_value(command.values, "indices", pshape.triangles);
    pshape.normals.resize(pshape.positions.size());
    // compute_normals(pshape.normals, pshape.triangles, pshape.positions);
  } else if (command.type == "plymesh") {
    pshape.filename_ = ""s;
    get_pbrt_value(command.values, "filename", pshape.filename_);
    get_alpha(command.values, "alpha", alphamap);
    if (ply_meshes) {
      auto error = string{};
      auto ply   = ply_model{};
      if (!load_ply(path_join(ply_dirname, pshape.filename_), ply, error))
        return false;
      get_positions(ply, pshape.positions);
      get_normals(ply, pshape.normals);
      get_texcoords(ply, pshape.texcoords);
      get_triangles(ply, pshape.triangles);
    }
  } else if (command.type == "sphere") {
    auto radius = 1.0f;
    get_pbrt_value(command.values, "radius", radius);
    make_sphere(pshape.triangles, pshape.positions, pshape.normals,
        pshape.texcoords, {32, 16}, radius);
  } else if (command.type == "disk") {
    auto radius = 1.0f;
    get_pbrt_value(command.values, "radius", radius);
    make_disk(pshape.triangles, pshape.positions, pshape.normals,
        pshape.texcoords, {32, 1}, radius);
  } else {
    return false;
  }
  return true;
}

// Convert pbrt arealights
[[nodiscard]] static bool convert_arealight(pbrt_arealight& parealight,
    const pbrt_command& command, const string& filename, bool verbose = false) {
  parealight.name = command.name;
  if (command.type == "diffuse") {
    auto l = vec3f{1, 1, 1}, scale = vec3f{1, 1, 1};
    get_pbrt_value(command.values, "L", l);
    get_pbrt_value(command.values, "scale", scale);
    parealight.emission = l * scale;
  } else {
    return false;
  }
  return true;
}

// Convert pbrt lights
[[nodiscard]] static bool convert_light(pbrt_light& plight,
    const pbrt_command& command, const string& filename, bool verbose = false) {
  plight.frame = command.frame;
  plight.frend = command.frend;
  if (command.type == "distant") {
    auto l = vec3f{1, 1, 1}, scale = vec3f{1, 1, 1};
    get_pbrt_value(command.values, "L", l);
    get_pbrt_value(command.values, "scale", scale);
    plight.emission = l * scale;
    plight.from     = vec3f{0, 0, 0};
    plight.to       = vec3f{0, 0, 1};
    get_pbrt_value(command.values, "from", plight.from);
    get_pbrt_value(command.values, "to", plight.to);
    plight.distant       = true;
    auto distant_dist    = 100.0f;
    auto size            = distant_dist * sin(5 * pif / 180);
    plight.area_emission = plight.emission * (distant_dist * distant_dist) /
                           (size * size);
    plight.area_frame = plight.frame *
                        lookat_frame(
                            normalize(plight.from - plight.to) * distant_dist,
                            {0, 0, 0}, {0, 1, 0}, true);
    plight.area_frend = plight.frend *
                        lookat_frame(
                            normalize(plight.from - plight.to) * distant_dist,
                            {0, 0, 0}, {0, 1, 0}, true);
    auto texcoords = vector<vec2f>{};
    make_quad(plight.area_triangles, plight.area_positions, plight.area_normals,
        texcoords, {4, 2}, size);
  } else if (command.type == "point" || command.type == "goniometric" ||
             command.type == "spot") {
    auto i = vec3f{1, 1, 1}, scale = vec3f{1, 1, 1};
    get_pbrt_value(command.values, "I", i);
    get_pbrt_value(command.values, "scale", scale);
    plight.emission = i * scale;
    plight.from     = {0, 0, 0};
    get_pbrt_value(command.values, "from", plight.from);
    plight.area_emission = plight.emission;
    plight.area_frame    = plight.frame * translation_frame(plight.from);
    plight.area_frend    = plight.frend * translation_frame(plight.from);
    auto texcoords       = vector<vec2f>{};
    make_sphere(plight.area_triangles, plight.area_positions,
        plight.area_normals, texcoords, {4, 2}, 0.0025f);
  } else {
    return false;
  }
  return true;
}

[[nodiscard]] static bool convert_environment(pbrt_environment& penvironment,
    const pbrt_command& command, unordered_map<string, int>& texture_map,
    const string& filename, bool verbose = false) {
  penvironment.frame = command.frame;
  penvironment.frend = command.frend;
  penvironment.frame = penvironment.frame *
                       frame3f{{1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0, 0}};
  penvironment.frend = penvironment.frend *
                       frame3f{{1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0, 0}};
  if (command.type == "infinite") {
    auto l = vec3f{1, 1, 1}, scale = vec3f{1, 1, 1};
    get_pbrt_value(command.values, "L", l);
    get_pbrt_value(command.values, "scale", scale);
    penvironment.emission     = scale * l;
    penvironment.emission_tex = -1;
    auto mapname              = ""s;
    get_pbrt_value(command.values, "mapname", mapname);
    if (!mapname.empty()) {
      if (texture_map.find(mapname) == texture_map.end()) {
        auto texture_id      = (int)texture_map.size();
        texture_map[mapname] = texture_id;
      }
      penvironment.emission_tex = texture_map.at(mapname);
    }
  } else {
    return false;
  }
  return true;
}

// pbrt stack ctm
struct pbrt_stack_element {
  frame3f        transform_start        = identity3x4f;
  frame3f        transform_end          = identity3x4f;
  pbrt_material  material               = {};
  pbrt_arealight arealight              = {};
  pbrt_medium    interior               = {};
  pbrt_medium    exterior               = {};
  bool           reverse                = false;
  bool           active_transform_start = true;
  bool           active_transform_end   = true;
};

// pbrt parsing context
struct pbrt_context {
  vector<pbrt_stack_element>                stack           = {};
  unordered_map<string, pbrt_stack_element> coordsys        = {};
  string                                    cur_object      = "";
  vec2i                                     film_resolution = {512, 512};
};

// load pbrt
static bool load_pbrt(const string& filename, pbrt_model& pbrt, string& error,
    pbrt_context& ctx, unordered_map<string, int>& material_map,
    unordered_map<string, int>&           texture_map,
    unordered_map<string, pbrt_material>& named_materials,
    unordered_map<string, pbrt_texture>&  named_textures,
    unordered_map<string, pbrt_medium>&   named_mediums,
    unordered_map<string, vector<int>>&   named_objects,
    const string& ply_dirname, bool ply_meshes) {
  // load data
  auto data = string{};
  if (!load_text(filename, data, error)) return false;

  // helpers
  auto set_transform = [](pbrt_stack_element& ctx, const frame3f& xform) {
    if (ctx.active_transform_start) ctx.transform_start = xform;
    if (ctx.active_transform_end) ctx.transform_end = xform;
  };
  auto concat_transform = [](pbrt_stack_element& ctx, const frame3f& xform) {
    if (ctx.active_transform_start) ctx.transform_start *= xform;
    if (ctx.active_transform_end) ctx.transform_end *= xform;
  };

  // init stack
  if (ctx.stack.empty()) ctx.stack.emplace_back();

  // parse command by command
  auto data_view   = string_view{data.data(), data.size()};
  auto line        = ""s;
  auto parse_error = [&filename, &error]() {
    error = filename + ": parse error";
    return false;
  };
  auto dependent_error = [&filename, &error]() {
    error = filename + ": error in " + error;
    return false;
  };
  while (read_pbrt_cmdline(data_view, line)) {
    auto str = string_view{line};
    // get command
    auto cmd = ""s;
    if (!parse_command(str, cmd)) return parse_error();
    if (cmd == "WorldBegin") {
      ctx.stack.push_back({});
    } else if (cmd == "WorldEnd") {
      if (ctx.stack.empty()) return parse_error();
      ctx.stack.pop_back();
      if (ctx.stack.size() != 1) return parse_error();
    } else if (cmd == "AttributeBegin") {
      ctx.stack.push_back(ctx.stack.back());
    } else if (cmd == "AttributeEnd") {
      if (ctx.stack.empty()) return parse_error();
      ctx.stack.pop_back();
    } else if (cmd == "TransformBegin") {
      ctx.stack.push_back(ctx.stack.back());
    } else if (cmd == "TransformEnd") {
      if (ctx.stack.empty()) return parse_error();
      ctx.stack.pop_back();
    } else if (cmd == "ObjectBegin") {
      ctx.stack.push_back(ctx.stack.back());
      if (!parse_param(str, ctx.cur_object)) return parse_error();
      named_objects[ctx.cur_object] = {};
    } else if (cmd == "ObjectEnd") {
      ctx.stack.pop_back();
      ctx.cur_object = "";
    } else if (cmd == "ObjectInstance") {
      auto object = ""s;
      if (!parse_param(str, object)) return parse_error();
      if (named_objects.find(object) == named_objects.end())
        return parse_error();
      auto& named_object = named_objects.at(object);
      for (auto& shape_id : named_object) {
        pbrt.shapes[shape_id].instances.push_back(
            ctx.stack.back().transform_start);
        pbrt.shapes[shape_id].instaends.push_back(
            ctx.stack.back().transform_end);
      }
    } else if (cmd == "ActiveTransform") {
      auto name = ""s;
      if (!parse_command(str, name)) return parse_error();
      if (name == "StartTime") {
        ctx.stack.back().active_transform_start = true;
        ctx.stack.back().active_transform_end   = false;
      } else if (name == "EndTime") {
        ctx.stack.back().active_transform_start = false;
        ctx.stack.back().active_transform_end   = true;
      } else if (name == "All") {
        ctx.stack.back().active_transform_start = true;
        ctx.stack.back().active_transform_end   = true;
      } else {
        std::out_of_range{"invalid command"};
      }
    } else if (cmd == "Transform") {
      auto xf = identity4x4f;
      if (!parse_param(str, xf)) return parse_error();
      set_transform(ctx.stack.back(), mat_to_frame(xf));
    } else if (cmd == "ConcatTransform") {
      auto xf = identity4x4f;
      if (!parse_param(str, xf)) return parse_error();
      concat_transform(ctx.stack.back(), mat_to_frame(xf));
    } else if (cmd == "Scale") {
      auto v = vec3f{0, 0, 0};
      if (!parse_param(str, v)) return parse_error();
      concat_transform(ctx.stack.back(), scaling_frame(v));
    } else if (cmd == "Translate") {
      auto v = vec3f{0, 0, 0};
      if (!parse_param(str, v)) return parse_error();
      concat_transform(ctx.stack.back(), translation_frame(v));
    } else if (cmd == "Rotate") {
      auto v = zero4f;
      if (!parse_param(str, v)) return parse_error();
      concat_transform(
          ctx.stack.back(), rotation_frame(vec3f{v.y, v.z, v.w}, radians(v.x)));
    } else if (cmd == "LookAt") {
      auto from = vec3f{0, 0, 0}, to = vec3f{0, 0, 0}, up = vec3f{0, 0, 0};
      if (!parse_param(str, from)) return parse_error();
      if (!parse_param(str, to)) return parse_error();
      if (!parse_param(str, up)) return parse_error();
      auto frame = lookat_frame(from, to, up, true);
      concat_transform(ctx.stack.back(), inverse(frame));
    } else if (cmd == "ReverseOrientation") {
      ctx.stack.back().reverse = !ctx.stack.back().reverse;
    } else if (cmd == "CoordinateSystem") {
      auto name = ""s;
      if (!parse_param(str, name)) return parse_error();
      ctx.coordsys[name].transform_start = ctx.stack.back().transform_start;
      ctx.coordsys[name].transform_end   = ctx.stack.back().transform_end;
    } else if (cmd == "CoordSysTransform") {
      auto name = ""s;
      if (!parse_param(str, name)) return parse_error();
      if (ctx.coordsys.find(name) != ctx.coordsys.end()) {
        ctx.stack.back().transform_start =
            ctx.coordsys.at(name).transform_start;
        ctx.stack.back().transform_end = ctx.coordsys.at(name).transform_end;
      }
    } else if (cmd == "Integrator") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
    } else if (cmd == "Sampler") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
    } else if (cmd == "PixelFilter") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
    } else if (cmd == "Film") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      auto film = pbrt_film{};
      if (!convert_film(film, command, filename)) return parse_error();
      ctx.film_resolution = film.resolution;
    } else if (cmd == "Accelerator") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
    } else if (cmd == "Camera") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      command.frame = ctx.stack.back().transform_start;
      command.frend = ctx.stack.back().transform_end;
      auto& camera  = pbrt.cameras.emplace_back();
      if (!convert_camera(camera, command, ctx.film_resolution, filename))
        return parse_error();
    } else if (cmd == "Texture") {
      auto command  = pbrt_command{};
      auto comptype = ""s;
      auto str_     = string{str};
      if (!parse_param(str, command.name)) return parse_error();
      if (!parse_param(str, comptype)) return parse_error();
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      if (!convert_texture(
              named_textures[command.name], command, named_textures, filename))
        return parse_error();
    } else if (cmd == "Material") {
      static auto material_id = 0;
      auto        command     = pbrt_command{};
      command.name = "__unnamed__material__" + std::to_string(material_id++);
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      if (command.type.empty()) {
        ctx.stack.back().material = {};
      } else {
        ctx.stack.back().material = {};
        if (!convert_material(ctx.stack.back().material, command, texture_map,
                named_materials, named_textures, filename))
          return parse_error();
      }
    } else if (cmd == "MakeNamedMaterial") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.name)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      command.type = "";
      for (auto& value : command.values)
        if (value.name == "type") command.type = value.value1s;
      if (!convert_material(named_materials[command.name], command, texture_map,
              named_materials, named_textures, filename))
        return parse_error();
    } else if (cmd == "NamedMaterial") {
      auto name = ""s;
      if (!parse_param(str, name)) return parse_error();
      if (named_materials.find(name) == named_materials.end())
        return parse_error();
      ctx.stack.back().material = named_materials.at(name);
    } else if (cmd == "Shape") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      command.frame  = ctx.stack.back().transform_start;
      command.frend  = ctx.stack.back().transform_end;
      auto& shape    = pbrt.shapes.emplace_back();
      auto  alphamap = ""s;
      if (!convert_shape(shape, command, alphamap, named_textures, ply_dirname,
              ply_meshes, filename))
        return parse_error();
      auto matkey = "?!!!?" + ctx.stack.back().material.name + "?!!!?" +
                    ctx.stack.back().arealight.name + "?!!!?" + alphamap;
      if (material_map.find(matkey) == material_map.end()) {
        auto& material    = pbrt.materials.emplace_back();
        material          = ctx.stack.back().material;
        material.name     = "material" + std::to_string(pbrt.materials.size());
        material.emission = ctx.stack.back().arealight.emission;
        // material.alpha_tex = alphamap;
        material_map[matkey] = (int)pbrt.materials.size() - 1;
      }
      shape.material = material_map.at(matkey);
      if (!ctx.cur_object.empty()) {
        named_objects[ctx.cur_object].push_back((int)pbrt.shapes.size() - 1);
        shape.instanced = true;
      }
    } else if (cmd == "AreaLightSource") {
      static auto arealight_id = 0;
      auto        command      = pbrt_command{};
      command.name = "__unnamed__arealight__" + std::to_string(arealight_id++);
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      command.frame = ctx.stack.back().transform_start;
      command.frend = ctx.stack.back().transform_end;
      if (!convert_arealight(ctx.stack.back().arealight, command, filename))
        return parse_error();
    } else if (cmd == "LightSource") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.type)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      command.frame = ctx.stack.back().transform_start;
      command.frend = ctx.stack.back().transform_end;
      if (command.type == "infinite") {
        auto& environment = pbrt.environments.emplace_back();
        if (!convert_environment(environment, command, texture_map, filename))
          return parse_error();
      } else {
        auto& light = pbrt.lights.emplace_back();
        if (!convert_light(light, command, filename)) return parse_error();
      }
    } else if (cmd == "MakeNamedMedium") {
      auto command = pbrt_command{};
      if (!parse_param(str, command.name)) return parse_error();
      if (!parse_params(str, command.values)) return parse_error();
      command.type = "";
      for (auto& value : command.values)
        if (command.name == "type") command.type = value.value1s;
      auto medium                 = pbrt_medium{};
      named_mediums[command.name] = medium;
    } else if (cmd == "MediumInterface") {
      auto interior = ""s, exterior = ""s;
      if (!parse_param(str, interior)) return parse_error();
      if (!parse_param(str, exterior)) return parse_error();
      ctx.stack.back().interior = named_mediums.at(interior);
      ctx.stack.back().exterior = named_mediums.at(exterior);
    } else if (cmd == "Include") {
      auto includename = ""s;
      if (!parse_param(str, includename)) return parse_error();
      if (!load_pbrt(path_join(path_dirname(filename), includename), pbrt,
              error, ctx, material_map, texture_map, named_materials,
              named_textures, named_mediums, named_objects, ply_dirname,
              ply_meshes))
        return dependent_error();
    } else {
      return parse_error();
    }
  }
  return true;
}

// load pbrt
bool load_pbrt(
    const string& filename, pbrt_model& pbrt, string& error, bool ply_meshes) {
  auto ctx             = pbrt_context{};
  auto material_map    = unordered_map<string, int>{};
  auto texture_map     = unordered_map<string, int>{};
  auto named_materials = unordered_map<string, pbrt_material>{{"", {}}};
  auto named_mediums   = unordered_map<string, pbrt_medium>{{"", {}}};
  auto named_textures  = unordered_map<string, pbrt_texture>{{"", {}}};
  auto named_objects   = unordered_map<string, vector<int>>{};
  if (!load_pbrt(filename, pbrt, error, ctx, material_map, texture_map,
          named_materials, named_textures, named_mediums, named_objects,
          path_dirname(filename), ply_meshes))
    return false;
  pbrt.textures.resize(texture_map.size());
  for (auto& [path, texture_id] : texture_map) {
    pbrt.textures[texture_id].filename = path;
  }
  return true;
}

static void format_value(string& str, const pbrt_value& value) {
  static auto type_labels = unordered_map<pbrt_type, string>{
      {pbrt_type::real, "float"},
      {pbrt_type::integer, "integer"},
      {pbrt_type::boolean, "bool"},
      {pbrt_type::string, "string"},
      {pbrt_type::point, "point"},
      {pbrt_type::normal, "normal"},
      {pbrt_type::vector, "vector"},
      {pbrt_type::texture, "texture"},
      {pbrt_type::color, "rgb"},
      {pbrt_type::point2, "point2"},
      {pbrt_type::vector2, "vector2"},
      {pbrt_type::spectrum, "spectrum"},
  };

  auto format_vector = [](string& str, auto& values) {
    str += "[ ";
    for (auto& value : values) {
      str += " ";
      format_value(str, value);
    }
    str += " ]";
  };

  format_values(str, "\"{} {}\" ", type_labels.at(value.type), value.name);
  switch (value.type) {
    case pbrt_type::real:
      if (!value.vector1f.empty()) {
        format_vector(str, value.vector1f);
      } else {
        format_value(str, value.value1f);
      }
      break;
    case pbrt_type::integer:
      if (!value.vector1f.empty()) {
        format_vector(str, value.vector1i);
      } else {
        format_value(str, value.value1i);
      }
      break;
    case pbrt_type::boolean:
      format_values(str, "\"{}\"", value.value1b ? "true" : "false");
      break;
    case pbrt_type::string:
    case pbrt_type::texture: format_values(str, "\"{}\"", value.value1s); break;
    case pbrt_type::point:
    case pbrt_type::vector:
    case pbrt_type::normal:
    case pbrt_type::color:
      if (!value.vector3f.empty()) {
        format_vector(str, value.vector3f);
      } else {
        format_values(str, "[ {} ]", value.value3f);
      }
      break;
    case pbrt_type::spectrum: format_vector(str, value.vector1f); break;
    case pbrt_type::point2:
    case pbrt_type::vector2:
      if (!value.vector2f.empty()) {
        format_vector(str, value.vector2f);
      } else {
        format_values(str, "[ {} ]", value.value2f);
      }
      break;
  }
}

static void format_value(string& str, const vector<pbrt_value>& values) {
  for (auto& value : values) {
    str += " ";
    format_value(str, value);
  }
}

bool save_pbrt(const string& filename, const pbrt_model& pbrt, string& error,
    bool ply_meshes) {
  // buffer
  auto buffer = string{};

  // save comments
  format_values(buffer, "#\n");
  format_values(buffer, "# Written by Yocto/GL\n");
  format_values(buffer, "# https://github.com/xelatihy/yocto-gl\n");
  format_values(buffer, "#\n\n");
  for (auto& comment : pbrt.comments) {
    format_values(buffer, "# {}\n", comment);
  }
  format_values(buffer, "\n");

  for (auto& camera : pbrt.cameras) {
    auto command = pbrt_command{};
    command.type = "image";
    command.values.push_back(
        make_pbrt_value("xresolution", camera.resolution.x));
    command.values.push_back(
        make_pbrt_value("yresolution", camera.resolution.y));
    command.values.push_back(make_pbrt_value("filename", "image.exr"s));
    format_values(buffer, "Film \"{}\" {}\n", command.type, command.values);
  }

  for (auto& camera : pbrt.cameras) {
    auto command  = pbrt_command{};
    command.type  = "perspective";
    command.frame = camera.frame;
    command.values.push_back(make_pbrt_value(
        "fov", 2 * tan(0.036f / (2 * camera.lens)) * 180 / pif));
    format_values(buffer, "LookAt {} {} {}\n", command.frame.o,
        command.frame.o - command.frame.z, command.frame.y);
    format_values(buffer, "Camera \"{}\" {}\n", command.type, command.values);
  }

  format_values(buffer, "\nWorldBegin\n\n");

  for (auto& light : pbrt.lights) {
    auto command  = pbrt_command{};
    command.frame = light.frame;
    if (light.distant) {
      command.type = "distance";
      command.values.push_back(make_pbrt_value("L", light.emission));
    } else {
      command.type = "point";
      command.values.push_back(make_pbrt_value("I", light.emission));
    }
    format_values(buffer, "AttributeBegin\n");
    format_values(buffer, "Transform {}\n", frame_to_mat(command.frame));
    format_values(
        buffer, "LightSource \"{}\" {}\n", command.type, command.values);
    format_values(buffer, "AttributeEnd\n");
  }

  for (auto& environment : pbrt.environments) {
    auto command  = pbrt_command{};
    command.frame = environment.frame;
    command.type  = "infinite";
    command.values.push_back(make_pbrt_value("L", environment.emission));
    command.values.push_back(
        make_pbrt_value("mapname", environment.emission_tex));
    format_values(buffer, "AttributeBegin\n");
    format_values(buffer, "Transform {}\n", frame_to_mat(command.frame));
    format_values(
        buffer, "LightSource \"{}\" {}\n", command.type, command.values);
    format_values(buffer, "AttributeEnd\n");
  }

  auto reflectivity_to_eta = [](const vec3f& reflectivity) {
    return (1 + sqrt(reflectivity)) / (1 - sqrt(reflectivity));
  };

  for (auto& material : pbrt.materials) {
    auto command = pbrt_command{};
    switch (material.type) {
      case pbrt_mtype::matte: {
        command.type = "matte";
        command.values.push_back(make_pbrt_value("Kd", material.color));
      } break;
      case pbrt_mtype::plastic: {
        command.type = "matte";
        command.values.push_back(make_pbrt_value("Kd", material.color));
        command.values.push_back(make_pbrt_value("Ks", vec3f{1, 1, 1}));
        command.values.push_back(
            make_pbrt_value("roughness", pow(material.roughness, 2)));
        command.values.push_back(
            make_pbrt_value("eta", reflectivity_to_eta(material.color)));
        command.values.push_back(make_pbrt_value("remaproughness", false));
      } break;
      case pbrt_mtype::metal: {
        command.type = "metal";
        command.values.push_back(make_pbrt_value("Kr", vec3f{1, 1, 1}));
        command.values.push_back(
            make_pbrt_value("roughness", pow(material.roughness, 2)));
        command.values.push_back(
            make_pbrt_value("eta", reflectivity_to_eta(material.color)));
        command.values.push_back(make_pbrt_value("remaproughness", false));
      } break;
      case pbrt_mtype::thinglass: {
        command.type = "uber";
        command.values.push_back(make_pbrt_value("Ks", vec3f{1, 1, 1}));
        command.values.push_back(make_pbrt_value("Kt", material.color));
        command.values.push_back(
            make_pbrt_value("roughness", pow(material.roughness, 2)));
        command.values.push_back(
            make_pbrt_value("eta", reflectivity_to_eta(material.color)));
        command.values.push_back(make_pbrt_value("remaproughness", false));
      } break;
      case pbrt_mtype::glass: {
        command.type = "glass";
        command.values.push_back(make_pbrt_value("Kr", vec3f{1, 1, 1}));
        command.values.push_back(make_pbrt_value("Kt", vec3f{1, 1, 1}));
        command.values.push_back(
            make_pbrt_value("roughness", pow(material.roughness, 2)));
        command.values.push_back(make_pbrt_value("eta", material.ior));
        command.values.push_back(make_pbrt_value("remaproughness", false));
      } break;
      case pbrt_mtype::subsurface: {
        command.type = "matte";
        command.values.push_back(make_pbrt_value("Kd", material.color));
      } break;
    }

    format_values(buffer,
        "MakeNamedMaterial \"{}\" \"string type\" \"{}\" {}\n", material.name,
        command.type, command.values);
  }

  auto object_id = 0;
  for (auto& shape : pbrt.shapes) {
    auto& material = pbrt.materials.at(shape.material);
    auto  command  = pbrt_command{};
    command.frame  = shape.frame;
    if (ply_meshes) {
      command.type = "plymesh";
      command.values.push_back(make_pbrt_value("filename", shape.filename_));
    } else {
      command.type = "trianglemesh";
      command.values.push_back(make_pbrt_value("indices", shape.triangles));
      command.values.push_back(
          make_pbrt_value("P", shape.positions, pbrt_type::point));
      if (!shape.normals.empty())
        command.values.push_back(
            make_pbrt_value("N", shape.triangles, pbrt_type::normal));
      if (!shape.texcoords.empty())
        command.values.push_back(make_pbrt_value("uv", shape.texcoords));
    }
    if (ply_meshes) {
      auto ply = ply_model{};
      add_positions(ply, shape.positions);
      add_normals(ply, shape.normals);
      add_texcoords(ply, shape.texcoords);
      add_triangles(ply, shape.triangles);
      if (!save_ply(path_dirname(filename) + "/" + shape.filename_, ply, error))
        return false;
    }
    auto object = "object" + std::to_string(object_id++);
    if (!shape.instances.empty())
      format_values(buffer, "ObjectBegin \"{}\"\n", object);
    format_values(buffer, "AttributeBegin\n");
    format_values(buffer, "Transform {}\n", frame_to_mat(shape.frame));
    if (material.emission != vec3f{0, 0, 0}) {
      auto acommand = pbrt_command{};
      acommand.type = "diffuse";
      acommand.values.push_back(make_pbrt_value("L", material.emission));
      format_values(buffer, "AreaLightSource \"{}\" {}\n", acommand.type,
          acommand.values);
    }
    format_values(buffer, "NamedMaterial \"{}\"\n", material.name);
    format_values(buffer, "Shape \"{}\" {}\n", command.type, command.values);
    format_values(buffer, "AttributeEnd\n");
    if (!shape.instances.empty()) format_values(buffer, "ObjectEnd\n");
    for (auto& iframe : shape.instances) {
      format_values(buffer, "AttributeBegin\n");
      format_values(buffer, "Transform {}\n", frame_to_mat(iframe));
      format_values(buffer, "ObjectInstance \"{}\"\n", object);
      format_values(buffer, "AttributeEnd\n");
    }
  }

  format_values(buffer, "\nWorldEnd\n\n");

  // save file
  if (!save_text(filename, buffer, error)) return false;

  // done
  return true;
}

}  // namespace yocto
