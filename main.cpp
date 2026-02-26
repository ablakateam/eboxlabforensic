#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <shlobj.h>
    #undef MOUSE_MOVED
#else
    #include <cstdlib>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <pwd.h>
#endif

#include "datastore.h"
#include "ui.h"
#include <string>
#include <filesystem>

static std::string get_data_dir() {
#ifdef _WIN32
    // Windows: %APPDATA%\EBOXLAB
    char path[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path) == S_OK) {
        std::string dir = std::string(path) + "\\EBOXLAB\\";
        std::filesystem::create_directories(dir);
        return dir;
    }
#else
    // macOS/Linux: ~/Library/Application Support/EBOXLAB or ~/.local/share/EBOXLAB
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (home) {
#ifdef __APPLE__
        std::string dir = std::string(home) + "/Library/Application Support/EBOXLAB/";
#else
        std::string dir = std::string(home) + "/.local/share/EBOXLAB/";
#endif
        std::filesystem::create_directories(dir);
        return dir;
    }
#endif
    // Fallback: current directory
    return "";
}

static void setup_console() {
#ifdef _WIN32
    // Set a large console font for better readability
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(hOut, FALSE, &cfi);
    cfi.dwFontSize.X = 0;   // auto width
    cfi.dwFontSize.Y = 28;  // 28px tall — big, readable DOS-style
    cfi.FontWeight = FW_BOLD;
    wcscpy(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);

    // Maximize the console window to fill the screen
    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        ShowWindow(hwnd, SW_MAXIMIZE);
        Sleep(150);
    }
#endif
    // macOS/Linux: terminal font and size are controlled by the terminal emulator
}

int main() {
    setup_console();

    std::string data_dir = get_data_dir();
    DataStore ds(data_dir);
    ds.load_all();
    if (!ds.has_data()) {
        ds.seed_demo_data();
    }

    UI ui(ds);
    ui.run();

    return 0;
}
