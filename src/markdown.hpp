#pragma once

#include <string>
#include <string_view>

namespace ssg {

std::string render_markdown(std::string_view src, const std::string& file, int first_line);

}  // namespace ssg
