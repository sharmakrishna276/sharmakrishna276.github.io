#include "template.hpp"

#include <algorithm>

#include "util.hpp"

namespace ssg {
namespace {

int line_of(std::string_view s, size_t pos) {
    return 1 + static_cast<int>(std::count(s.begin(), s.begin() + static_cast<std::ptrdiff_t>(pos), '\n'));
}

}  // namespace

std::string render_template(std::string_view tpl, const Vars& vars, const std::string& name) {
    std::string out;
    out.reserve(tpl.size());
    size_t i = 0;
    while (true) {
        const size_t open = tpl.find("{{", i);
        if (open == std::string_view::npos) {
            out += tpl.substr(i);
            return out;
        }
        out += tpl.substr(i, open - i);

        const size_t close = tpl.find("}}", open + 2);
        if (close == std::string_view::npos) fail(name, line_of(tpl, open), "unclosed `{{`");

        const std::string_view key = trim(tpl.substr(open + 2, close - open - 2));
        const auto it = vars.find(key);
        if (it == vars.end())
            fail(name, line_of(tpl, open), "unknown variable `" + std::string(key) + "`");
        out += it->second;
        i = close + 2;
    }
}

}  // namespace ssg
