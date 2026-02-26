#ifndef UI_H
#define UI_H

#ifdef _WIN32
    #include <curses.h>      // PDCurses on Windows
#else
    #include <ncurses.h>     // ncurses on macOS/Linux
#endif
#include <string>
#include <vector>
#include <utility>
#include "datastore.h"

// Color pairs
#define CP_NORMAL    1   // White on blue
#define CP_TITLE     2   // Yellow on blue
#define CP_HIGHLIGHT 3   // Black on cyan
#define CP_INPUT     4   // White on black
#define CP_STATUS    5   // Black on white
#define CP_ERROR     6   // Red on blue
#define CP_HINT      7   // Cyan on blue

// Form field types
enum FieldType {
    FT_TEXT,
    FT_ENUM,
    FT_FK_CLIENT,
    FT_FK_CASE,
    FT_FK_EVIDENCE,
    FT_FK_EXAMINER,
    FT_READONLY
};

struct FormField {
    const char* label;
    const char* hint;
    int max_len;
    FieldType type;
    std::string value;
    uint32_t id_value;
    int enum_value;
    const char** enum_labels;
    int enum_count;

    FormField() : label(""), hint(""), max_len(0), type(FT_TEXT),
                  id_value(0), enum_value(0), enum_labels(nullptr), enum_count(0) {}
};

struct ColDef {
    const char* header;
    int width_pct;
};

class UI {
public:
    UI(DataStore& ds);
    ~UI();
    void run();

private:
    DataStore& ds_;
    int max_y_, max_x_;
    int theme_;  // 0 = blue, 1 = retro green

    void apply_theme();
    void fill_background();
    void handle_resize();
    void show_status(const std::string& msg);
    void show_error(const std::string& msg);
    void wait_key();

    std::string input_field(int y, int x, int width, const std::string& existing = "", int max_chars = 0);
    std::string edit_text_area(const std::string& title, const std::string& existing, int max_chars);

    // Generic components
    void draw_main_menu(int selected = 0);
    int show_submenu(const std::string& title, const std::vector<std::string>& options);
    int select_from_list(const std::string& title, const std::vector<ColDef>& cols,
                         const std::vector<std::vector<std::string>>& rows);
    bool run_form(const std::string& title, std::vector<FormField>& fields);
    void display_detail(const std::string& title,
                        const std::vector<std::pair<std::string,std::string>>& fields);
    bool confirm_delete(const std::string& what, uint32_t id);
    int select_enum(const char** labels, int count, const std::string& title, int current = 0);

    // FK pickers
    uint32_t pick_client();
    uint32_t pick_case();
    uint32_t pick_evidence(uint32_t case_id = 0);
    uint32_t pick_examiner();

    // Entity management
    void client_menu();
    void case_menu();
    void evidence_menu();
    void custody_menu();
    void examiner_menu();
    void custodian_menu();
    void activity_menu();
    void search_all();

    // Tools
    void file_manager();
    void hash_calculator();
    void show_file_detail(const std::string& filepath);
};

#endif
