#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <iomanip>
#include <windows.h>
#include <winhttp.h>
#include <conio.h>

#pragma comment(lib, "winhttp.lib")

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
COORD gpos()
{
    CONSOLE_SCREEN_BUFFER_INFO i;
    GetConsoleScreenBufferInfo(hCon, &i);
    return i.dwCursorPosition;
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

#define KE 13
#define KX 27
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

void cprt(const std::string &s, int col = WHITE)
{
    int p = (CW() - (int)s.size()) / 2;
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
//  LOADING
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
//  DATABASE  (local)
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
    std::getline(std::cin, s);
    rc();
    return trimS(s);
}

// ════════════════════════════════════════════════════
//  NETWORK
// ════════════════════════════════════════════════════
std::string fetchPage(const std::wstring &host, const std::wstring &path)
{
    std::string result;
    HINTERNET hS = WinHttpOpen(L"DigitTracker/1.2", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
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
            unsigned char t = (unsigned char)s[i + 2];
            if (t == 0x93 || t == 0x94)
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

bool isOnline()
{
    HINTERNET hS = WinHttpOpen(L"DigitTracker/1.2", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hS)
        return false;
    HINTERNET hC = WinHttpConnect(hS, L"www.lottopcso.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hC)
    {
        WinHttpCloseHandle(hS);
        return false;
    }
    HINTERNET hR = WinHttpOpenRequest(hC, L"HEAD", L"/", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    DWORD to = 6000;
    WinHttpSetOption(hR, WINHTTP_OPTION_CONNECT_TIMEOUT, &to, sizeof(to));
    WinHttpSetOption(hR, WINHTTP_OPTION_RECEIVE_TIMEOUT, &to, sizeof(to));
    bool ok = hR && WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hR, NULL);
    if (hR)
        WinHttpCloseHandle(hR);
    WinHttpCloseHandle(hC);
    WinHttpCloseHandle(hS);
    return ok;
}

// Valid digit cell: has at least one non-dash character
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

// Fetch one year page and return entries
std::vector<Entry> fetchYear(int yr, int cy)
{
    std::wstring path;
    if (yr == cy)
        path = L"/swertres-results-today-history-and-summary/";
    else
    {
        std::wstring ys = std::to_wstring(yr);
        path = L"/swertres-results-today-history-and-summary-" + ys + L"/";
    }
    std::string html = fetchPage(L"www.lottopcso.com", path);
    if (html.empty())
        return {};
    return rowsToEntries(parseHTML(html));
}

// Merge newEntries into existing, skipping duplicates (date+time+digit all match)
// Returns count of actually added entries
int mergeIntoDB(const std::vector<Entry> &newEntries)
{
    auto existing = loadDB();
    // Build a set of keys for fast lookup
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

void noInternet()
{
    std::cout << "\n";
    hl('-', RED);
    sc(RED);
    std::cout << "  [!] NO INTERNET\n";
    rc();
    sc(WHITE);
    std::cout << "  Could not reach lottopcso.com.\n";
    sc(DGRAY);
    std::cout << "  Returning to main menu...\n";
    rc();
    hl('-', RED);
    ms(2000);
    cls();
}

// ════════════════════════════════════════════════════
//  INTRO
// ════════════════════════════════════════════════════
void showIntro()
{
    cls();
    cur(false);
    int H = CH();
    ms(150);
    std::vector<std::pair<std::string, int>> art = {
        {" ____  _       _ _    v1.0", CYAN},
        {"|  _\\(_) __ _(_) |_ ", CYAN},
        {"| | | | |/ _` | | __|", CYAN},
        {"| |_| | | (_| | | |_ ", CYAN},
        {"|____/|_|\\__,|_|\\__|", CYAN},
        {"         |___/       ", CYAN},
        {"", WHITE},
        {"  _____               _             ", MAGENTA},
        {" |_   _| __ __ _  ___| | _____ _ __ ", MAGENTA},
        {"   | || '__/ _` |/ __| |/ / _ \\ '__|", MAGENTA},
        {"   | || | | (_| | (__|   <  __/| | ", MAGENTA},
        {"   |_||_| \\__,_|\\__|_|\\_\\__|_|  ", MAGENTA},
    };
    int start = (H - (int)art.size() - 10) / 2;
    if (start < 1)
        start = 1;
    for (int i = 0; i < (int)art.size(); i++)
    {
        gotoxy(0, start + i);
        clrLine(start + i);
        int w = CW(), pad = (w - (int)art[i].first.size()) / 2;
        if (pad > 0)
            std::cout << std::string(pad, ' ');
        sc(art[i].second);
        std::cout << art[i].first;
        rc();
        std::cout << std::flush;
        ms(50);
    }
    ms(300);
    int br = start + (int)art.size();
    gotoxy(0, br + 1);
    cprt("=================================================================", DGRAY);
    gotoxy(0, br + 2);
    cprt("  Local & Online  |  Swertres 3D  |  Full History Sync  ", DGRAY);
    gotoxy(0, br + 3);
    cprt("=================================================================", DGRAY);
    ms(300);
    gotoxy(0, br + 5);
    cprt("  >> 3D Lotto  2PM  5PM  9PM  |  lottopcso.com  <<  ", GREEN);
    std::cout << "\n";
    gotoxy(2, br + 7);
    showLoad("Initializing System", 1400);
    gotoxy(0, br + 8);
    int w2 = CW(), pad = (w2 - 30) / 2;
    if (pad > 0)
        std::cout << std::string(pad, ' ');
    sc(WHITE);
    std::cout << "Press ";
    sc(YELLOW);
    std::cout << "[ENTER]";
    sc(WHITE);
    std::cout << " to begin...";
    rc();
    cur(true);
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    while (true)
    {
        int k = rk();
        if (k == KE || k == '\r' || k == '\n')
            break;
    }
}

// ════════════════════════════════════════════════════
//  CONFIRM DIALOG  (number keys only)
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
        std::cout << "  Press 1 = YES   2 = NO   Esc = NO";
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
//  SUB-MENU A: [1] Continue Local  [2] Continue Online  [3] Help  [4] Back
//  Returns: 0=local, 1=online, -1=back
// ════════════════════════════════════════════════════
int subMenu(const std::string &name, const std::string &helpText)
{
    int sr = gpos().Y;
    auto draw = [&](int hi)
    {
        gotoxy(0, sr);
        clrLine(sr);
        sc(CYAN);
        std::cout << "  [ " << name << " ]";
        rc();
        const char *opts[] = {"[1] Continue Local", "[2] Continue Online", "[3] Help", "[4] Back"};
        for (int i = 0; i < 4; i++)
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
        clrLine(sr + 5);
        gotoxy(0, sr + 5);
        sc(DGRAY);
        std::cout << "  Press 1 / 2 / 3 / 4";
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
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            return 0;
        }
        if (k == '2')
        {
            draw(1);
            ms(120);
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            return 1;
        }
        if (k == '3')
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
        if (k == '4' || k == KX)
        {
            for (int r = sr; r <= sr + 5; r++)
                clrLine(r);
            gotoxy(0, sr);
            return -1;
        }
    }
}

// ════════════════════════════════════════════════════
//  SUB-MENU B: [1] Continue  [2] Help  [3] Back
//  Returns: true=continue, false=back
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
//  YEAR PICKER: shown after Continue in Sync DB
//  Lets user pick: a specific year, a range, or all
//  Returns list of years to fetch, empty = cancelled
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
        std::string sy1 = inp("  From year: ");
        std::string sy2 = inp("  To year  : ");
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
            years.push_back(y); // newest first
    }
    else
    {
        for (int y = cy; y >= 2009; y--)
            years.push_back(y);
    }

    // Show summary and confirm
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
    std::cout << "  Duplicate entries will be skipped.\n\n";
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
        "  and stores them in digit_data.txt.\n"
        "\n"
        "  After continuing, you will choose:\n"
        "    [1] A specific year (e.g. 2024)\n"
        "    [2] A year range   (e.g. 2020-2024)\n"
        "    [3] All years      (2009 to present)\n"
        "\n"
        "  DUPLICATE PROTECTION:\n"
        "    If digit_data.txt already has entries for\n"
        "    the same date, time, and digit, they are\n"
        "    skipped. Only new records are added.\n"
        "\n"
        "  Requires an active internet connection.\n";

    if (!subMenu2("SYNC DB", help))
    {
        cls();
        return;
    }

    // Check connection before showing year picker
    cls();
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Checking connection...";
    rc();
    std::cout << std::flush;
    if (!isOnline())
    {
        sc(RED);
        std::cout << "\n\n  [!] No internet. Cannot sync.\n";
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

    SYSTEMTIME st;
    GetLocalTime(&st);
    int cy = st.wYear;
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

    int totalAdded = 0, totalSkipped = 0, totalFailed = 0;
    int done = 0, total = (int)years.size();
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
        int skipped = (int)entries.size() - added;
        totalAdded += added;
        totalSkipped += skipped;
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
//  SEARCH
// ════════════════════════════════════════════════════
void doSearchLocal()
{
    std::cout << "\n";
    std::string digit = inp("  Digit combo (e.g. 5-9-2): ");
    std::string date = inp("  Date  (YYYY-MM-DD)      : ");
    std::string t = inp("  Time  (2pm/5pm/9pm)     : ");
    std::cout << "\n";
    if (!vtm(t))
    {
        sc(RED);
        std::cout << "  Invalid time.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    if (!confirmDlg("Search LOCAL: " + digit + " | " + date + " | " + t))
    {
        cls();
        return;
    }
    pulse("Searching local database", 2);
    auto data = loadDB();
    bool found = false;
    for (const auto &e : data)
        if (trimS(e.digit) == trimS(digit) && e.date == date && e.time == t)
        {
            found = true;
            std::cout << "\n";
            hl('-', GREEN);
            sc(GREEN);
            std::cout << "  [v] FOUND  [LOCAL]\n";
            rc();
            sc(WHITE);
            std::cout << "  Digit : ";
            sc(YELLOW);
            std::cout << e.digit;
            rc();
            std::cout << "\n";
            sc(WHITE);
            std::cout << "  Date  : ";
            sc(CYAN);
            std::cout << e.date;
            rc();
            std::cout << "\n";
            sc(WHITE);
            std::cout << "  Time  : ";
            sc(MAGENTA);
            std::cout << e.time;
            rc();
            std::cout << "\n";
            hl('-', GREEN);
            break;
        }
    if (!found)
    {
        std::cout << "\n";
        hl('-', RED);
        sc(RED);
        std::cout << "  [x] NOT FOUND  [LOCAL]\n";
        rc();
        sc(DGRAY);
        std::cout << "  No matching record in digit_data.txt.\n";
        rc();
        hl('-', RED);
    }
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

void doSearchOnline()
{
    std::cout << "\n";
    std::string digit = inp("  Digit combo (e.g. 5-9-2)  : ");
    std::string date = inp("  Date (e.g. Mar 4, 2026)   : ");
    std::string t = inp("  Time  (2pm/5pm/9pm)        : ");
    std::cout << "\n";
    if (!vtm(t))
    {
        sc(RED);
        std::cout << "  Invalid time.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    // Extract year from date string
    int targetYear = 0;
    for (int i = 0; i < (int)date.size() - 3; i++)
    {
        if (isdigit(date[i]) && isdigit(date[i + 1]) && isdigit(date[i + 2]) && isdigit(date[i + 3]))
        {
            targetYear = std::stoi(date.substr(i, 4));
            break;
        }
    }
    if (targetYear < 2009)
    {
        sc(RED);
        std::cout << "  Could not find year in date. Use: Mar 4, 2026\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    if (!confirmDlg("Search ONLINE: " + digit + " | " + date + " | " + t))
    {
        cls();
        return;
    }
    sc(DGRAY);
    std::cout << "  Checking connection...";
    rc();
    std::cout << std::flush;
    if (!isOnline())
    {
        noInternet();
        return;
    }
    SYSTEMTIME st;
    GetLocalTime(&st);
    int cy = st.wYear;
    int lr = gpos().Y + 1;
    std::cout << "\n";
    gotoxy(2, lr);
    sc(CYAN);
    std::cout << "  Fetching year " << targetYear << "...";
    rc();
    std::cout << std::flush;
    auto entries = fetchYear(targetYear, cy);
    clrLine(lr);
    gotoxy(0, lr);
    if (entries.empty())
    {
        sc(RED);
        std::cout << "\n  Fetch failed or no data.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    bool found = false;
    for (const auto &e : entries)
    {
        if (trimS(e.digit) == trimS(digit) && e.time == t && e.date == date)
        {
            found = true;
            std::cout << "\n";
            hl('-', GREEN);
            sc(GREEN);
            std::cout << "  [v] FOUND  [ONLINE]\n";
            rc();
            sc(WHITE);
            std::cout << "  Digit : ";
            sc(YELLOW);
            std::cout << e.digit;
            rc();
            std::cout << "\n";
            sc(WHITE);
            std::cout << "  Date  : ";
            sc(CYAN);
            std::cout << e.date;
            rc();
            std::cout << "\n";
            sc(WHITE);
            std::cout << "  Time  : ";
            sc(MAGENTA);
            std::cout << e.time;
            rc();
            std::cout << "\n";
            hl('-', GREEN);
            break;
        }
    }
    if (!found)
    {
        std::cout << "\n";
        hl('-', RED);
        sc(RED);
        std::cout << "  [x] NOT FOUND  [ONLINE]\n";
        rc();
        sc(DGRAY);
        std::cout << "  Use exact format shown on site: 'Mar 4, 2026'\n";
        rc();
        hl('-', RED);
    }
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

void doSearch()
{
    cls();
    std::cout << "\n";
    hl('=', CYAN);
    sc(CYAN);
    std::cout << "  [ SEARCH ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  LOCAL:  Searches your digit_data.txt file.\n"
        "          Date format: YYYY-MM-DD\n"
        "          Digit format: 5-9-2\n"
        "  ONLINE: Fetches only the specific year page\n"
        "          for the date you enter.\n"
        "          Date format: Mar 4, 2026\n";
    int c = subMenu("SEARCH", help);
    if (c == -1)
    {
        cls();
        return;
    }
    if (c == 0)
        doSearchLocal();
    else
        doSearchOnline();
}

// ════════════════════════════════════════════════════
//  INSERT / EDIT
// ════════════════════════════════════════════════════
void doInsert()
{
    cls();
    std::cout << "\n";
    hl('=', MAGENTA);
    sc(MAGENTA);
    std::cout << "  [ INSERT / EDIT ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help =
        "  Enter date (YYYY-MM-DD), time (2pm/5pm/9pm),\n"
        "  and the digit combo (e.g. 5-9-2).\n"
        "  Existing entry for that date+time is UPDATED.\n"
        "  Otherwise a NEW entry is created.\n";
    if (!subMenu2("INSERT / EDIT", help))
    {
        cls();
        return;
    }
    std::cout << "\n";
    std::string date = inp("  Date  (YYYY-MM-DD)    : ");
    std::string t = inp("  Time  (2pm/5pm/9pm)   : ");
    std::string digit = inp("  Digit (e.g. 5-9-2)    : ");
    std::cout << "\n";
    if (!vtm(t))
    {
        sc(RED);
        std::cout << "  Invalid time.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    if (digit.empty())
    {
        sc(RED);
        std::cout << "  Digit cannot be empty.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    if (!confirmDlg("Save " + digit + " on " + date + " at " + t))
    {
        cls();
        return;
    }
    pulse("Writing to database", 2);
    auto data = loadDB();
    bool upd = false;
    for (auto &e : data)
        if (e.date == date && e.time == t)
        {
            std::cout << "\n  ";
            sc(YELLOW);
            std::cout << "UPDATED";
            rc();
            std::cout << "  ";
            sc(DGRAY);
            std::cout << e.digit;
            rc();
            std::cout << " -> ";
            sc(GREEN);
            std::cout << digit;
            rc();
            std::cout << "\n";
            e.digit = digit;
            upd = true;
            break;
        }
    if (!upd)
    {
        data.push_back({date, t, digit});
        std::cout << "\n  ";
        sc(GREEN);
        std::cout << "INSERTED";
        rc();
        std::cout << "  ";
        sc(YELLOW);
        std::cout << digit;
        rc();
        std::cout << "  ";
        sc(CYAN);
        std::cout << date;
        rc();
        std::cout << "  ";
        sc(MAGENTA);
        std::cout << t;
        rc();
        std::cout << "\n";
    }
    saveDB(data);
    hl('-', GREEN);
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

// ════════════════════════════════════════════════════
//  SHOW ALL
// ════════════════════════════════════════════════════
void showTableLocal()
{
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
}

void showTableOnline()
{
    sc(DGRAY);
    std::cout << "\n  Checking connection...";
    rc();
    std::cout << std::flush;
    if (!isOnline())
    {
        noInternet();
        return;
    }
    SYSTEMTIME st;
    GetLocalTime(&st);
    int cy = st.wYear;
    const int FY = 2009;
    int total = cy - FY + 1, done = 0;
    std::vector<DrawRow> all;
    int lr = gpos().Y + 1;
    std::cout << "\n";
    // Fetch newest first: current year down to 2009
    for (int yr = cy; yr >= FY; yr--)
    {
        gotoxy(0, lr);
        clrLine(lr);
        sc(CYAN);
        std::cout << "  [" << std::setw(3) << done << "/" << total << "]  Fetching " << yr << "...";
        rc();
        std::cout << std::flush;
        std::wstring path;
        if (yr == cy)
            path = L"/swertres-results-today-history-and-summary/";
        else
        {
            std::wstring ys = std::to_wstring(yr);
            path = L"/swertres-results-today-history-and-summary-" + ys + L"/";
        }
        std::string html = fetchPage(L"www.lottopcso.com", path);
        if (!html.empty())
        {
            auto r = parseHTML(html);
            all.insert(all.end(), r.begin(), r.end());
        }
        done++;
        ms(150);
    }
    clrLine(lr);
    gotoxy(0, lr);
    std::cout << "\n";
    hl('=', GREEN);
    sc(GREEN);
    std::cout << "  SWERTRES FULL HISTORY  (" << all.size() << " draw days)\n";
    rc();
    sc(DGRAY);
    std::cout << "  Source: lottopcso.com  |  Newest first\n";
    rc();
    hl('-', DGRAY);
    sc(GREEN);
    std::cout << "  " << std::left << std::setw(22) << "DATE" << std::setw(12) << "2:00 PM" << std::setw(12) << "5:00 PM" << std::setw(12) << "9:00 PM\n";
    rc();
    hl('-', DGRAY);
    int shown = 0;
    for (int i = 0; i < (int)all.size(); i++)
    {
        sc(i % 2 == 0 ? WHITE : DGRAY);
        std::cout << "  " << std::left << std::setw(22) << all[i].date;
        sc(YELLOW);
        std::cout << std::setw(12) << (all[i].pm2.empty() ? "--" : all[i].pm2);
        sc(CYAN);
        std::cout << std::setw(12) << (all[i].pm5.empty() ? "--" : all[i].pm5);
        sc(MAGENTA);
        std::cout << std::setw(12) << (all[i].pm9.empty() ? "--" : all[i].pm9);
        rc();
        std::cout << "\n";
        shown++;
        if (shown % 40 == 0)
        {
            sc(DGRAY);
            std::cout << "  -- " << shown << "/" << all.size() << "  [Space=next  Q=stop] --";
            rc();
            int k = rk();
            if (k == 'q' || k == 'Q')
                break;
        }
    }
    hl('=', GREEN);
    sc(DGRAY);
    std::cout << "  " << shown << " of " << all.size() << " rows shown.\n";
    rc();
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
        "  LOCAL:  Shows all records from digit_data.txt.\n"
        "          Run Sync DB first to populate.\n"
        "          Paginated 40 rows at a time.\n"
        "  ONLINE: Fetches and streams the full history\n"
        "          from lottopcso.com (2009 to present)\n"
        "          fetching newest year first.\n"
        "          Paginated 40 rows at a time.\n"
        "          Press Space to continue, Q to stop.\n";
    int c = subMenu("SHOW ALL", help);
    if (c == -1)
    {
        cls();
        return;
    }
    std::cout << "\n";
    if (c == 0)
        showTableLocal();
    else
        showTableOnline();
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

// ════════════════════════════════════════════════════
//  SHARED PROBABILITY CALCULATION + DISPLAY
// ════════════════════════════════════════════════════
// Extract year integer from a date label like "Mar 4, 2026" -> 2026
// Returns 0 if not found
int yearFromLabel(const std::string &d)
{
    // find last 4-digit number
    int yr = 0;
    for (int i = 0; i + 3 < (int)d.size(); i++)
    {
        if (isdigit(d[i]) && isdigit(d[i + 1]) && isdigit(d[i + 2]) && isdigit(d[i + 3]))
        {
            int y = std::stoi(d.substr(i, 4));
            if (y > 1900 && y < 2100)
                yr = y;
        }
    }
    return yr;
}

// Convert date label to sortable integer: "Mar 4, 2026" -> 20260304
// For ordering within same year we embed month+day
static const char *MON_IDX[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
int dateToInt(const std::string &d)
{
    // find year
    int yr = yearFromLabel(d);
    if (yr == 0)
        return 0;
    // find month name
    int mon = 0;
    for (int m = 0; m < 12; m++)
    {
        if (d.find(MON_IDX[m]) != std::string::npos)
        {
            mon = m + 1;
            break;
        }
    }
    // find day (first 1-2 digit number that isn't the year)
    int day = 0;
    for (size_t i = 0; i < d.size(); i++)
    {
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
    }
    return yr * 10000 + mon * 100 + day;
}

void showProbResult(const std::vector<Entry> &entries, const std::string &digit, const std::string &tf, const std::string &src)
{
    int total = 0, matches = 0;
    std::vector<Entry> hits;
    for (const auto &e : entries)
    {
        if (!tf.empty() && e.time != tf)
            continue;
        total++;
        if (trimS(e.digit) == trimS(digit))
        {
            matches++;
            hits.push_back(e);
        }
    }

    // Sort hits: newest first (highest dateToInt first)
    std::sort(hits.begin(), hits.end(), [](const Entry &a, const Entry &b)
              { return dateToInt(a.date) > dateToInt(b.date); });

    double prob = (total > 0) ? (100.0 * matches / total) : 0.0;

    std::cout << "\n";
    hl('=', YELLOW);
    sc(YELLOW);
    std::cout << "  COMBO: ";
    sc(WHITE);
    std::cout << digit;
    sc(DGRAY);
    std::cout << "  [" << src << "]";
    if (!tf.empty())
    {
        sc(DGRAY);
        std::cout << "  [time: " << tf << "]";
    }
    rc();
    std::cout << "\n";
    hl('-', DGRAY);
    std::cout << "\n";

    sc(WHITE);
    std::cout << "  Appearances  ";
    sc(DGRAY);
    std::cout << "(how many times this combo was drawn)";
    rc();
    std::cout << "\n";
    std::cout << "  ";
    sc(GREEN);
    std::cout << matches;
    rc();
    std::cout << (matches == 1 ? " time" : " times") << "\n\n";

    sc(WHITE);
    std::cout << "  Scope Total  ";
    sc(DGRAY);
    std::cout << "(total draws checked";
    if (!tf.empty())
        std::cout << " at " << tf;
    else
        std::cout << " across all slots";
    std::cout << ")";
    rc();
    std::cout << "\n";
    std::cout << "  ";
    sc(CYAN);
    std::cout << total;
    rc();
    std::cout << " draw results\n\n";

    sc(WHITE);
    std::cout << "  Probability  ";
    sc(DGRAY);
    std::cout << "(appearances / scope total)";
    rc();
    std::cout << "\n";
    std::cout << "  ";
    sc(YELLOW);
    std::cout << matches << " / " << total << " = " << std::fixed << std::setprecision(4) << prob << "%";
    rc();
    std::cout << "\n\n";

    int fill = (int)(prob / 100.0 * 40);
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
    std::cout << std::fixed << std::setprecision(2) << prob << "%";
    rc();
    std::cout << "\n\n";

    if (!hits.empty())
    {
        hl('-', DGRAY);
        sc(WHITE);
        std::cout << "  All appearances of ";
        sc(YELLOW);
        std::cout << digit;
        sc(DGRAY);
        std::cout << "  (newest first)";
        rc();
        std::cout << "\n";
        hl('-', DGRAY);
        int shown = 0;
        int lastYr = -1;
        for (int i = 0; i < (int)hits.size(); i++)
        {
            int yr = yearFromLabel(hits[i].date);
            if (yr != lastYr)
            {
                // Year group header
                sc(DARK_CYAN);
                std::cout << "  ── " << yr << " ──────────────\n";
                rc();
                lastYr = yr;
            }
            sc(DGRAY);
            std::cout << "  " << std::setw(4) << (i + 1) << ". ";
            sc(CYAN);
            std::cout << std::left << std::setw(20) << hits[i].date;
            sc(MAGENTA);
            std::cout << hits[i].time;
            rc();
            std::cout << "\n";
            shown++;
            if (shown % 40 == 0)
            {
                sc(DGRAY);
                std::cout << "  -- " << shown << "/" << hits.size() << "  [Space=next  Q=stop] --";
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
        std::cout << "  This combo has never appeared in the loaded data.\n";
        rc();
    }
    hl('=', YELLOW);
}

// ════════════════════════════════════════════════════
//  PROBABILITY
// ════════════════════════════════════════════════════
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
        "  HOW IT WORKS:\n"
        "\n"
        "  APPEARANCES:\n"
        "    How many times the combo (e.g. 5-9-2)\n"
        "    appeared within the selected scope.\n"
        "\n"
        "  SCOPE TOTAL:\n"
        "    Total draw results checked in the scope.\n"
        "    No time filter = all 3 slots counted.\n"
        "    Time filter (e.g. 2pm) = only 2pm draws.\n"
        "    Example: 1000 draw days x 3 = 3000 total.\n"
        "\n"
        "  PROBABILITY:\n"
        "    appearances / scope total x 100\n"
        "    E.g. 3 hits out of 3000 = 0.10%\n"
        "\n"
        "  LOCAL:  Uses digit_data.txt\n"
        "          Run Sync DB first for full history.\n"
        "  ONLINE: Fetches all years from lottopcso.com\n"
        "          newest year first, then analyzes.\n"
        "\n"
        "  DIGIT FORMAT: full combo  e.g.  5-9-2\n";
    int choice = subMenu("PROBABILITY", help);
    if (choice == -1)
    {
        cls();
        return;
    }

    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Digit format: full combo e.g. 5-9-2\n";
    rc();
    std::string digit = inp("  Digit combo to analyze   : ");
    std::string tf = inp("  Filter by time (or blank): ");
    std::cout << "\n";
    if (!tf.empty() && !vtm(tf))
    {
        sc(RED);
        std::cout << "  Invalid time. Use 2pm, 5pm, or 9pm.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    if (digit.empty())
    {
        sc(RED);
        std::cout << "  Digit cannot be empty.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }

    std::string src = choice == 0 ? "LOCAL" : "ONLINE";
    if (!confirmDlg("Analyze [" + src + "]: " + digit + (tf.empty() ? "" : " | " + tf)))
    {
        cls();
        return;
    }

    if (choice == 0)
    {
        // LOCAL
        pulse("Analyzing local data", 2);
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
        showProbResult(entries, digit, tf, "LOCAL");
    }
    else
    {
        // ONLINE — fetch all years newest first
        sc(DGRAY);
        std::cout << "  Checking connection...";
        rc();
        std::cout << std::flush;
        if (!isOnline())
        {
            noInternet();
            return;
        }
        SYSTEMTIME st;
        GetLocalTime(&st);
        int cy = st.wYear;
        const int FY = 2009;
        int total = cy - FY + 1, done = 0;
        std::vector<Entry> all;
        int lr = gpos().Y + 1;
        std::cout << "\n";
        for (int yr = cy; yr >= FY; yr--)
        {
            gotoxy(0, lr);
            clrLine(lr);
            sc(CYAN);
            std::cout << "  [" << std::setw(3) << done << "/" << total << "]  Fetching " << yr << "...";
            rc();
            std::cout << std::flush;
            auto entries = fetchYear(yr, cy);
            all.insert(all.end(), entries.begin(), entries.end());
            done++;
            ms(150);
        }
        clrLine(lr);
        gotoxy(0, lr);
        if (all.empty())
        {
            sc(RED);
            std::cout << "\n  No data retrieved.\n";
            rc();
            std::cout << "\n  Press any key...\n";
            rk();
            cls();
            return;
        }
        showProbResult(all, digit, tf, "ONLINE — " + std::to_string(all.size()) + " entries");
    }

    std::cout << "\n  Press any key...\n";
    rk();
    cls();
}

// ════════════════════════════════════════════════════
//  LAST DIGIT
//
//  Algorithm:
//  1. Load all entries, sort oldest -> newest (dateToInt asc).
//  2. Walk entries from NEWEST to OLDEST.
//     Maintain a set of "seen combos" as we go backward.
//     The first time we see a combo (i.e. its LAST occurrence
//     chronologically) we record it.
//  3. Keep walking until we have collected N unique combos.
//     The Nth unique combo found this way is the one that
//     went the LONGEST without reappearing — the "last digit".
//  4. Ties on the same day: all combos first appearing on
//     that day count as ONE slot each (no doubles/triples
//     within a single draw session collapse it).
//
//  "Show multiples" follow-up: finds combos that appeared
//  on BOTH the last-digit day AND the most recent draw day.
// ════════════════════════════════════════════════════

struct LastDigitRecord
{
    std::string digit, date, time;
    int posInWindow; // position in the 999-draw window (1=start, 999=oldest)
    int uniqueRank;  // rank among unique combos (1=most recent, last=oldest = "last digit")
};

// entries must already be sorted newest->oldest before calling.
// startIdx: index into that sorted array where the window begins (0 = most current draw).
// N: how many last-digits to return (the bottom N of the unique list).
std::vector<LastDigitRecord> computeLastDigits(const std::vector<Entry> &sorted, int startIdx, int N)
{
    int total = (int)sorted.size();
    int windowEnd = std::min(startIdx + 999, total);

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

    // Return last N from unique list, re-ranked so #1 = oldest last-seen
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

// Get all combos drawn on a specific date
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
        "  WHAT IS LAST DIGIT?\n"
        "\n"
        "  Looks at 999 draws starting from a chosen\n"
        "  point (draw #1 = start, draw #999 = oldest).\n"
        "\n"
        "  AUTO FIND:\n"
        "    Starts from the most current draw in the DB.\n"
        "\n"
        "  STARTING POINT:\n"
        "    You pick a specific date & time to start from.\n"
        "    The 999-draw window counts backward from there.\n"
        "    Useful for checking a past point in history.\n"
        "\n"
        "  DEDUPLICATION:\n"
        "    Each combo counts ONCE — only its most recent\n"
        "    appearance within the window is kept.\n"
        "    Repeated combos (2x, 3x) collapse into one.\n"
        "\n"
        "  RESULT RANKING:\n"
        "    #1 = true last digit: combo whose last\n"
        "         appearance is FARTHEST back in the window\n"
        "    #N = Nth oldest last-seen combo\n"
        "\n"
        "  SELECT N (1-9) after choosing start mode.\n"
        "\n"
        "  FOLLOW-UP:\n"
        "    See combos shared between the last-digit day\n"
        "    and the starting draw day.\n"
        "\n"
        "  Uses local digit_data.txt.\n";

    // ── Custom 4-option entry menu ──────────────────
    int sr = gpos().Y;
    int entryChoice = 0;
    {
        auto drawEntry = [&](int hi)
        {
            gotoxy(0, sr);
            clrLine(sr);
            sc(CYAN);
            std::cout << "  [ LAST DIGIT ]";
            rc();
            const char *opts[] = {"[1] Auto Find", "[2] Starting Point", "[3] Help", "[4] Back"};
            const char *desc[] = {"  Use most current draw as start",
                                  "  Pick a date & time to start from",
                                  "  How this feature works",
                                  "  Return to main menu"};
            for (int i = 0; i < 4; i++)
            {
                clrLine(sr + 1 + i);
                gotoxy(0, sr + 1 + i);
                std::cout << "  ";
                if (i == hi)
                {
                    sc(CYAN);
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
                // Show help inline
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
                std::cout << "  Press any key to go back...";
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

    // ── Count picker ────────────────────────────────
    cls();
    std::cout << "\n";
    hl('=', DARK_MAG);
    sc(MAGENTA);
    std::cout << "  [ LAST DIGIT — Select Count ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    sc(WHITE);
    std::cout << "  How many last digits do you want to see?\n";
    sc(DGRAY);
    std::cout << "  (from the bottom of the 999-draw unique list)\n\n";
    rc();
    for (int i = 1; i <= 9; i++)
    {
        sc(DGRAY);
        std::cout << "  [" << i << "] ";
        sc(WHITE);
        if (i == 1)
            std::cout << "Only #1  (the true last digit)\n";
        else
            std::cout << "Last " << i << "  (#1=oldest last-seen, #" << i << "=most recent of set)\n";
        rc();
    }
    std::cout << "\n";
    hl('-', DGRAY);
    sc(DGRAY);
    std::cout << "  Press 1 - 9  (Esc = back)\n";
    rc();
    int N = 0;
    while (true)
    {
        int k = rk();
        if (k == KX)
        {
            cls();
            return;
        }
        if (k >= '1' && k <= '9')
        {
            N = k - '0';
            break;
        }
    }

    // ── Load & sort ─────────────────────────────────
    pulse("Loading digit_data.txt...", 2);
    auto entries = loadDB();
    if (entries.empty())
    {
        sc(RED);
        std::cout << "\n  No data. Run Sync DB or Import CSV first.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }

    auto slotOrd = [](const std::string &t) -> int
    {
        if (t == "9pm")
            return 2;
        if (t == "5pm")
            return 1;
        return 0;
    };
    std::sort(entries.begin(), entries.end(), [&](const Entry &a, const Entry &b)
              {
        int da=dateToInt(a.date),db=dateToInt(b.date);
        if(da!=db)return da>db;
        return slotOrd(a.time)>slotOrd(b.time); });

    // ── Determine startIdx ───────────────────────────
    int startIdx = 0;
    std::string startLabel = ""; // shown in header

    if (entryChoice == 2)
    {
        // Ask user for date + time
        cls();
        std::cout << "\n";
        hl('=', DARK_MAG);
        sc(MAGENTA);
        std::cout << "  [ LAST DIGIT — Starting Point ]\n";
        rc();
        hl('-', DGRAY);
        std::cout << "\n";
        sc(DGRAY);
        std::cout << "  Enter the draw to start counting backward from.\n";
        std::cout << "  The 999-draw window will begin at this draw.\n\n";
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

        // Find that entry in sorted list
        bool found = false;
        for (int i = 0; i < (int)entries.size(); i++)
        {
            if (entries[i].date == sDate && entries[i].time == sTime)
            {
                startIdx = i;
                found = true;
                break;
            }
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
            std::cout << "  Make sure the date & time exist in your DB.\n";
            rc();
            std::cout << "\n  Press any key...\n";
            rk();
            cls();
            return;
        }
        startLabel = sDate + " " + sTime;
    }
    else
    {
        startLabel = entries[0].date + " " + entries[0].time + " (latest)";
    }

    // ── Compute ─────────────────────────────────────
    int totalDB = (int)entries.size();
    int windowEnd = std::min(startIdx + 999, totalDB);
    std::string startDateDisplay = entries[startIdx].date;
    std::string draw999Display = entries[windowEnd - 1].date;

    auto results = computeLastDigits(entries, startIdx, N);
    if (results.empty())
    {
        sc(RED);
        std::cout << "\n  Not enough data in window.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }

    // Count total unique in window
    std::set<std::string> tmpSeen;
    int totalUnique = 0;
    for (int i = startIdx; i < windowEnd; i++)
        if (tmpSeen.insert(trimS(entries[i].digit)).second)
            totalUnique++;

    // ── Display ─────────────────────────────────────
    cls();
    std::cout << "\n";
    hl('=', DARK_MAG);
    sc(MAGENTA);
    std::cout << "  [ LAST DIGIT — Last " << N << " in 999-Draw Window ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";

    sc(DGRAY);
    std::cout << "  Start  : ";
    sc(CYAN);
    std::cout << "draw #1 = " << startDateDisplay;
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
    std::cout << "  End    : ";
    sc(CYAN);
    std::cout << "draw #" << (windowEnd - startIdx) << " = " << draw999Display;
    rc();
    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Unique combos in window: ";
    sc(YELLOW);
    std::cout << totalUnique;
    sc(DGRAY);
    std::cout << "   |  showing last " << N;
    rc();
    std::cout << "\n\n";
    hl('-', DGRAY);
    sc(DGRAY);
    std::cout << "  Rank  Combo       Last seen in window          Window pos\n";
    rc();
    hl('-', DGRAY);

    for (const auto &r : results)
    {
        if (r.uniqueRank == 1)
        {
            sc(RED);
        }
        else if (r.uniqueRank == 2)
        {
            sc(YELLOW);
        }
        else
        {
            sc(WHITE);
        }
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
    std::cout << "  #1 = combo whose last appearance is farthest back in window\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";

    // ── Follow-up ────────────────────────────────────
    std::string lastDay = results[0].date;
    std::string topDay = entries[startIdx].date; // the starting draw day

    while (true)
    {
        sc(DGRAY);
        std::cout << "  [1] Back to main menu\n";
        std::cout << "  [2] Show combos drawn on BOTH:\n";
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
//  DELETE
// ════════════════════════════════════════════════════
void doDelete()
{
    cls();
    std::cout << "\n";
    hl('=', RED);
    sc(RED);
    std::cout << "  [ DELETE ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";
    std::string help = "  Enter date (YYYY-MM-DD) and time.\n  The matching record is permanently removed.\n";
    if (!subMenu2("DELETE", help))
    {
        cls();
        return;
    }
    std::cout << "\n";
    std::string date = inp("  Date  (YYYY-MM-DD) : ");
    std::string t = inp("  Time  (2pm/5pm/9pm): ");
    std::cout << "\n";
    if (!vtm(t))
    {
        sc(RED);
        std::cout << "  Invalid time.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    auto data = loadDB();
    auto it = std::find_if(data.begin(), data.end(), [&](const Entry &e)
                           { return e.date == date && e.time == t; });
    if (it == data.end())
    {
        sc(RED);
        std::cout << "  No record found.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    sc(WHITE);
    std::cout << "  Found: ";
    sc(YELLOW);
    std::cout << it->digit;
    sc(WHITE);
    std::cout << "  |  ";
    sc(CYAN);
    std::cout << it->date;
    sc(WHITE);
    std::cout << "  |  ";
    sc(MAGENTA);
    std::cout << it->time;
    rc();
    std::cout << "\n\n";
    if (!confirmDlg("DELETE this entry permanently?"))
    {
        cls();
        return;
    }
    data.erase(it);
    saveDB(data);
    sc(GREEN);
    std::cout << "\n  Entry deleted.\n";
    rc();
    std::cout << "\n  Press any key...\n";
    rk();
    cls();
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
    std::string help = "  Fetches current year Swertres results\n  from lottopcso.com.\n  Requires internet connection.\n";
    if (!subMenu2("BROWSE", help))
    {
        cls();
        return;
    }
    if (!confirmDlg("Fetch current year results?"))
    {
        cls();
        return;
    }
    sc(DGRAY);
    std::cout << "\n  Checking connection...";
    rc();
    std::cout << std::flush;
    if (!isOnline())
    {
        noInternet();
        return;
    }
    int lr = gpos().Y;
    gotoxy(2, lr);
    sc(CYAN);
    std::cout << "  Fetching...";
    rc();
    std::cout << std::flush;
    std::string html = fetchPage(L"www.lottopcso.com", L"/swertres-results-today-history-and-summary/");
    clrLine(lr);
    gotoxy(0, lr);
    if (html.empty())
    {
        sc(RED);
        std::cout << "\n  Fetch failed.\n";
        rc();
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }
    auto rows = parseHTML(html);
    std::cout << "\n";
    hl('=', GREEN);
    sc(GREEN);
    std::cout << "  SWERTRES — Current Year\n";
    rc();
    sc(DGRAY);
    std::cout << "  Source: lottopcso.com\n";
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
//  IMPORT CSV
//  CSV format: ID,DD.MM.YYYY,HH:MM,d1,d2,d3
//  Time mapping:
//    11:00 / 14:00  ->  2pm
//    16:00 / 17:00  ->  5pm
//    21:00          ->  9pm
//  Digit: d1-d2-d3  (e.g. 05,09,02 -> 5-9-2)
//  Date converted to: "Mar 4, 2026" format
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

// DD.MM.YYYY -> "Mar 4, 2026"
std::string csvDateToLabel(const std::string &d)
{
    // d is "DD.MM.YYYY"
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

// Parse one CSV line into an Entry. Returns false if invalid/skip.
bool parseCSVLine(const std::string &line, Entry &out)
{
    // Fields: id, date, time, d1, d2, d3
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
    // Remove leading zeros from digits: "05" -> "5"
    auto strip0 = [](std::string s) -> std::string
    {
        while (s.size() > 1 && s[0] == '0')
            s.erase(s.begin());
        return s;
    };
    std::string combo = strip0(d1) + "-" + strip0(d2) + "-" + strip0(d3);
    out.date = csvDateToLabel(date);
    out.time = slot;
    out.digit = combo;
    return true;
}

void doImportCSV()
{
    cls();
    std::cout << "\n";
    hl('=', YELLOW);
    sc(YELLOW);
    std::cout << "  [ IMPORT CSV ]\n";
    rc();
    hl('-', DGRAY);
    std::cout << "\n";

    std::string help =
        "  Imports 3D Lotto results from a CSV file\n"
        "  and merges them into digit_data.txt.\n"
        "\n"
        "  EXPECTED CSV FORMAT:\n"
        "    ID, DD.MM.YYYY, HH:MM, d1, d2, d3\n"
        "    e.g.  00001,02.01.2007,11:00,05,08,07\n"
        "\n"
        "  TIME MAPPING:\n"
        "    11:00 or 14:00  ->  2pm\n"
        "    16:00 or 17:00  ->  5pm\n"
        "    21:00           ->  9pm\n"
        "\n"
        "  DIGIT FORMAT stored: 5-9-2  (leading zeros removed)\n"
        "  DATE FORMAT stored:  Mar 4, 2026\n"
        "\n"
        "  DUPLICATES: Entries already in digit_data.txt\n"
        "  (same date + time + digit) are skipped.\n"
        "\n"
        "  Place the CSV file in the same folder as\n"
        "  digit.exe, then enter its filename below.\n";

    if (!subMenu2("IMPORT CSV", help))
    {
        cls();
        return;
    }

    std::cout << "\n";
    sc(DGRAY);
    std::cout << "  Place the CSV in the same folder as digit.exe\n";
    rc();
    std::string filename = inp("  CSV filename (e.g. swertres.csv): ");
    if (filename.empty())
    {
        cls();
        return;
    }

    // Try to open file
    std::ifstream f(filename);
    if (!f.is_open())
    {
        // Try with .csv appended if not already
        if (filename.find('.') == std::string::npos)
        {
            f.open(filename + ".csv");
            if (f.is_open())
                filename += ".csv";
        }
    }
    if (!f.is_open())
    {
        std::cout << "\n";
        hl('-', RED);
        sc(RED);
        std::cout << "  [!] Cannot open file: ";
        sc(WHITE);
        std::cout << filename;
        rc();
        std::cout << "\n";
        sc(DGRAY);
        std::cout << "  Make sure the file is in the same folder as digit.exe\n";
        rc();
        hl('-', RED);
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }

    // Count lines first for progress
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

    // Parse all valid entries
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
        std::cout << "  [!] No valid records found in CSV.\n";
        rc();
        sc(DGRAY);
        std::cout << "  Check the file format matches the expected format.\n";
        rc();
        hl('-', RED);
        std::cout << "\n  Press any key...\n";
        rk();
        cls();
        return;
    }

    // Merge into DB (deduplication)
    pulse("Merging into digit_data.txt", 2);
    int added = mergeIntoDB(parsed);
    int dupSkipped = (int)parsed.size() - added;

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

// ════════════════════════════════════════════════════
//  MAIN MENU
// ════════════════════════════════════════════════════
void mainMenu()
{
    struct MI
    {
        std::string code, label, desc;
    };
    std::vector<MI> items = {
        {"[1]", "Search", "Find a digit by date & time"},
        {"[2]", "Insert/Edit", "Add or update a local entry"},
        {"[3]", "Show All", "Browse all records"},
        {"[4]", "Probability", "Analyze digit frequency"},
        {"[5]", "Last Digit", "Find combos unseen the longest"},
        {"[6]", "Browse", "View latest Swertres results"},
        {"[7]", "Sync DB", "Fetch history -> digit_data.txt"},
        {"[8]", "Import CSV", "Load CSV file -> digit_data.txt"},
        {"[9]", "Delete", "Remove a local entry"},
        {"[0]", "Exit", "Quit the application"},
    };
    int sel = 0, n = (int)items.size();
    while (true)
    {
        cls();
        cur(false);
        std::cout << "\n";
        hl('=', CYAN);
        cprt("  D I G I T   T R A C K E R   v 1 . 3  ", CYAN);
        std::cout << "\n";
        cprt("  Local & Online  |  Swertres 3D  |  lottopcso.com  ", DGRAY);
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
        std::cout << " records";
        rc();
        std::cout << "\n";
        cur(false);
        int k = rk();
        if (k == 72 + 256 || k == 'w' || k == 'W')
            sel = (sel - 1 + n) % n;
        else if (k == 80 + 256 || k == 's' || k == 'S')
            sel = (sel + 1) % n;
        else if (k >= '1' && k <= '9')
        {
            sel = k - '1';
            goto act;
        }
        else if (k == '0')
        {
            sel = 9;
            goto act;
        }
        else if (k == KE)
            goto act;
        else if (k == 'q' || k == 'Q')
        {
            sel = 9;
            goto act;
        }
        continue;
    act:
        cur(true);
        if (sel == 0)
            doSearch();
        else if (sel == 1)
            doInsert();
        else if (sel == 2)
            doShowAll();
        else if (sel == 3)
            doProb();
        else if (sel == 4)
            doLastDigit();
        else if (sel == 5)
            doBrowse();
        else if (sel == 6)
            doSyncDB();
        else if (sel == 7)
            doImportCSV();
        else if (sel == 8)
            doDelete();
        else if (sel == 9)
        {
            if (confirmDlg("Exit?"))
            {
                cls();
                std::cout << "\n\n";
                cprt("  Thank you for using Digit Tracker v1.0!  ", CYAN);
                std::cout << "\n";
                cprt("  Data saved to " + DB + "  ", DGRAY);
                std::cout << "\n\n";
                return;
            }
        }
    }
}

int main()
{
    hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    DWORD mode;
    GetConsoleMode(hCon, &mode);
    SetConsoleMode(hCon, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleTitleA("Digit Tracker v1.0");
    SMALL_RECT ws = {0, 0, 99, 44};
    SetConsoleWindowInfo(hCon, TRUE, &ws);
    COORD bs = {100, 3000};
    SetConsoleScreenBufferSize(hCon, bs);
    showIntro();
    mainMenu();
    rc();
    cur(true);
    return 0;
}