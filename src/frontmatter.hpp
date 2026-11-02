#pragma once

#include <map>
#include <string>
#include <string_view>

namespace ssg {

using Meta = std::map<std::string, std::string>;

struct Document {
    Meta meta;
    std::string body;
    int body_line = 1;  // 1-based line in the source file where `body` starts
};

// Parses `key: value` lines. Blank lines and lines starting with `#` are
// skipped; matching surrounding quotes are stripped from values.
Meta parse_kv(std::string_view text, const std::string& file, int first_line);

// Splits a `---`-delimited frontmatter block off the top of a document.
Document parse_document(std::string_view text, const std::string& file);

}  // namespace ssg
