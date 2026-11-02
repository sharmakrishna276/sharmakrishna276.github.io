#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ssg {

struct Error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Throws an Error formatted as `file:line: msg`.
[[noreturn]] void fail(const std::string& file, int line, const std::string& msg);

std::string read_file(const std::filesystem::path& path);
void write_file(const std::filesystem::path& path, std::string_view data);

// Escapes & < > " ' for use in HTML text and attribute values.
void append_escaped(std::string& out, std::string_view s);
std::string html_escape(std::string_view s);

// Splits on '\n', dropping a trailing '\r' from each line. The views point
// into `s`, so `s` must outlive the result.
std::vector<std::string_view> split_lines(std::string_view s);

std::string_view trim(std::string_view s);

}  // namespace ssg
