#include "ui.h"
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <sys/stat.h>
#include "crypto.h"

// ncurses uses getmouse(), PDCurses uses nc_getmouse()
#ifndef _WIN32
    #define nc_getmouse getmouse
#endif

// ── Helper functions ───────────────────────────────────────────────────────

static std::string format_dt(time_t t) {
    if (t == 0) return "(not set)";
    char buf[32];
    struct tm* tm = localtime(&t);
    if (!tm) return "(invalid)";
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
    return buf;
}

static FormField make_text(const char* label, const char* hint, int max_len,
                           const std::string& value = "") {
    FormField f;
    f.label = label; f.hint = hint; f.max_len = max_len;
    f.type = FT_TEXT; f.value = value;
    return f;
}

static FormField make_enum_field(const char* label, const char* hint,
                                 const char** labels, int count, int current = 0) {
    FormField f;
    f.label = label; f.hint = hint; f.max_len = 0;
    f.type = FT_ENUM; f.enum_labels = labels; f.enum_count = count;
    f.enum_value = current; f.value = labels[current];
    return f;
}

static FormField make_fk(FieldType ft, const char* label, const char* hint,
                          uint32_t id = 0, const std::string& display = "") {
    FormField f;
    f.label = label; f.hint = hint; f.max_len = 0;
    f.type = ft; f.id_value = id;
    f.value = display.empty() ? "(press ENTER to select)" : display;
    return f;
}

// ── File manager helpers ──────────────────────────────────────────────────

struct FileEntry {
    std::string name;
    bool is_directory;
    uint64_t size;
    std::filesystem::file_time_type last_write;
    std::string last_write_str;
    std::string size_str;
};

static std::string format_file_time(const std::filesystem::file_time_type& ft) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ft - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    time_t tt = std::chrono::system_clock::to_time_t(sctp);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&tt));
    return buf;
}

static std::string format_file_size(uint64_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024*1024) return std::to_string(bytes/1024) + " KB";
    if (bytes < 1024ULL*1024*1024) return std::to_string(bytes/(1024*1024)) + " MB";
    return std::to_string(bytes/(1024ULL*1024*1024)) + " GB";
}

// ── Drawing primitives ─────────────────────────────────────────────────────

static void draw_box(int y, int x, int h, int w, int cp) {
    attron(COLOR_PAIR(cp) | A_BOLD);
    mvaddch(y, x, ACS_ULCORNER);
    for (int i = 1; i < w - 1; i++) mvaddch(y, x + i, ACS_HLINE);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    for (int i = 1; i < h - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        for (int j = 1; j < w - 1; j++) mvaddch(y + i, x + j, ' ');
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    for (int i = 1; i < w - 1; i++) mvaddch(y + h - 1, x + i, ACS_HLINE);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(cp) | A_BOLD);
}

static void draw_hline(int y, int x, int w, int cp) {
    attron(COLOR_PAIR(cp) | A_BOLD);
    mvaddch(y, x, ACS_LTEE);
    for (int i = 1; i < w - 1; i++) mvaddch(y, x + i, ACS_HLINE);
    mvaddch(y, x + w - 1, ACS_RTEE);
    attroff(COLOR_PAIR(cp) | A_BOLD);
}

static void draw_title(int y, int x, int w, const std::string& title, int cp) {
    attron(COLOR_PAIR(cp) | A_BOLD);
    int tx = x + (w - (int)title.size() - 2) / 2;
    if (tx < x + 1) tx = x + 1;
    mvprintw(y, tx, " %s ", title.c_str());
    attroff(COLOR_PAIR(cp) | A_BOLD);
}

// ── UI core ────────────────────────────────────────────────────────────────

UI::UI(DataStore& ds) : ds_(ds), theme_(0) {
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    curs_set(0);
    getmaxyx(stdscr, max_y_, max_x_);
    apply_theme();
}

UI::~UI() { endwin(); }

void UI::apply_theme() {
    if (theme_ == 0) {
        // Blue corporate theme
        init_pair(CP_NORMAL,    COLOR_WHITE,  COLOR_BLUE);
        init_pair(CP_TITLE,     COLOR_YELLOW, COLOR_BLUE);
        init_pair(CP_HIGHLIGHT, COLOR_BLACK,  COLOR_CYAN);
        init_pair(CP_INPUT,     COLOR_WHITE,  COLOR_BLACK);
        init_pair(CP_STATUS,    COLOR_BLACK,  COLOR_WHITE);
        init_pair(CP_ERROR,     COLOR_RED,    COLOR_BLUE);
        init_pair(CP_HINT,      COLOR_CYAN,   COLOR_BLUE);
    } else {
        // Retro green CRT terminal
        init_pair(CP_NORMAL,    COLOR_GREEN,  COLOR_BLACK);
        init_pair(CP_TITLE,     COLOR_GREEN,  COLOR_BLACK);
        init_pair(CP_HIGHLIGHT, COLOR_BLACK,  COLOR_GREEN);
        init_pair(CP_INPUT,     COLOR_GREEN,  COLOR_BLACK);
        init_pair(CP_STATUS,    COLOR_BLACK,  COLOR_GREEN);
        init_pair(CP_ERROR,     COLOR_RED,    COLOR_BLACK);
        init_pair(CP_HINT,      COLOR_CYAN,   COLOR_BLACK);
    }
    bkgd(COLOR_PAIR(CP_NORMAL));
    erase();
    refresh();
}

void UI::fill_background() {
    bkgd(COLOR_PAIR(CP_NORMAL));
    erase();
    attron(COLOR_PAIR(CP_STATUS));
    mvhline(0, 0, ' ', max_x_);
    mvprintw(0, 2, " EBOXLAB - Digital Forensics & eDiscovery ");
    attroff(COLOR_PAIR(CP_STATUS));
    attron(COLOR_PAIR(CP_STATUS));
    mvhline(max_y_ - 1, 0, ' ', max_x_);
    mvprintw(max_y_ - 1, 2, "Law Stars - Trial Lawyers and Legal Services Colorado LLC");
    attroff(COLOR_PAIR(CP_STATUS));
}

void UI::handle_resize() {
#ifdef _WIN32
    resize_term(0, 0);  // PDCurses: auto-detect new console size
#endif
    getmaxyx(stdscr, max_y_, max_x_);
    erase();
    refresh();
}

void UI::show_status(const std::string& msg) {
    attron(COLOR_PAIR(CP_STATUS));
    mvhline(max_y_ - 1, 0, ' ', max_x_);
    mvprintw(max_y_ - 1, 2, "%s", msg.c_str());
    attroff(COLOR_PAIR(CP_STATUS));
    refresh();
}

void UI::show_error(const std::string& msg) {
    attron(COLOR_PAIR(CP_ERROR) | A_BOLD);
    mvhline(max_y_ - 2, 0, ' ', max_x_);
    mvprintw(max_y_ - 2, 2, " ERROR: %s", msg.c_str());
    attroff(COLOR_PAIR(CP_ERROR) | A_BOLD);
    refresh();
}

void UI::wait_key() {
    show_status("Press any key to continue...");
    int ch;
    do { ch = getch(); } while (ch == KEY_RESIZE && (handle_resize(), true));
}

// ── Input field ────────────────────────────────────────────────────────────

std::string UI::input_field(int y, int x, int width, const std::string& existing, int max_chars) {
    char buf[512];
    memset(buf, 0, sizeof(buf));
    if (!existing.empty()) strncpy(buf, existing.c_str(), sizeof(buf) - 1);
    int pos = (int)strlen(buf);
    int max_len = (max_chars > 0) ? max_chars : (width - 1);
    if (max_len > 510) max_len = 510;
    int scroll = 0;
    if (pos >= width) scroll = pos - width + 2;
    curs_set(1);

    while (true) {
        // Horizontal scroll: show buf[scroll .. scroll+width-1]
        attron(COLOR_PAIR(CP_INPUT));
        mvhline(y, x, ' ', width);
        int show_len = (int)strlen(buf + scroll);
        if (show_len > width) show_len = width;
        mvprintw(y, x, "%.*s", show_len, buf + scroll);
        // Show scroll indicator if text extends beyond view
        if (scroll > 0) mvaddch(y, x, '<');
        if ((int)strlen(buf) > scroll + width) mvaddch(y, x + width - 1, '>');
        move(y, x + (pos - scroll));
        attroff(COLOR_PAIR(CP_INPUT));
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) break;
        if (ch == 27) { curs_set(0); return existing; }
        if (ch == KEY_LEFT) {
            if (pos > 0) pos--;
        } else if (ch == KEY_RIGHT) {
            if (pos < (int)strlen(buf)) pos++;
        } else if (ch == KEY_HOME) {
            pos = 0;
        } else if (ch == KEY_END) {
            pos = (int)strlen(buf);
        } else if (ch == KEY_BACKSPACE || ch == 8 || ch == 127) {
            if (pos > 0) {
                memmove(buf + pos - 1, buf + pos, strlen(buf + pos) + 1);
                pos--;
            }
        } else if (ch == KEY_DC) {
            if (pos < (int)strlen(buf)) {
                memmove(buf + pos, buf + pos + 1, strlen(buf + pos));
            }
        } else if (ch >= 32 && ch < 127 && (int)strlen(buf) < max_len) {
            memmove(buf + pos + 1, buf + pos, strlen(buf + pos) + 1);
            buf[pos++] = (char)ch;
        }
        // Adjust scroll to keep cursor visible
        if (pos < scroll) scroll = pos;
        if (pos >= scroll + width - 1) scroll = pos - width + 2;
        if (scroll < 0) scroll = 0;
    }
    curs_set(0);
    return std::string(buf);
}

// ── Multi-line text area editor ─────────────────────────────────────────────

std::string UI::edit_text_area(const std::string& title, const std::string& existing, int max_chars) {
    char buf[512];
    memset(buf, 0, sizeof(buf));
    if (!existing.empty()) strncpy(buf, existing.c_str(), sizeof(buf) - 1);
    int pos = (int)strlen(buf);
    int max_len = max_chars;
    if (max_len > 510) max_len = 510;
    int vscroll = 0;
    curs_set(1);

    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int box_w = std::max(60, max_x_ * 80 / 100);
        if (box_w > max_x_ - 4) box_w = max_x_ - 4;
        int text_w = box_w - 6;
        if (text_w < 10) text_w = 10;
        int box_h = std::max(12, max_y_ * 50 / 100);
        if (box_h > max_y_ - 4) box_h = max_y_ - 4;
        int text_h = box_h - 6;
        if (text_h < 3) text_h = 3;
        int sy = (max_y_ - box_h) / 2;
        int sx = (max_x_ - box_w) / 2;
        int text_y = sy + 2;
        int text_x = sx + 3;

        draw_box(sy, sx, box_h, box_w, CP_TITLE);
        draw_title(sy, sx, box_w, title, CP_TITLE);

        int len = (int)strlen(buf);

        // Cursor line/col in wrapped display (simple character wrapping)
        int cursor_line = (text_w > 0) ? pos / text_w : 0;
        int cursor_col  = (text_w > 0) ? pos % text_w : 0;
        int total_lines = (text_w > 0) ? (len / text_w) + 1 : 1;

        // Adjust scroll to keep cursor visible
        if (cursor_line < vscroll) vscroll = cursor_line;
        if (cursor_line >= vscroll + text_h) vscroll = cursor_line - text_h + 1;

        // Draw text area background
        attron(COLOR_PAIR(CP_INPUT));
        for (int r = 0; r < text_h; r++)
            mvhline(text_y + r, text_x, ' ', text_w);

        // Draw wrapped text
        for (int i = 0; i < len; i++) {
            int line = i / text_w;
            int col  = i % text_w;
            if (line >= vscroll && line < vscroll + text_h) {
                mvaddch(text_y + (line - vscroll), text_x + col, buf[i]);
            }
        }

        // Position cursor
        move(text_y + (cursor_line - vscroll), text_x + cursor_col);
        attroff(COLOR_PAIR(CP_INPUT));

        // Scroll indicator
        if (total_lines > text_h) {
            attron(COLOR_PAIR(CP_HINT));
            if (vscroll > 0)
                mvprintw(text_y, sx + box_w - 4, " ^ ");
            if (vscroll + text_h < total_lines)
                mvprintw(text_y + text_h - 1, sx + box_w - 4, " v ");
            attroff(COLOR_PAIR(CP_HINT));
        }

        // Status bar
        draw_hline(sy + box_h - 3, sx, box_w, CP_TITLE);
        attron(COLOR_PAIR(CP_HINT));
        mvprintw(sy + box_h - 2, sx + 3, "Chars: %d/%d | Line: %d/%d",
                 len, max_len, cursor_line + 1, total_lines);
        attroff(COLOR_PAIR(CP_HINT));
        attron(COLOR_PAIR(CP_TITLE));
        mvprintw(sy + box_h - 2, sx + box_w - 30, "F2: Save | ESC: Cancel");
        attroff(COLOR_PAIR(CP_TITLE));

        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_F(2) || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            curs_set(0);
            return std::string(buf);
        }
        if (ch == 27) { curs_set(0); return existing; }

        if (ch == KEY_LEFT) {
            if (pos > 0) pos--;
        } else if (ch == KEY_RIGHT) {
            if (pos < len) pos++;
        } else if (ch == KEY_UP) {
            if (cursor_line > 0) pos -= text_w;
            if (pos < 0) pos = 0;
        } else if (ch == KEY_DOWN) {
            pos += text_w;
            if (pos > len) pos = len;
        } else if (ch == KEY_HOME) {
            pos = cursor_line * text_w;
        } else if (ch == KEY_END) {
            int line_end = (cursor_line + 1) * text_w;
            pos = (line_end > len) ? len : line_end;
        } else if (ch == KEY_BACKSPACE || ch == 8 || ch == 127) {
            if (pos > 0) {
                memmove(buf + pos - 1, buf + pos, strlen(buf + pos) + 1);
                pos--;
            }
        } else if (ch == KEY_DC) {
            if (pos < len) {
                memmove(buf + pos, buf + pos + 1, strlen(buf + pos));
            }
        } else if (ch >= 32 && ch < 127 && len < max_len) {
            memmove(buf + pos + 1, buf + pos, strlen(buf + pos) + 1);
            buf[pos++] = (char)ch;
        }
    }
}

// ── Show sub-menu ──────────────────────────────────────────────────────────

int UI::show_submenu(const std::string& title, const std::vector<std::string>& options) {
    int selected = 0;
    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int menu_w = std::max(50, max_x_ * 55 / 100);
        for (const auto& o : options)
            if ((int)o.size() + 14 > menu_w) menu_w = (int)o.size() + 14;
        if (menu_w > max_x_ - 4) menu_w = max_x_ - 4;
        int menu_h = (int)options.size() + 6;
        int sy = (max_y_ - menu_h) / 2;
        int sx = (max_x_ - menu_w) / 2;

        draw_box(sy, sx, menu_h, menu_w, CP_TITLE);
        draw_title(sy, sx, menu_w, title, CP_TITLE);
        draw_hline(sy + 2, sx, menu_w, CP_TITLE);

        for (int i = 0; i < (int)options.size(); i++) {
            if (i == selected) attron(COLOR_PAIR(CP_HIGHLIGHT));
            else attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
            mvhline(sy + 3 + i, sx + 1, ' ', menu_w - 2);
            mvprintw(sy + 3 + i, sx + 4, "%d. %s", i + 1, options[i].c_str());
            if (i == selected) attroff(COLOR_PAIR(CP_HIGHLIGHT));
            else attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
        }

        // Small forensic icon in top-right corner
        if (menu_w >= 40) {
            attron(COLOR_PAIR(CP_HINT));
            mvprintw(sy + 1, sx + menu_w - 7, " {*} ");
            attroff(COLOR_PAIR(CP_HINT));
        }

        show_status("Click/Tab/Arrows: Navigate | ENTER: Select | ESC: Back");
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON1_CLICKED) {
                    int clicked = mev.y - (sy + 3);
                    if (clicked >= 0 && clicked < (int)options.size()
                        && mev.x > sx && mev.x < sx + menu_w - 1)
                        return clicked;
                }
                if (mev.bstate & BUTTON4_PRESSED && selected > 0) selected--;
                if (mev.bstate & BUTTON5_PRESSED && selected < (int)options.size() - 1) selected++;
            }
            continue;
        }
        if (ch == KEY_UP && selected > 0) selected--;
        if (ch == KEY_DOWN && selected < (int)options.size() - 1) selected++;
        if (ch == 9) { selected++; if (selected >= (int)options.size()) selected = 0; }
        if (ch == KEY_BTAB) { selected--; if (selected < 0) selected = (int)options.size() - 1; }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return selected;
        if (ch == 27) return -1;
        if (ch >= '1' && ch <= '9') {
            int idx = ch - '1';
            if (idx < (int)options.size()) return idx;
        }
    }
}

// ── Select from list (generic scrollable table) ────────────────────────────

int UI::select_from_list(const std::string& title, const std::vector<ColDef>& cols,
                         const std::vector<std::vector<std::string>>& rows) {
    if (rows.empty()) {
        show_error("No records found.");
        wait_key();
        return -1;
    }

    int selected = 0, offset = 0;
    int data_start = 5;

    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int visible = max_y_ - data_start - 2;
        if (visible < 1) visible = 1;

        // Compute column widths
        int avail = max_x_ - 4;
        std::vector<int> cw(cols.size());
        for (int i = 0; i < (int)cols.size(); i++)
            cw[i] = std::max(6, avail * cols[i].width_pct / 100);

        // Title bar
        attron(COLOR_PAIR(CP_STATUS));
        mvhline(0, 0, ' ', max_x_);
        mvprintw(0, 2, " EBOXLAB ");
        int tlen = (int)title.size();
        mvprintw(0, (max_x_ - tlen) / 2, "%s", title.c_str());
        attroff(COLOR_PAIR(CP_STATUS));

        // Column headers
        attron(COLOR_PAIR(CP_STATUS));
        mvhline(3, 0, ' ', max_x_);
        int cx = 2;
        for (int i = 0; i < (int)cols.size(); i++) {
            mvprintw(3, cx, "%-*.*s", cw[i], cw[i], cols[i].header);
            cx += cw[i] + 1;
        }
        attroff(COLOR_PAIR(CP_STATUS));

        attron(COLOR_PAIR(CP_TITLE));
        mvhline(4, 0, ACS_HLINE, max_x_);
        attroff(COLOR_PAIR(CP_TITLE));

        // Rows
        for (int i = 0; i < visible && (i + offset) < (int)rows.size(); i++) {
            int idx = i + offset;
            int row = data_start + i;

            if (idx == selected) attron(COLOR_PAIR(CP_HIGHLIGHT));
            else attron(COLOR_PAIR(CP_NORMAL));

            mvhline(row, 0, ' ', max_x_);
            cx = 2;
            for (int c = 0; c < (int)cols.size() && c < (int)rows[idx].size(); c++) {
                mvprintw(row, cx, "%-*.*s", cw[c], cw[c], rows[idx][c].c_str());
                cx += cw[c] + 1;
            }

            if (idx == selected) attroff(COLOR_PAIR(CP_HIGHLIGHT));
            else attroff(COLOR_PAIR(CP_NORMAL));
        }

        attron(COLOR_PAIR(CP_TITLE));
        mvprintw(max_y_ - 2, 2, "Showing %d-%d of %d",
                 offset + 1, std::min(offset + visible, (int)rows.size()), (int)rows.size());
        attroff(COLOR_PAIR(CP_TITLE));

        show_status("Click/Tab/Arrows: Navigate | ENTER: Select | ESC: Back");
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON1_CLICKED) {
                    int row_idx = mev.y - data_start;
                    if (row_idx >= 0 && row_idx < visible) {
                        int actual = row_idx + offset;
                        if (actual < (int)rows.size()) selected = actual;
                    }
                }
                if (mev.bstate & BUTTON1_DOUBLE_CLICKED) {
                    int row_idx = mev.y - data_start;
                    if (row_idx >= 0 && row_idx < visible) {
                        int actual = row_idx + offset;
                        if (actual < (int)rows.size()) return actual;
                    }
                }
                if (mev.bstate & BUTTON4_PRESSED && selected > 0) {
                    selected--;
                    if (selected < offset) offset = selected;
                }
                if (mev.bstate & BUTTON5_PRESSED && selected < (int)rows.size() - 1) {
                    selected++;
                    if (selected >= offset + visible) offset = selected - visible + 1;
                }
            }
            continue;
        }
        if (ch == KEY_UP && selected > 0) {
            selected--;
            if (selected < offset) offset = selected;
        } else if (ch == KEY_DOWN && selected < (int)rows.size() - 1) {
            selected++;
            if (selected >= offset + visible) offset = selected - visible + 1;
        } else if (ch == 9 && selected < (int)rows.size() - 1) {
            selected++;
            if (selected >= offset + visible) offset = selected - visible + 1;
        } else if (ch == KEY_BTAB && selected > 0) {
            selected--;
            if (selected < offset) offset = selected;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            return selected;
        } else if (ch == 27) {
            return -1;
        }
    }
}

// ── Select enum ────────────────────────────────────────────────────────────

int UI::select_enum(const char** labels, int count, const std::string& title, int current) {
    int selected = current;
    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int menu_w = std::max(40, max_x_ * 45 / 100);
        for (int i = 0; i < count; i++)
            if ((int)strlen(labels[i]) + 12 > menu_w) menu_w = (int)strlen(labels[i]) + 12;
        if (menu_w > max_x_ - 4) menu_w = max_x_ - 4;
        int menu_h = count + 6;
        int sy = (max_y_ - menu_h) / 2;
        int sx = (max_x_ - menu_w) / 2;

        draw_box(sy, sx, menu_h, menu_w, CP_TITLE);
        draw_title(sy, sx, menu_w, title, CP_TITLE);
        draw_hline(sy + 2, sx, menu_w, CP_TITLE);

        for (int i = 0; i < count; i++) {
            if (i == selected) attron(COLOR_PAIR(CP_HIGHLIGHT));
            else attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
            mvhline(sy + 3 + i, sx + 1, ' ', menu_w - 2);
            mvprintw(sy + 3 + i, sx + 4, "%d. %s", i + 1, labels[i]);
            if (i == selected) attroff(COLOR_PAIR(CP_HIGHLIGHT));
            else attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
        }

        show_status("Click/Tab/Arrows + ENTER to select | ESC to cancel");
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON1_CLICKED) {
                    int clicked = mev.y - (sy + 3);
                    if (clicked >= 0 && clicked < count
                        && mev.x > sx && mev.x < sx + menu_w - 1)
                        return clicked;
                }
                if (mev.bstate & BUTTON4_PRESSED && selected > 0) selected--;
                if (mev.bstate & BUTTON5_PRESSED && selected < count - 1) selected++;
            }
            continue;
        }
        if (ch == KEY_UP && selected > 0) selected--;
        if (ch == KEY_DOWN && selected < count - 1) selected++;
        if (ch == 9) { selected++; if (selected >= count) selected = 0; }
        if (ch == KEY_BTAB) { selected--; if (selected < 0) selected = count - 1; }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return selected;
        if (ch == 27) return current;
    }
}

// ── Run form (guided, scrollable) ──────────────────────────────────────────

bool UI::run_form(const std::string& title, std::vector<FormField>& fields) {
    int selected = 0, scroll = 0;

    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int form_w = std::max(78, max_x_ * 80 / 100);
        if (form_w > max_x_ - 4) form_w = max_x_ - 4;

        // Find longest label
        int max_label = 0;
        for (const auto& f : fields)
            if ((int)strlen(f.label) > max_label) max_label = (int)strlen(f.label);
        max_label += 1; // for ':'

        int sx = (max_x_ - form_w) / 2;
        int field_x = sx + max_label + 4;
        int field_w = form_w - max_label - 7;
        if (field_w < 10) field_w = 10;

        // Visible fields (leave room for border, title, hint area, help)
        int visible = std::min((int)fields.size(), max_y_ - 10);
        if (visible < 1) visible = 1;
        int form_h = visible + 6;
        int sy = (max_y_ - form_h) / 2;

        draw_box(sy, sx, form_h, form_w, CP_TITLE);
        draw_title(sy, sx, form_w, title, CP_TITLE);

        // Draw fields
        for (int i = 0; i < visible && (i + scroll) < (int)fields.size(); i++) {
            int idx = i + scroll;
            int row = sy + 2 + i;

            // Label
            attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
            mvprintw(row, sx + 2, "%*s:", max_label, fields[idx].label);
            attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

            // Value
            if (idx == selected) attron(COLOR_PAIR(CP_HIGHLIGHT));
            else attron(COLOR_PAIR(CP_NORMAL));

            std::string disp = fields[idx].value;
            if (disp.empty() && fields[idx].type != FT_READONLY)
                disp = (fields[idx].type == FT_TEXT) ? "" : "(press ENTER to select)";
            mvprintw(row, field_x, " %-*.*s", field_w - 2, field_w - 2, disp.c_str());

            if (idx == selected) {
                attroff(COLOR_PAIR(CP_HIGHLIGHT));
                attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
                mvprintw(row, sx + form_w - 3, "<");
                attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
            } else {
                attroff(COLOR_PAIR(CP_NORMAL));
            }
        }

        // Hint area
        int hint_row = sy + form_h - 3;
        draw_hline(hint_row - 1, sx, form_w, CP_TITLE);
        attron(COLOR_PAIR(CP_HINT));
        mvprintw(hint_row, sx + 3, "> %.*s", form_w - 7, fields[selected].hint);
        attroff(COLOR_PAIR(CP_HINT));

        attron(COLOR_PAIR(CP_TITLE));
        mvprintw(hint_row + 1, sx + 3, "ENTER: Edit | Tab/Arrows: Navigate | F2: Save | ESC: Cancel");
        attroff(COLOR_PAIR(CP_TITLE));

        show_status("Editing: " + title);
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON1_CLICKED) {
                    int clicked = mev.y - (sy + 2);
                    if (clicked >= 0 && clicked < visible) {
                        int actual = clicked + scroll;
                        if (actual < (int)fields.size()) selected = actual;
                    }
                }
                if (mev.bstate & BUTTON4_PRESSED && selected > 0) {
                    selected--;
                    if (selected < scroll) scroll = selected;
                }
                if (mev.bstate & BUTTON5_PRESSED && selected < (int)fields.size() - 1) {
                    selected++;
                    if (selected >= scroll + visible) scroll = selected - visible + 1;
                }
            }
            continue;
        }
        if (ch == KEY_UP && selected > 0) {
            selected--;
            if (selected < scroll) scroll = selected;
        } else if (ch == KEY_DOWN && selected < (int)fields.size() - 1) {
            selected++;
            if (selected >= scroll + visible) scroll = selected - visible + 1;
        } else if (ch == 9) {
            selected++;
            if (selected >= (int)fields.size()) selected = 0;
            if (selected < scroll) scroll = selected;
            if (selected >= scroll + visible) scroll = selected - visible + 1;
        } else if (ch == KEY_BTAB) {
            selected--;
            if (selected < 0) selected = (int)fields.size() - 1;
            if (selected < scroll) scroll = selected;
            if (selected >= scroll + visible) scroll = selected - visible + 1;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            auto& f = fields[selected];
            if (f.type == FT_TEXT) {
                if (f.max_len >= 64) {
                    f.value = edit_text_area(f.label, f.value, f.max_len);
                } else {
                    int row = sy + 2 + (selected - scroll);
                    f.value = input_field(row, field_x + 1, field_w - 2, f.value, f.max_len);
                }
            } else if (f.type == FT_ENUM) {
                int idx = select_enum(f.enum_labels, f.enum_count, f.label, f.enum_value);
                if (idx >= 0) { f.enum_value = idx; f.value = f.enum_labels[idx]; }
            } else if (f.type == FT_FK_CLIENT) {
                uint32_t id = pick_client();
                if (id) { f.id_value = id; auto* e = ds_.clients.find_by_id(id); f.value = e ? e->name : "?"; }
            } else if (f.type == FT_FK_CASE) {
                uint32_t id = pick_case();
                if (id) { f.id_value = id; auto* e = ds_.cases.find_by_id(id); f.value = e ? e->title : "?"; }
            } else if (f.type == FT_FK_EVIDENCE) {
                uint32_t id = pick_evidence();
                if (id) { f.id_value = id; auto* e = ds_.evidence.find_by_id(id); f.value = e ? e->description : "?"; }
            } else if (f.type == FT_FK_EXAMINER) {
                uint32_t id = pick_examiner();
                if (id) { f.id_value = id; auto* e = ds_.examiners.find_by_id(id); f.value = e ? e->name : "?"; }
            }
        } else if (ch == KEY_F(2)) {
            return true;
        } else if (ch == 27) {
            return false;
        }
    }
}

// ── Display detail (scrollable key-value view) ─────────────────────────────

void UI::display_detail(const std::string& title,
                        const std::vector<std::pair<std::string,std::string>>& fields) {
    int scroll = 0;
    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int win_w = std::max(78, max_x_ * 80 / 100);
        if (win_w > max_x_ - 4) win_w = max_x_ - 4;
        int max_label = 0;
        for (const auto& f : fields)
            if ((int)f.first.size() > max_label) max_label = (int)f.first.size();
        max_label += 1;

        int value_w = win_w - max_label - 6;
        if (value_w < 10) value_w = 10;

        // Build display rows: wrap long values across multiple lines
        // Each display row = { label (or ""), value_line }
        struct DRow { std::string label; std::string value; };
        std::vector<DRow> rows;
        for (const auto& f : fields) {
            const std::string& val = f.second;
            if ((int)val.size() <= value_w) {
                rows.push_back({f.first, val});
            } else {
                // Wrap value across multiple rows
                int pos = 0;
                bool first = true;
                while (pos < (int)val.size()) {
                    int chunk = std::min(value_w, (int)val.size() - pos);
                    rows.push_back({first ? f.first : "", val.substr(pos, chunk)});
                    pos += chunk;
                    first = false;
                }
            }
        }

        int total = (int)rows.size();
        int visible = max_y_ - 8;
        if (visible < 1) visible = 1;
        int win_h = std::min(total, visible) + 5;
        int sy = (max_y_ - win_h) / 2;
        int sx = (max_x_ - win_w) / 2;
        int vx = sx + max_label + 4;

        draw_box(sy, sx, win_h, win_w, CP_TITLE);
        draw_title(sy, sx, win_w, title, CP_TITLE);

        int shown = std::min(total - scroll, visible);
        for (int i = 0; i < shown; i++) {
            int idx = i + scroll;
            int row = sy + 2 + i;
            if (!rows[idx].label.empty()) {
                attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
                mvprintw(row, sx + 2, "%*s:", max_label, rows[idx].label.c_str());
                attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
            }
            attron(COLOR_PAIR(CP_NORMAL));
            mvprintw(row, vx, "%.*s", value_w, rows[idx].value.c_str());
            attroff(COLOR_PAIR(CP_NORMAL));
        }

        // Scroll info
        attron(COLOR_PAIR(CP_TITLE));
        if (total > visible)
            mvprintw(sy + win_h - 2, sx + 3, "UP/DOWN: Scroll (%d-%d of %d) | ESC/Any key: Back",
                     scroll + 1, std::min(scroll + visible, total), total);
        else
            mvprintw(sy + win_h - 2, sx + 3, "ESC or any key: Back");
        attroff(COLOR_PAIR(CP_TITLE));

        show_status("Viewing: " + title + " | Scroll: Arrows/Wheel | Any key: Back");
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON4_PRESSED && scroll > 0) { scroll--; continue; }
                if (mev.bstate & BUTTON5_PRESSED && scroll + visible < total) { scroll++; continue; }
                if (mev.bstate & BUTTON1_CLICKED) break;
            }
            continue;
        }
        if (ch == KEY_UP && scroll > 0) { scroll--; continue; }
        if (ch == KEY_DOWN && scroll + visible < total) { scroll++; continue; }
        break;
    }
}

// ── Confirm delete ─────────────────────────────────────────────────────────

bool UI::confirm_delete(const std::string& what, uint32_t id) {
    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int box_w = std::max(56, max_x_ * 50 / 100);
        if (box_w > max_x_ - 4) box_w = max_x_ - 4;
        int box_h = 10;
        int sy = (max_y_ - box_h) / 2;
        int sx = (max_x_ - box_w) / 2;

        draw_box(sy, sx, box_h, box_w, CP_TITLE);
        draw_title(sy, sx, box_w, "Confirm Delete", CP_TITLE);

        attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(sy + 2, sx + 3, "You are about to delete:");
        attroff(COLOR_PAIR(CP_NORMAL));
        attron(COLOR_PAIR(CP_ERROR) | A_BOLD);
        mvprintw(sy + 4, sx + 3, "#%u  %.*s", id, box_w - 12, what.c_str());
        attroff(COLOR_PAIR(CP_ERROR) | A_BOLD);
        draw_hline(sy + 6, sx, box_w, CP_TITLE);
        attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
        mvprintw(sy + 7, sx + 3, "Are you sure?  [Y] Yes  [N] No");
        attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);

        refresh();
        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON1_CLICKED && mev.y == sy + 7) {
                    // Click on [Y] Yes area (left half) or [N] No area (right half)
                    int mid = sx + box_w / 2;
                    if (mev.x >= sx + 3 && mev.x < mid) return true;
                    if (mev.x >= mid && mev.x < sx + box_w - 1) return false;
                }
            }
            continue;
        }
        return (ch == 'y' || ch == 'Y');
    }
}

// ── FK pickers ─────────────────────────────────────────────────────────────

uint32_t UI::pick_client() {
    auto items = ds_.clients.get_all_active();
    if (items.empty()) { show_error("No clients. Add a client first."); wait_key(); return 0; }
    std::vector<ColDef> cols = {{"ID", 8}, {"Name", 30}, {"Company", 30}, {"Phone", 20}};
    std::vector<std::vector<std::string>> rows;
    for (const auto& c : items)
        rows.push_back({std::to_string(c.id), c.name, c.company, c.phone});
    int sel = select_from_list("Select Client", cols, rows);
    return (sel >= 0) ? items[sel].id : 0;
}

uint32_t UI::pick_case() {
    auto items = ds_.cases.get_all_active();
    if (items.empty()) { show_error("No cases. Add a case first."); wait_key(); return 0; }
    std::vector<ColDef> cols = {{"ID", 8}, {"Case #", 20}, {"Title", 40}, {"Status", 20}};
    std::vector<std::vector<std::string>> rows;
    for (const auto& c : items)
        rows.push_back({std::to_string(c.id), c.case_number, c.title, CaseStatusLabels[c.status]});
    int sel = select_from_list("Select Case", cols, rows);
    return (sel >= 0) ? items[sel].id : 0;
}

uint32_t UI::pick_evidence(uint32_t case_id) {
    auto all = ds_.evidence.get_all_active();
    std::vector<Evidence> items;
    for (const auto& e : all)
        if (case_id == 0 || e.case_id == case_id) items.push_back(e);
    if (items.empty()) { show_error("No evidence items found."); wait_key(); return 0; }
    std::vector<ColDef> cols = {{"ID", 8}, {"Evd #", 15}, {"Description", 45}, {"Type", 20}};
    std::vector<std::vector<std::string>> rows;
    for (const auto& e : items)
        rows.push_back({std::to_string(e.id), e.evidence_number, e.description, EvidenceTypeLabels[e.type]});
    int sel = select_from_list("Select Evidence", cols, rows);
    return (sel >= 0) ? items[sel].id : 0;
}

uint32_t UI::pick_examiner() {
    auto items = ds_.examiners.get_all_active();
    if (items.empty()) { show_error("No examiners. Add an examiner first."); wait_key(); return 0; }
    std::vector<ColDef> cols = {{"ID", 10}, {"Name", 30}, {"Title", 30}, {"Badge", 15}};
    std::vector<std::vector<std::string>> rows;
    for (const auto& e : items)
        rows.push_back({std::to_string(e.id), e.name, e.title, e.badge_id});
    int sel = select_from_list("Select Examiner", cols, rows);
    return (sel >= 0) ? items[sel].id : 0;
}

// ── Main Menu ──────────────────────────────────────────────────────────────

void UI::draw_main_menu(int selected) {
    getmaxyx(stdscr, max_y_, max_x_);
    fill_background();

    // Use most of the screen width
    int menu_w = std::max(58, max_x_ * 70 / 100);
    if (menu_w > max_x_ - 4) menu_w = max_x_ - 4;

    // Layout: border(1) + brand(1) + subtitle(1) + gap(1) + firm(1) + hline(1)
    //       + 10 items + hline(1) + exit(1) + border(1) = 20 minimum
    int menu_h = std::min(max_y_ - 2, 22);
    if (menu_h < 20) menu_h = std::min(max_y_ - 2, 20);
    int sy = (max_y_ - menu_h) / 2;
    int sx = (max_x_ - menu_w) / 2;

    draw_box(sy, sx, menu_h, menu_w, CP_TITLE);

    // Brand header — compact layout
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    std::string brand = "E  B  O  X  L  A  B";
    mvprintw(sy + 1, sx + (menu_w - (int)brand.size()) / 2, "%s", brand.c_str());
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    attron(COLOR_PAIR(CP_TITLE));
    std::string sub = "Digital Forensics & eDiscovery Platform v1.0";
    mvprintw(sy + 2, sx + (menu_w - (int)sub.size()) / 2, "%s", sub.c_str());
    attroff(COLOR_PAIR(CP_TITLE));

    attron(COLOR_PAIR(CP_NORMAL));
    std::string firm = "Law Stars - Trial Lawyers and Legal Services Colorado LLC";
    mvprintw(sy + 4, sx + (menu_w - (int)firm.size()) / 2, "%s", firm.c_str());
    attroff(COLOR_PAIR(CP_NORMAL));

    draw_hline(sy + 5, sx, menu_w, CP_TITLE);

    // Menu items — with highlight bar on selected
    const char* labels[] = {
        "1.  Client Management",
        "2.  Case Management",
        "3.  Evidence & Exhibits",
        "4.  Chain of Custody",
        "5.  Examiners",
        "6.  Custodians (eDiscovery)",
        "7.  Activity Log",
        "8.  Search All",
        "9.  File Manager",
        "0.  Hash Calculator",
    };
    int lx = sx + 8;
    int row = sy + 6;

    // Calculate gap: use gap=1 always with 10 items, gap=2 only if huge terminal
    int items_space = menu_h - 6 - 3; // header(6) + hline+exit+border(3)
    int gap = (items_space >= 20) ? 2 : 1;

    for (int i = 0; i < 10; i++) {
        int item_row = row + gap * i;
        if (item_row >= sy + menu_h - 1) break; // safety: don't draw outside box
        if (i == selected) {
            attron(COLOR_PAIR(CP_HIGHLIGHT));
            mvhline(item_row, sx + 1, ' ', menu_w - 2);
            mvprintw(item_row, lx, "%s", labels[i]);
            attroff(COLOR_PAIR(CP_HIGHLIGHT));
        } else {
            attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
            mvprintw(item_row, lx, "%s", labels[i]);
            attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
        }
    }

    int exit_row = row + gap*10 + 1;
    if (exit_row >= sy + menu_h - 1) exit_row = sy + menu_h - 2;
    draw_hline(exit_row - 1, sx, menu_w, CP_TITLE);
    if (selected == 10) {
        attron(COLOR_PAIR(CP_HIGHLIGHT));
        mvhline(exit_row, sx + 1, ' ', menu_w - 2);
        mvprintw(exit_row, lx, "Q.  Exit");
        attroff(COLOR_PAIR(CP_HIGHLIGHT));
    } else {
        attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
        mvprintw(exit_row, lx, "Q.  Exit");
        attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    }

    // Forensic ASCII art on right side (only if wide enough and tall enough)
    if (menu_w >= 62 && menu_h >= 20) {
        int rx = sx + menu_w - 18;

        // Shield badge (beside brand area)
        attron(COLOR_PAIR(CP_HINT));
        mvprintw(sy + 1, rx, "   /|=====|\\");
        mvprintw(sy + 2, rx, "  | DIGITAL |");
        mvprintw(sy + 3, rx, "  |FORENSICS|");
        mvprintw(sy + 4, rx, "   \\|=====|/");
        attroff(COLOR_PAIR(CP_HINT));

        // Evidence monitor readout (beside menu items)
        attron(COLOR_PAIR(CP_HINT));
        mvprintw(row,         rx, " .-----------.");
        mvprintw(row + gap,   rx, " | > ACQUIRE |");
        mvprintw(row + gap*2, rx, " | > ANALYZE |");
        mvprintw(row + gap*3, rx, " | > IMAGE   |");
        mvprintw(row + gap*4, rx, " | > HASH    |");
        mvprintw(row + gap*5, rx, " | > REPORT  |");
        mvprintw(row + gap*6, rx, " '-----------'");
        mvprintw(row + gap*7, rx, "    |==|==|");
        attroff(COLOR_PAIR(CP_HINT));
    }

    // Date
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char datebuf[32];
    strftime(datebuf, sizeof(datebuf), "%B %d, %Y", t);
    attron(COLOR_PAIR(CP_STATUS));
    mvhline(max_y_ - 1, 0, ' ', max_x_);
    mvprintw(max_y_ - 1, 2, "Law Stars - Trial Lawyers and Legal Services Colorado LLC");
    mvprintw(max_y_ - 1, max_x_ - (int)strlen(datebuf) - 2, "%s", datebuf);
    attroff(COLOR_PAIR(CP_STATUS));

    refresh();
}

void UI::run() {
    int selected = 0;
    while (true) {
        draw_main_menu(selected);
        show_status("Click/Tab/Arrows: Navigate | ENTER: Select | 1-0: Quick | Q: Exit | F3: Theme");
        refresh();
        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON1_CLICKED) {
                    int mw = std::max(58, max_x_ * 70 / 100);
                    if (mw > max_x_ - 4) mw = max_x_ - 4;
                    int mh = std::min(max_y_ - 2, 22);
                    if (mh < 20) mh = std::min(max_y_ - 2, 20);
                    int msy = (max_y_ - mh) / 2;
                    int msx = (max_x_ - mw) / 2;
                    int mitems_space = mh - 6 - 3;
                    int mgap = (mitems_space >= 20) ? 2 : 1;
                    int mrow = msy + 6;
                    if (mev.x > msx && mev.x < msx + mw - 1) {
                        for (int i = 0; i < 10; i++) {
                            if (mev.y == mrow + mgap * i) {
                                selected = i;
                                switch(i) {
                                    case 0: client_menu(); break;
                                    case 1: case_menu(); break;
                                    case 2: evidence_menu(); break;
                                    case 3: custody_menu(); break;
                                    case 4: examiner_menu(); break;
                                    case 5: custodian_menu(); break;
                                    case 6: activity_menu(); break;
                                    case 7: search_all(); break;
                                    case 8: file_manager(); break;
                                    case 9: hash_calculator(); break;
                                }
                                break;
                            }
                        }
                        int erow = mrow + mgap*10 + 1;
                        if (erow >= msy + mh - 1) erow = msy + mh - 2;
                        if (mev.y == erow) return;
                    }
                }
                if (mev.bstate & BUTTON4_PRESSED && selected > 0) selected--;
                if (mev.bstate & BUTTON5_PRESSED && selected < 10) selected++;
            }
            continue;
        }
        if (ch == KEY_F(3)) { theme_ = (theme_ + 1) % 2; apply_theme(); continue; }
        if (ch == KEY_UP && selected > 0) { selected--; continue; }
        if (ch == KEY_DOWN && selected < 10) { selected++; continue; }
        if (ch == 9) { selected++; if (selected > 10) selected = 0; continue; }
        if (ch == KEY_BTAB) { selected--; if (selected < 0) selected = 10; continue; }
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            switch (selected) {
                case 0: client_menu(); break;
                case 1: case_menu(); break;
                case 2: evidence_menu(); break;
                case 3: custody_menu(); break;
                case 4: examiner_menu(); break;
                case 5: custodian_menu(); break;
                case 6: activity_menu(); break;
                case 7: search_all(); break;
                case 8: file_manager(); break;
                case 9: hash_calculator(); break;
                case 10: return;
            }
            continue;
        }
        switch (ch) {
            case '1': client_menu(); break;
            case '2': case_menu(); break;
            case '3': evidence_menu(); break;
            case '4': custody_menu(); break;
            case '5': examiner_menu(); break;
            case '6': custodian_menu(); break;
            case '7': activity_menu(); break;
            case '8': search_all(); break;
            case '9': file_manager(); break;
            case '0': hash_calculator(); break;
            case 'q': case 'Q': return;
            default: break;
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// CLIENT MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void UI::client_menu() {
    while (true) {
        int c = show_submenu("Client Management", {
            "View All Clients", "Add New Client", "Search Clients",
            "Edit Client", "Delete Client", "Back to Main Menu"
        });
        if (c == 0) {
            auto items = ds_.clients.get_all_active();
            std::vector<ColDef> cols = {{"ID",8},{"Name",25},{"Company",20},{"Phone",15},{"Law Firm",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cl : items) rows.push_back({std::to_string(cl.id), cl.name, cl.company, cl.phone, cl.law_firm});
            int sel = select_from_list("All Clients", cols, rows);
            if (sel >= 0) {
                const auto& cl = items[sel];
                display_detail("Client: " + std::string(cl.name), {
                    {"ID", std::to_string(cl.id)}, {"Name", cl.name}, {"Company", cl.company},
                    {"Phone", cl.phone}, {"Email", cl.email}, {"Address", cl.address},
                    {"Law Firm", cl.law_firm}, {"Attorney", cl.attorney_name},
                    {"Attorney Phone", cl.attorney_phone}, {"Attorney Email", cl.attorney_email},
                    {"Jurisdiction", cl.jurisdiction}, {"Notes", cl.notes},
                    {"Created", format_dt(cl.created_at)}, {"Modified", format_dt(cl.modified_at)}
                });
            }
        } else if (c == 1) {
            std::vector<FormField> fields = {
                make_text("Name", "Full legal name of the client or organization", 63),
                make_text("Company", "Company/organization name (if different from client name)", 63),
                make_text("Phone", "Primary contact phone with area code, e.g. (555) 123-4567", 31),
                make_text("Email", "Primary email address for case correspondence", 63),
                make_text("Address", "Full mailing address (street, city, state, ZIP)", 127),
                make_text("Law Firm", "Name of the retaining law firm, e.g. \"Smith & Associates LLP\"", 63),
                make_text("Attorney", "Lead attorney handling this client's matters", 63),
                make_text("Atty Phone", "Attorney's direct phone number", 31),
                make_text("Atty Email", "Attorney's email for legal communications", 63),
                make_text("Jurisdiction", "Primary legal jurisdiction, e.g. \"Southern District of NY\"", 63),
                make_text("Notes", "Additional context: referral source, special instructions, etc.", 255)
            };
            if (run_form("Add New Client", fields)) {
                Client cl;
                cl.set_name(fields[0].value.c_str());
                cl.set_company(fields[1].value.c_str());
                cl.set_phone(fields[2].value.c_str());
                cl.set_email(fields[3].value.c_str());
                cl.set_address(fields[4].value.c_str());
                cl.set_law_firm(fields[5].value.c_str());
                cl.set_attorney_name(fields[6].value.c_str());
                cl.set_attorney_phone(fields[7].value.c_str());
                cl.set_attorney_email(fields[8].value.c_str());
                cl.set_jurisdiction(fields[9].value.c_str());
                cl.set_notes(fields[10].value.c_str());
                if (ds_.clients.save(cl)) {
                    ds_.log_activity(0, 0, "Client Added", cl.name);
                    show_status("Client saved successfully!");
                } else show_error("Failed to save client.");
                wait_key();
            }
        } else if (c == 2) {
            getmaxyx(stdscr, max_y_, max_x_);
            fill_background();
            int bw = std::max(60, max_x_ * 60 / 100); if (bw > max_x_ - 4) bw = max_x_ - 4; int bh = 8;
            int sy = (max_y_ - bh)/2, sx = (max_x_ - bw)/2;
            draw_box(sy, sx, bh, bw, CP_TITLE);
            draw_title(sy, sx, bw, "Search Clients", CP_TITLE);
            attron(COLOR_PAIR(CP_TITLE)|A_BOLD); mvprintw(sy+2, sx+3, "Query:"); attroff(COLOR_PAIR(CP_TITLE)|A_BOLD);
            attron(COLOR_PAIR(CP_HINT)); mvprintw(sy+4, sx+3, "Searches name, company, email, law firm, attorney"); attroff(COLOR_PAIR(CP_HINT));
            refresh();
            std::string q = input_field(sy+2, sx+12, bw-15);
            if (!q.empty()) {
                auto results = ds_.search_clients(q);
                std::vector<ColDef> cols = {{"ID",8},{"Name",30},{"Company",25},{"Phone",15},{"Law Firm",20}};
                std::vector<std::vector<std::string>> rows;
                for (const auto& cl : results) rows.push_back({std::to_string(cl.id), cl.name, cl.company, cl.phone, cl.law_firm});
                int sel = select_from_list("Search: " + q + " (" + std::to_string(results.size()) + " found)", cols, rows);
                if (sel >= 0) {
                    const auto& cl = results[sel];
                    display_detail("Client: " + std::string(cl.name), {
                        {"ID", std::to_string(cl.id)}, {"Name", cl.name}, {"Company", cl.company},
                        {"Phone", cl.phone}, {"Email", cl.email}, {"Address", cl.address},
                        {"Law Firm", cl.law_firm}, {"Attorney", cl.attorney_name},
                        {"Jurisdiction", cl.jurisdiction}, {"Notes", cl.notes}
                    });
                }
            }
        } else if (c == 3) {
            auto items = ds_.clients.get_all_active();
            std::vector<ColDef> cols = {{"ID",8},{"Name",30},{"Company",30},{"Phone",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cl : items) rows.push_back({std::to_string(cl.id), cl.name, cl.company, cl.phone});
            int sel = select_from_list("Select Client to Edit", cols, rows);
            if (sel >= 0) {
                Client cl = items[sel];
                std::vector<FormField> fields = {
                    make_text("Name", "Full legal name of the client or organization", 63, cl.name),
                    make_text("Company", "Company/organization name", 63, cl.company),
                    make_text("Phone", "Primary contact phone with area code", 31, cl.phone),
                    make_text("Email", "Primary email address for case correspondence", 63, cl.email),
                    make_text("Address", "Full mailing address (street, city, state, ZIP)", 127, cl.address),
                    make_text("Law Firm", "Name of the retaining law firm", 63, cl.law_firm),
                    make_text("Attorney", "Lead attorney handling this client's matters", 63, cl.attorney_name),
                    make_text("Atty Phone", "Attorney's direct phone number", 31, cl.attorney_phone),
                    make_text("Atty Email", "Attorney's email for legal communications", 63, cl.attorney_email),
                    make_text("Jurisdiction", "Primary legal jurisdiction", 63, cl.jurisdiction),
                    make_text("Notes", "Additional context: referral source, special instructions", 255, cl.notes)
                };
                if (run_form("Edit Client #" + std::to_string(cl.id), fields)) {
                    cl.set_name(fields[0].value.c_str()); cl.set_company(fields[1].value.c_str());
                    cl.set_phone(fields[2].value.c_str()); cl.set_email(fields[3].value.c_str());
                    cl.set_address(fields[4].value.c_str()); cl.set_law_firm(fields[5].value.c_str());
                    cl.set_attorney_name(fields[6].value.c_str()); cl.set_attorney_phone(fields[7].value.c_str());
                    cl.set_attorney_email(fields[8].value.c_str()); cl.set_jurisdiction(fields[9].value.c_str());
                    cl.set_notes(fields[10].value.c_str());
                    if (ds_.clients.update(cl)) {
                        ds_.log_activity(0, 0, "Client Updated", cl.name);
                        show_status("Client updated!");
                    } else show_error("Failed to update.");
                    wait_key();
                }
            }
        } else if (c == 4) {
            auto items = ds_.clients.get_all_active();
            std::vector<ColDef> cols = {{"ID",10},{"Name",35},{"Company",35},{"Phone",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cl : items) rows.push_back({std::to_string(cl.id), cl.name, cl.company, cl.phone});
            int sel = select_from_list("Select Client to Delete", cols, rows);
            if (sel >= 0 && confirm_delete(items[sel].name, items[sel].id)) {
                if (ds_.clients.remove(items[sel].id)) {
                    ds_.log_activity(0, 0, "Client Deleted", items[sel].name);
                    show_status("Client deleted.");
                } else show_error("Failed to delete.");
                wait_key();
            }
        } else return;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// CASE MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void UI::case_menu() {
    while (true) {
        int c = show_submenu("Case Management", {
            "View All Cases", "Add New Case", "Search Cases",
            "Edit Case", "Delete Case", "Case Dashboard", "Back to Main Menu"
        });
        if (c == 0) {
            auto items = ds_.cases.get_all_active();
            std::vector<ColDef> cols = {{"ID",6},{"Case #",15},{"Title",30},{"Status",15},{"Type",15},{"Law Firm",15}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cs : items)
                rows.push_back({std::to_string(cs.id), cs.case_number, cs.title,
                               CaseStatusLabels[cs.status], CaseTypeLabels[cs.case_type], cs.law_firm});
            int sel = select_from_list("All Cases", cols, rows);
            if (sel >= 0) {
                const auto& cs = items[sel];
                std::string client_name = "(none)";
                auto* cl = ds_.clients.find_by_id(cs.client_id);
                if (cl) client_name = cl->name;
                std::string exam_name = "(none)";
                auto* ex = ds_.examiners.find_by_id(cs.assigned_examiner_id);
                if (ex) exam_name = ex->name;
                display_detail("Case: " + std::string(cs.title), {
                    {"ID", std::to_string(cs.id)}, {"Case Number", cs.case_number},
                    {"Court Docket", cs.court_case_number}, {"Title", cs.title},
                    {"Description", cs.description}, {"Case Type", CaseTypeLabels[cs.case_type]},
                    {"Status", CaseStatusLabels[cs.status]}, {"Client", client_name},
                    {"Examiner", exam_name}, {"Law Firm", cs.law_firm},
                    {"Attorney", cs.attorney_name}, {"Judge", cs.judge_name},
                    {"Jurisdiction", cs.jurisdiction}, {"Location", cs.location},
                    {"Opened", format_dt(cs.opened_date)}, {"Closed", format_dt(cs.closed_date)},
                    {"Created", format_dt(cs.created_at)}, {"Modified", format_dt(cs.modified_at)}
                });
            }
        } else if (c == 1) {
            std::vector<FormField> fields = {
                make_fk(FT_FK_CLIENT, "Client", "Select the client this case belongs to"),
                make_text("Case Number", "Internal case tracking number, e.g. \"EBX-2026-0042\"", 31),
                make_text("Court Docket", "Court docket number, e.g. \"2:24-cv-01234\" (blank if pre-litigation)", 31),
                make_text("Title", "Short descriptive case title, e.g. \"Corp Data Breach Investigation\"", 127),
                make_text("Description", "Brief summary of the matter and forensic objectives", 255),
                make_enum_field("Case Type", "Type of legal matter: Civil, Criminal, Regulatory, Internal, Insurance", CaseTypeLabels, CTYPE_COUNT),
                make_enum_field("Status", "Lifecycle stage: Intake > Collection > Processing > Analysis > Review > Production > Closed", CaseStatusLabels, STATUS_COUNT),
                make_fk(FT_FK_EXAMINER, "Examiner", "Lead forensic examiner assigned to this case"),
                make_text("Law Firm", "Law firm handling this specific case", 63),
                make_text("Attorney", "Lead attorney on this case", 63),
                make_text("Judge", "Assigned judge (if filed), e.g. \"Hon. Jane Smith\"", 63),
                make_text("Jurisdiction", "Court/jurisdiction, e.g. \"US District Court, SDNY\"", 63),
                make_text("Location", "Primary physical location relevant to the case", 127)
            };
            if (run_form("Add New Case", fields)) {
                Case cs;
                cs.client_id = fields[0].id_value;
                cs.set_case_number(fields[1].value.c_str());
                cs.set_court_case_number(fields[2].value.c_str());
                cs.set_title(fields[3].value.c_str());
                cs.set_description(fields[4].value.c_str());
                cs.case_type = (CaseType)fields[5].enum_value;
                cs.status = (CaseStatus)fields[6].enum_value;
                cs.assigned_examiner_id = fields[7].id_value;
                cs.set_law_firm(fields[8].value.c_str());
                cs.set_attorney_name(fields[9].value.c_str());
                cs.set_judge_name(fields[10].value.c_str());
                cs.set_jurisdiction(fields[11].value.c_str());
                cs.set_location(fields[12].value.c_str());
                cs.opened_date = time(nullptr);
                if (ds_.cases.save(cs)) {
                    ds_.log_activity(cs.id, cs.assigned_examiner_id, "Case Created", cs.title);
                    show_status("Case saved!");
                } else show_error("Failed to save case.");
                wait_key();
            }
        } else if (c == 2) {
            getmaxyx(stdscr, max_y_, max_x_);
            fill_background();
            int bw = std::min(60, max_x_-4), bh = 8;
            int sy = (max_y_-bh)/2, sx = (max_x_-bw)/2;
            draw_box(sy, sx, bh, bw, CP_TITLE);
            draw_title(sy, sx, bw, "Search Cases", CP_TITLE);
            attron(COLOR_PAIR(CP_TITLE)|A_BOLD); mvprintw(sy+2, sx+3, "Query:"); attroff(COLOR_PAIR(CP_TITLE)|A_BOLD);
            attron(COLOR_PAIR(CP_HINT)); mvprintw(sy+4, sx+3, "Searches case #, title, description, attorney, judge"); attroff(COLOR_PAIR(CP_HINT));
            refresh();
            std::string q = input_field(sy+2, sx+12, bw-15);
            if (!q.empty()) {
                auto results = ds_.search_cases(q);
                std::vector<ColDef> cols = {{"ID",8},{"Case #",18},{"Title",35},{"Status",18},{"Type",18}};
                std::vector<std::vector<std::string>> rows;
                for (const auto& cs : results)
                    rows.push_back({std::to_string(cs.id), cs.case_number, cs.title, CaseStatusLabels[cs.status], CaseTypeLabels[cs.case_type]});
                select_from_list("Search: " + q + " (" + std::to_string(results.size()) + ")", cols, rows);
            }
        } else if (c == 3) {
            auto items = ds_.cases.get_all_active();
            std::vector<ColDef> cols = {{"ID",8},{"Case #",20},{"Title",40},{"Status",18}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cs : items) rows.push_back({std::to_string(cs.id), cs.case_number, cs.title, CaseStatusLabels[cs.status]});
            int sel = select_from_list("Select Case to Edit", cols, rows);
            if (sel >= 0) {
                Case cs = items[sel];
                std::string cl_name, ex_name;
                auto* cl = ds_.clients.find_by_id(cs.client_id);
                cl_name = cl ? cl->name : "";
                auto* ex = ds_.examiners.find_by_id(cs.assigned_examiner_id);
                ex_name = ex ? ex->name : "";
                std::vector<FormField> fields = {
                    make_fk(FT_FK_CLIENT, "Client", "Select the client this case belongs to", cs.client_id, cl_name),
                    make_text("Case Number", "Internal case tracking number", 31, cs.case_number),
                    make_text("Court Docket", "Court docket number (blank if pre-litigation)", 31, cs.court_case_number),
                    make_text("Title", "Short descriptive case title", 127, cs.title),
                    make_text("Description", "Brief summary of the matter", 255, cs.description),
                    make_enum_field("Case Type", "Type of legal matter", CaseTypeLabels, CTYPE_COUNT, cs.case_type),
                    make_enum_field("Status", "Lifecycle stage", CaseStatusLabels, STATUS_COUNT, cs.status),
                    make_fk(FT_FK_EXAMINER, "Examiner", "Lead forensic examiner", cs.assigned_examiner_id, ex_name),
                    make_text("Law Firm", "Law firm handling this case", 63, cs.law_firm),
                    make_text("Attorney", "Lead attorney on this case", 63, cs.attorney_name),
                    make_text("Judge", "Assigned judge", 63, cs.judge_name),
                    make_text("Jurisdiction", "Court/jurisdiction", 63, cs.jurisdiction),
                    make_text("Location", "Primary physical location", 127, cs.location)
                };
                if (run_form("Edit Case #" + std::to_string(cs.id), fields)) {
                    CaseStatus old_status = cs.status;
                    cs.client_id = fields[0].id_value;
                    cs.set_case_number(fields[1].value.c_str());
                    cs.set_court_case_number(fields[2].value.c_str());
                    cs.set_title(fields[3].value.c_str());
                    cs.set_description(fields[4].value.c_str());
                    cs.case_type = (CaseType)fields[5].enum_value;
                    cs.status = (CaseStatus)fields[6].enum_value;
                    cs.assigned_examiner_id = fields[7].id_value;
                    cs.set_law_firm(fields[8].value.c_str());
                    cs.set_attorney_name(fields[9].value.c_str());
                    cs.set_judge_name(fields[10].value.c_str());
                    cs.set_jurisdiction(fields[11].value.c_str());
                    cs.set_location(fields[12].value.c_str());
                    if (cs.status == STATUS_CLOSED && old_status != STATUS_CLOSED)
                        cs.closed_date = time(nullptr);
                    if (ds_.cases.update(cs)) {
                        if (cs.status != old_status)
                            ds_.log_activity(cs.id, cs.assigned_examiner_id, "Status Changed",
                                (std::string(CaseStatusLabels[old_status]) + " -> " + CaseStatusLabels[cs.status]).c_str());
                        show_status("Case updated!");
                    } else show_error("Failed to update.");
                    wait_key();
                }
            }
        } else if (c == 4) {
            auto items = ds_.cases.get_all_active();
            std::vector<ColDef> cols = {{"ID",10},{"Case #",25},{"Title",40},{"Status",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cs : items) rows.push_back({std::to_string(cs.id), cs.case_number, cs.title, CaseStatusLabels[cs.status]});
            int sel = select_from_list("Select Case to Delete", cols, rows);
            if (sel >= 0 && confirm_delete(items[sel].title, items[sel].id)) {
                if (ds_.cases.remove(items[sel].id)) {
                    ds_.log_activity(items[sel].id, 0, "Case Deleted", items[sel].title);
                    show_status("Case deleted.");
                } else show_error("Failed to delete.");
                wait_key();
            }
        } else if (c == 5) {
            // Case Dashboard
            auto items = ds_.cases.get_all_active();
            std::vector<ColDef> cols = {{"ID",8},{"Case #",20},{"Title",40},{"Status",18}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cs : items) rows.push_back({std::to_string(cs.id), cs.case_number, cs.title, CaseStatusLabels[cs.status]});
            int sel = select_from_list("Select Case for Dashboard", cols, rows);
            if (sel >= 0) {
                const auto& cs = items[sel];
                auto evds = ds_.evidence.find_by_fk(&Evidence::case_id, cs.id);
                auto custs = ds_.custodians.find_by_fk(&Custodian::case_id, cs.id);
                auto logs = ds_.activity.find_by_fk(&ActivityLog::case_id, cs.id);
                std::string cl_name = "(none)";
                auto* cl = ds_.clients.find_by_id(cs.client_id);
                if (cl) cl_name = cl->name;
                std::string ex_name = "(none)";
                auto* ex = ds_.examiners.find_by_id(cs.assigned_examiner_id);
                if (ex) ex_name = ex->name;
                display_detail("Dashboard: " + std::string(cs.title), {
                    {"Case Number", cs.case_number}, {"Court Docket", cs.court_case_number},
                    {"Status", CaseStatusLabels[cs.status]}, {"Type", CaseTypeLabels[cs.case_type]},
                    {"Client", cl_name}, {"Examiner", ex_name},
                    {"Law Firm", cs.law_firm}, {"Attorney", cs.attorney_name},
                    {"Judge", cs.judge_name}, {"Jurisdiction", cs.jurisdiction},
                    {"Location", cs.location},
                    {"Evidence Items", std::to_string(evds.size())},
                    {"Custodians", std::to_string(custs.size())},
                    {"Activity Entries", std::to_string(logs.size())},
                    {"Opened", format_dt(cs.opened_date)}, {"Closed", format_dt(cs.closed_date)}
                });
            }
        } else return;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// EVIDENCE & EXHIBITS
// ════════════════════════════════════════════════════════════════════════════

void UI::evidence_menu() {
    while (true) {
        int c = show_submenu("Evidence & Exhibits", {
            "View All Evidence", "Add New Evidence", "Search Evidence",
            "Edit Evidence", "Delete Evidence", "Back to Main Menu"
        });
        if (c == 0) {
            auto items = ds_.evidence.get_all_active();
            std::vector<ColDef> cols = {{"ID",6},{"Evd #",12},{"Description",30},{"Type",15},{"MD5",20},{"Location",15}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& e : items)
                rows.push_back({std::to_string(e.id), e.evidence_number, e.description,
                               EvidenceTypeLabels[e.type], e.md5, e.location});
            int sel = select_from_list("All Evidence", cols, rows);
            if (sel >= 0) {
                const auto& e = items[sel];
                std::string case_title = "(none)";
                auto* cs = ds_.cases.find_by_id(e.case_id);
                if (cs) case_title = std::string(cs->case_number) + " - " + cs->title;
                std::string sz = std::to_string(e.size_bytes) + " bytes";
                display_detail("Evidence: " + std::string(e.evidence_number), {
                    {"ID", std::to_string(e.id)}, {"Evidence #", e.evidence_number},
                    {"Case", case_title}, {"Description", e.description},
                    {"Type", EvidenceTypeLabels[e.type]}, {"Source", e.source},
                    {"MD5", e.md5}, {"SHA-256", e.sha256}, {"Size", sz},
                    {"Location", e.location}, {"Seized By", e.seized_by},
                    {"Seized At", e.seized_location},
                    {"Acquired", format_dt(e.acquisition_date)}, {"Notes", e.notes},
                    {"Created", format_dt(e.created_at)}, {"Modified", format_dt(e.modified_at)}
                });
            }
        } else if (c == 1) {
            std::vector<FormField> fields = {
                make_fk(FT_FK_CASE, "Case", "Select the case this evidence belongs to"),
                make_text("Evidence #", "Unique evidence tag number, e.g. \"EVD-001\"", 31),
                make_text("Description", "What is this evidence? e.g. \"Employee laptop - Dell Latitude 5520\"", 127),
                make_enum_field("Type", "Media type: Hard Drive, USB, Mobile, Cloud, Email, Document, Image", EvidenceTypeLabels, ETYPE_COUNT),
                make_text("Source", "Where/who collected from, e.g. \"John Doe's office, Desk #3\"", 127),
                make_text("MD5", "MD5 hash of the forensic image (32 hex chars) - verify integrity", 32),
                make_text("SHA-256", "SHA-256 hash of the forensic image (64 hex chars) - verify integrity", 64),
                make_text("Size (bytes)", "Total size in bytes of the evidence/image file", 20),
                make_text("Location", "Current storage location, e.g. \"Evidence Locker B, Shelf 3\"", 127),
                make_text("Seized By", "Name of person who collected/seized this evidence", 63),
                make_text("Seized At", "Physical address where evidence was collected", 127),
                make_text("Notes", "Condition on receipt, special handling instructions, etc.", 255)
            };
            if (run_form("Add New Evidence", fields)) {
                Evidence e;
                e.case_id = fields[0].id_value;
                e.set_evidence_number(fields[1].value.c_str());
                e.set_description(fields[2].value.c_str());
                e.type = (EvidenceType)fields[3].enum_value;
                e.set_source(fields[4].value.c_str());
                e.set_md5(fields[5].value.c_str());
                e.set_sha256(fields[6].value.c_str());
                e.size_bytes = strtoull(fields[7].value.c_str(), nullptr, 10);
                e.set_location(fields[8].value.c_str());
                e.set_seized_by(fields[9].value.c_str());
                e.set_seized_location(fields[10].value.c_str());
                e.set_notes(fields[11].value.c_str());
                e.acquisition_date = time(nullptr);
                if (ds_.evidence.save(e)) {
                    ds_.log_activity(e.case_id, 0, "Evidence Added", e.evidence_number);
                    show_status("Evidence saved!");
                } else show_error("Failed to save.");
                wait_key();
            }
        } else if (c == 2) {
            getmaxyx(stdscr, max_y_, max_x_);
            fill_background();
            int bw = std::min(60, max_x_-4), bh = 8;
            int sy = (max_y_-bh)/2, sx = (max_x_-bw)/2;
            draw_box(sy, sx, bh, bw, CP_TITLE);
            draw_title(sy, sx, bw, "Search Evidence", CP_TITLE);
            attron(COLOR_PAIR(CP_TITLE)|A_BOLD); mvprintw(sy+2, sx+3, "Query:"); attroff(COLOR_PAIR(CP_TITLE)|A_BOLD);
            attron(COLOR_PAIR(CP_HINT)); mvprintw(sy+4, sx+3, "Searches evidence #, description, source, hash, location"); attroff(COLOR_PAIR(CP_HINT));
            refresh();
            std::string q = input_field(sy+2, sx+12, bw-15);
            if (!q.empty()) {
                auto results = ds_.search_evidence(q);
                std::vector<ColDef> cols = {{"ID",8},{"Evd #",15},{"Description",35},{"Type",20},{"Location",20}};
                std::vector<std::vector<std::string>> rows;
                for (const auto& e : results)
                    rows.push_back({std::to_string(e.id), e.evidence_number, e.description, EvidenceTypeLabels[e.type], e.location});
                select_from_list("Search: " + q + " (" + std::to_string(results.size()) + ")", cols, rows);
            }
        } else if (c == 3) {
            auto items = ds_.evidence.get_all_active();
            std::vector<ColDef> cols = {{"ID",8},{"Evd #",15},{"Description",40},{"Type",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& e : items) rows.push_back({std::to_string(e.id), e.evidence_number, e.description, EvidenceTypeLabels[e.type]});
            int sel = select_from_list("Select Evidence to Edit", cols, rows);
            if (sel >= 0) {
                Evidence e = items[sel];
                std::string case_disp;
                auto* cs = ds_.cases.find_by_id(e.case_id);
                if (cs) case_disp = cs->title;
                std::vector<FormField> fields = {
                    make_fk(FT_FK_CASE, "Case", "Select the case this evidence belongs to", e.case_id, case_disp),
                    make_text("Evidence #", "Unique evidence tag number", 31, e.evidence_number),
                    make_text("Description", "What is this evidence?", 127, e.description),
                    make_enum_field("Type", "Media type", EvidenceTypeLabels, ETYPE_COUNT, e.type),
                    make_text("Source", "Where/who collected from", 127, e.source),
                    make_text("MD5", "MD5 hash (32 hex chars)", 32, e.md5),
                    make_text("SHA-256", "SHA-256 hash (64 hex chars)", 64, e.sha256),
                    make_text("Size (bytes)", "Total size in bytes", 20, std::to_string(e.size_bytes)),
                    make_text("Location", "Current storage location", 127, e.location),
                    make_text("Seized By", "Name of person who collected", 63, e.seized_by),
                    make_text("Seized At", "Physical address where collected", 127, e.seized_location),
                    make_text("Notes", "Condition, handling instructions", 255, e.notes)
                };
                if (run_form("Edit Evidence #" + std::to_string(e.id), fields)) {
                    e.case_id = fields[0].id_value;
                    e.set_evidence_number(fields[1].value.c_str());
                    e.set_description(fields[2].value.c_str());
                    e.type = (EvidenceType)fields[3].enum_value;
                    e.set_source(fields[4].value.c_str());
                    e.set_md5(fields[5].value.c_str());
                    e.set_sha256(fields[6].value.c_str());
                    e.size_bytes = strtoull(fields[7].value.c_str(), nullptr, 10);
                    e.set_location(fields[8].value.c_str());
                    e.set_seized_by(fields[9].value.c_str());
                    e.set_seized_location(fields[10].value.c_str());
                    e.set_notes(fields[11].value.c_str());
                    if (ds_.evidence.update(e)) {
                        ds_.log_activity(e.case_id, 0, "Evidence Updated", e.evidence_number);
                        show_status("Evidence updated!");
                    } else show_error("Failed to update.");
                    wait_key();
                }
            }
        } else if (c == 4) {
            auto items = ds_.evidence.get_all_active();
            std::vector<ColDef> cols = {{"ID",10},{"Evd #",20},{"Description",40},{"Type",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& e : items) rows.push_back({std::to_string(e.id), e.evidence_number, e.description, EvidenceTypeLabels[e.type]});
            int sel = select_from_list("Select Evidence to Delete", cols, rows);
            if (sel >= 0 && confirm_delete(items[sel].evidence_number, items[sel].id)) {
                if (ds_.evidence.remove(items[sel].id)) {
                    ds_.log_activity(items[sel].case_id, 0, "Evidence Deleted", items[sel].evidence_number);
                    show_status("Evidence deleted.");
                } else show_error("Failed to delete.");
                wait_key();
            }
        } else return;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// CHAIN OF CUSTODY
// ════════════════════════════════════════════════════════════════════════════

void UI::custody_menu() {
    while (true) {
        int c = show_submenu("Chain of Custody", {
            "View Custody Timeline", "Add Custody Entry", "Back to Main Menu"
        });
        if (c == 0) {
            // Pick evidence, then show its chain
            uint32_t eid = pick_evidence();
            if (eid) {
                auto entries = ds_.chain.find_by_fk(&ChainOfCustody::evidence_id, eid);
                auto* ev = ds_.evidence.find_by_id(eid);
                std::string title = "Chain of Custody";
                if (ev) title += ": " + std::string(ev->evidence_number);
                if (entries.empty()) {
                    show_error("No custody entries for this evidence.");
                    wait_key();
                } else {
                    std::vector<ColDef> cols = {{"ID",6},{"Action",15},{"Handler",20},{"From",20},{"To",20},{"Date",15}};
                    std::vector<std::vector<std::string>> rows;
                    for (const auto& e : entries)
                        rows.push_back({std::to_string(e.id), CustodyActionLabels[e.action],
                                       e.handler_name, e.from_location, e.to_location, format_dt(e.date)});
                    int sel = select_from_list(title, cols, rows);
                    if (sel >= 0) {
                        const auto& e = entries[sel];
                        display_detail("Custody Entry #" + std::to_string(e.id), {
                            {"Action", CustodyActionLabels[e.action]},
                            {"Handler", e.handler_name},
                            {"From", e.from_location}, {"To", e.to_location},
                            {"Date", format_dt(e.date)}, {"Notes", e.notes},
                            {"Created", format_dt(e.created_at)}
                        });
                    }
                }
            }
        } else if (c == 1) {
            std::vector<FormField> fields = {
                make_fk(FT_FK_EVIDENCE, "Evidence", "Select the evidence item this custody event applies to"),
                make_enum_field("Action", "What happened: Received, Transferred, Analyzed, Stored, Returned, Disposed", CustodyActionLabels, CUSTODY_COUNT),
                make_text("Handler", "Name of person who performed this action", 63),
                make_text("From", "Where the evidence was before this action", 127),
                make_text("To", "Where the evidence went after this action", 127),
                make_text("Notes", "Reason for transfer, condition, witness present, etc.", 255)
            };
            if (run_form("Add Custody Entry", fields)) {
                ChainOfCustody coc;
                coc.evidence_id = fields[0].id_value;
                coc.action = (CustodyAction)fields[1].enum_value;
                coc.set_handler_name(fields[2].value.c_str());
                coc.set_from_location(fields[3].value.c_str());
                coc.set_to_location(fields[4].value.c_str());
                coc.set_notes(fields[5].value.c_str());
                coc.date = time(nullptr);
                if (ds_.chain.save(coc)) {
                    // Find case_id for logging
                    auto* ev = ds_.evidence.find_by_id(coc.evidence_id);
                    uint32_t cid = ev ? ev->case_id : 0;
                    ds_.log_activity(cid, 0, "Custody Entry Added",
                        (std::string(CustodyActionLabels[coc.action]) + " - " + coc.handler_name).c_str());
                    show_status("Custody entry saved!");
                } else show_error("Failed to save.");
                wait_key();
            }
        } else return;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// EXAMINER MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void UI::examiner_menu() {
    while (true) {
        int c = show_submenu("Examiner Management", {
            "View All Examiners", "Add New Examiner", "Search Examiners",
            "Edit Examiner", "Delete Examiner", "Back to Main Menu"
        });
        if (c == 0) {
            auto items = ds_.examiners.get_all_active();
            std::vector<ColDef> cols = {{"ID",8},{"Name",25},{"Title",25},{"Badge",12},{"Email",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& e : items) rows.push_back({std::to_string(e.id), e.name, e.title, e.badge_id, e.email});
            int sel = select_from_list("All Examiners", cols, rows);
            if (sel >= 0) {
                const auto& e = items[sel];
                display_detail("Examiner: " + std::string(e.name), {
                    {"ID", std::to_string(e.id)}, {"Name", e.name}, {"Title", e.title},
                    {"Badge ID", e.badge_id}, {"Phone", e.phone}, {"Email", e.email},
                    {"Certifications", e.certifications}, {"Notes", e.notes},
                    {"Created", format_dt(e.created_at)}, {"Modified", format_dt(e.modified_at)}
                });
            }
        } else if (c == 1) {
            std::vector<FormField> fields = {
                make_text("Name", "Full legal name of the forensic examiner", 63),
                make_text("Title", "Job title, e.g. \"Senior Digital Forensic Analyst\"", 63),
                make_text("Badge ID", "Employee/badge ID number", 31),
                make_text("Phone", "Direct phone number", 31),
                make_text("Email", "Professional email address", 63),
                make_text("Certifications", "Credentials: EnCE, CCE, GCFE, CFCE, ACE, etc. (comma-separated)", 255),
                make_text("Notes", "Specializations, years of experience, court testimony history", 255)
            };
            if (run_form("Add New Examiner", fields)) {
                Examiner e;
                e.set_name(fields[0].value.c_str());
                e.set_title(fields[1].value.c_str());
                e.set_badge_id(fields[2].value.c_str());
                e.set_phone(fields[3].value.c_str());
                e.set_email(fields[4].value.c_str());
                e.set_certifications(fields[5].value.c_str());
                e.set_notes(fields[6].value.c_str());
                if (ds_.examiners.save(e)) {
                    ds_.log_activity(0, e.id, "Examiner Added", e.name);
                    show_status("Examiner saved!");
                } else show_error("Failed to save.");
                wait_key();
            }
        } else if (c == 2) {
            getmaxyx(stdscr, max_y_, max_x_);
            fill_background();
            int bw = std::min(60, max_x_-4), bh = 8;
            int sy = (max_y_-bh)/2, sx = (max_x_-bw)/2;
            draw_box(sy, sx, bh, bw, CP_TITLE);
            draw_title(sy, sx, bw, "Search Examiners", CP_TITLE);
            attron(COLOR_PAIR(CP_TITLE)|A_BOLD); mvprintw(sy+2, sx+3, "Query:"); attroff(COLOR_PAIR(CP_TITLE)|A_BOLD);
            attron(COLOR_PAIR(CP_HINT)); mvprintw(sy+4, sx+3, "Searches name, title, badge, email, certifications"); attroff(COLOR_PAIR(CP_HINT));
            refresh();
            std::string q = input_field(sy+2, sx+12, bw-15);
            if (!q.empty()) {
                auto results = ds_.search_examiners(q);
                std::vector<ColDef> cols = {{"ID",10},{"Name",30},{"Title",30},{"Certifications",25}};
                std::vector<std::vector<std::string>> rows;
                for (const auto& e : results)
                    rows.push_back({std::to_string(e.id), e.name, e.title, e.certifications});
                select_from_list("Search: " + q + " (" + std::to_string(results.size()) + ")", cols, rows);
            }
        } else if (c == 3) {
            auto items = ds_.examiners.get_all_active();
            std::vector<ColDef> cols = {{"ID",10},{"Name",30},{"Title",30},{"Badge",15}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& e : items) rows.push_back({std::to_string(e.id), e.name, e.title, e.badge_id});
            int sel = select_from_list("Select Examiner to Edit", cols, rows);
            if (sel >= 0) {
                Examiner e = items[sel];
                std::vector<FormField> fields = {
                    make_text("Name", "Full legal name", 63, e.name),
                    make_text("Title", "Job title", 63, e.title),
                    make_text("Badge ID", "Employee/badge ID", 31, e.badge_id),
                    make_text("Phone", "Direct phone number", 31, e.phone),
                    make_text("Email", "Professional email", 63, e.email),
                    make_text("Certifications", "Credentials: EnCE, CCE, GCFE, etc.", 255, e.certifications),
                    make_text("Notes", "Specializations, experience, testimony history", 255, e.notes)
                };
                if (run_form("Edit Examiner #" + std::to_string(e.id), fields)) {
                    e.set_name(fields[0].value.c_str()); e.set_title(fields[1].value.c_str());
                    e.set_badge_id(fields[2].value.c_str()); e.set_phone(fields[3].value.c_str());
                    e.set_email(fields[4].value.c_str()); e.set_certifications(fields[5].value.c_str());
                    e.set_notes(fields[6].value.c_str());
                    if (ds_.examiners.update(e)) show_status("Examiner updated!");
                    else show_error("Failed to update.");
                    wait_key();
                }
            }
        } else if (c == 4) {
            auto items = ds_.examiners.get_all_active();
            std::vector<ColDef> cols = {{"ID",10},{"Name",35},{"Title",35},{"Badge",15}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& e : items) rows.push_back({std::to_string(e.id), e.name, e.title, e.badge_id});
            int sel = select_from_list("Select Examiner to Delete", cols, rows);
            if (sel >= 0 && confirm_delete(items[sel].name, items[sel].id)) {
                if (ds_.examiners.remove(items[sel].id)) show_status("Examiner deleted.");
                else show_error("Failed to delete.");
                wait_key();
            }
        } else return;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// CUSTODIAN MANAGEMENT (eDiscovery)
// ════════════════════════════════════════════════════════════════════════════

void UI::custodian_menu() {
    while (true) {
        int c = show_submenu("Custodians (eDiscovery)", {
            "View All Custodians", "Add New Custodian", "Search Custodians",
            "Edit Custodian", "Delete Custodian", "Back to Main Menu"
        });
        if (c == 0) {
            auto items = ds_.custodians.get_all_active();
            std::vector<ColDef> cols = {{"ID",8},{"Name",25},{"Title",20},{"Dept",15},{"Email",20},{"Phone",15}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cu : items) rows.push_back({std::to_string(cu.id), cu.name, cu.title, cu.department, cu.email, cu.phone});
            int sel = select_from_list("All Custodians", cols, rows);
            if (sel >= 0) {
                const auto& cu = items[sel];
                std::string case_title = "(none)";
                auto* cs = ds_.cases.find_by_id(cu.case_id);
                if (cs) case_title = std::string(cs->case_number) + " - " + cs->title;
                display_detail("Custodian: " + std::string(cu.name), {
                    {"ID", std::to_string(cu.id)}, {"Case", case_title},
                    {"Name", cu.name}, {"Title", cu.title}, {"Department", cu.department},
                    {"Email", cu.email}, {"Phone", cu.phone}, {"Notes", cu.notes},
                    {"Created", format_dt(cu.created_at)}, {"Modified", format_dt(cu.modified_at)}
                });
            }
        } else if (c == 1) {
            std::vector<FormField> fields = {
                make_fk(FT_FK_CASE, "Case", "Select the case this custodian is associated with"),
                make_text("Name", "Full name of the data custodian / key person", 63),
                make_text("Title", "Job title at their organization", 63),
                make_text("Department", "Department, e.g. \"IT\", \"Finance\", \"Legal\"", 63),
                make_text("Email", "Email address (may be used for legal hold notices)", 63),
                make_text("Phone", "Direct phone number", 31),
                make_text("Notes", "Data sources held, preservation status, interview notes", 255)
            };
            if (run_form("Add New Custodian", fields)) {
                Custodian cu;
                cu.case_id = fields[0].id_value;
                cu.set_name(fields[1].value.c_str());
                cu.set_title(fields[2].value.c_str());
                cu.set_department(fields[3].value.c_str());
                cu.set_email(fields[4].value.c_str());
                cu.set_phone(fields[5].value.c_str());
                cu.set_notes(fields[6].value.c_str());
                if (ds_.custodians.save(cu)) {
                    ds_.log_activity(cu.case_id, 0, "Custodian Added", cu.name);
                    show_status("Custodian saved!");
                } else show_error("Failed to save.");
                wait_key();
            }
        } else if (c == 2) {
            getmaxyx(stdscr, max_y_, max_x_);
            fill_background();
            int bw = std::min(60, max_x_-4), bh = 8;
            int sy = (max_y_-bh)/2, sx = (max_x_-bw)/2;
            draw_box(sy, sx, bh, bw, CP_TITLE);
            draw_title(sy, sx, bw, "Search Custodians", CP_TITLE);
            attron(COLOR_PAIR(CP_TITLE)|A_BOLD); mvprintw(sy+2, sx+3, "Query:"); attroff(COLOR_PAIR(CP_TITLE)|A_BOLD);
            attron(COLOR_PAIR(CP_HINT)); mvprintw(sy+4, sx+3, "Searches name, title, department, email, notes"); attroff(COLOR_PAIR(CP_HINT));
            refresh();
            std::string q = input_field(sy+2, sx+12, bw-15);
            if (!q.empty()) {
                auto results = ds_.search_custodians(q);
                std::vector<ColDef> cols = {{"ID",10},{"Name",30},{"Title",25},{"Dept",20},{"Email",20}};
                std::vector<std::vector<std::string>> rows;
                for (const auto& cu : results)
                    rows.push_back({std::to_string(cu.id), cu.name, cu.title, cu.department, cu.email});
                select_from_list("Search: " + q + " (" + std::to_string(results.size()) + ")", cols, rows);
            }
        } else if (c == 3) {
            auto items = ds_.custodians.get_all_active();
            std::vector<ColDef> cols = {{"ID",10},{"Name",30},{"Title",25},{"Dept",20},{"Email",20}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cu : items) rows.push_back({std::to_string(cu.id), cu.name, cu.title, cu.department, cu.email});
            int sel = select_from_list("Select Custodian to Edit", cols, rows);
            if (sel >= 0) {
                Custodian cu = items[sel];
                std::string case_disp;
                auto* cs = ds_.cases.find_by_id(cu.case_id);
                if (cs) case_disp = cs->title;
                std::vector<FormField> fields = {
                    make_fk(FT_FK_CASE, "Case", "Select associated case", cu.case_id, case_disp),
                    make_text("Name", "Full name of the custodian", 63, cu.name),
                    make_text("Title", "Job title", 63, cu.title),
                    make_text("Department", "Department", 63, cu.department),
                    make_text("Email", "Email address", 63, cu.email),
                    make_text("Phone", "Phone number", 31, cu.phone),
                    make_text("Notes", "Data sources, preservation status, interview notes", 255, cu.notes)
                };
                if (run_form("Edit Custodian #" + std::to_string(cu.id), fields)) {
                    cu.case_id = fields[0].id_value;
                    cu.set_name(fields[1].value.c_str()); cu.set_title(fields[2].value.c_str());
                    cu.set_department(fields[3].value.c_str()); cu.set_email(fields[4].value.c_str());
                    cu.set_phone(fields[5].value.c_str()); cu.set_notes(fields[6].value.c_str());
                    if (ds_.custodians.update(cu)) show_status("Custodian updated!");
                    else show_error("Failed to update.");
                    wait_key();
                }
            }
        } else if (c == 4) {
            auto items = ds_.custodians.get_all_active();
            std::vector<ColDef> cols = {{"ID",10},{"Name",35},{"Title",25},{"Dept",25}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& cu : items) rows.push_back({std::to_string(cu.id), cu.name, cu.title, cu.department});
            int sel = select_from_list("Select Custodian to Delete", cols, rows);
            if (sel >= 0 && confirm_delete(items[sel].name, items[sel].id)) {
                if (ds_.custodians.remove(items[sel].id)) show_status("Custodian deleted.");
                else show_error("Failed to delete.");
                wait_key();
            }
        } else return;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// ACTIVITY LOG
// ════════════════════════════════════════════════════════════════════════════

void UI::activity_menu() {
    while (true) {
        int c = show_submenu("Activity Log", {
            "View All Activity", "View Activity by Case", "Add Manual Entry",
            "Back to Main Menu"
        });
        if (c == 0) {
            auto items = ds_.activity.get_all_active();
            std::sort(items.begin(), items.end(), [](const ActivityLog& a, const ActivityLog& b){ return a.timestamp > b.timestamp; });
            std::vector<ColDef> cols = {{"ID",6},{"Case",10},{"Action",25},{"Description",35},{"Time",18}};
            std::vector<std::vector<std::string>> rows;
            for (const auto& a : items) {
                std::string cid = a.case_id ? std::to_string(a.case_id) : "-";
                rows.push_back({std::to_string(a.id), cid, a.action, a.description, format_dt(a.timestamp)});
            }
            int sel = select_from_list("All Activity", cols, rows);
            if (sel >= 0) {
                const auto& a = items[sel];
                std::string case_title = "-";
                auto* cs = ds_.cases.find_by_id(a.case_id);
                if (cs) case_title = std::string(cs->case_number) + " - " + cs->title;
                std::string exam_name = "-";
                auto* ex = ds_.examiners.find_by_id(a.examiner_id);
                if (ex) exam_name = ex->name;
                display_detail("Activity Entry #" + std::to_string(a.id), {
                    {"Action", a.action}, {"Description", a.description},
                    {"Case", case_title}, {"Examiner", exam_name},
                    {"Timestamp", format_dt(a.timestamp)}, {"Created", format_dt(a.created_at)}
                });
            }
        } else if (c == 1) {
            uint32_t cid = pick_case();
            if (cid) {
                auto entries = ds_.activity.find_by_fk(&ActivityLog::case_id, cid);
                std::sort(entries.begin(), entries.end(), [](const ActivityLog& a, const ActivityLog& b){ return a.timestamp > b.timestamp; });
                auto* cs = ds_.cases.find_by_id(cid);
                std::string title = "Activity";
                if (cs) title += ": " + std::string(cs->case_number);
                std::vector<ColDef> cols = {{"ID",8},{"Action",25},{"Description",40},{"Time",20}};
                std::vector<std::vector<std::string>> rows;
                for (const auto& a : entries)
                    rows.push_back({std::to_string(a.id), a.action, a.description, format_dt(a.timestamp)});
                select_from_list(title, cols, rows);
            }
        } else if (c == 2) {
            std::vector<FormField> fields = {
                make_fk(FT_FK_CASE, "Case", "Which case this activity relates to"),
                make_fk(FT_FK_EXAMINER, "Examiner", "Examiner who performed the action (optional)"),
                make_text("Action", "What was done, e.g. \"Evidence Acquired\", \"Interview Conducted\"", 127),
                make_text("Description", "Detailed description of the activity", 255)
            };
            if (run_form("Add Activity Entry", fields)) {
                ActivityLog a;
                a.case_id = fields[0].id_value;
                a.examiner_id = fields[1].id_value;
                a.set_action(fields[2].value.c_str());
                a.set_description(fields[3].value.c_str());
                a.timestamp = time(nullptr);
                if (ds_.activity.save(a)) show_status("Activity logged!");
                else show_error("Failed to save.");
                wait_key();
            }
        } else return;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// SEARCH ALL
// ════════════════════════════════════════════════════════════════════════════

void UI::search_all() {
    getmaxyx(stdscr, max_y_, max_x_);
    fill_background();

    int bw = std::max(60, max_x_ * 60 / 100); if (bw > max_x_ - 4) bw = max_x_ - 4; int bh = 8;
    int sy = (max_y_ - bh) / 2, sx = (max_x_ - bw) / 2;
    draw_box(sy, sx, bh, bw, CP_TITLE);
    draw_title(sy, sx, bw, "Search All Entities", CP_TITLE);

    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvprintw(sy + 2, sx + 3, "Query:");
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
    attron(COLOR_PAIR(CP_HINT));
    mvprintw(sy + 4, sx + 3, "Searches clients, cases, evidence, examiners, custodians");
    attroff(COLOR_PAIR(CP_HINT));
    refresh();

    std::string q = input_field(sy + 2, sx + 12, bw - 15);
    if (q.empty()) return;

    // Gather results from all entities
    std::vector<ColDef> cols = {{"Type", 15}, {"ID", 8}, {"Name/Title", 40}, {"Detail", 30}};
    std::vector<std::vector<std::string>> rows;

    for (const auto& c : ds_.search_clients(q))
        rows.push_back({"Client", std::to_string(c.id), c.name, c.company});
    for (const auto& c : ds_.search_cases(q))
        rows.push_back({"Case", std::to_string(c.id), c.title, c.case_number});
    for (const auto& e : ds_.search_evidence(q))
        rows.push_back({"Evidence", std::to_string(e.id), e.description, e.evidence_number});
    for (const auto& e : ds_.search_examiners(q))
        rows.push_back({"Examiner", std::to_string(e.id), e.name, e.title});
    for (const auto& c : ds_.search_custodians(q))
        rows.push_back({"Custodian", std::to_string(c.id), c.name, c.department});

    if (rows.empty()) {
        show_error("No results found for: " + q);
        wait_key();
        return;
    }

    select_from_list("Search: \"" + q + "\" (" + std::to_string(rows.size()) + " results)", cols, rows);
}

// ════════════════════════════════════════════════════════════════════════════
// FILE DETAIL (shared by file manager and hash calculator)
// ════════════════════════════════════════════════════════════════════════════

void UI::show_file_detail(const std::string& filepath) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path fp(filepath);

    auto fsize = fs::file_size(fp, ec);
    std::string size_str = ec ? "(unavailable)" :
        std::to_string(fsize) + " bytes (" + format_file_size(fsize) + ")";

    auto lwt = fs::last_write_time(fp, ec);
    std::string mod_time = ec ? "(unavailable)" : format_file_time(lwt);

    std::string create_time = "(unavailable)";
    std::string access_time = "(unavailable)";
#ifdef _WIN32
    struct _stat64 st;
    if (_stat64(filepath.c_str(), &st) == 0) {
#else
    struct stat st;
    if (::stat(filepath.c_str(), &st) == 0) {
#endif
        create_time = format_dt(st.st_ctime);
        access_time = format_dt(st.st_atime);
    }

    auto status = fs::status(fp, ec);
    std::string perms = "";
    if (!ec) {
        auto p = status.permissions();
        perms += ((p & fs::perms::owner_read) != fs::perms::none) ? "r" : "-";
        perms += ((p & fs::perms::owner_write) != fs::perms::none) ? "w" : "-";
        perms += ((p & fs::perms::owner_exec) != fs::perms::none) ? "x" : "-";
    } else {
        perms = "(unavailable)";
    }

    std::string ext = fp.extension().string();
    std::string ftype = ext.empty() ? "Unknown" : ext;

    show_status("Computing SHA-256 hash... (please wait)");
    refresh();
    std::string sha256_hash = crypto::sha256_file(filepath);

    show_status("Computing MD5 hash... (please wait)");
    refresh();
    std::string md5_hash = crypto::md5_file(filepath);

    display_detail("File: " + fp.filename().string(), {
        {"File Name",   fp.filename().string()},
        {"Full Path",   fp.string()},
        {"File Size",   size_str},
        {"Type",        ftype},
        {"Permissions", perms},
        {"",            ""},
        {"Created",     create_time},
        {"Modified",    mod_time},
        {"Accessed",    access_time},
        {"",            ""},
        {"SHA-256",     sha256_hash},
        {"MD5",         md5_hash}
    });
}

// ════════════════════════════════════════════════════════════════════════════
// HASH CALCULATOR
// ════════════════════════════════════════════════════════════════════════════

void UI::hash_calculator() {
    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        int bw = std::max(70, max_x_ * 70 / 100);
        if (bw > max_x_ - 4) bw = max_x_ - 4;
        int bh = 10;
        int sy = (max_y_ - bh) / 2, sx = (max_x_ - bw) / 2;
        draw_box(sy, sx, bh, bw, CP_TITLE);
        draw_title(sy, sx, bw, "SHA-256 / MD5 Hash Calculator", CP_TITLE);

        attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvprintw(sy + 2, sx + 3, "File Path:");
        attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

        attron(COLOR_PAIR(CP_HINT));
        mvprintw(sy + 4, sx + 3, "Enter the full path to a file to compute its forensic hash");
        mvprintw(sy + 5, sx + 3, "Tip: Use the File Manager (item 9) to browse and hash files");
        attroff(COLOR_PAIR(CP_HINT));

        attron(COLOR_PAIR(CP_HINT));
        mvprintw(sy + 7, sx + 3, "1. Enter Path   2. Browse (File Manager)   ESC. Back");
        attroff(COLOR_PAIR(CP_HINT));

        show_status("Choose an option or press ESC to return");
        refresh();

        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }
        if (ch == 27) return;
        if (ch == '2') { file_manager(); continue; }
        if (ch == '1' || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            refresh();
            std::string path = input_field(sy + 2, sx + 16, bw - 19, "", 500);
            if (path.empty()) continue;

            namespace fs = std::filesystem;
            if (!fs::exists(path)) {
                show_error("File not found: " + path);
                wait_key();
                continue;
            }
            if (fs::is_directory(path)) {
                show_error("Path is a directory. Use File Manager to browse.");
                wait_key();
                continue;
            }
            show_file_detail(path);
        }
    }
}

// ════════════════════════════════════════════════════════════════════════════
// FILE MANAGER
// ════════════════════════════════════════════════════════════════════════════

void UI::file_manager() {
    namespace fs = std::filesystem;

    fs::path current_path = fs::current_path();
    int selected = 0, offset = 0;
    int sort_mode = 0; // 0=name, 1=size, 2=date
    bool sort_asc = true;
    static const char* sort_names[] = {"Name", "Size", "Date"};

    while (true) {
        getmaxyx(stdscr, max_y_, max_x_);
        fill_background();

        // Read directory
        std::vector<FileEntry> entries;
        try {
            for (const auto& entry : fs::directory_iterator(current_path,
                     fs::directory_options::skip_permission_denied)) {
                FileEntry fe;
                fe.name = entry.path().filename().string();
                fe.is_directory = entry.is_directory();
                std::error_code ec;
                fe.size = fe.is_directory ? 0 : fs::file_size(entry.path(), ec);
                fe.last_write = fs::last_write_time(entry.path(), ec);
                fe.last_write_str = format_file_time(fe.last_write);
                fe.size_str = fe.is_directory ? "<DIR>" : format_file_size(fe.size);
                entries.push_back(fe);
            }
        } catch (const fs::filesystem_error&) {
            show_error("Cannot read directory: " + current_path.string());
            wait_key();
            return;
        }

        // Sort: directories first, then by sort_mode
        std::sort(entries.begin(), entries.end(), [&](const FileEntry& a, const FileEntry& b) {
            if (a.is_directory != b.is_directory) return a.is_directory > b.is_directory;
            switch (sort_mode) {
                case 1: return sort_asc ? (a.size < b.size) : (a.size > b.size);
                case 2: return sort_asc ? (a.last_write < b.last_write) : (a.last_write > b.last_write);
                default: {
                    std::string al = a.name, bl = b.name;
                    std::transform(al.begin(), al.end(), al.begin(), ::tolower);
                    std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
                    return sort_asc ? (al < bl) : (al > bl);
                }
            }
        });

        // Insert ".." at top if not at root
        if (current_path.has_parent_path() && current_path != current_path.root_path()) {
            FileEntry up;
            up.name = "..";
            up.is_directory = true;
            up.size = 0;
            up.size_str = "<UP>";
            up.last_write_str = "";
            entries.insert(entries.begin(), up);
        }

        // Clamp selection
        if (selected >= (int)entries.size()) selected = std::max(0, (int)entries.size() - 1);
        if (selected < 0) selected = 0;

        // Title bar
        attron(COLOR_PAIR(CP_STATUS));
        mvhline(0, 0, ' ', max_x_);
        mvprintw(0, 2, "EBOXLAB - File Manager");
        attroff(COLOR_PAIR(CP_STATUS));

        // Path bar
        attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvhline(1, 0, ' ', max_x_);
        std::string pathstr = current_path.string();
        mvprintw(1, 2, "Path: %.*s", max_x_ - 10, pathstr.c_str());
        attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

        // Sort info
        attron(COLOR_PAIR(CP_HINT));
        mvhline(2, 0, ' ', max_x_);
        mvprintw(2, 2, "Sort: %s %s | F5: Name | F6: Size | F7: Date | F8: Asc/Desc",
                 sort_names[sort_mode], sort_asc ? "ASC" : "DESC");
        attroff(COLOR_PAIR(CP_HINT));

        // Column headers
        int name_w = max_x_ - 32;
        if (name_w < 10) name_w = 10;
        attron(COLOR_PAIR(CP_STATUS));
        mvhline(3, 0, ' ', max_x_);
        mvprintw(3, 2, "%-*s %10s  %16s", name_w, "Name", "Size", "Modified");
        attroff(COLOR_PAIR(CP_STATUS));

        // File list
        int data_start = 4;
        int visible = max_y_ - data_start - 2;
        if (visible < 1) visible = 1;

        if (selected < offset) offset = selected;
        if (selected >= offset + visible) offset = selected - visible + 1;

        for (int i = 0; i < visible && (i + offset) < (int)entries.size(); i++) {
            int idx = i + offset;
            int row = data_start + i;

            if (idx == selected) attron(COLOR_PAIR(CP_HIGHLIGHT));
            else if (entries[idx].is_directory) attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
            else attron(COLOR_PAIR(CP_NORMAL));

            mvhline(row, 0, ' ', max_x_);

            std::string dname = entries[idx].name;
            if (entries[idx].is_directory && dname != "..")
                dname = "[" + dname + "]";

            mvprintw(row, 2, "%-*.*s %10s  %16s",
                     name_w, name_w, dname.c_str(),
                     entries[idx].size_str.c_str(),
                     entries[idx].last_write_str.c_str());

            if (idx == selected) attroff(COLOR_PAIR(CP_HIGHLIGHT));
            else if (entries[idx].is_directory) attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
            else attroff(COLOR_PAIR(CP_NORMAL));
        }

        // Footer
        attron(COLOR_PAIR(CP_TITLE));
        mvhline(max_y_ - 2, 0, ' ', max_x_);
        mvprintw(max_y_ - 2, 2, "%d items | Showing %d-%d",
                 (int)entries.size(), offset + 1,
                 std::min(offset + visible, (int)entries.size()));
        attroff(COLOR_PAIR(CP_TITLE));

        show_status("ENTER: Open | BS: Up | F5-F7: Sort | F9: Hash | D: Drive | ESC: Back");
        refresh();

        // Input
        int ch = getch();
        if (ch == KEY_RESIZE) { handle_resize(); continue; }

        if (ch == KEY_MOUSE) {
            MEVENT mev;
            if (nc_getmouse(&mev) == OK) {
                if (mev.bstate & BUTTON1_CLICKED) {
                    int ri = mev.y - data_start;
                    if (ri >= 0 && ri < visible) {
                        int actual = ri + offset;
                        if (actual < (int)entries.size()) selected = actual;
                    }
                }
                if (mev.bstate & BUTTON1_DOUBLE_CLICKED) {
                    int ri = mev.y - data_start;
                    if (ri >= 0 && ri < visible) {
                        int actual = ri + offset;
                        if (actual < (int)entries.size()) {
                            if (entries[actual].is_directory) {
                                if (entries[actual].name == "..")
                                    current_path = current_path.parent_path();
                                else
                                    current_path = current_path / entries[actual].name;
                                selected = 0; offset = 0;
                            } else {
                                show_file_detail((current_path / entries[actual].name).string());
                            }
                        }
                    }
                }
                if (mev.bstate & BUTTON4_PRESSED && selected > 0) selected--;
                if (mev.bstate & BUTTON5_PRESSED && selected < (int)entries.size() - 1) selected++;
            }
            continue;
        }

        if (ch == KEY_UP && selected > 0) {
            selected--;
        } else if (ch == KEY_DOWN && selected < (int)entries.size() - 1) {
            selected++;
        } else if (ch == KEY_PPAGE) {
            selected -= visible;
            if (selected < 0) selected = 0;
        } else if (ch == KEY_NPAGE) {
            selected += visible;
            if (selected >= (int)entries.size()) selected = (int)entries.size() - 1;
        } else if (ch == KEY_HOME) {
            selected = 0; offset = 0;
        } else if (ch == KEY_END) {
            selected = (int)entries.size() - 1;
        } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            if (!entries.empty()) {
                if (entries[selected].is_directory) {
                    if (entries[selected].name == "..")
                        current_path = current_path.parent_path();
                    else
                        current_path = current_path / entries[selected].name;
                    selected = 0; offset = 0;
                } else {
                    show_file_detail((current_path / entries[selected].name).string());
                }
            }
        } else if (ch == KEY_BACKSPACE || ch == 8 || ch == 127) {
            if (current_path.has_parent_path() && current_path != current_path.root_path()) {
                current_path = current_path.parent_path();
                selected = 0; offset = 0;
            }
        } else if (ch == KEY_F(5)) {
            sort_mode = 0;
        } else if (ch == KEY_F(6)) {
            sort_mode = 1;
        } else if (ch == KEY_F(7)) {
            sort_mode = 2;
        } else if (ch == KEY_F(8)) {
            sort_asc = !sort_asc;
        } else if (ch == KEY_F(9)) {
            if (!entries.empty() && !entries[selected].is_directory) {
                show_file_detail((current_path / entries[selected].name).string());
            }
        } else if (ch == 'd' || ch == 'D') {
            std::vector<std::string> drives;
#ifdef _WIN32
            for (char d = 'A'; d <= 'Z'; d++) {
                std::string dp = std::string(1, d) + ":\\";
                if (fs::exists(dp)) drives.push_back(dp);
            }
#else
            // Unix: show common mount points
            for (const auto& mp : {"/", "/home", "/tmp", "/Volumes", "/mnt", "/media"}) {
                if (fs::exists(mp)) drives.push_back(mp);
            }
#endif
            if (!drives.empty()) {
                int sel = show_submenu("Select Drive", drives);
                if (sel >= 0 && sel < (int)drives.size()) {
                    current_path = drives[sel];
                    selected = 0; offset = 0;
                }
            }
        } else if (ch == 27) {
            return;
        }
    }
}
