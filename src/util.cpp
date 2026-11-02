#include "util.hpp"

#include <fstream>
#include <sstream>

namespace ssg {

void fail(const std::string& file, int line, const std::string& msg) {
    throw Error(file + ":" + std::to_string(line) + ": " + msg);
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw Error("cannot read " + path.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    return std::move(ss).str();
}

void write_file(const std::filesystem::path& path, std::string_view data) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out) throw Error("cannot write " + path.string());
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

void append_escaped(std::string& out, std::string_view s) {
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
}

std::string html_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    append_escaped(out, s);
    return out;
}

std::vector<std::string_view> split_lines(std::string_view s) {
    std::vector<std::string_view> lines;
    size_t start = 0;
    while (start < s.size()) {
        size_t end = s.find('\n', start);
        if (end == std::string_view::npos) end = s.size();
        std::string_view line = s.substr(start, end - start);
        if (line.ends_with('\r')) line.remove_suffix(1);
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

std::string_view trim(std::string_view s) {
    const size_t begin = s.find_first_not_of(" \t");
    if (begin == std::string_view::npos) return {};
    const size_t end = s.find_last_not_of(" \t");
    return s.substr(begin, end - begin + 1);
}

}  // namespace ssg
