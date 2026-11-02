#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "frontmatter.hpp"
#include "markdown.hpp"
#include "template.hpp"
#include "util.hpp"

namespace fs = std::filesystem;
namespace chr = std::chrono;
using namespace ssg;

namespace {

struct Post {
    std::string slug, title, date, description, html;
    chr::year_month_day ymd;
};

const std::string& require(const Meta& meta, const std::string& key, const std::string& file) {
    const auto it = meta.find(key);
    if (it == meta.end() || it->second.empty()) fail(file, 1, "missing required key `" + key + "`");
    return it->second;
}

std::string get_or(const Meta& meta, const std::string& key, const std::string& fallback) {
    const auto it = meta.find(key);
    return it == meta.end() || it->second.empty() ? fallback : it->second;
}

// Parses YYYY/MM/DD, rejecting impossible dates like 2026/02/30.
chr::year_month_day parse_date(const std::string& s, const std::string& file) {
    auto field = [&](size_t pos, size_t len) {
        int v = -1;
        const char* first = s.data() + pos;
        const auto [ptr, ec] = std::from_chars(first, first + len, v);
        return ec == std::errc{} && ptr == first + len ? v : -1;
    };
    if (s.size() == 10 && s[4] == '/' && s[7] == '/') {
        const int y = field(0, 4), m = field(5, 2), d = field(8, 2);
        if (y >= 0 && m >= 0 && d >= 0) {
            const chr::year_month_day ymd{chr::year{y}, chr::month{static_cast<unsigned>(m)},
                                          chr::day{static_cast<unsigned>(d)}};
            if (ymd.ok()) return ymd;
        }
    }
    fail(file, 1, "bad date `" + s + "` (expected YYYY/MM/DD)");
}

// RFC 822 date for RSS, e.g. "Mon, 02 Nov 2026 00:00:00 +0000".
std::string rfc822(chr::year_month_day ymd) {
    static constexpr const char* kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static constexpr const char* kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const chr::weekday wd{chr::sys_days{ymd}};
    char buf[40];
    std::snprintf(buf, sizeof buf, "%s, %02u %s %d 00:00:00 +0000", kDays[wd.c_encoding()],
                  static_cast<unsigned>(ymd.day()), kMonths[static_cast<unsigned>(ymd.month()) - 1],
                  static_cast<int>(ymd.year()));
    return buf;
}

std::string iso_date(std::string date) {
    std::replace(date.begin(), date.end(), '/', '-');
    return date;
}

void replace_all(std::string& s, std::string_view from, std::string_view to) {
    for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
        s.replace(pos, from.size(), to);
}

std::vector<fs::path> markdown_files(const fs::path& dir) {
    std::vector<fs::path> files;
    if (!fs::is_directory(dir)) return files;
    for (const auto& entry : fs::directory_iterator(dir))
        if (entry.is_regular_file() && entry.path().extension() == ".md") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    return files;
}

int run(const fs::path& root, const fs::path& out) {
    const fs::path conf_path = root / "site.conf";
    const std::string conf_file = conf_path.string();
    const Meta site = parse_kv(read_file(conf_path), conf_file, 1);
    const std::string& site_name = require(site, "name", conf_file);
    const std::string& site_desc = require(site, "description", conf_file);
    std::string site_url = require(site, "url", conf_file);
    while (site_url.ends_with('/')) site_url.pop_back();

    Vars site_vars;
    for (const auto& [key, value] : site) site_vars["site." + key] = html_escape(value);

    const std::string base_tpl = read_file(root / "templates/base.html");
    const std::string post_tpl = read_file(root / "templates/post.html");

    // `rel_root` is the relative path back to the site root ("" or "../"), so
    // the output works from any subdirectory and when opened from disk.
    auto layout = [&](const std::string& title, const std::string& description,
                      const std::string& rel_root, std::string content) {
        Vars vars = site_vars;
        vars["title"] = html_escape(title);
        vars["description"] = html_escape(description);
        vars["root"] = rel_root;
        vars["content"] = std::move(content);
        return render_template(base_tpl, vars, "templates/base.html");
    };

    fs::create_directories(out);

    // Posts, newest first.
    std::vector<Post> posts;
    for (const fs::path& path : markdown_files(root / "content/posts")) {
        const std::string file = path.string();
        const Document doc = parse_document(read_file(path), file);
        Post p;
        p.slug = path.stem().string();
        p.title = require(doc.meta, "title", file);
        p.date = require(doc.meta, "date", file);
        p.ymd = parse_date(p.date, file);
        p.description = get_or(doc.meta, "description", site_desc);
        p.html = render_markdown(doc.body, file, doc.body_line);
        posts.push_back(std::move(p));
    }
    std::sort(posts.begin(), posts.end(), [](const Post& a, const Post& b) {
        return a.date != b.date ? a.date > b.date : a.slug < b.slug;
    });

    for (const Post& p : posts) {
        Vars vars = site_vars;
        vars["title"] = html_escape(p.title);
        vars["date"] = p.date;
        vars["datetime"] = iso_date(p.date);
        vars["root"] = "../";
        vars["content"] = p.html;
        std::string article = render_template(post_tpl, vars, "templates/post.html");
        write_file(out / "posts" / (p.slug + ".html"), layout(p.title, p.description, "../", std::move(article)));
    }

    // The post index, substituted for `{{posts}}` in top-level pages.
    std::string post_list;
    if (posts.empty()) {
        post_list = "<p>Nothing here yet.</p>\n";
    } else {
        post_list = "<ul class=\"posts\">\n";
        for (const Post& p : posts) {
            post_list += "<li><time datetime=\"" + iso_date(p.date) + "\">" + p.date + "</time> <a href=\"posts/" +
                         html_escape(p.slug) + ".html\">" + html_escape(p.title) + "</a></li>\n";
        }
        post_list += "</ul>\n";
    }

    // Top-level pages.
    size_t pages = 0;
    for (const fs::path& path : markdown_files(root / "content")) {
        const std::string file = path.string();
        const Document doc = parse_document(read_file(path), file);
        const std::string& title = require(doc.meta, "title", file);
        std::string html = render_markdown(doc.body, file, doc.body_line);
        replace_all(html, "{{posts}}", post_list);
        write_file(out / (path.stem().string() + ".html"),
                   layout(title, get_or(doc.meta, "description", site_desc), "", std::move(html)));
        ++pages;
    }

    // RSS 2.0 feed.
    std::string rss = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<rss version=\"2.0\">\n<channel>\n";
    rss += "<title>" + html_escape(site_name) + "</title>\n";
    rss += "<link>" + html_escape(site_url) + "/</link>\n";
    rss += "<description>" + html_escape(site_desc) + "</description>\n";
    for (const Post& p : posts) {
        const std::string link = html_escape(site_url + "/posts/" + p.slug + ".html");
        rss += "<item>\n<title>" + html_escape(p.title) + "</title>\n<link>" + link + "</link>\n<guid>" + link +
               "</guid>\n<pubDate>" + rfc822(p.ymd) + "</pubDate>\n<description>" + html_escape(p.html) +
               "</description>\n</item>\n";
    }
    rss += "</channel>\n</rss>\n";
    write_file(out / "rss.xml", rss);

    if (fs::is_directory(root / "static"))
        fs::copy(root / "static", out, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    std::printf("ssg: %zu pages, %zu posts -> %s\n", pages, posts.size(), out.string().c_str());
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <site-root> <out-dir>\n", argv[0]);
        return 2;
    }
    try {
        return run(argv[1], argv[2]);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "ssg: error: %s\n", e.what());
        return 1;
    }
}
