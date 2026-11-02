#pragma once

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace ssg {

using Vars = std::map<std::string, std::string, std::less<>>;

std::string render_template(std::string_view tpl, const Vars& vars, const std::string& name);

}  // namespace ssg
