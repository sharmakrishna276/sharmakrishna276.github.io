# About this Project

My personal site, built with a small dependency-free static site generator
written in C++20. Markdown goes in, plain HTML comes out.
There is no JavaScript and the pages are a few KB each.

## Usage

```sh
make          # build the generator (CMake) and render the site into _site/
make serve    # ... then serve it at http://localhost:8000
make asan     # build and run under AddressSanitizer + UBSan
make clean
```

Requires a C++20 compiler (clang 16+ / gcc 13+), CMake 3.20+, and Python 3
for `make serve`.

## Layout

```
site.conf            name, url, links; available in templates as {{site.<key>}}
content/*.md         top-level pages  -> _site/<name>.html
content/posts/*.md   blog posts       -> _site/posts/<name>.html (+ rss.xml)
templates/base.html  wraps every page
templates/post.html  wraps each post
static/              copied verbatim (css, favicon, resume.pdf)
src/                 the generator
```

## Writing a post

Create `content/posts/< >.md`:

```md
---
title: 
date: 
description:
---

Body in Markdown.
```

Posts are listed newest first wherever a page contains a `{{posts}}` line.

### Supported Markdown

- `#`–`######` headings (styled as `(* h1 *)`, `(** h2 **)`, `(*** h3 ***)`)
- paragraphs, `---` rules, `>` blockquotes
- `-` / `*` / `+` and `1.` lists, nested by indenting
- fenced code blocks with a language: ```` ```cpp ````
- `` `code` ``, `**strong**`, `*em*`, `[links](url)`, `![images](src)`, `\` escapes
- a line starting with `<` begins a raw HTML block that runs to the next blank line

Not supported: `_underscore_` emphasis, tables, indented code blocks,
reference-style links.

Errors are reported as `file:line: message` (for example an unclosed code
fence, bad frontmatter, an impossible date, or an unknown template variable).
