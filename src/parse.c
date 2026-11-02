#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
} md_ctx;

typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
    int    err;
} md_out;

static int md_emit(md_out *o, const char *s) {
    if (!o || !s) return -1;
    
    size_t n = strlen(s);
    if (o->len + n + 1 > o->cap) {
        size_t nc = o->cap ? o->cap : 256;
        while (nc < o->len + n + 1) nc *= 2;
        char *nb = realloc(o->buf, nc);
        if (!nb) { o->err = 1; return -1; }
        o->buf = nb;
        o->cap = nc;
    }
    memcpy(o->buf + o->len, s, n);
    o->len += n;
    o->buf[o->len] = '\0';
    return 0;
}

static void md_putc(md_out *o, char ch) {
    char t[2] = { ch, 0 };
    md_emit(o, t);
}

static void md_printf(md_out *o, const char *fmt, size_t n) {
    char tmp[32];
    snprintf(tmp, sizeof tmp, fmt, n);
    md_emit(o, tmp);
}

static char mdpeek(md_ctx *c) {
    return (c->pos < c->len) ? c->src[c->pos] : '\0';
}

static char mdpeek_at(md_ctx *c, size_t off) {
    size_t p = c->pos + off;
    return (p < c->len) ? c->src[p] : '\0';
}

static char mdnext(md_ctx *c) {
    return (c->pos < c->len) ? c->src[c->pos++] : '\0';
}

static int mdline_start(md_ctx *c) {
    return c->pos == 0 || c->src[c->pos - 1] == '\n';
}

static void html_escape(md_ctx *c, md_out *o) {
    char ch = mdnext(c);
    switch (ch) {
    case '&': md_emit(o, "&amp;"); break;
    case '<': md_emit(o, "&lt;"); break;
    case '>': md_emit(o, "&gt;"); break;
    case '"': md_emit(o, "&quot;"); break;
    default: md_putc(o, ch);
    }
}

static void md_inline(md_ctx *c, md_out *o, int in_link);

static void md_code_span(md_ctx *c, md_out *o) {
    mdnext(c);
    md_emit(o, "<code>");
    while (mdpeek(c) && mdpeek(c) != '`')
        html_escape(c, o);
    if (mdpeek(c)) mdnext(c);
    md_emit(o, "</code>");
}

static void md_emphasis(md_ctx *c, md_out *o) {
    if (mdpeek_at(c, 1) == '*') {
        mdnext(c); mdnext(c);
        md_emit(o, "<strong>");
        while (mdpeek(c) && !(mdpeek(c) == '*' && mdpeek_at(c, 1) == '*'))
            md_inline(c, o, 0);
        if (mdpeek(c)) { mdnext(c); mdnext(c); }
        md_emit(o, "</strong>");
    } else {
        mdnext(c);
        md_emit(o, "<em>");
        while (mdpeek(c) && mdpeek(c) != '*')
            md_inline(c, o, 0);
        if (mdpeek(c)) mdnext(c);
        md_emit(o, "</em>");
    }
}

static void md_link(md_ctx *c, md_out *o) {
    mdnext(c);
    md_emit(o, "<a href=\"");
    while (mdpeek(c) && mdpeek(c) != ')' && mdpeek(c) != ' ')
        html_escape(c, o);
    while (mdpeek(c) && mdpeek(c) != ')') mdnext(c);
    if (mdpeek(c)) mdnext(c);
    md_emit(o, "\">link</a>");
}

static void md_inline(md_ctx *c, md_out *o, int in_link) {
    while (mdpeek(c) && mdpeek(c) != '\n') {
        char ch = mdpeek(c);
        if (ch == '\\' && mdpeek_at(c, 1)) {
            mdnext(c);
            html_escape(c, o);
        } else if (ch == '`') {
            md_code_span(c, o);
        } else if (ch == '*' && !in_link) {
            md_emphasis(c, o);
        } else if (ch == '[' && !in_link) {
            md_link(c, o);
        } else {
            html_escape(c, o);
        }
    }
}

static int md_is_hr(md_ctx *c) {
    char ch = mdpeek(c);
    if (ch != '-' && ch != '*' && ch != '_') return 0;
    size_t i = 0, n = 0;
    while (mdpeek_at(c, i) == ch || mdpeek_at(c, i) == ' ') {
        if (mdpeek_at(c, i) == ch) n++;
        i++;
    }
    return n >= 3 && (mdpeek_at(c, i) == '\n' || mdpeek_at(c, i) == '\0');
}

static void md_hr(md_ctx *c, md_out *o) {
    md_emit(o, "<hr>\n");
    while (mdpeek(c) && mdpeek(c) != '\n') mdnext(c);
    if (mdpeek(c)) mdnext(c);
}

static void md_heading(md_ctx *c, md_out *o) {
    size_t level = 0;
    while (mdpeek_at(c, level) == '#' && level < 6) level++;
    char after = mdpeek_at(c, level);
    if (after != ' ' && after != '\n' && after != '\0' && after != '#') {
        html_escape(c, o);
        return;
    }
    for (size_t i = 0; i < level; i++) mdnext(c);
    if (mdpeek(c) == ' ') mdnext(c);
    md_printf(o, "<h%zu>", level);
    while (mdpeek(c) && mdpeek(c) != '\n') {
        if (mdpeek(c) == '#' && (mdpeek_at(c, 1) == '\n' || mdpeek_at(c, 1) == '\0')) {
            mdnext(c);
            break;
        }
        md_inline(c, o, 0);
    }
    md_printf(o, "</h%zu>\n", level);
    if (mdpeek(c) == '\n') mdnext(c);
}

static void md_blockquote(md_ctx *c, md_out *o) {
    md_emit(o, "<blockquote>\n");
    while (mdpeek(c) == '>') {
        mdnext(c);
        if (mdpeek(c) == ' ') mdnext(c);
        md_inline(c, o, 0);
        if (mdpeek(c) == '\n') mdnext(c);
        while (mdpeek(c) == '>') {
            mdnext(c);
            if (mdpeek(c) == ' ') mdnext(c);
        }
    }
    md_emit(o, "</blockquote>\n");
}

static void md_code_block(md_ctx *c, md_out *o) {
    mdnext(c); mdnext(c); mdnext(c);
    while (mdpeek(c) && mdpeek(c) != '\n') mdnext(c);
    if (mdpeek(c)) mdnext(c);
    md_emit(o, "<pre><code>");
    while (mdpeek(c)) {
        if (mdpeek(c) == '`' && mdpeek_at(c, 1) == '`' && mdpeek_at(c, 2) == '`') {
            mdnext(c); mdnext(c); mdnext(c);
            break;
        }
        html_escape(c, o);
    }
    md_emit(o, "</code></pre>\n");
    while (mdpeek(c) && mdpeek(c) != '\n') mdnext(c);
    if (mdpeek(c)) mdnext(c);
}

static void md_list(md_ctx *c, md_out *o, int ordered) {
    md_emit(o, ordered ? "<ol>\n" : "<ul>\n");
    while (mdpeek(c)) {
        if (ordered) {
            if (mdpeek(c) < '0' || mdpeek(c) > '9') break;
            while (mdpeek(c) >= '0' && mdpeek(c) <= '9') mdnext(c);
            if (mdpeek(c) != '.') break;
            mdnext(c);
        } else {
            if (mdpeek(c) != '-' && mdpeek(c) != '*' && mdpeek(c) != '+') break;
            mdnext(c);
        }
        if (mdpeek(c) == ' ') mdnext(c);
        md_emit(o, "<li>");
        while (mdpeek(c) && mdpeek(c) != '\n')
            md_inline(c, o, 0);
        md_emit(o, "</li>\n");
        if (mdpeek(c) == '\n') mdnext(c);
        if (!(ordered ?
              (mdpeek(c) >= '0' && mdpeek(c) <= '9') :
              (mdpeek(c) == '-' || mdpeek(c) == '*' || mdpeek(c) == '+')))
            break;
    }
    md_emit(o, ordered ? "</ol>\n" : "</ul>\n");
}

static int md_is_ordered(md_ctx *c) {
    if (mdpeek(c) < '0' || mdpeek(c) > '9') return 0;
    size_t i = 0;
    while (mdpeek_at(c, i) >= '0' && mdpeek_at(c, i) <= '9') i++;
    return mdpeek_at(c, i) == '.' && mdpeek_at(c, i + 1) == ' ';
}

static int md_is_unordered(md_ctx *c) {
    char ch = mdpeek(c);
    return (ch == '-' || ch == '*' || ch == '+') && mdpeek_at(c, 1) == ' ';
}

void mdparse(const char *src, md_out *o) {
    md_ctx c = { src, 0, strlen(src) };

    while (mdpeek(&c)) {
        if (mdpeek(&c) == '\n') {
            mdnext(&c);
        } else if (mdline_start(&c) && mdpeek(&c) == '`' &&
                   mdpeek_at(&c, 1) == '`' && mdpeek_at(&c, 2) == '`') {
            md_code_block(&c, o);
        } else if (mdline_start(&c) && mdpeek(&c) == '#') {
            md_heading(&c, o);
        } else if (mdline_start(&c) && md_is_hr(&c)) {
            md_hr(&c, o);
        } else if (mdline_start(&c) && mdpeek(&c) == '>') {
            md_blockquote(&c, o);
        } else if (mdline_start(&c) && md_is_ordered(&c)) {
            md_list(&c, o, 1);
        } else if (mdline_start(&c) && md_is_unordered(&c)) {
            md_list(&c, o, 0);
        } else {
            md_emit(o, "<p>");
            while (mdpeek(&c) && mdpeek(&c) != '\n') {
                if (mdline_start(&c) && (mdpeek(&c) == '#' || mdpeek(&c) == '>' ||
                    md_is_hr(&c) || md_is_ordered(&c) || md_is_unordered(&c) ||
                    (mdpeek(&c) == '`' && mdpeek_at(&c, 1) == '`' && mdpeek_at(&c, 2) == '`')))
                    break;
                md_inline(&c, o, 0);
            }
            md_emit(o, "</p>\n");
            if (mdpeek(&c) == '\n') mdnext(&c);
        }
    }
}