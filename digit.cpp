// ============================================================
//  Digit Tracker v1.1
//  Swertres 3D Lotto Analyzer
//  Cross-platform: Windows (MinGW) + Linux / Android Termux
//
//  Windows build:
//    g++ -std=c++17 -O2 -o digit.exe digit.cpp -lwinhttp
//
//  Linux / Termux build:
//    pkg install clang libcurl   (Termux)
//    apt install g++ libcurl4-openssl-dev  (Debian/Ubuntu)
//    g++ -std=c++17 -O2 -o digit digit.cpp -lcurl
//
//  Changelog v1.1:
//    - New DIGIT TRACKER block-letter logo
//    - S-R-W: filter view (All/Strong/Random/Weak) + top-5 highlight
//    - Last Digit: shows repeating-digit combos in the gap window
//    - Import: file browser (GUI/list) + terminal path input
//    - Insert/Edit and Delete removed
//    - Local + Online auto-mixed: internet silently syncs latest draws
//    - Last Digit window = 1000 draws; N input is free text (1-999)
//    - Digit input accepts nodash format: "592" -> "5-9-2"
// ============================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <cmath>

// ── Platform detection ───────────────────────────────────
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <conio.h>
#include <commdlg.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comdlg32.lib")
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <curl/curl.h>
#endif

// ════════════════════════════════════════════════════
//  CONSOLE HELPERS
// ════════════════════════════════════════════════════
enum Color
{
    BLACK = 0,
    DARK_BLUE = 1,
    DARK_GREEN = 2,
    DARK_CYAN = 3,
    DARK_RED = 4,
    DARK_MAG = 5,
    DARK_YEL = 6,
    LGRAY = 7,
    DGRAY = 8,
    BLUE = 9,
    GREEN = 10,
    CYAN = 11,
    RED = 12,
    MAGENTA = 13,
    YELLOW = 14,
    WHITE = 15
};
struct Pos
{
    int X, Y;
};

#ifdef _WIN32

HANDLE hCon;
void sc(int fg, int bg = 0) { SetConsoleTextAttribute(hCon, (WORD)(bg << 4 | fg)); }
void rc() { sc(WHITE, BLACK); }
void gotoxy(int x, int y)
{
    COORD c;
    c.X = (SHORT)x;
    c.Y = (SHORT)y;
    SetConsoleCursorPosition(hCon, c);
}
Pos gpos()
{
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(hCon, &i);
    Pos p;
    p.X = i.dwCursorPosition.X;
    p.Y = i.dwCursorPosition.Y;
    return p;
}
int CW()
{
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(hCon, &i);
    return i.srWindow.Right - i.srWindow.Left + 1;
}
int CH()
{
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(hCon, &i);
    return i.srWindow.Bottom - i.srWindow.Top + 1;
}
void cls()
{
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(hCon, &i);
    DWORD n = (DWORD)(i.dwSize.X * i.dwSize.Y), w;
    COORD h = {0, 0};
    FillConsoleOutputCharacter(hCon, ' ', n, h, &w);
    FillConsoleOutputAttribute(hCon, i.wAttributes, n, h, &w);
    SetConsoleCursorPosition(hCon, h);
}
void clrLine(int y)
{
    COORD c = {0, (SHORT)y};
    DWORD w;
    FillConsoleOutputCharacter(hCon, ' ', (DWORD)CW(), c, &w);
    SetConsoleCursorPosition(hCon, c);
}
void cur(bool s)
{
    CONSOLE_CURSOR_INFO i;
    GetConsoleCursorInfo(hCon, &i);
    i.bVisible = s;
    SetConsoleCursorInfo(hCon, &i);
}
void ms(int t) { Sleep((DWORD)t); }
int rk()
{
    int c = _getch();
    if (c == 0 || c == 224)
    {
        int c2 = _getch();
        return c2 + 256;
    }
    return c;
}

#else // Linux / Termux ─────────────────────────────

static const char *ANSI_FG[] = {
    "\033[30m", "\033[34m", "\033[32m", "\033[36m", "\033[31m", "\033[35m",
    "\033[33m", "\033[37m", "\033[90m", "\033[94m", "\033[92m", "\033[96m",
    "\033[91m", "\033[95m", "\033[93m", "\033[97m"};
void sc(int fg, int bg = 0)
{
    (void)bg;
    if (fg >= 0 && fg < 16)
        std::cout << ANSI_FG[fg];
}
void rc() { std::cout << "\033[0m"; }
void gotoxy(int x, int y) { std::cout << "\033[" << (y + 1) << ";" << (x + 1) << "H" << std::flush; }
Pos gpos()
{
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::cout << "\033[6n" << std::flush;
    int row = 0, col = 0;
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != '\033')
        ;
    read(STDIN_FILENO, &c, 1);
    while (read(STDIN_FILENO, &c, 1) == 1 && c != ';')
    {
        if (c >= '0' && c <= '9')
            row = row * 10 + (c - '0');
    }
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'R')
    {
        if (c >= '0' && c <= '9')
            col = col * 10 + (c - '0');
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    Pos p;
    p.X = col - 1;
    p.Y = row - 1;
    return p;
}
int CW()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col > 0 ? w.ws_col : 80;
}
int CH()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row > 0 ? w.ws_row : 24;
}
void cls() { std::cout << "\033[H\033[2J" << std::flush; }
void clrLine(int y)
{
    gotoxy(0, y);
    int w = CW();
    for (int i = 0; i < w; i++)
        std::cout << ' ';
    gotoxy(0, y);
    std::cout << std::flush;
}
void cur(bool s) { std::cout << (s ? "\033[?25h" : "\033[?25l") << std::flush; }
void ms(int t) { usleep((useconds_t)t * 1000); }
int rk()
{
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    unsigned char c = 0;
    read(STDIN_FILENO, &c, 1);
    if (c == 27)
    {
        unsigned char c2 = 0, c3 = 0;
        newt.c_cc[VMIN] = 0;
        newt.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        if (read(STDIN_FILENO, &c2, 1) == 1 && c2 == '[')
        {
            if (read(STDIN_FILENO, &c3, 1) == 1)
            {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                if (c3 == 'A')
                    return 72 + 256;
                if (c3 == 'B')
                    return 80 + 256;
                if (c3 == 'C')
                    return 77 + 256;
                if (c3 == 'D')
                    return 75 + 256;
            }
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return 27;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return (int)c;
}

#endif // platform

#define KE 13
#define KX 27

void cprt(const std::string &s, int col = WHITE)
{
    int w = CW();
    int p = (w - (int)s.size()) / 2;
    if (p > 0)
        std::cout << std::string(p, ' ');
    sc(col);
    std::cout << s;
    rc();
}
void hl(char c, int col = DGRAY)
{
    int w = CW();
    sc(col);
    for (int i = 0; i < w; i++)
        std::cout << c;
    rc();
    std::cout << "\n";
}

// ════════════════════════════════════════════════════
//  LOADING ANIMATIONS
// ════════════════════════════════════════════════════
void showLoad(const std::string &msg, int ms2 = 1600)
{
    cur(false);
    const char *fr[] = {"[|]", "[/]", "[-]", "[\\]", "[|]", "[/]", "[-]", "[\\]"};
    int steps = ms2 / 100;
    if (steps < 1)
        steps = 1;
    int row = gpos().Y;
    for (int i = 0; i <= steps; i++)
    {
        int pct = (i * 100) / steps, fill = (i * 30) / steps;
        gotoxy(2, row);
        sc(CYAN);
        std::cout << fr[i % 8];
        sc(WHITE);
        std::cout << "  " << msg << "  [";
        sc(CYAN);
        for (int b = 0; b < 30; b++)
            std::cout << (b < fill ? '#' : '.');
        sc(WHITE);
        std::cout << "] ";
        sc(YELLOW);
        std::cout << std::setw(3) << pct << "%  ";
        rc();
        std::cout << std::flush;
        ms(100);
    }
    clrLine(row);
    cur(true);
}
void pulse(const std::string &msg, int cyc = 2)
{
    cur(false);
    std::cout << "\n";
    const char *b[] = {"_", ".", "o", "O", "0", "O", "o", "."};
    int row = gpos().Y;
    for (int c = 0; c < cyc; c++)
        for (int f = 0; f < 8; f++)
        {
            gotoxy(2, row);
            sc(CYAN);
            for (int i = 0; i < 8; i++)
                std::cout << b[(f + i) % 8];
            sc(WHITE);
            std::cout << "  " << msg << "   ";
            rc();
            std::cout << std::flush;
            ms(80);
        }
    clrLine(row);
    std::cout << "\n";
    cur(true);
}

// ════════════════════════════════════════════════════
//  DATABASE
// ════════════════════════════════════════════════════
const std::string DB = "digit_data.txt";
const std::vector<std::string> VT = {"2pm", "5pm", "9pm"};
struct Entry
{
    std::string date, time, digit;
};
struct DrawRow
{
    std::string date, pm2, pm5, pm9;
};

std::string trimS(std::string s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.pop_back();
    return s;
}
std::string normalizeDigit(const std::string &raw)
{
    std::string s = trimS(raw);
    std::vector<char> digits;
    for (char c : s)
        if (isdigit(c))
            digits.push_back(c);
    if (digits.size() != 3)
        return "";
    std::string r;
    r += digits[0];
    r += '-';
    r += digits[1];
    r += '-';
    r += digits[2];
    return r;
}
std::vector<Entry> loadDB()
{
    std::vector<Entry> v;
    std::ifstream f(DB);
    if (!f)
        return v;
    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty())
            continue;
        std::istringstream ss(line);
        Entry e;
        std::getline(ss, e.date, '|');
        std::getline(ss, e.time, '|');
        std::getline(ss, e.digit, '|');
        e.date = trimS(e.date);
        e.time = trimS(e.time);
        e.digit = trimS(e.digit);
        if (!e.date.empty() && !e.time.empty() && !e.digit.empty())
            v.push_back(e);
    }
    return v;
}
void saveDB(const std::vector<Entry> &v)
{
    std::ofstream f(DB, std::ios::trunc);
    for (const auto &e : v)
        f << e.date << "|" << e.time << "|" << e.digit << "\n";
}
bool vtm(const std::string &t)
{
    for (auto &x : VT)
        if (x == t)
            return true;
    return false;
}
std::string inp(const std::string &p, int pc = WHITE, int ic = YELLOW)
{
    sc(pc);
    std::cout << p;
    sc(ic);
    std::string s;
    cur(true);
#ifdef _WIN32
    std::getline(std::cin, s);
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::getline(std::cin, s);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    rc();
    return trimS(s);
}
std::string inpDigit(const std::string &p)
{
    while (true)
    {
        std::string raw = inp(p);
        if (raw.empty())
        {
            sc(RED);
            std::cout << "  Digit cannot be empty.\n";
            rc();
            continue;
        }
        std::string norm = normalizeDigit(raw);
        if (norm.empty())
        {
            sc(RED);
            std::cout << "  Invalid digit. Use: 5-9-2 or 592\n";
            rc();
            continue;
        }
        if (norm != raw)
        {
            sc(DGRAY);
            std::cout << "  -> ";
            sc(YELLOW);
            std::cout << norm;
            rc();
            std::cout << "\n";
        }
        return norm;
    }
}

// ════════════════════════════════════════════════════
//  NETWORK
// ════════════════════════════════════════════════════
#ifdef _WIN32

std::string fetchPageW(const std::wstring &host, const std::wstring &path)
{
    std::string result;
    HINTERNET hS = WinHttpOpen(L"DigitTracker/1.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hS)
        return "";
    HINTERNET hC = WinHttpConnect(hS, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hC)
    {
        WinHttpCloseHandle(hS);
        return "";
    }
    HINTERNET hR = WinHttpOpenRequest(hC, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hR)
    {
        WinHttpCloseHandle(hC);
        WinHttpCloseHandle(hS);
        return "";
    }
    DWORD to = 12000;
    WinHttpSetOption(hR, WINHTTP_OPTION_CONNECT_TIMEOUT, &to, sizeof(to));
    WinHttpSetOption(hR, WINHTTP_OPTION_RECEIVE_TIMEOUT, &to, sizeof(to));
    if (!WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hR, NULL))
    {
        WinHttpCloseHandle(hR);
        WinHttpCloseHandle(hC);
        WinHttpCloseHandle(hS);
        return "";
    }
    DWORD sz = 0;
    char buf[8192];
    do
    {
        sz = 0;
        WinHttpQueryDataAvailable(hR, &sz);
        if (sz == 0)
            break;
        if (sz > sizeof(buf) - 1)
            sz = sizeof(buf) - 1;
        DWORD dl = 0;
        WinHttpReadData(hR, buf, sz, &dl);
        buf[dl] = '\0';
        result += buf;
    } while (sz > 0);
    WinHttpCloseHandle(hR);
    WinHttpCloseHandle(hC);
    WinHttpCloseHandle(hS);
    return result;
}
bool isOnline()
{
    HINTERNET hS = WinHttpOpen(L"DigitTracker/1.1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hS)
        return false;
    HINTERNET hC = WinHttpConnect(hS, L"www.lottopcso.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hC)
    {
        WinHttpCloseHandle(hS);
        return false;
    }
    HINTERNET hR = WinHttpOpenRequest(hC, L"HEAD", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    DWORD to = 5000;
    WinHttpSetOption(hR, WINHTTP_OPTION_CONNECT_TIMEOUT, &to, sizeof(to));
    WinHttpSetOption(hR, WINHTTP_OPTION_RECEIVE_TIMEOUT, &to, sizeof(to));
    bool ok = hR && WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hR, NULL);
    if (hR)
        WinHttpCloseHandle(hR);
    WinHttpCloseHandle(hC);
    WinHttpCloseHandle(hS);
    return ok;
}
std::string fetchPageForYear(int yr, int cy)
{
    std::wstring path;
    if (yr == cy)
        path = L"/swertres-results-today-history-and-summary/";
    else
    {
        std::wstring ys = std::to_wstring(yr);
        path = L"/swertres-results-today-history-and-summary-" + ys + L"/";
    }
    return fetchPageW(L"www.lottopcso.com", path);
}
int getCurrentYear()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    return st.wYear;
}

#else

static size_t curlWrite(void *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append((char *)ptr, size * nmemb);
    return size * nmemb;
}
std::string fetchPageCurl(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return "";
    std::string result;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DigitTracker/1.1");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK) ? result : "";
}
bool isOnline()
{
    CURL *curl = curl_easy_init();
    if (!curl)
        return false;
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.lottopcso.com/");
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}
std::string fetchPageForYear(int yr, int cy)
{
    std::string url;
    if (yr == cy)
        url = "https://www.lottopcso.com/swertres-results-today-history-and-summary/";
    else
        url = "https://www.lottopcso.com/swertres-results-today-history-and-summary-" + std::to_string(yr) + "/";
    return fetchPageCurl(url);
}
int getCurrentYear()
{
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    return t->tm_year + 1900;
}

#endif

// ── HTML parsing ─────────────────────────────────────
std::string stripTags(const std::string &s)
{
    std::string r;
    bool in = false;
    for (size_t i = 0; i < s.size();)
    {
        unsigned char c = (unsigned char)s[i];
        if (c == '<')
        {
            in = true;
            i++;
            continue;
        }
        if (c == '>')
        {
            in = false;
            i++;
            continue;
        }
        if (in)
        {
            i++;
            continue;
        }
        if (c == 0xC2 && i + 1 < s.size() && (unsigned char)s[i + 1] == 0xA0)
        {
            r += ' ';
            i += 2;
            continue;
        }
        if (c == 0xE2 && i + 2 < s.size() && (unsigned char)s[i + 1] == 0x80)
        {
            unsigned char t2 = (unsigned char)s[i + 2];
            if (t2 == 0x93 || t2 == 0x94)
            {
                r += '-';
                i += 3;
                continue;
            }
        }
        r += s[i];
        i++;
    }
    while (!r.empty() && (r.front() == ' ' || r.front() == '\n' || r.front() == '\r' || r.front() == '\t'))
        r.erase(r.begin());
    while (!r.empty() && (r.back() == ' ' || r.back() == '\n' || r.back() == '\r' || r.back() == '\t'))
        r.pop_back();
    return r;
}
std::vector<DrawRow> parseHTML(const std::string &html)
{
    std::vector<DrawRow> rows;
    size_t pos = 0;
    while (true)
    {
        size_t tr = html.find("<tr>", pos);
        if (tr == std::string::npos)
            break;
        size_t trd = html.find("</tr>", tr);
        if (trd == std::string::npos)
            break;
        std::string row = html.substr(tr, trd - tr);
        std::vector<std::string> cells;
        size_t cp = 0;
        while (true)
        {
            size_t td = row.find("<td", cp);
            if (td == std::string::npos)
                break;
            size_t tde = row.find("</td>", td);
            if (tde == std::string::npos)
                break;
            size_t gt = row.find('>', td);
            std::string cell;
            if (gt != std::string::npos && gt < tde)
                cell = stripTags(row.substr(gt + 1, tde - gt - 1));
            cells.push_back(cell);
            cp = tde + 5;
        }
        if (cells.size() >= 4)
        {
            DrawRow dr;
            dr.date = cells[0];
            dr.pm2 = cells[1];
            dr.pm5 = cells[2];
            dr.pm9 = cells[3];
            if (!dr.date.empty() && dr.date.find("20") != std::string::npos)
                rows.push_back(dr);
        }
        pos = trd + 5;
    }
    return rows;
}
bool validCell(const std::string &s)
{
    if (s.empty())
        return false;
    for (char c : s)
        if (c != '-' && c != ' ')
            return true;
    return false;
}
std::vector<Entry> rowsToEntries(const std::vector<DrawRow> &rows)
{
    std::vector<Entry> v;
    for (const auto &r : rows)
    {
        if (validCell(r.pm2))
            v.push_back({r.date, "2pm", trimS(r.pm2)});
        if (validCell(r.pm5))
            v.push_back({r.date, "5pm", trimS(r.pm5)});
        if (validCell(r.pm9))
            v.push_back({r.date, "9pm", trimS(r.pm9)});
    }
    return v;
}
std::vector<Entry> fetchYear(int yr, int cy)
{
    std::string html = fetchPageForYear(yr, cy);
    if (html.empty())
        return {};
    return rowsToEntries(parseHTML(html));
}
int mergeIntoDB(const std::vector<Entry> &newEntries)
{
    auto existing = loadDB();
    std::set<std::string> keys;
    for (const auto &e : existing)
        keys.insert(e.date + "|" + e.time + "|" + e.digit);
    int added = 0;
    std::vector<Entry> merged = existing;
    for (const auto &e : newEntries)
    {
        std::string k = e.date + "|" + e.time + "|" + e.digit;
        if (keys.find(k) == keys.end())
        {
            merged.push_back(e);
            keys.insert(k);
            added++;
        }
    }
    saveDB(merged);
    return added;
}

// ── Auto-sync ────────────────────────────────────────
int tryAutoSync(bool showStatus = true)
{
    if (!isOnline())
        return 0;
    int cy = getCurrentYear();
    auto entries = fetchYear(cy, cy);
    if (entries.empty())
        return 0;
    int added = mergeIntoDB(entries);
    if (added > 0 && showStatus)
    {
        sc(GREEN);
        std::cout << "  [+]";
        sc(DGRAY);
        std::cout << " " << added << " new record" << (added == 1 ? "" : "s") << " synced\n";
        rc();
    }
    return added;
}

// ── Date helpers ─────────────────────────────────────
int yearFromLabel(const std::string &d)
{
    int yr = 0;
    for (int i = 0; i + 3 < (int)d.size(); i++)
        if (isdigit(d[i]) && isdigit(d[i + 1]) && isdigit(d[i + 2]) && isdigit(d[i + 3]))
        {
            int y = std::stoi(d.substr(i, 4));
            if (y > 1900 && y < 2100)
                yr = y;
        }
    return yr;
}
static const char *MON_IDX[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
int dateToInt(const std::string &d)
{
    int yr = yearFromLabel(d);
    if (yr == 0)
        return 0;
    int mon = 0;
    for (int m = 0; m < 12; m++)
        if (d.find(MON_IDX[m]) != std::string::npos)
        {
            mon = m + 1;
            break;
        }
    int day = 0;
    for (size_t i = 0; i < d.size(); i++)
        if (isdigit(d[i]))
        {
            int num = 0;
            size_t j = i;
            while (j < d.size() && isdigit(d[j]))
            {
                num = num * 10 + (d[j] - '0');
                j++;
            }
            if (num >= 1 && num <= 31)
            {
                day = num;
                break;
            }
            i = j - 1;
        }
    return yr * 10000 + mon * 100 + day;
}

// ════════════════════════════════════════════════════
//  INTRO — new block-letter logo (UTF-8)
// ════════════════════════════════════════════════════
void showIntro()
{
    cls();
    cur(false);
    int H = CH();
    ms(150);

    // Block letter logo lines — DIGIT on top, TRACKER below
    // Each line stored as UTF-8 string literal
    std::vector<std::pair<std::string, int>> art = {
        {"\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97      v1.1", CYAN},
        {"\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d", CYAN},
        {"\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91", CYAN},
        {"\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91", CYAN},
        {"\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91", CYAN},
        {"\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d   \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d", DGRAY},
        {"", WHITE},
        {"\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97", MAGENTA},
        {"\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97", MAGENTA},
        {"   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91     \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d  \xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x9d", MAGENTA},
        {"   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91     \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97 \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97", MAGENTA},
        {"   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91   \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x95\x9a\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x97\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91\xe2\x96\x88\xe2\x96\x88\xe2\x95\x91  \xe2\x96\x88\xe2\x96\x88\xe2\x95\x91", MAGENTA},
        {"   \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d   \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d  \xe2\x95\x9a\xe2\x95\x90\xe2\x95\x9d", DGRAY},
    };

    // For centering: use a fixed visual width reference (longest visible line ~60 chars wide visually)
    // The block chars are multi-byte but visually 1 wide each — use a fixed 62 as reference
    int refW = 62;
    int start = (H - (int)art.size() - 9) / 2;
    if (start < 1)
        start = 1;
    int w = CW();
    for (int i = 0; i < (int)art.size(); i++)
    {
        gotoxy(0, start + i);
        clrLine(start + i);
        if (!art[i].first.empty())
        {
            int pad = (w - refW) / 2;
            if (pad < 0)
                pad = 0;
            std::cout << std::string(pad, ' ');
            sc(art[i].second);
            std::cout << art[i].first;
            rc();
        }
        std::cout << std::flush;
        ms(40);
    }
    ms(300);
    int br = start + (int)art.size();
    gotoxy(0, br + 1);
    cprt("═══════════════════════════════════════════════════════════════", DGRAY);
    gotoxy(0, br + 2);
    cprt("  Local + Online  |  Swertres 3D  |  Auto-Sync  ", DGRAY);
    gotoxy(0, br + 3);
    cprt("═══════════════════════════════════════════════════════════════", DGRAY);
    ms(300);
    gotoxy(0, br + 5);
    cprt("  >> 3D Lotto  2PM  5PM  9PM  |  lottopcso.com  <<  ", GREEN);
    std::cout << "\n";
    gotoxy(2, br + 7);
    showLoad("Initializing System", 1400);
    gotoxy(0, br + 8);
    int pad2 = (w - 32) / 2;
    if (pad2 < 0)
        pad2 = 0;
    std::cout << std::string(pad2, ' ');
    sc(WHITE);
    std::cout << "Press ";
    sc(YELLOW);
    std::cout << "[ENTER]";
    sc(WHITE);
    std::cout << " to begin...";
    rc();
    cur(true);
#ifdef _WIN32
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#else
    tcflush(STDIN_FILENO, TCIFLUSH);
#endif
    while (true)
    {
        int k = rk();
        if (k == KE || k == '\r' || k == '\n')
            break;
    }
}

// ════════════════════════════════════════════════════
//  CONFIRM DIALOG
// ════════════════════════════════════════════════════
bool confirmDlg(const std::string &action)
{
    cur(false);
    int sel = 0, sr = gpos().Y;
    auto draw = [&]()
    {
        gotoxy(0, sr);
        sc(DGRAY);
        for (int i = 0; i < CW(); i++)
            std::cout << '-';
        rc();
        clrLine(sr + 1);
        gotoxy(0, sr + 1);
        sc(WHITE);
        std::cout << "  Confirm: ";
        sc(YELLOW);
        std::cout << action;
        rc();
        clrLine(sr + 2);
        gotoxy(0, sr + 2);
        sc(DGRAY);
        for (int i = 0; i < CW(); i++)
            std::cout << '-';
        rc();
        clrLine(sr + 3);
        gotoxy(0, sr + 3);
        std::cout << "  ";
        if (sel == 0)
        {
            sc(CYAN);
            std::cout << "[1] YES  <--";
        }
        else
        {
            sc(DGRAY);
            std::cout << "[1] YES     ";
        }
        rc();
        std::cout << "     ";
        if (sel == 1)
        {
            sc(RED);
            std::cout << "[2] NO   <--";
        }
        else
        {
            sc(DGRAY);
            std::cout << "[2] NO      ";
        }
        rc();
        clrLine(sr + 4);
        gotoxy(0, sr + 4);
        sc(DGRAY);
        std::cout << "  Press 1=YES  2=NO  Esc=NO";
        rc();
    };
    draw();
    while (true)
    {
        int k = rk();
        if (k == '1')
        {
            sel = 0;
            break;
        }
        else if (k == '2')
        {
            sel = 1;
            break;
        }
        else if (k == KE && sel == 0)
            break;
        else if (k == KX)
        {
            sel = 1;
            break;
        }
    }
    for (int r = sr; r <= sr + 4; r++)
        clrLine(r);
    gotoxy(0, sr);
    cur(true);
    return sel == 0;
}

// ════════════════════════════════════════════════════
//  SUB-MENU: Continue / Help / Back
// ════════════════════════════════════════════════════
bool subMenu2(const std::string &name, const std::string &helpText)
{
    int sr = gpos().Y;
    auto draw = [&](int hi)
    {
        gotoxy(0, sr);
        clrLine(sr);
        sc(CYAN);
        std::cout << "  [ " << name << " ]";
        rc();
        const char *opts[] = {"[1] Continue", "[2] Help", "[3] Back"};
        for (int i = 0; i < 3; i++)
        {
            clrLine(sr + 1 + i);
            gotoxy(0, sr + 1 + i);
            std::cout << "  ";
            if (i == hi)
            {
                sc(CYAN);
                std::cout << "-> " << opts[i];
            }
            else
            {
                sc(DGRAY);
                std::cout << "   " << opts[i];
            }
            rc();
        }
        clrLine(sr + 4);
        gotoxy(0, sr + 4);
        sc(DGRAY);
        std::cout << "  Press 1 / 2 / 3";
        rc();
    };
    draw(-1);
    while (true)
    {
        int k = rk();
        if (k == '1')
        {
            draw(0);
            ms(120);
            for (int r = sr; r <= sr + 4; r++)
                clrLine(r);
            gotoxy(0, sr);
            return true;
        }
        if (k == '2')
        {
            gotoxy(0, sr);
            clrLine(sr);
            sc(YELLOW);
            std::cout << "  HELP - " << name;
            rc();
            std::istringstream hs(helpText);
            std::string hl2;
            int hr = sr + 1;
            while (std::getline(hs, hl2))
            {
                clrLine(hr);
                gotoxy(0, hr);
                sc(WHITE);
                std::cout << hl2;
                rc();
                hr++;
            }
            clrLine(hr);
            gotoxy(0, hr);
            sc(DGRAY);
            std::cout << "  Press any key to go back...";
            rc();
            rk();
            draw(-1);
            continue;
        }
        if (k == '3' || k == KX)
        {
            for (int r = sr; r <= sr + 4; r++)
                clrLine(r);
            gotoxy(0, sr);
            return false;
        }
    }
}

// ════════════════════════════════════════════════════
//  YEAR PICKER
// ════════════════════════════════════════════════════
std::vector<int> yearPicker(int cy)
{
    cls();
    std::cout << "\n";
    hl('=', CYAN);
    sc(CYAN);
    std::cout << "  [ SYNC DB — Select Year Range ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Available years: ";
    sc(YELLOW);
    std::cout << "2009";
    sc(WHITE);
    std::cout << " to ";
    sc(YELLOW);
    std::cout << cy;
    rc();
    std::cout << "\n\n";
    sc(DGRAY);
    std::cout << "  [1] Specific year         e.g.  2024\n";
    std::cout << "  [2] Year range            e.g.  2020 to 2024\n";
    std::cout << "  [3] All years             2009 to " << cy << "\n";
    std::cout << "  [4] Cancel\n\n";
    rc();
    sc(DGRAY);
    std::cout << "  Press 1 / 2 / 3 / 4 : ";
    rc();
    int choice = 0;
    while (true)
    {
        int k = rk();
        if (k >= '1' && k <= '4')
        {
            choice = k - '0';
            break;
        }
    }
    if (choice == 4)
    {
        cls();
        return {};
    }
    std::vector<int> years;
    if (choice == 1)
    {
        std::cout << "\n";
        std::string sy = inp("  Enter year (e.g. 2024): ");
        int yr = 0;
        try
        {
            yr = std::stoi(sy);
        }
        catch (...)
        {
            yr = 0;
        }
        if (yr < 2009 || yr > cy)
        {
            sc(RED);
            std::cout << "\n  Invalid year.\n";
            rc();
            std::cout << "\n  Press any key...\n";
            rk();
            cls();
            return {};
        }
        years.push_back(yr);
    }
    else if (choice == 2)
    {
        std::cout << "\n";
        std::string sy1 = inp("  From year: "), sy2 = inp("  To year  : ");
        int y1 = 0, y2 = 0;
        try
        {
            y1 = std::stoi(sy1);
            y2 = std::stoi(sy2);
        }
        catch (...)
        {
            y1 = y2 = 0;
        }
        if (y1 < 2009 || y2 > cy || y1 > y2)
        {
            sc(RED);
            std::cout << "\n  Invalid range.\n";
            rc();
            std::cout << "\n  Press any key...\n";
            rk();
            cls();
            return {};
        }
        for (int y = y2; y >= y1; y--)
            years.push_back(y);
    }
    else
    {
        for (int y = cy; y >= 2009; y--)
            years.push_back(y);
    }
    std::cout << "\n";
    if (years.size() == 1)
    {
        sc(WHITE);
        std::cout << "  Will fetch: year ";
        sc(YELLOW);
        std::cout << years[0];
        rc();
        std::cout << "\n";
    }
    else
    {
        sc(WHITE);
        std::cout << "  Will fetch: ";
        sc(YELLOW);
        std::cout << years.size();
        sc(WHITE);
        std::cout << " years (";
        sc(YELLOW);
        std::cout << years.back() << " - " << years.front();
        sc(WHITE);
        std::cout << ")\n";
        rc();
    }
    sc(DGRAY);
    std::cout << "  Duplicates will be skipped.\n\n";
    rc();
    if (!confirmDlg("Start sync?"))
    {
        cls();
        return {};
    }
    return years;
}

// ════════════════════════════════════════════════════
//  SYNC DB
// ════════════════════════════════════════════════════
void doSyncDB()
{
    cls();
    std::cout << "\n";
    hl('=', CYAN);
    sc(CYAN);
    std::cout << "  [ SYNC LOCAL DATABASE ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  Fetches 3D Lotto results from lottopcso.com\n"
        "  and stores them in digit_data.txt.\n\n"
        "  Auto-Sync already fetches today's draws.\n"
        "  Use this for bulk-fetching older years.\n\n"
        "  Duplicate entries are skipped.\n";
    if (!subMenu2("SYNC DB", help))
    {
        cls();
        return;
    }
    cls();
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Checking connection...";
    rc();
    std::cout << std::flush;
    if (!isOnline())
    {
        sc(RED);
        std::cout << "\n\n  [!] No internet.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    sc(GREEN);
    std::cout << " Connected.\n";
    rc();
    ms(400);
    int cy = getCurrentYear();
    auto years = yearPicker(cy);
    if (years.empty())
        return;
    cls();
    std::cout << "\n";
    hl('=', CYAN);
    sc(CYAN);
    std::cout << "  [ SYNCING... ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    int totalAdded = 0, totalSkipped = 0, totalFailed = 0, done = 0, total = (int)years.size();
    int lr = gpos().Y;
    for (int yr : years)
    {
        gotoxy(0, lr);
        clrLine(lr);
        sc(CYAN);
        std::cout << "  [" << std::setw(3) << done << "/" << total << "]  Fetching " << yr << "...";
        rc();
        std::cout << std::flush;
        auto entries = fetchYear(yr, cy);
        if (entries.empty())
        {
            totalFailed++;
            done++;
            ms(150);
            continue;
        }
        int added = mergeIntoDB(entries);
        totalAdded += added;
        totalSkipped += (int)entries.size() - added;
        done++;
        ms(150);
    }
    gotoxy(0, lr);
    clrLine(lr);
    std::cout << "\n";
    hl('-', GREEN);
    sc(GREEN);
    std::cout << "  [v] SYNC COMPLETE\n";
    rc();
    sc(WHITE);
    std::cout << "  Years fetched : ";
    sc(YELLOW);
    std::cout << done - totalFailed;
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  New records   : ";
    sc(GREEN);
    std::cout << totalAdded;
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Skipped (dup) : ";
    sc(DGRAY);
    std::cout << totalSkipped;
    rc();
    std::cout << "\n";
    if (totalFailed > 0)
    {
        sc(WHITE);
        std::cout << "  Failed pages  : ";
        sc(RED);
        std::cout << totalFailed;
        rc();
        std::cout << "\n";
    }
    sc(WHITE);
    std::cout << "  Total in DB   : ";
    sc(CYAN);
    std::cout << loadDB().size();
    rc();
    std::cout << "\n";
    hl('-', GREEN);
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

// ════════════════════════════════════════════════════
//  S-R-W — STRONG / RANDOM / WEAK
// ════════════════════════════════════════════════════
// ════════════════════════════════════════════════════
//  S-R-W  STRONG / RANDOM / WEAK
//
//  ALGORITHM — "1 out of 1000" pattern model:
//
//  Step 1: 35-draw rolling window (current month pattern)
//    Count each digit 0-9 across all 3 positions
//    (35 draws × 3 positions = up to 105 digit slots)
//    digit_freq[d] = count[d] / total_slots
//
//  Step 2: Per-combo pattern score
//    For combo a-b-c:
//      pattern_score = digit_freq[a] × digit_freq[b]
//                    × digit_freq[c] × 1000
//    Meaning: expected appearances per 1000 draws
//    if digits were drawn at observed frequencies.
//
//  Step 3: Blend with all-time history
//    hist_score    = (all_time_hits / total_draws) × 1000
//    final_score   = 0.7 × pattern_score
//                  + 0.3 × hist_score
//
//  Step 4: Classify (relative to median combo score)
//    STRONG   ≥ median × 2.0   (far above average)
//    RANDOM   ≥ median × 0.5   (near average)
//    WEAK     <  median × 0.5  (far below average)
//
//  Step 5: "Strongest" = top 10 by final_score
//    These represent combos with the highest joint
//    probability based on the current digit pattern.
//
//  ONE NUMBER section: same digit_freq logic —
//    digit_score[d] = digit_freq[d] × 3 × 1000
//    (expected appearances per 1000 draws across 3 slots)
// ════════════════════════════════════════════════════

enum SRW_Class
{
    STRONG,
    RANDOM,
    WEAK
};

struct SRWEntry
{
    std::string combo;
    double score;     // final blended score (per-1000)
    double patScore;  // pattern score (per-1000)
    double histScore; // historical score (per-1000)
    int oneInN;       // "1 in N" inverse probability
    int totalHits;    // all-time appearances
    int recentHits;   // appearances in last 35 draws
    SRW_Class cls;
};

// ── Compute digit frequencies from last N draws ──────
// Fills freq[0..9] with per-slot probability
// window_size: how many draws to use (default 35)
static void calcDigitFreq(const std::vector<Entry> &sorted,
                          double freq[10], int window_size = 35)
{
    for (int d = 0; d < 10; d++)
        freq[d] = 0.0;
    int W = std::min(window_size, (int)sorted.size());
    if (W == 0)
        return;
    for (int i = 0; i < W; i++)
    {
        const std::string &dg = sorted[i].digit;
        if ((int)dg.size() >= 5)
        {
            int a = dg[0] - '0', b = dg[2] - '0', c = dg[4] - '0';
            if (a >= 0 && a <= 9)
                freq[a]++;
            if (b >= 0 && b <= 9)
                freq[b]++;
            if (c >= 0 && c <= 9)
                freq[c]++;
        }
    }
    double slots = W * 3.0;
    for (int d = 0; d < 10; d++)
        freq[d] /= slots;
}

std::vector<SRWEntry> computeSRW(const std::vector<Entry> &sorted)
{
    int total = (int)sorted.size();
    // All-time hit counts
    std::map<std::string, int> allCount;
    for (const auto &e : sorted)
        allCount[trimS(e.digit)]++;
    // Last-35 hit counts for display
    int W = std::min(35, total);
    std::map<std::string, int> win35;
    for (int i = 0; i < W; i++)
        win35[trimS(sorted[i].digit)]++;
    // Digit frequencies from 35-draw window
    double freq[10];
    calcDigitFreq(sorted, freq, 35);

    std::vector<SRWEntry> result;
    result.reserve(1000);
    for (int a = 0; a <= 9; a++)
        for (int b = 0; b <= 9; b++)
            for (int c = 0; c <= 9; c++)
            {
                std::string combo = std::to_string(a) + "-" + std::to_string(b) + "-" + std::to_string(c);
                int th = (allCount.count(combo) ? allCount.at(combo) : 0);
                int rh = (win35.count(combo) ? win35.at(combo) : 0);
                double pat = freq[a] * freq[b] * freq[c] * 1000.0;
                double hist = (total > 0) ? (double)th / total * 1000.0 : 0.0;
                double score = 0.7 * pat + 0.3 * hist;
                int oneInN = (score > 1e-9) ? (int)std::round(1000.0 / score) : 9999;
                if (oneInN < 1)
                    oneInN = 1;
                if (oneInN > 9999)
                    oneInN = 9999;
                SRWEntry e;
                e.combo = combo;
                e.score = score;
                e.patScore = pat;
                e.histScore = hist;
                e.oneInN = oneInN;
                e.totalHits = th;
                e.recentHits = rh;
                e.cls = RANDOM;
                result.push_back(e);
            }
    // Classification by median
    std::vector<double> sv;
    sv.reserve(1000);
    for (const auto &e : result)
        sv.push_back(e.score);
    std::sort(sv.begin(), sv.end());
    double median = sv[sv.size() / 2];
    double strongT = median * 2.0, weakT = median * 0.5;
    for (auto &e : result)
    {
        if (e.score >= strongT)
            e.cls = STRONG;
        else if (e.score >= weakT)
            e.cls = RANDOM;
        else
            e.cls = WEAK;
    }
    // Sort: Strong first, then by score desc within class
    std::sort(result.begin(), result.end(), [](const SRWEntry &a, const SRWEntry &b)
              {
        if(a.cls!=b.cls)return(int)a.cls<(int)b.cls;
        return a.score>b.score; });
    return result;
}

int srwColor(SRW_Class c)
{
    if (c == STRONG)
        return RED;
    if (c == RANDOM)
        return GREEN;
    return DGRAY;
}
const char *srwLabel(SRW_Class c)
{
    if (c == STRONG)
        return "STRONG";
    if (c == RANDOM)
        return "RANDOM";
    return "WEAK  ";
}

static void printOneInN(int n)
{
    if (n >= 9999)
        std::cout << "1 in >9999";
    else
        std::cout << "1 in " << std::setw(5) << std::left << n;
}

// ── Search: look up a single combo ───────────────────
static void doSRWSearch(const std::vector<SRWEntry> &srw)
{
    cls();
    std::cout << "\n";
    hl('=', RED);
    sc(RED);
    std::cout << "  [ S-R-W — SEARCH COMBO ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Type a combo to check its class and 1-in-N probability.\n";
    std::cout << "  Format: 5-9-2  or  592   |  blank = back\n\n";
    rc();
    hl('-', DGRAY);
    while (true)
    {
        sc(WHITE);
        std::cout << "  Combo (blank=back): ";
        rc();
        std::string raw = inp("");
        if (raw.empty())
        {
            cls();
            return;
        }
        std::string combo = normalizeDigit(raw);
        if (combo.empty())
        {
            sc(RED);
            std::cout << "  Invalid — use 5-9-2 or 592.\n";
            rc();
            continue;
        }
        if (combo != raw)
        {
            sc(DGRAY);
            std::cout << "  -> ";
            sc(YELLOW);
            std::cout << combo;
            rc();
            std::cout << "\n";
        }
        // Locate in sorted srw list (any position)
        const SRWEntry *found = nullptr;
        int rank = 0;
        for (int i = 0; i < (int)srw.size(); i++)
        {
            if (srw[i].combo == combo)
            {
                found = &srw[i];
                rank = i + 1;
                break;
            }
        }
        if (!found)
        {
            sc(RED);
            std::cout << "  Not found.\n";
            rc();
            continue;
        }
        const SRWEntry &e = *found;
        std::cout << "\n";
        hl('=', srwColor(e.cls));
        sc(srwColor(e.cls));
        std::cout << "  " << e.combo;
        sc(WHITE);
        std::cout << "   CLASS: ";
        sc(srwColor(e.cls));
        std::cout << srwLabel(e.cls);
        rc();
        std::cout << "\n";
        hl('-', DGRAY);
        sc(WHITE);
        std::cout << "  Rank (global)    : ";
        sc(YELLOW);
        std::cout << "#" << rank << " of 1000\n";
        rc();
        sc(WHITE);
        std::cout << "  Score (per 1000) : ";
        sc(YELLOW);
        std::cout << std::fixed << std::setprecision(5) << e.score << "\n";
        rc();
        sc(WHITE);
        std::cout << "  Probability      : ";
        sc(srwColor(e.cls));
        printOneInN(e.oneInN);
        rc();
        std::cout << "\n";
        sc(WHITE);
        std::cout << "  Pattern (35-drw) : ";
        sc(CYAN);
        std::cout << std::fixed << std::setprecision(5) << e.patScore << "\n";
        rc();
        sc(WHITE);
        std::cout << "  History (all-t)  : ";
        sc(DGRAY);
        std::cout << std::fixed << std::setprecision(5) << e.histScore << "\n";
        rc();
        sc(WHITE);
        std::cout << "  All-time hits    : ";
        sc(DGRAY);
        std::cout << e.totalHits << "\n";
        rc();
        sc(WHITE);
        std::cout << "  Last 35 draws    : ";
        sc(e.recentHits > 0 ? CYAN : DGRAY);
        std::cout << e.recentHits << "\n";
        rc();
        hl('=', srwColor(e.cls));
        std::cout << "\n  Search another (blank=back):\n\n";
    }
}

// ── SRW Digits display ───────────────────────────────
// filter: -1=All  0=Strong  1=Random  2=Weak  3=Strongest(top10)
void doSRWDigits(const std::vector<Entry> &sorted, int filter = -1)
{
    cls();
    std::cout << "\n";
    hl('=', RED);
    sc(RED);
    if (filter == -1)
        std::cout << "  [ S-R-W DIGITS — ALL ]\n";
    else if (filter == 0)
        std::cout << "  [ S-R-W DIGITS — STRONG ]\n";
    else if (filter == 1)
        std::cout << "  [ S-R-W DIGITS — RANDOM ]\n";
    else if (filter == 2)
        std::cout << "  [ S-R-W DIGITS — WEAK ]\n";
    else
        std::cout << "  [ S-R-W DIGITS — STRONGEST TOP 10 ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Computing 35-draw pattern scores...";
    rc();
    std::cout << std::flush;
    auto srw = computeSRW(sorted);
    clrLine(gpos().Y);
    gotoxy(0, gpos().Y);

    int ns = 0, nr = 0, nw = 0;
    for (const auto &e : srw)
    {
        if (e.cls == STRONG)
            ns++;
        else if (e.cls == RANDOM)
            nr++;
        else
            nw++;
    }

    // Build view list
    std::vector<const SRWEntry *> view;
    if (filter == 3)
    {
        // True top 10 by score regardless of class
        std::vector<const SRWEntry *> all;
        all.reserve(1000);
        for (const auto &e : srw)
            all.push_back(&e);
        std::sort(all.begin(), all.end(), [](const SRWEntry *a, const SRWEntry *b)
                  { return a->score > b->score; });
        int lim = std::min(10, (int)all.size());
        for (int i = 0; i < lim; i++)
            view.push_back(all[i]);
    }
    else
    {
        for (const auto &e : srw)
        {
            if (filter == -1)
                view.push_back(&e);
            else if (filter == 0 && e.cls == STRONG)
                view.push_back(&e);
            else if (filter == 1 && e.cls == RANDOM)
                view.push_back(&e);
            else if (filter == 2 && e.cls == WEAK)
                view.push_back(&e);
        }
    }

    std::cout << "\n";
    hl('=', RED);
    sc(RED);
    std::cout << "  STRONG ";
    sc(WHITE);
    std::cout << ns;
    sc(DGRAY);
    std::cout << "   |   ";
    sc(GREEN);
    std::cout << "RANDOM ";
    sc(WHITE);
    std::cout << nr;
    sc(DGRAY);
    std::cout << "   |   ";
    sc(DGRAY);
    std::cout << "WEAK ";
    sc(WHITE);
    std::cout << nw;
    rc();
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  PER-1000 = expected hits per 1000 draws (35-draw pattern + history)\n";
    std::cout << "  Top 5 marked *\n";
    rc();
    hl('-', DGRAY);
    sc(DGRAY);
    std::cout << "  " << std::left
              << std::setw(6) << "RANK"
              << std::setw(9) << "COMBO"
              << std::setw(9) << "CLASS"
              << std::setw(12) << "PER-1000"
              << std::setw(12) << "1-IN-N"
              << std::setw(10) << "ALL-HITS"
              << std::setw(9) << "35-WIN"
              << "\n";
    rc();
    hl('-', DGRAY);

    SRW_Class lastCls = (SRW_Class)-1;
    int shown = 0;
    for (int idx = 0; idx < (int)view.size(); idx++)
    {
        const SRWEntry &e = *view[idx];
        bool top5 = (idx < 5);

        // Group header for "All" view
        if (filter == -1 && e.cls != lastCls)
        {
            if (lastCls != (SRW_Class)-1)
                std::cout << "\n";
            sc(srwColor(e.cls));
            std::cout << "  ── " << srwLabel(e.cls) << " ────────────────────────────────────\n";
            rc();
            lastCls = e.cls;
        }

        int col = top5 ? YELLOW : srwColor(e.cls);
        sc(col);
        std::string nin = (e.oneInN >= 9999) ? "1in>9999" : ("1in" + std::to_string(e.oneInN));
        std::cout << "  " << std::left
                  << std::setw(6) << (idx + 1)
                  << std::setw(9) << e.combo
                  << std::setw(9) << srwLabel(e.cls)
                  << std::fixed << std::setprecision(5) << std::setw(12) << e.score
                  << std::setw(12) << nin;
        sc(DGRAY);
        std::cout << std::setw(10) << e.totalHits << std::setw(9) << e.recentHits;
        if (top5)
        {
            sc(YELLOW);
            std::cout << " *";
        }
        rc();
        std::cout << "\n";
        shown++;
        if (shown % 40 == 0 && filter != 3)
        {
            sc(DGRAY);
            std::cout << "  -- " << shown << "/" << (int)view.size() << "  [Space/Enter=next  Q=stop] --";
            rc();
            int k = rk();
            if (k == 'q' || k == 'Q')
                break;
        }
    }
    hl('=', RED);
    if (filter == 3)
    {
        sc(RED);
        std::cout << "  TOP 10 STRONGEST — highest joint digit pattern probability\n";
        rc();
    }
    sc(YELLOW);
    std::cout << "  * = Top 5 in this view\n";
    rc();
    sc(DGRAY);
    std::cout << "  " << shown << " combos shown. Score = 70% 35-draw pattern + 30% all-time.\n";
    rc();
    std::cout << "\n  [1] Search a combo   [2] Back\n  ";
    sc(DGRAY);
    std::cout << "Press 1 or 2: ";
    rc();
    while (true)
    {
        int k = rk();
        if (k == '1')
        {
            doSRWSearch(srw);
        }
        else if (k == '2' || k == KX)
        {
            cls();
            return;
        }
    }
}

// ── SRW One Number — per-digit 35-draw frequency ─────
void doSRWOneNumber(const std::vector<Entry> &sorted)
{
    cls();
    std::cout << "\n";
    hl('=', RED);
    sc(RED);
    std::cout << "  [ S-R-W ONE NUMBER ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Computing digit frequency in last 35 draws...";
    rc();
    std::cout << std::flush;

    int total = (int)sorted.size();
    int W = std::min(35, total);
    // Per-digit counts in 35-draw window
    int cnt35[10] = {};
    int cntAll[10] = {};
    for (int i = 0; i < W; i++)
    {
        const std::string &dg = trimS(sorted[i].digit);
        if ((int)dg.size() >= 5)
        {
            int a = dg[0] - '0', b = dg[2] - '0', c = dg[4] - '0';
            if (a >= 0 && a <= 9)
                cnt35[a]++;
            if (b >= 0 && b <= 9)
                cnt35[b]++;
            if (c >= 0 && c <= 9)
                cnt35[c]++;
        }
    }
    for (const auto &e : sorted)
    {
        const std::string &dg = trimS(e.digit);
        if ((int)dg.size() >= 5)
        {
            int a = dg[0] - '0', b = dg[2] - '0', c = dg[4] - '0';
            if (a >= 0 && a <= 9)
                cntAll[a]++;
            if (b >= 0 && b <= 9)
                cntAll[b]++;
            if (c >= 0 && c <= 9)
                cntAll[c]++;
        }
    }
    int slots35 = W * 3, slotsAll = total * 3;
    double freq[10];
    calcDigitFreq(sorted, freq, 35);

    struct ND
    {
        int d, c35, cAll;
        double freq35, score;
        int oneInN;
        SRW_Class cls;
    };
    ND nums[10];
    for (int d = 0; d < 10; d++)
    {
        double f35 = freq[d];
        double fAll = (slotsAll > 0) ? (double)cntAll[d] / slotsAll : 0.0;
        // Expected appearances per 1000 draws (across 3 positions)
        double pat = f35 * 3.0 * 1000.0;
        double hist = fAll * 3.0 * 1000.0;
        double score = 0.7 * pat + 0.3 * hist;
        // Classify: uniform baseline = 300 per 1000
        SRW_Class cls;
        if (score >= 360)
            cls = STRONG; // >20% above uniform
        else if (score >= 240)
            cls = RANDOM; // within ±20%
        else
            cls = WEAK;
        int oin = (score > 1e-9) ? (int)std::round(1000.0 / score) : 9999;
        if (oin < 1)
            oin = 1;
        if (oin > 9999)
            oin = 9999;
        nums[d] = {d, cnt35[d], cntAll[d], f35, score, oin, cls};
    }
    // Sort by score desc
    std::sort(std::begin(nums), std::end(nums), [](const ND &a, const ND &b)
              { return a.score > b.score; });

    clrLine(gpos().Y);
    gotoxy(0, gpos().Y);
    std::cout << "\n";
    hl('=', RED);
    sc(DGRAY);
    std::cout << "  How often each digit 0-9 appears across all 3 draw positions.\n";
    std::cout << "  Window: last " << W << " draws = " << slots35 << " slots  |  Baseline: ~300 per 1000\n";
    rc();
    hl('-', DGRAY);
    sc(DGRAY);
    std::cout << "  " << std::left
              << std::setw(5) << "RANK" << std::setw(8) << "DIGIT" << std::setw(9) << "CLASS"
              << std::setw(12) << "PER-1000" << std::setw(10) << "1-IN-N"
              << std::setw(9) << "35-WIN" << std::setw(12) << "35-FREQ%" << std::setw(12) << "ALL-TIME"
              << "\n";
    rc();
    hl('-', DGRAY);

    for (int i = 0; i < 10; i++)
    {
        const ND &e = nums[i];
        bool top5 = (i < 5);
        int col = top5 ? YELLOW : srwColor(e.cls);
        sc(col);
        std::string nin = (e.oneInN >= 9999) ? "1in>9999" : ("1in" + std::to_string(e.oneInN));
        std::cout << "  " << std::left
                  << std::setw(5) << (i + 1) << std::setw(8) << e.d
                  << std::setw(9) << srwLabel(e.cls)
                  << std::fixed << std::setprecision(2) << std::setw(12) << e.score
                  << std::setw(10) << nin
                  << std::setw(9) << e.c35
                  << std::setw(12) << std::fixed << std::setprecision(2) << (e.freq35 * 100.0);
        sc(DGRAY);
        std::cout << std::setw(12) << e.cAll;
        if (top5)
        {
            sc(YELLOW);
            std::cout << " *";
        }
        rc();
        std::cout << "\n";
    }
    hl('=', RED);
    sc(YELLOW);
    std::cout << "  * = Top 5 most active digits in 35-draw pattern\n";
    rc();
    sc(DGRAY);
    std::cout << "  STRONG >= 360   RANDOM 240-359   WEAK < 240  (per 1000 draws)\n";
    std::cout << "  TIP: Cross these digits with S-R-W Digits to find strong combos.\n";
    sc(YELLOW);
    std::cout << "  Pattern guide — NOT a prediction.\n";
    rc();
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

void doSRW()
{
    cls();
    std::cout << "\n";
    hl('=', RED);
    sc(RED);
    std::cout << "  [ S-R-W  STRONG · RANDOM · WEAK ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  S-R-W — 1 out of 1000 pattern model\n\n"
        "  Uses the last 35 draws as a pattern window\n"
        "  to score all 1000 combos and all 10 digits.\n\n"
        "  ALGORITHM:\n"
        "  1. Count digit 0-9 freq across all 3 positions\n"
        "     in last 35 draws (up to 105 digit slots)\n"
        "  2. combo_score = freq[a]*freq[b]*freq[c]*1000\n"
        "  3. final = 70% pattern + 30% all-time history\n\n"
        "  CLASSIFICATION (vs median combo score):\n"
        "  STRONG  score >= 2x median\n"
        "  RANDOM  0.5x to 2x median\n"
        "  WEAK    below 0.5x median\n\n"
        "  [1] S-R-W Digits:\n"
        "      Search / All / Strong / Random\n"
        "      Weak / Strongest (top 10)\n"
        "  [2] S-R-W One Number:\n"
        "      Each digit 0-9 ranked by 35-draw freq\n\n"
        "  Pattern guide — NOT a prediction tool.\n";

    int sr = gpos().Y;
    auto drawSRW = [&](int hi)
    {
        gotoxy(0, sr);
        clrLine(sr);
        sc(RED);
        std::cout << "  [ S-R-W ]";
        rc();
        const char *opts[] = {"[1] S-R-W Digits", "[2] S-R-W One Number", "[3] Help", "[4] Back"};
        const char *desc[] = {
            "  Search / All / Strong / Random / Weak / Strongest",
            "  Digits 0-9 ranked by 35-draw pattern",
            "  How the algorithm works",
            "  Return to main menu"};
        for (int i = 0; i < 4; i++)
        {
            clrLine(sr + 1 + i);
            gotoxy(0, sr + 1 + i);
            std::cout << "  ";
            if (i == hi)
            {
                sc(RED);
                std::cout << "-> " << opts[i];
                sc(DGRAY);
                std::cout << desc[i];
            }
            else
            {
                sc(DGRAY);
                std::cout << "   " << opts[i] << desc[i];
            }
            rc();
        }
        clrLine(sr + 5);
        gotoxy(0, sr + 5);
        sc(DGRAY);
        std::cout << "  Press 1 / 2 / 3 / 4";
        rc();
    };
    drawSRW(-1);
    int choice = 0;
    while (true)
    {
        int k = rk();
        if (k == '1')
        {
            drawSRW(0);
            ms(120);
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            choice = 1;
            break;
        }
        if (k == '2')
        {
            drawSRW(1);
            ms(120);
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            choice = 2;
            break;
        }
        if (k == '3')
        {
            gotoxy(0, sr);
            clrLine(sr);
            sc(YELLOW);
            std::cout << "  HELP - S-R-W";
            rc();
            std::istringstream hs(help);
            std::string hl2;
            int hr = sr + 1;
            while (std::getline(hs, hl2))
            {
                clrLine(hr);
                gotoxy(0, hr);
                sc(WHITE);
                std::cout << hl2;
                rc();
                hr++;
            }
            clrLine(hr);
            gotoxy(0, hr);
            sc(DGRAY);
            std::cout << "  Press any key...";
            rc();
            rk();
            drawSRW(-1);
            continue;
        }
        if (k == '4' || k == KX)
        {
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            cls();
            return;
        }
    }

    // Load + sort newest-first
    cls();
    std::cout << "\n";
    hl('=', RED);
    sc(RED);
    std::cout << "  [ S-R-W — Loading ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Checking for new draws...\n";
    rc();
    tryAutoSync(true);
    pulse("Loading and analyzing database", 2);
    auto entries = loadDB();
    if (entries.empty())
    {
        sc(RED);
        std::cout << "\n  No data. Run Sync DB first.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    auto slotOrd = [](const std::string &t) -> int
    {if(t=="9pm")return 2;if(t=="5pm")return 1;return 0; };
    std::sort(entries.begin(), entries.end(), [&](const Entry &a, const Entry &b)
              {
        int da=dateToInt(a.date),db=dateToInt(b.date);if(da!=db)return da>db;
        return slotOrd(a.time)>slotOrd(b.time); });
    int W = std::min(35, (int)entries.size());
    sc(DGRAY);
    std::cout << "  Total draws: ";
    sc(CYAN);
    std::cout << entries.size();
    rc();
    std::cout << "   Pattern window: ";
    sc(YELLOW);
    std::cout << W << " draws\n\n";
    rc();

    if (choice == 1)
    {
        // Digits — filter sub-menu
        cls();
        std::cout << "\n";
        hl('=', RED);
        sc(RED);
        std::cout << "  [ S-R-W DIGITS — View ]\n";
        rc();
        hl('-', DGRAY);
        std::cout << "\n";
        sc(DGRAY);
        std::cout << "  [1] Search         Look up a specific combo\n";
        std::cout << "  [2] All            All 1000 combos (score desc)\n";
        std::cout << "  [3] Strong         Score >= 2x median\n";
        std::cout << "  [4] Random         0.5x to 2x median\n";
        std::cout << "  [5] Weak           Below 0.5x median\n";
        std::cout << "  [6] Strongest      Top 10 — highest probability\n";
        std::cout << "  [7] Back\n\n";
        rc();
        hl('-', DGRAY);
        sc(DGRAY);
        std::cout << "  Press 1-7: ";
        rc();
        while (true)
        {
            int k = rk();
            if (k == '1')
            {
                // Compute first, then hand off to search
                cls();
                std::cout << "\n";
                hl('=', RED);
                sc(RED);
                std::cout << "  [ S-R-W — Computing ]\n";
                rc();
                sc(DGRAY);
                std::cout << "  Building scores...";
                rc();
                std::cout << std::flush;
                auto srw = computeSRW(entries);
                clrLine(gpos().Y);
                gotoxy(0, gpos().Y);
                doSRWSearch(srw);
                break;
            }
            if (k == '2')
            {
                doSRWDigits(entries, -1);
                break;
            }
            if (k == '3')
            {
                doSRWDigits(entries, 0);
                break;
            }
            if (k == '4')
            {
                doSRWDigits(entries, 1);
                break;
            }
            if (k == '5')
            {
                doSRWDigits(entries, 2);
                break;
            }
            if (k == '6')
            {
                doSRWDigits(entries, 3);
                break;
            }
            if (k == '7' || k == KX)
            {
                cls();
                return;
            }
        }
    }
    else
    {
        doSRWOneNumber(entries);
    }
}

void doShowAll()
{
    cls();
    std::cout << "\n";
    hl('=', CYAN);
    sc(CYAN);
    std::cout << "  [ SHOW ALL ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  Shows all records from digit_data.txt.\n"
        "  Auto-syncs latest draws if online.\n"
        "  Paginated 40 rows. Space=next, Q=stop.\n";
    if (!subMenu2("SHOW ALL", help))
    {
        cls();
        return;
    }
    cls();
    std::cout << "\n";
    hl('=', CYAN);
    sc(CYAN);
    std::cout << "  [ SHOW ALL — Loading ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Checking for new draws...\n";
    rc();
    tryAutoSync(true);
    pulse("Loading local records", 2);
    auto data = loadDB();
    std::cout << "\n";
    hl('=', CYAN);
    sc(CYAN);
    std::cout << "  " << std::left << std::setw(6) << "#" << std::setw(14) << "DIGIT" << std::setw(18) << "DATE" << std::setw(8) << "TIME\n";
    rc();
    hl('-', DGRAY);
    if (data.empty())
    {
        sc(DGRAY);
        std::cout << "\n  No local records. Run Sync DB to populate.\n";
        rc();
    }
    else
    {
        int shown = 0;
        for (int i = 0; i < (int)data.size(); i++)
        {
            sc(i % 2 == 0 ? WHITE : DGRAY);
            std::cout << "  " << std::left << std::setw(6) << (i + 1);
            sc(YELLOW);
            std::cout << std::setw(14) << data[i].digit;
            sc(CYAN);
            std::cout << std::setw(18) << data[i].date;
            sc(MAGENTA);
            std::cout << std::setw(8) << data[i].time;
            rc();
            std::cout << "\n";
            shown++;
            if (shown % 40 == 0)
            {
                sc(DGRAY);
                std::cout << "  -- " << shown << "/" << data.size() << "  [Space=next  Q=stop] --";
                rc();
                int k = rk();
                if (k == 'q' || k == 'Q')
                    break;
            }
        }
        hl('=', CYAN);
        sc(WHITE);
        std::cout << "  Total: ";
        sc(YELLOW);
        std::cout << data.size();
        rc();
        std::cout << " records\n";
    }
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

// ════════════════════════════════════════════════════
//  PROBABILITY
// ════════════════════════════════════════════════════

// ════════════════════════════════════════════════════
//  PROBABILITY
//  - 1-out-of-1000 scoring (same model as S-R-W)
//  - No time filter: digit → confirm → results
//  - Recurrence gap: finds when this combo last
//    appeared and whether it is currently "overdue"
//  - Lists every gap between consecutive appearances
// ════════════════════════════════════════════════════
void showProbResult(const std::vector<Entry> &sorted, const std::string &digit)
{
    int total = (int)sorted.size();

    // Collect positions (0 = most recent draw) where this combo appeared
    std::vector<int> hitPos;
    for (int i = 0; i < total; i++)
        if (trimS(sorted[i].digit) == digit)
            hitPos.push_back(i);
    int matches = (int)hitPos.size();

    // Gap analysis
    // drawsSinceLast: how many draws have passed since last appearance
    int drawsSinceLast = (hitPos.empty()) ? -1 : hitPos[0];
    // Per-interval gaps between consecutive appearances
    std::vector<int> gaps;
    for (int i = 0; i + 1 < (int)hitPos.size(); i++)
        gaps.push_back(hitPos[i + 1] - hitPos[i]);
    double avgGap = 0.0;
    if (!gaps.empty())
    {
        double s = 0;
        for (int g : gaps)
            s += g;
        avgGap = s / gaps.size();
    }
    else if (matches == 1)
        avgGap = (double)total;
    double expectedGap = (matches > 0) ? (double)total / matches : 0.0;
    bool overdue = (drawsSinceLast > 0 && avgGap > 0 && drawsSinceLast >= avgGap * 0.85);

    // 1-out-of-1000 score
    double freq[10];
    calcDigitFreq(sorted, freq, 35);
    double patScore = 0.0;
    if (digit.size() >= 5)
    {
        int a = digit[0] - '0', b = digit[2] - '0', c = digit[4] - '0';
        if (a >= 0 && a <= 9 && b >= 0 && b <= 9 && c >= 0 && c <= 9)
            patScore = freq[a] * freq[b] * freq[c] * 1000.0;
    }
    double histScore = (total > 0) ? (double)matches / total * 1000.0 : 0.0;
    double score = 0.7 * patScore + 0.3 * histScore;
    int oneInN = (score > 1e-9) ? (int)std::round(1000.0 / score) : 9999;
    if (oneInN < 1)
        oneInN = 1;
    if (oneInN > 9999)
        oneInN = 9999;

    // Classify
    // Build quick median from all 1000 combos — use a lightweight estimate
    // Expected uniform score ≈ 1.0 per 1000; thresholds same as SRW
    SRW_Class cls;
    if (score >= 2.0)
        cls = STRONG;
    else if (score >= 0.5)
        cls = RANDOM;
    else
        cls = WEAK;

    std::cout << "\n";
    hl('=', YELLOW);
    sc(YELLOW);
    std::cout << "  COMBO: ";
    sc(WHITE);
    std::cout << digit;
    rc();
    std::cout << "\n";
    hl('-', DGRAY);
    std::cout << "\n";

    // ── Probability block ──────────────────────────
    sc(WHITE);
    std::cout << "  S-R-W Class      : ";
    sc(srwColor(cls));
    std::cout << srwLabel(cls);
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Score (per 1000) : ";
    sc(YELLOW);
    std::cout << std::fixed << std::setprecision(5) << score;
    sc(DGRAY);
    std::cout << "  expected hits per 1000 draws";
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Probability      : ";
    sc(srwColor(cls));
    printOneInN(oneInN);
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Pattern (35-drw) : ";
    sc(CYAN);
    std::cout << std::fixed << std::setprecision(5) << patScore;
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  History (all-t)  : ";
    sc(DGRAY);
    std::cout << std::fixed << std::setprecision(5) << histScore;
    rc();
    std::cout << "\n\n";

    // Bar scaled: 1.0 = exactly expected (uniform). Cap display at 5× expected.
    double barNorm = std::min(score / 5.0, 1.0);
    int fill = (int)(barNorm * 40);
    if (fill == 0 && matches > 0)
        fill = 1;
    std::cout << "  [";
    for (int i = 0; i < 40; i++)
    {
        if (i < fill)
        {
            sc(YELLOW);
            std::cout << '#';
        }
        else
        {
            sc(DGRAY);
            std::cout << '.';
        }
    }
    rc();
    std::cout << "] ";
    sc(YELLOW);
    std::cout << std::fixed << std::setprecision(5) << score;
    sc(DGRAY);
    std::cout << " / 1000";
    rc();
    std::cout << "\n\n";

    // ── Recurrence gap block ───────────────────────
    hl('-', DGRAY);
    sc(WHITE);
    std::cout << "  Appearances      : ";
    sc(GREEN);
    std::cout << matches;
    sc(DGRAY);
    std::cout << (matches == 1 ? " time" : " times") << " in " << total << " draws";
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Expected gap     : ";
    sc(DGRAY);
    if (matches > 0)
        std::cout << std::fixed << std::setprecision(1) << expectedGap << " draws  (total÷hits)";
    else
        std::cout << "N/A";
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Avg actual gap   : ";
    sc(DGRAY);
    if (matches >= 2)
        std::cout << std::fixed << std::setprecision(1) << avgGap << " draws  (" << (int)gaps.size() << " intervals)";
    else if (matches == 1)
        std::cout << "only 1 hit — no gap data";
    else
        std::cout << "never appeared";
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Draws since last : ";
    if (drawsSinceLast < 0)
    {
        sc(DGRAY);
        std::cout << "Never appeared";
    }
    else if (drawsSinceLast == 0)
    {
        sc(GREEN);
        std::cout << "Appeared in the most recent draw";
    }
    else
    {
        if (overdue)
            sc(RED);
        else
            sc(CYAN);
        std::cout << drawsSinceLast << " draws ago";
    }
    rc();
    std::cout << "\n";
    if (overdue)
    {
        sc(RED);
        std::cout << "  *** OVERDUE: " << drawsSinceLast << " draws passed";
        std::cout << " (avg gap " << std::fixed << std::setprecision(1) << avgGap << ")";
        rc();
        std::cout << "\n";
    }
    std::cout << "\n";

    // ── All appearances with per-interval gaps ─────
    if (!hitPos.empty())
    {
        hl('-', DGRAY);
        sc(WHITE);
        std::cout << "  All appearances of ";
        sc(YELLOW);
        std::cout << digit;
        sc(DGRAY);
        std::cout << "  (newest first, gap = draws until next hit)";
        rc();
        std::cout << "\n";
        hl('-', DGRAY);
        int shown = 0, lastYr = -1;
        for (int i = 0; i < (int)hitPos.size(); i++)
        {
            const Entry &e = sorted[hitPos[i]];
            int yr = yearFromLabel(e.date);
            if (yr != lastYr)
            {
                sc(DARK_CYAN);
                std::cout << "  ── " << yr << " ──\n";
                rc();
                lastYr = yr;
            }
            sc(DGRAY);
            std::cout << "  " << std::setw(4) << (i + 1) << ". ";
            sc(CYAN);
            std::cout << std::left << std::setw(22) << e.date;
            sc(MAGENTA);
            std::cout << std::setw(6) << e.time;
            if (i < (int)gaps.size())
            {
                sc(DGRAY);
                std::cout << "  gap→next: ";
                int g = gaps[i];
                if (g > (int)avgGap * 1.5)
                {
                    sc(RED);
                    std::cout << g << " draws";
                }
                else if (g < (int)avgGap * 0.5)
                {
                    sc(GREEN);
                    std::cout << g << " draws";
                }
                else
                {
                    sc(DGRAY);
                    std::cout << g << " draws";
                }
            }
            rc();
            std::cout << "\n";
            shown++;
            if (shown % 40 == 0)
            {
                sc(DGRAY);
                std::cout << "  -- " << shown << "/" << hitPos.size() << "  [Space=next  Q=stop] --";
                rc();
                int k = rk();
                if (k == 'q' || k == 'Q')
                    break;
            }
        }
    }
    else
    {
        sc(DGRAY);
        std::cout << "  Never appeared in database.\n";
        rc();
    }
    hl('=', YELLOW);
}

void doProb()
{
    cls();
    std::cout << "\n";
    hl('=', YELLOW);
    sc(YELLOW);
    std::cout << "  [ PROBABILITY ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  PROBABILITY — 1 out of 1000 model\n\n"
        "  Analyzes a combo's expected frequency and\n"
        "  recurrence gap from the draw history.\n\n"
        "  SCORE = 70% 35-draw digit pattern\n"
        "        + 30% all-time historical frequency\n"
        "  Expressed as X per 1000 draws | 1-in-N\n\n"
        "  RECURRENCE GAP:\n"
        "  How many draws since the last appearance.\n"
        "  OVERDUE = gap >= 85% of average gap.\n"
        "  Each appearance also shows the interval\n"
        "  to the next hit (long gaps in red).\n\n"
        "  DIGIT: 5-9-2 or 592 (auto-converted)\n"
        "  No time filter needed — just the combo.\n";
    if (!subMenu2("PROBABILITY", help))
    {
        cls();
        return;
    }
    cls();
    std::cout << "\n";
    hl('=', YELLOW);
    sc(YELLOW);
    std::cout << "  [ PROBABILITY — Loading ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Checking for new draws...\n";
    rc();
    tryAutoSync(true);
    auto entries = loadDB();
    if (entries.empty())
    {
        sc(RED);
        std::cout << "\n  No data. Run Sync DB first.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    // Sort newest-first
    auto slotOrd = [](const std::string &t) -> int
    {if(t=="9pm")return 2;if(t=="5pm")return 1;return 0; };
    std::sort(entries.begin(), entries.end(), [&](const Entry &a, const Entry &b)
              {
        int da=dateToInt(a.date),db=dateToInt(b.date);if(da!=db)return da>db;
        return slotOrd(a.time)>slotOrd(b.time); });
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Combo: e.g. 5-9-2 or 592\n\n";
    rc();
    sc(WHITE);
    std::cout << "  Combo to analyze: ";
    rc();
    std::string digit = inpDigit("");
    std::cout << "\n";
    if (!confirmDlg("Analyze: " + digit))
    {
        cls();
        return;
    }
    pulse("Analyzing pattern and recurrence", 2);
    showProbResult(entries, digit);
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

//  COMBO section: repeating-digit combos in the gap
// ════════════════════════════════════════════════════
struct LastDigitRecord
{
    std::string digit, date, time;
    int posInWindow, uniqueRank;
};

// Returns true if combo has a repeated digit: e.g. 1-1-5, 5-5-5, 2-7-2
bool hasRepeatDigit(const std::string &combo)
{
    if (combo.size() < 5)
        return false;
    int a = combo[0] - '0', b = combo[2] - '0', c = combo[4] - '0';
    return a == b || b == c || a == c;
}

std::vector<LastDigitRecord> computeLastDigits(const std::vector<Entry> &sorted, int startIdx, int windowSize, int N)
{
    int total = (int)sorted.size();
    int windowEnd = std::min(startIdx + windowSize, total);
    std::set<std::string> seen;
    std::vector<LastDigitRecord> unique;
    for (int i = startIdx; i < windowEnd; i++)
    {
        std::string key = trimS(sorted[i].digit);
        if (seen.find(key) == seen.end())
        {
            seen.insert(key);
            LastDigitRecord r;
            r.digit = key;
            r.date = sorted[i].date;
            r.time = sorted[i].time;
            r.posInWindow = i - startIdx + 1;
            r.uniqueRank = (int)unique.size() + 1;
            unique.push_back(r);
        }
    }
    std::vector<LastDigitRecord> result;
    int uTotal = (int)unique.size();
    int start2 = std::max(0, uTotal - N);
    for (int i = uTotal - 1; i >= start2; i--)
    {
        LastDigitRecord r = unique[i];
        r.uniqueRank = uTotal - i;
        result.push_back(r);
    }
    return result;
}
std::vector<Entry> getDrawsOnDate(const std::vector<Entry> &all, const std::string &date)
{
    std::vector<Entry> v;
    for (const auto &e : all)
        if (e.date == date)
            v.push_back(e);
    return v;
}

void doLastDigit()
{
    cls();
    std::cout << "\n";
    hl('=', DARK_MAG);
    sc(MAGENTA);
    std::cout << "  [ LAST DIGIT ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  Scans a draw window and finds which combos\n"
        "  have NOT appeared recently (last digits).\n\n"
        "  AUTO FIND: window starts at most recent draw.\n"
        "  STARTING POINT: pick a specific date & time.\n\n"
        "  WINDOW SIZE: how many draws deep to scan.\n"
        "  e.g. 10 = look at the last 10 draws only,\n"
        "       100 = last 100, 1000 = last 1000.\n"
        "  Each combo counted ONCE (most recent first).\n"
        "  #1 = combo farthest back = true last digit.\n\n"
        "  N = how many ranked results to display.\n\n"
        "  Uses local DB + auto-synced latest draws.\n";

    int sr = gpos().Y, entryChoice = 0;
    {
        auto drawEntry = [&](int hi)
        {
            gotoxy(0, sr);
            clrLine(sr);
            sc(MAGENTA);
            std::cout << "  [ LAST DIGIT ]";
            rc();
            const char *opts[] = {"[1] Auto Find", "[2] Starting Point", "[3] Help", "[4] Back"};
            const char *desc[] = {"  Most current draw", "  Pick a specific draw", "  How this works", "  Return to main menu"};
            for (int i = 0; i < 4; i++)
            {
                clrLine(sr + 1 + i);
                gotoxy(0, sr + 1 + i);
                std::cout << "  ";
                if (i == hi)
                {
                    sc(MAGENTA);
                    std::cout << "-> " << opts[i];
                    sc(DGRAY);
                    std::cout << desc[i];
                }
                else
                {
                    sc(DGRAY);
                    std::cout << "   " << opts[i] << desc[i];
                }
                rc();
            }
            clrLine(sr + 5);
            gotoxy(0, sr + 5);
            sc(DGRAY);
            std::cout << "  Press 1 / 2 / 3 / 4";
            rc();
        };
        drawEntry(-1);
        while (true)
        {
            int k = rk();
            if (k == '1')
            {
                drawEntry(0);
                ms(120);
                for (int r = sr; r <= sr + 5; r++)
                    clrLine(r);
                gotoxy(0, sr);
                entryChoice = 1;
                break;
            }
            if (k == '2')
            {
                drawEntry(1);
                ms(120);
                for (int r = sr; r <= sr + 5; r++)
                    clrLine(r);
                gotoxy(0, sr);
                entryChoice = 2;
                break;
            }
            if (k == '3')
            {
                gotoxy(0, sr);
                clrLine(sr);
                sc(YELLOW);
                std::cout << "  HELP - LAST DIGIT";
                rc();
                std::istringstream hs(help);
                std::string hl2;
                int hr = sr + 1;
                while (std::getline(hs, hl2))
                {
                    clrLine(hr);
                    gotoxy(0, hr);
                    sc(WHITE);
                    std::cout << hl2;
                    rc();
                    hr++;
                }
                clrLine(hr);
                gotoxy(0, hr);
                sc(DGRAY);
                std::cout << "  Press any key...";
                rc();
                rk();
                drawEntry(-1);
                continue;
            }
            if (k == '4' || k == KX)
            {
                for (int r = sr; r <= sr + 5; r++)
                    clrLine(r);
                gotoxy(0, sr);
                cls();
                return;
            }
        }
    }

    // ── Step 1: Window size (how many draws deep to scan) ──
    cls();
    std::cout << "\n";
    hl('=', DARK_MAG);
    sc(MAGENTA);
    std::cout << "  [ LAST DIGIT — Window Size ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  How many draws to include in the scan window?\n\n";
    sc(DGRAY);
    std::cout << "  This is the END of your window — the program scans\n";
    std::cout << "  this many draws starting from your chosen start point.\n\n";
    std::cout << "  Examples:\n";
    std::cout << "    10   = scan only the last 10 draws\n";
    std::cout << "    100  = scan the last 100 draws\n";
    std::cout << "    500  = scan the last 500 draws\n";
    std::cout << "    1000 = full 1000-draw window (maximum)\n\n";
    sc(CYAN);
    std::cout << "  Quick picks: [1]=10  [2]=50  [3]=100  [4]=500  [5]=1000\n";
    sc(DGRAY);
    std::cout << "  Or type any number from 1 to 1000.\n\n";
    rc();
    hl('-', DGRAY);
    int windowSize = 0;
    while (true)
    {
        std::string ws = inp("  Window size (1-1000) or quick pick 1-5: ");
        if (ws == "1")
        {
            windowSize = 10;
            break;
        }
        if (ws == "2")
        {
            windowSize = 50;
            break;
        }
        if (ws == "3")
        {
            windowSize = 100;
            break;
        }
        if (ws == "4")
        {
            windowSize = 500;
            break;
        }
        if (ws == "5")
        {
            windowSize = 1000;
            break;
        }
        try
        {
            windowSize = std::stoi(ws);
        }
        catch (...)
        {
            windowSize = 0;
        }
        if (windowSize >= 1 && windowSize <= 1000)
            break;
        sc(RED);
        std::cout << "  Invalid. Enter 1-1000 (or quick pick 1-5).\n";
        rc();
    }
    sc(CYAN);
    std::cout << "  Window set to: ";
    sc(WHITE);
    std::cout << windowSize << " draws\n\n";
    rc();

    // ── Step 2: How many ranked results to show ─────────
    cls();
    std::cout << "\n";
    hl('=', DARK_MAG);
    sc(MAGENTA);
    std::cout << "  [ LAST DIGIT — Results to Show ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  How many last digits to display?\n\n";
    sc(DGRAY);
    std::cout << "  The window (" << windowSize << " draws) may contain hundreds of unique\n";
    std::cout << "  combos. Choose how many to rank and show:\n\n";
    std::cout << "  1   = only the #1 true last digit\n";
    std::cout << "  10  = last 10 unseen combos\n";
    std::cout << "  999 = full ranked list\n\n";
    rc();
    hl('-', DGRAY);
    int N = 0;
    while (true)
    {
        std::string ns = inp("  Enter count (1-999): ");
        try
        {
            N = std::stoi(ns);
        }
        catch (...)
        {
            N = 0;
        }
        if (N >= 1 && N <= 999)
            break;
        sc(RED);
        std::cout << "  Invalid. Enter 1-999.\n";
        rc();
    }

    // Load
    cls();
    std::cout << "\n";
    hl('=', DARK_MAG);
    sc(MAGENTA);
    std::cout << "  [ LAST DIGIT — Loading ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Checking for new draws...\n";
    rc();
    tryAutoSync(true);
    pulse("Loading digit_data.txt...", 2);
    auto entries = loadDB();
    if (entries.empty())
    {
        sc(RED);
        std::cout << "\n  No data.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    auto slotOrd = [](const std::string &t) -> int
    {if(t=="9pm")return 2;if(t=="5pm")return 1;return 0; };
    std::sort(entries.begin(), entries.end(), [&](const Entry &a, const Entry &b)
              {
        int da=dateToInt(a.date),db=dateToInt(b.date);if(da!=db)return da>db;
        return slotOrd(a.time)>slotOrd(b.time); });

    int startIdx = 0;
    if (entryChoice == 2)
    {
        cls();
        std::cout << "\n";
        hl('=', DARK_MAG);
        sc(MAGENTA);
        std::cout << "  [ LAST DIGIT — Starting Point ]\n";
        rc();
        hl('-', DGRAY);
        std::cout << "\n";
        sc(DGRAY);
        std::cout << "  The 1000-draw window begins at this draw.\n\n";
        rc();
        std::string sDate = inp("  Date  (e.g. Mar 5, 2026) : ");
        std::string sTime = inp("  Time  (2pm / 5pm / 9pm)  : ");
        std::cout << "\n";
        if (!vtm(sTime))
        {
            sc(RED);
            std::cout << "  Invalid time.\n";
            rc();
            std::cout << "\n  Press any key...\n";
            rk();
            cls();
            return;
        }
        bool found = false;
        for (int i = 0; i < (int)entries.size(); i++)
            if (entries[i].date == sDate && entries[i].time == sTime)
            {
                startIdx = i;
                found = true;
                break;
            }
        if (!found)
        {
            sc(RED);
            std::cout << "  Draw not found: ";
            sc(WHITE);
            std::cout << sDate << " " << sTime;
            rc();
            std::cout << "\n";
            sc(DGRAY);
            std::cout << "  Make sure it exists in your DB.\n";
            rc();
            std::cout << "\n  Press any key...\n";
            rk();
            cls();
            return;
        }
    }

    int totalDB = (int)entries.size();
    int windowEnd = std::min(startIdx + windowSize, totalDB); // user-defined window end
    std::string startDateDisplay = entries[startIdx].date;
    std::string drawEndDisplay = entries[windowEnd - 1].date;
    auto results = computeLastDigits(entries, startIdx, windowSize, N);
    if (results.empty())
    {
        sc(RED);
        std::cout << "\n  Not enough data in this window.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }

    std::set<std::string> tmpSeen;
    int totalUnique = 0;
    for (int i = startIdx; i < windowEnd; i++)
        if (tmpSeen.insert(trimS(entries[i].digit)).second)
            totalUnique++;

    // Display last digit results
    cls();
    std::cout << "\n";
    hl('=', DARK_MAG);
    sc(MAGENTA);
    std::cout << "  [ LAST DIGIT — Last " << N << " in " << windowSize << "-Draw Window ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Start : draw #1 = ";
    sc(CYAN);
    std::cout << startDateDisplay;
    if (entryChoice == 2)
    {
        sc(YELLOW);
        std::cout << "  " << entries[startIdx].time;
        sc(DGRAY);
        std::cout << "  [custom]";
    }
    rc();
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  End   : draw #" << (windowEnd - startIdx) << " = ";
    sc(CYAN);
    std::cout << drawEndDisplay;
    rc();
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Window: ";
    sc(YELLOW);
    std::cout << windowSize;
    sc(DGRAY);
    std::cout << " draws  |  Unique combos: ";
    sc(YELLOW);
    std::cout << totalUnique;
    sc(DGRAY);
    std::cout << "  |  showing last " << N;
    rc();
    std::cout << "\n\n";
    hl('-', DGRAY);
    sc(DGRAY);
    std::cout << "  Rank  Combo       Last seen                   Window pos\n";
    rc();
    hl('-', DGRAY);

    for (const auto &r : results)
    {
        if (r.uniqueRank == 1)
            sc(RED);
        else if (r.uniqueRank == 2)
            sc(YELLOW);
        else
            sc(WHITE);
        std::cout << "  #" << std::left << std::setw(4) << r.uniqueRank;
        sc(WHITE);
        std::cout << std::setw(12) << r.digit;
        sc(CYAN);
        std::cout << std::setw(20) << r.date;
        sc(MAGENTA);
        std::cout << std::setw(6) << r.time;
        sc(DGRAY);
        std::cout << "  draw #" << r.posInWindow;
        rc();
        std::cout << "\n";
    }
    std::cout << "\n";
    hl('-', DGRAY);
    sc(DGRAY);
    std::cout << "  #1 = combo farthest back in window (true last digit)\n";
    rc();

    // Follow-up options
    std::cout << "\n";
    hl('-', DGRAY);
    std::string lastDay = results[0].date, topDay = entries[startIdx].date;
    while (true)
    {
        sc(DGRAY);
        std::cout << "  [1] Back to main menu\n  [2] Show combos drawn on BOTH:\n";
        sc(WHITE);
        std::cout << "      last-digit day (";
        sc(RED);
        std::cout << lastDay;
        sc(WHITE);
        std::cout << ")  AND  start day (";
        sc(CYAN);
        std::cout << topDay;
        sc(WHITE);
        std::cout << ")\n";
        rc();
        std::cout << "\n  ";
        sc(DGRAY);
        std::cout << "Press 1 or 2 : ";
        rc();
        int k = rk();
        if (k == '1' || k == KX)
        {
            cls();
            return;
        }
        if (k != '2')
            continue;
        auto lastDayDraws = getDrawsOnDate(entries, lastDay);
        auto topDayDraws = getDrawsOnDate(entries, topDay);
        std::set<std::string> lastSet, topSet;
        for (const auto &e : lastDayDraws)
            lastSet.insert(trimS(e.digit));
        for (const auto &e : topDayDraws)
            topSet.insert(trimS(e.digit));
        std::vector<std::string> both;
        for (const auto &d : lastSet)
            if (topSet.count(d))
                both.push_back(d);
        cls();
        std::cout << "\n";
        hl('=', DARK_MAG);
        sc(MAGENTA);
        std::cout << "  [ SHARED COMBOS ]\n";
        rc();
        hl('-', DGRAY);
        std::cout << "\n";
        sc(WHITE);
        std::cout << "  Last-digit day : ";
        sc(RED);
        std::cout << lastDay;
        rc();
        std::cout << "\n";
        sc(WHITE);
        std::cout << "  Start day      : ";
        sc(CYAN);
        std::cout << topDay;
        rc();
        std::cout << "\n\n";
        if (both.empty())
        {
            sc(DGRAY);
            std::cout << "  No combos appeared on both days.\n";
            rc();
        }
        else
        {
            hl('-', DGRAY);
            sc(GREEN);
            std::cout << "  " << both.size() << " shared combo" << (both.size() > 1 ? "s" : "") << ":\n";
            rc();
            hl('-', DGRAY);
            for (int i = 0; i < (int)both.size(); i++)
            {
                sc(YELLOW);
                std::cout << "  " << std::setw(3) << (i + 1) << ".  ";
                sc(WHITE);
                std::cout << std::left << std::setw(10) << both[i];
                rc();
                sc(DGRAY);
                std::cout << "  last-digit day: ";
                for (const auto &e : lastDayDraws)
                    if (trimS(e.digit) == both[i])
                    {
                        sc(RED);
                        std::cout << e.time;
                        rc();
                    }
                sc(DGRAY);
                std::cout << "   start day: ";
                for (const auto &e : topDayDraws)
                    if (trimS(e.digit) == both[i])
                    {
                        sc(CYAN);
                        std::cout << e.time;
                        rc();
                    }
                rc();
                std::cout << "\n";
            }
        }
        hl('=', DARK_MAG);
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
}

// ════════════════════════════════════════════════════
//  BROWSE
// ════════════════════════════════════════════════════
void doBrowse()
{
    cls();
    std::cout << "\n";
    hl('=', GREEN);
    sc(GREEN);
    std::cout << "  [ BROWSE - Swertres Results ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help = "  Shows current year Swertres results.\n  Auto-syncs new draws if online.\n  Falls back to local DB if offline.\n";
    if (!subMenu2("BROWSE", help))
    {
        cls();
        return;
    }
    cls();
    std::cout << "\n";
    hl('=', GREEN);
    sc(GREEN);
    std::cout << "  [ BROWSE — Loading ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Fetching current year results...\n";
    rc();
    int cy = getCurrentYear();
    bool online = isOnline();
    std::vector<DrawRow> rows;
    if (online)
    {
        std::string html = fetchPageForYear(cy, cy);
        if (!html.empty())
        {
            rows = parseHTML(html);
            auto newEntries = rowsToEntries(rows);
            int added = mergeIntoDB(newEntries);
            if (added > 0)
            {
                sc(GREEN);
                std::cout << "  [+] " << added << " new record(s) saved\n";
                rc();
            }
        }
    }
    if (rows.empty())
    {
        sc(YELLOW);
        std::cout << "  " << (online ? "Fetch failed — " : "Offline — ") << "showing local DB.\n";
        rc();
        auto data = loadDB();
        std::map<std::string, DrawRow> byDate;
        for (const auto &e : data)
        {
            if (yearFromLabel(e.date) != cy)
                continue;
            DrawRow &dr = byDate[e.date];
            dr.date = e.date;
            if (e.time == "2pm")
                dr.pm2 = e.digit;
            else if (e.time == "5pm")
                dr.pm5 = e.digit;
            else if (e.time == "9pm")
                dr.pm9 = e.digit;
        }
        for (auto &p : byDate)
            rows.push_back(p.second);
        std::sort(rows.begin(), rows.end(), [](const DrawRow &a, const DrawRow &b)
                  { return dateToInt(a.date) > dateToInt(b.date); });
    }
    std::cout << "\n";
    hl('=', GREEN);
    sc(GREEN);
    std::cout << "  SWERTRES — " << cy << "\n";
    rc();
    sc(DGRAY);
    std::cout << "  Source: " << (online ? "lottopcso.com" : "local DB") << "\n";
    rc();
    hl('-', DGRAY);
    sc(GREEN);
    std::cout << "  " << std::left << std::setw(22) << "DATE" << std::setw(12) << "2:00 PM" << std::setw(12) << "5:00 PM" << std::setw(12) << "9:00 PM\n";
    rc();
    hl('-', DGRAY);
    for (int i = 0; i < (int)rows.size(); i++)
    {
        sc(i % 2 == 0 ? WHITE : DGRAY);
        std::cout << "  " << std::left << std::setw(22) << rows[i].date;
        sc(YELLOW);
        std::cout << std::setw(12) << (rows[i].pm2.empty() ? "--" : rows[i].pm2);
        sc(CYAN);
        std::cout << std::setw(12) << (rows[i].pm5.empty() ? "--" : rows[i].pm5);
        sc(MAGENTA);
        std::cout << std::setw(12) << (rows[i].pm9.empty() ? "--" : rows[i].pm9);
        rc();
        std::cout << "\n";
    }
    hl('=', GREEN);
    sc(DGRAY);
    std::cout << "  " << rows.size() << " entries.\n";
    rc();
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

// ════════════════════════════════════════════════════
//  IMPORT — file browser + terminal input
// ════════════════════════════════════════════════════
static const char *MONTH_NAMES[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
std::string csvTimeToSlot(const std::string &t)
{
    if (t == "11:00" || t == "14:00")
        return "2pm";
    if (t == "16:00" || t == "17:00")
        return "5pm";
    if (t == "21:00")
        return "9pm";
    return "";
}
std::string csvDateToLabel(const std::string &d)
{
    if (d.size() < 10)
        return d;
    int day = 0, mon = 0, yr = 0;
    try
    {
        day = std::stoi(d.substr(0, 2));
        mon = std::stoi(d.substr(3, 2));
        yr = std::stoi(d.substr(6, 4));
    }
    catch (...)
    {
        return d;
    }
    if (mon < 1 || mon > 12 || day < 1 || day > 31)
        return d;
    return std::string(MONTH_NAMES[mon]) + " " + std::to_string(day) + ", " + std::to_string(yr);
}
bool parseCSVLine(const std::string &line, Entry &out)
{
    std::istringstream ss(line);
    std::string id, date, time, d1, d2, d3;
    if (!std::getline(ss, id, ','))
        return false;
    if (!std::getline(ss, date, ','))
        return false;
    if (!std::getline(ss, time, ','))
        return false;
    if (!std::getline(ss, d1, ','))
        return false;
    if (!std::getline(ss, d2, ','))
        return false;
    if (!std::getline(ss, d3))
        return false;
    d1 = trimS(d1);
    d2 = trimS(d2);
    d3 = trimS(d3);
    date = trimS(date);
    time = trimS(time);
    if (date.empty() || time.empty() || d1.empty() || d2.empty() || d3.empty())
        return false;
    std::string slot = csvTimeToSlot(time);
    if (slot.empty())
        return false;
    auto strip0 = [](std::string s) -> std::string
    {while(s.size()>1&&s[0]=='0')s.erase(s.begin());return s; };
    out.date = csvDateToLabel(date);
    out.time = slot;
    out.digit = strip0(d1) + "-" + strip0(d2) + "-" + strip0(d3);
    return true;
}
bool endsWithCSV(const std::string &s)
{
    if (s.size() < 4)
        return false;
    std::string ext = s.substr(s.size() - 4);
    for (auto &c : ext)
        c = (char)tolower(c);
    return ext == ".csv";
}

// Platform-specific file picker
// Returns selected filename or "" if cancelled
std::string pickCSVFile()
{
#ifdef _WIN32
    // Windows: use GetOpenFileName dialog
    char buf[MAX_PATH] = {0};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select CSV File";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn))
        return std::string(buf);
    return "";
#else
    // Linux/Termux: list .csv files in current directory
    std::vector<std::string> csvFiles;
    DIR *dir = opendir(".");
    if (dir)
    {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL)
        {
            std::string name = ent->d_name;
            if (endsWithCSV(name))
                csvFiles.push_back(name);
        }
        closedir(dir);
    }
    std::sort(csvFiles.begin(), csvFiles.end());
    if (csvFiles.empty())
    {
        sc(YELLOW);
        std::cout << "\n  No .csv files found in current directory.\n";
        rc();
        sc(DGRAY);
        std::cout << "  Use option [2] to type the file path manually.\n";
        rc();
        return "";
    }
    cls();
    std::cout << "\n";
    hl('=', YELLOW);
    sc(YELLOW);
    std::cout << "  [ IMPORT — Select CSV File ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  CSV files in current directory:\n\n";
    rc();
    for (int i = 0; i < (int)csvFiles.size(); i++)
    {
        sc(CYAN);
        std::cout << "  [" << (i + 1) << "] ";
        sc(WHITE);
        std::cout << csvFiles[i];
        rc();
        std::cout << "\n";
    }
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  [0] Cancel\n\n";
    rc();
    hl('-', DGRAY);
    while (true)
    {
        std::string ns = inp("  Enter number (0 to cancel): ");
        int sel = -1;
        try
        {
            sel = std::stoi(ns);
        }
        catch (...)
        {
            sel = -1;
        }
        if (sel == 0)
            return "";
        if (sel >= 1 && sel <= (int)csvFiles.size())
            return csvFiles[sel - 1];
        sc(RED);
        std::cout << "  Invalid. Enter 0-" << csvFiles.size() << "\n";
        rc();
    }
#endif
}

void doImportCSV(const std::string &filename)
{
    if (filename.empty())
    {
        cls();
        return;
    }
    // Validate extension
    if (!endsWithCSV(filename))
    {
        std::cout << "\n";
        hl('-', RED);
        sc(RED);
        std::cout << "  [!] Only .csv files are allowed.\n";
        rc();
        sc(DGRAY);
        std::cout << "  Selected: ";
        sc(WHITE);
        std::cout << filename;
        rc();
        std::cout << "\n";
        hl('-', RED);
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    std::ifstream f(filename);
    if (!f.is_open())
    {
        std::cout << "\n";
        hl('-', RED);
        sc(RED);
        std::cout << "  [!] Cannot open: ";
        sc(WHITE);
        std::cout << filename;
        rc();
        std::cout << "\n";
        sc(DGRAY);
        std::cout << "  Check the file path.\n";
        rc();
        hl('-', RED);
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    int totalLines = 0;
    {
        std::string tmp;
        while (std::getline(f, tmp))
            if (!tmp.empty())
                totalLines++;
    }
    f.clear();
    f.seekg(0);
    if (!confirmDlg("Import " + std::to_string(totalLines) + " rows from " + filename + "?"))
    {
        f.close();
        cls();
        return;
    }
    std::cout << "\n";
    int lr = gpos().Y;
    std::vector<Entry> parsed;
    int lineNum = 0, skippedParse = 0;
    std::string line;
    while (std::getline(f, line))
    {
        line = trimS(line);
        if (line.empty())
            continue;
        lineNum++;
        if (lineNum % 200 == 0)
        {
            gotoxy(0, lr);
            clrLine(lr);
            sc(CYAN);
            std::cout << "  Parsing... " << lineNum << "/" << totalLines;
            rc();
            std::cout << std::flush;
        }
        Entry e;
        if (parseCSVLine(line, e))
            parsed.push_back(e);
        else
            skippedParse++;
    }
    f.close();
    clrLine(lr);
    gotoxy(0, lr);
    if (parsed.empty())
    {
        std::cout << "\n";
        hl('-', RED);
        sc(RED);
        std::cout << "  [!] No valid records.\n";
        rc();
        sc(DGRAY);
        std::cout << "  Check CSV format.\n";
        rc();
        hl('-', RED);
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    pulse("Merging into digit_data.txt", 2);
    int added = mergeIntoDB(parsed), dupSkipped = (int)parsed.size() - added;
    std::cout << "\n";
    hl('-', GREEN);
    sc(GREEN);
    std::cout << "  [v] IMPORT COMPLETE\n";
    rc();
    sc(WHITE);
    std::cout << "  File          : ";
    sc(YELLOW);
    std::cout << filename;
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Rows in CSV   : ";
    sc(CYAN);
    std::cout << totalLines;
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Valid parsed  : ";
    sc(CYAN);
    std::cout << parsed.size();
    rc();
    std::cout << "\n";
    if (skippedParse > 0)
    {
        sc(WHITE);
        std::cout << "  Bad format    : ";
        sc(RED);
        std::cout << skippedParse;
        rc();
        std::cout << "\n";
    }
    sc(WHITE);
    std::cout << "  New records   : ";
    sc(GREEN);
    std::cout << added;
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Skipped (dup) : ";
    sc(DGRAY);
    std::cout << dupSkipped;
    rc();
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  Total in DB   : ";
    sc(CYAN);
    std::cout << loadDB().size();
    rc();
    std::cout << "\n";
    hl('-', GREEN);
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

void doImport()
{
    cls();
    std::cout << "\n";
    hl('=', YELLOW);
    sc(YELLOW);
    std::cout << "  [ IMPORT ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  Imports a CSV file into digit_data.txt.\n\n"
        "  FORMAT: ID,DD.MM.YYYY,HH:MM,d1,d2,d3\n"
        "  e.g.  00001,02.01.2007,11:00,05,08,07\n\n"
        "  TIME: 11:00/14:00->2pm  16:00->5pm  21:00->9pm\n"
        "  Only .csv files are accepted.\n"
        "  Duplicates are skipped automatically.\n\n"
        "  [1] Browse  - Open file picker / list CSVs\n"
        "  [2] Type    - Enter filename or path manually\n";

    int sr = gpos().Y;
    auto drawImport = [&](int hi)
    {
        gotoxy(0, sr);
        clrLine(sr);
        sc(YELLOW);
        std::cout << "  [ IMPORT ]";
        rc();
        const char *opts[] = {"[1] Browse File", "[2] Type Filename / Path", "[3] Help", "[4] Back"};
        const char *desc[] = {
#ifdef _WIN32
            "  Open Windows file picker (CSV only)",
#else
            "  List .csv files in current folder",
#endif
            "  Type the filename or path manually",
            "  Format & usage info",
            "  Return to main menu"};
        for (int i = 0; i < 4; i++)
        {
            clrLine(sr + 1 + i);
            gotoxy(0, sr + 1 + i);
            std::cout << "  ";
            if (i == hi)
            {
                sc(YELLOW);
                std::cout << "-> " << opts[i];
                sc(DGRAY);
                std::cout << desc[i];
            }
            else
            {
                sc(DGRAY);
                std::cout << "   " << opts[i] << desc[i];
            }
            rc();
        }
        clrLine(sr + 5);
        gotoxy(0, sr + 5);
        sc(DGRAY);
        std::cout << "  Press 1 / 2 / 3 / 4";
        rc();
    };
    drawImport(-1);
    while (true)
    {
        int k = rk();
        if (k == '1')
        {
            drawImport(0);
            ms(120);
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            std::string file = pickCSVFile();
            if (!file.empty())
                doImportCSV(file);
            else
            {
                cls();
                std::cout << "\n";
                sc(DGRAY);
                std::cout << "  No file selected.\n";
                rc();
                ms(1000);
                cls();
            }
            return;
        }
        if (k == '2')
        {
            drawImport(1);
            ms(120);
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            cls();
            std::cout << "\n";
            hl('=', YELLOW);
            sc(YELLOW);
            std::cout << "  [ IMPORT — Enter File Path ]\n";
            rc();
            hl('-', DGRAY);
            std::cout << "\n";
            sc(DGRAY);
            std::cout << "  Enter the CSV filename or full path.\n";
            std::cout << "  Examples:\n";
            std::cout << "    swertres.csv\n";
            std::cout << "    /home/user/downloads/swertres.csv\n\n";
            rc();
            hl('-', DGRAY);
            std::string filename = inp("  CSV file (or path): ");
            if (filename.empty())
            {
                cls();
                return;
            }
            // Auto-append .csv if missing
            if (!endsWithCSV(filename))
            {
                std::ifstream test(filename + ".csv");
                if (test.is_open())
                {
                    test.close();
                    filename += ".csv";
                }
            }
            doImportCSV(filename);
            return;
        }
        if (k == '3')
        {
            gotoxy(0, sr);
            clrLine(sr);
            sc(YELLOW);
            std::cout << "  HELP - IMPORT";
            rc();
            std::istringstream hs(help);
            std::string hl2;
            int hr = sr + 1;
            while (std::getline(hs, hl2))
            {
                clrLine(hr);
                gotoxy(0, hr);
                sc(WHITE);
                std::cout << hl2;
                rc();
                hr++;
            }
            clrLine(hr);
            gotoxy(0, hr);
            sc(DGRAY);
            std::cout << "  Press any key...";
            rc();
            rk();
            drawImport(-1);
            continue;
        }
        if (k == '4' || k == KX)
        {
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            cls();
            return;
        }
    }
}

// ════════════════════════════════════════════════════
//  MAIN MENU  — 8 items (removed Insert/Edit, Delete)
// ════════════════════════════════════════════════════
void mainMenu()
{
    struct MI
    {
        std::string code, label, desc;
    };
    std::vector<MI> items = {
        {"[1]", "S-R-W", "Strong / Random / Weak combo analyzer"},
        {"[2]", "Show All", "Browse all records (auto-synced)"},
        {"[3]", "Probability", "Analyze digit frequency"},
        {"[4]", "Last Digit", "Find combos + repeating digit combos"},
        {"[5]", "Browse", "View current year results"},
        {"[6]", "Sync DB", "Full manual sync from lottopcso.com"},
        {"[7]", "Import", "Load CSV file into digit_data.txt"},
        {"[0]", "Exit", "Quit the application"},
    };
    int sel = 0, n = (int)items.size();
    while (true)
    {
        cls();
        cur(false);
        std::cout << "\n";
        hl('=', CYAN);
        cprt("D I G I T   T R A C K E R   v 1 . 1", CYAN);
        std::cout << "\n";
        cprt("Local + Online  |  Swertres 3D  |  lottopcso.com", DGRAY);
        std::cout << "\n";
        hl('=', CYAN);
        std::cout << "\n  ";
        sc(DGRAY);
        std::cout << "Navigate: Up/Down or W/S     Select: Enter or number key";
        rc();
        std::cout << "\n\n";
        for (int i = 0; i < n; i++)
        {
            bool a = (i == sel);
            std::cout << "  ";
            if (a)
            {
                sc(CYAN);
                std::cout << "-> ";
                sc(WHITE);
                std::cout << items[i].code << "  " << std::left << std::setw(16) << items[i].label;
                sc(DGRAY);
                std::cout << "  " << items[i].desc;
            }
            else
            {
                sc(DGRAY);
                std::cout << "   " << items[i].code << "  " << std::left << std::setw(16) << items[i].label << "  " << items[i].desc;
            }
            rc();
            std::cout << "\n";
        }
        std::cout << "\n";
        hl('-', DGRAY);
        auto d = loadDB();
        sc(DGRAY);
        std::cout << "  Local DB: ";
        sc(GREEN);
        std::cout << d.size();
        sc(DGRAY);
        std::cout << " records  |  Auto-Sync: ON when connected";
        rc();
        std::cout << "\n";
        cur(false);
        int k = rk();
        if (k == 72 + 256 || k == 'w' || k == 'W')
            sel = (sel - 1 + n) % n;
        else if (k == 80 + 256 || k == 's' || k == 'S')
            sel = (sel + 1) % n;
        else if (k >= '1' && k <= '7')
        {
            sel = k - '1';
            goto act;
        }
        else if (k == '0')
        {
            sel = n - 1;
            goto act;
        }
        else if (k == KE)
            goto act;
        else if (k == 'q' || k == 'Q')
        {
            sel = n - 1;
            goto act;
        }
        continue;
    act:
        cur(true);
        if (sel == 0)
            doSRW();
        else if (sel == 1)
            doShowAll();
        else if (sel == 2)
            doProb();
        else if (sel == 3)
            doLastDigit();
        else if (sel == 4)
            doBrowse();
        else if (sel == 5)
            doSyncDB();
        else if (sel == 6)
            doImport();
        else if (sel == 7)
        {
            if (confirmDlg("Exit?"))
            {
                cls();
                std::cout << "\n\n";
                cprt("Thank you for using Digit Tracker v1.1!", CYAN);
                std::cout << "\n";
                cprt("Data saved to " + DB, DGRAY);
                std::cout << "\n\n";
                return;
            }
        }
    }
}

// ════════════════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════════════════
int main()
{
#ifdef _WIN32
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    DWORD mode;
    GetConsoleMode(hCon, &mode);
    SetConsoleMode(hCon, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleTitleA("Digit Tracker v1.1");
    SMALL_RECT ws = {0, 0, 119, 44};
    SetConsoleWindowInfo(hCon, TRUE, &ws);
    COORD bs = {120, 3000};
    SetConsoleScreenBufferSize(hCon, bs);
#else
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
    showIntro();
    mainMenu();
    rc();
    cur(true);
#ifndef _WIN32
    curl_global_cleanup();
#endif
    return 0;
}