#include "markdown.hpp"

#include <cctype>
#include <optional>
#include <vector>

#include "util.hpp"

namespace ssg {
namespace {

constexpr size_t npos = std::string_view::npos;

using Lines = std::vector<std::string_view>;

// ---------------------------------------------------------------------------
// Inline elements
// ---------------------------------------------------------------------------

std::string render_inline(std::string_view s);

// Closing `**` of a strong span. A run like `***` closes on its last two
// stars, so `**bold *em***` nests as expected.
size_t find_strong_close(std::string_view s, size_t from) {
    size_t e = s.find("**", from);
    while (e != npos && e + 2 < s.size() && s[e + 2] == '*') ++e;
    return e;
}

// Closing `*` of an emphasis span. Steps over `**strong**` pairs and code
// spans so that `*a **b** c*` nests correctly.
size_t find_em_close(std::string_view s, size_t from) {
    for (size_t j = from; j < s.size(); ++j) {
        if (s[j] == '\\') {
            ++j;
        } else if (s[j] == '`') {
            j = s.find('`', j + 1);
            if (j == npos) return npos;
        } else if (s[j] == '*') {
            if (j + 1 < s.size() && s[j + 1] == '*') {
                j = s.find("**", j + 2);
                if (j == npos) return npos;
                ++j;
            } else {
                return j;
            }
        }
    }
    return npos;
}

struct Link {
    std::string_view text, url;
    size_t end;  // index of the closing ')'
};

// Parses `[text](url)` starting at s[open] == '['.
std::optional<Link> parse_link(std::string_view s, size_t open) {
    const size_t close = s.find(']', open + 1);
    if (close == npos || close + 1 >= s.size() || s[close + 1] != '(') return std::nullopt;
    const size_t paren = s.find(')', close + 2);
    if (paren == npos) return std::nullopt;
    return Link{s.substr(open + 1, close - open - 1), trim(s.substr(close + 2, paren - close - 2)), paren};
}

std::string render_inline(std::string_view s) {
    std::string out;
    out.reserve(s.size() + s.size() / 4);

    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        const char next = i + 1 < s.size() ? s[i + 1] : '\0';

        if (c == '\\' && std::ispunct(static_cast<unsigned char>(next))) {
            append_escaped(out, s.substr(i + 1, 1));
            ++i;
            continue;
        }
        if (c == '`') {
            if (const size_t e = s.find('`', i + 1); e != npos) {
                out += "<code>";
                append_escaped(out, s.substr(i + 1, e - i - 1));
                out += "</code>";
                i = e;
                continue;
            }
        }
        if (c == '*' && next == '*') {
            if (const size_t e = find_strong_close(s, i + 2); e != npos && e > i + 2) {
                out += "<strong>" + render_inline(s.substr(i + 2, e - i - 2)) + "</strong>";
                i = e + 1;
                continue;
            }
        }
        if (c == '*' && next != ' ' && next != '*') {
            if (const size_t e = find_em_close(s, i + 1); e != npos && e > i + 1) {
                out += "<em>" + render_inline(s.substr(i + 1, e - i - 1)) + "</em>";
                i = e;
                continue;
            }
        }
        if (c == '!' && next == '[') {
            if (const auto link = parse_link(s, i + 1)) {
                out += "<img src=\"" + html_escape(link->url) + "\" alt=\"" + html_escape(link->text) + "\">";
                i = link->end;
                continue;
            }
        }
        if (c == '[') {
            if (const auto link = parse_link(s, i)) {
                out += "<a href=\"" + html_escape(link->url) + "\">" + render_inline(link->text) + "</a>";
                i = link->end;
                continue;
            }
        }
        append_escaped(out, s.substr(i, 1));
    }
    return out;
}

// ---------------------------------------------------------------------------
// Block elements
// ---------------------------------------------------------------------------

std::string render_blocks(const Lines& lines, const std::string& file, int first_line);

bool is_blank(std::string_view s) { return trim(s).empty(); }
bool is_indented(std::string_view s) { return !s.empty() && (s[0] == ' ' || s[0] == '\t'); }
bool is_rule(std::string_view t) { return t.size() >= 3 && t.find_first_not_of('-') == npos; }

int heading_level(std::string_view t) {
    size_t level = 0;
    while (level < t.size() && t[level] == '#') ++level;
    if (level == 0 || level > 6 || (level < t.size() && t[level] != ' ')) return 0;
    return static_cast<int>(level);
}

std::string slugify(std::string_view s) {
    std::string out;
    for (char c : s) {
        const auto u = static_cast<unsigned char>(c);
        if (std::isalnum(u)) out += static_cast<char>(std::tolower(u));
        else if (!out.empty() && out.back() != '-') out += '-';
    }
    while (out.ends_with('-')) out.pop_back();
    return out;
}

struct ListMarker {
    bool ordered;
    size_t width;  // characters before the item text, e.g. 2 for "- "
};

std::optional<ListMarker> list_marker(std::string_view t) {
    if (t.size() >= 2 && (t[0] == '-' || t[0] == '*' || t[0] == '+') && t[1] == ' ')
        return ListMarker{false, 2};
    size_t d = 0;
    while (d < t.size() && d < 9 && std::isdigit(static_cast<unsigned char>(t[d]))) ++d;
    if (d > 0 && d + 1 < t.size() && t[d] == '.' && t[d + 1] == ' ') return ListMarker{true, d + 2};
    return std::nullopt;
}

// Marker of an unindented line, or nothing. Indented markers belong to a
// nested list inside the current item.
std::optional<ListMarker> top_marker(std::string_view raw) {
    return is_indented(raw) ? std::nullopt : list_marker(trim(raw));
}

std::string_view dedent(std::string_view s, size_t width) {
    size_t k = 0;
    while (k < width && k < s.size() && s[k] == ' ') ++k;
    if (k == 0 && s.starts_with('\t')) k = 1;
    return s.substr(k);
}

// "<p>text</p>\n..." -> "text..." for tight list items.
std::string unwrap_paragraph(std::string html) {
    if (!html.starts_with("<p>")) return html;
    const size_t e = html.find("</p>\n");
    return html.substr(3, e - 3) + html.substr(e + 5);
}

// Renders the list starting at lines[i]; returns the index just past it.
// Each item's (dedented) lines are rendered recursively, which gives nested
// lists and code blocks inside items for free.
size_t render_list(const Lines& lines, size_t i, const std::string& file, int first_line, std::string& out) {
    const bool ordered = top_marker(lines[i])->ordered;
    struct Item {
        Lines lines;
        int line;
    };
    std::vector<Item> items;
    bool loose = false;
    size_t width = 0;

    size_t j = i;
    for (; j < lines.size(); ++j) {
        const std::string_view raw = lines[j];
        const auto marker = top_marker(raw);

        if (marker && marker->ordered == ordered) {
            width = marker->width;
            items.push_back({{trim(raw).substr(width)}, first_line + static_cast<int>(j)});
        } else if (marker || (!is_indented(raw) && !is_blank(raw))) {
            break;  // a different kind of list, or unindented text
        } else if (is_blank(raw)) {
            size_t k = j + 1;
            while (k < lines.size() && is_blank(lines[k])) ++k;
            if (k == lines.size()) break;
            const auto next = top_marker(lines[k]);
            if (!is_indented(lines[k]) && !(next && next->ordered == ordered)) break;
            loose = true;
            items.back().lines.emplace_back();
        } else {
            items.back().lines.push_back(dedent(raw, width));
        }
    }

    out += ordered ? "<ol>\n" : "<ul>\n";
    for (const Item& item : items) {
        std::string body = render_blocks(item.lines, file, item.line);
        out += "<li>" + (loose ? body : unwrap_paragraph(std::move(body))) + "</li>\n";
    }
    out += ordered ? "</ol>\n" : "</ul>\n";
    return j;
}

std::string render_blocks(const Lines& lines, const std::string& file, int first_line) {
    std::string out;
    std::string para;
    auto flush = [&] {
        if (para.empty()) return;
        out += "<p>" + render_inline(para) + "</p>\n";
        para.clear();
    };

    size_t i = 0;
    while (i < lines.size()) {
        const std::string_view raw = lines[i];
        const std::string_view t = trim(raw);
        const int lineno = first_line + static_cast<int>(i);

        if (t.empty()) {
            flush();
            ++i;
            continue;
        }

        if (t.starts_with("```")) {
            flush();
            const std::string_view lang = trim(t.substr(3));
            std::string code;
            size_t j = i + 1;
            for (; j < lines.size() && !trim(lines[j]).starts_with("```"); ++j) {
                code += lines[j];
                code += '\n';
            }
            if (j == lines.size()) fail(file, lineno, "unclosed code fence");
            out += lang.empty() ? std::string("<pre><code>")
                                : "<pre><code class=\"language-" + html_escape(lang) + "\">";
            append_escaped(out, code);
            out += "</code></pre>\n";
            i = j + 1;
            continue;
        }

        if (const int level = heading_level(t)) {
            flush();
            std::string_view text = trim(t.substr(static_cast<size_t>(level)));
            // Optional closing hashes: "## Title ##"
            if (const size_t k = text.find_last_not_of('#'); k != npos && k + 1 < text.size() && text[k] == ' ')
                text = trim(text.substr(0, k));
            const std::string h = std::to_string(level);
            out += "<h" + h + " id=\"" + slugify(text) + "\">" + render_inline(text) + "</h" + h + ">\n";
            ++i;
            continue;
        }

        if (is_rule(t)) {
            flush();
            out += "<hr>\n";
            ++i;
            continue;
        }

        if (t.starts_with('>')) {
            flush();
            Lines quoted;
            size_t j = i;
            for (; j < lines.size(); ++j) {
                std::string_view q = trim(lines[j]);
                if (!q.starts_with('>')) break;
                q.remove_prefix(1);
                if (q.starts_with(' ')) q.remove_prefix(1);
                quoted.push_back(q);
            }
            out += "<blockquote>\n" + render_blocks(quoted, file, lineno) + "</blockquote>\n";
            i = j;
            continue;
        }

        if (top_marker(raw)) {
            flush();
            i = render_list(lines, i, file, first_line, out);
            continue;
        }

        // Raw HTML, or a {{placeholder}} filled in later, passes through
        // untouched until the next blank line.
        if (para.empty() && (t.starts_with('<') || t.starts_with("{{"))) {
            for (; i < lines.size() && !is_blank(lines[i]); ++i) {
                out += lines[i];
                out += '\n';
            }
            continue;
        }

        if (!para.empty()) para += '\n';
        para += t;
        ++i;
    }
    flush();
    return out;
}

}  // namespace

std::string render_markdown(std::string_view src, const std::string& file, int first_line) {
    return render_blocks(split_lines(src), file, first_line);
}

}  // namespace ssg
