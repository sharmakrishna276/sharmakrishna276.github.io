#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace ssg {

using Vars = std::map<std::string, std::string, std::less<>>;

// Replaces each `{{name}}` in `tpl` with vars[name]. Values are inserted
// verbatim (callers escape them) and are never re-scanned, so content can't
// inject placeholders. Unknown names are errors, which catches typos.
std::string render_template(std::string_view tpl, const Vars& vars, const std::string& name);

}  // namespace ssg
