#include "frontmatter.hpp"

#include <vector>

#include "util.hpp"

namespace ssg {
namespace {

std::string_view unquote(std::string_view v) {
    if (v.size() >= 2 && (v.front() == '"' || v.front() == '\'') && v.back() == v.front())
        return v.substr(1, v.size() - 2);
    return v;
}

Meta parse_lines(const std::vector<std::string_view>& lines, size_t begin, size_t end,
                 const std::string& file, int first_line) {
    Meta meta;
    for (size_t i = begin; i < end; ++i) {
        const std::string_view t = trim(lines[i]);
        if (t.empty() || t.starts_with('#')) continue;

        const int lineno = first_line + static_cast<int>(i);
        const size_t colon = t.find(':');
        if (colon == std::string_view::npos)
            fail(file, lineno, "expected `key: value`, got `" + std::string(t) + "`");

        const std::string_view key = trim(t.substr(0, colon));
        if (key.empty()) fail(file, lineno, "empty key");
        meta[std::string(key)] = std::string(unquote(trim(t.substr(colon + 1))));
    }
    return meta;
}

}  // namespace

Meta parse_kv(std::string_view text, const std::string& file, int first_line) {
    const auto lines = split_lines(text);
    return parse_lines(lines, 0, lines.size(), file, first_line);
}

Document parse_document(std::string_view text, const std::string& file) {
    const auto lines = split_lines(text);
    if (lines.empty() || trim(lines[0]) != "---")
        fail(file, 1, "missing frontmatter: file must start with a `---` line");

    for (size_t end = 1; end < lines.size(); ++end) {
        if (trim(lines[end]) != "---") continue;

        Document doc;
        doc.meta = parse_lines(lines, 1, end, file, 1);
        for (size_t i = end + 1; i < lines.size(); ++i) {
            doc.body += lines[i];
            doc.body += '\n';
        }
        doc.body_line = static_cast<int>(end) + 2;
        return doc;
    }
    fail(file, 1, "unclosed frontmatter: no closing `---` line");
}

}  // namespace ssg
