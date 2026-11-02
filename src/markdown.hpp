#pragma once

#include <string>
#include <string_view>

namespace ssg {

// Renders a pragmatic Markdown subset to HTML:
//
//   blocks:  # headings, paragraphs, - / * / 1. lists (nestable), > quotes,
//            ``` fenced code ```, --- rules, raw HTML / {{placeholder}} lines
//   inline:  `code`, **strong**, *em*, [links](url), ![images](src), \escapes
//
// `file` and `first_line` are used for error messages (e.g. unclosed fences).
std::string render_markdown(std::string_view src, const std::string& file, int first_line);

}  // namespace ssg
