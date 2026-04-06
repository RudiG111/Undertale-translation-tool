#include <windows.h>
#include <string>
#include <sstream>

#define ID_EDIT            1001
#define ID_BTN_SAVE        1002
#define ID_BTN_RESTART     1003
#define ID_STATUS          1004
#define ID_GAME_PANEL      1005
#define ID_TIMER_ATTACH    1006
#define ID_FIND_EDIT       1007
#define ID_BTN_FIND        1008
#define ID_BTN_FINDNEXT    1009
#define ID_BTN_FOCUSGAME   1010

HINSTANCE g_hInst = NULL;
HWND g_mainWnd = NULL;
HWND g_edit = NULL;
HWND g_status = NULL;
HWND g_gamePanel = NULL;

HWND g_btnSave = NULL;
HWND g_btnRestart = NULL;
HWND g_btnFocusGame = NULL;
HWND g_findEdit = NULL;
HWND g_btnFind = NULL;
HWND g_btnFindNext = NULL;

HWND g_gameWnd = NULL;
HWND g_gameOriginalParent = NULL;
LONG_PTR g_gameOriginalStyle = 0;
LONG_PTR g_gameOriginalExStyle = 0;
bool g_isAttached = false;

std::wstring g_currentFile;
std::wstring g_appDir;
PROCESS_INFORMATION g_pi = {};
bool g_startedGame = false;
int g_attachTryCount = 0;

HFONT g_fontUi = NULL;
HFONT g_fontEdit = NULL;
HBRUSH g_brushMain = NULL;
HBRUSH g_brushEdit = NULL;

COLORREF g_colBg = RGB(24, 24, 24);
COLORREF g_colEdit = RGB(18, 18, 18);
COLORREF g_colText = RGB(230, 230, 230);

std::wstring g_lastFindText;
int g_lastFindPos = 0;

std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (len <= 0) return L"";
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len);
    return out;
}

std::string WideToUtf8(const std::wstring& s)
{
    if (s.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len, NULL, NULL);
    return out;
}

void SetStatus(const std::wstring& s)
{
    SetWindowTextW(g_status, s.c_str());
}

std::wstring GetExeDirectory()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);

    std::wstring full = path;
    size_t p = full.find_last_of(L"\\/");
    if (p == std::wstring::npos)
        return L".";

    return full.substr(0, p);
}

std::wstring JoinPath(const std::wstring& a, const std::wstring& b)
{
    if (a.empty()) return b;
    if (a.back() == L'\\' || a.back() == L'/')
        return a + b;
    return a + L"\\" + b;
}

bool FileExists(const std::wstring& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

std::string ReadFileBytes(const std::wstring& path)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return "";

    LARGE_INTEGER size;
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0)
    {
        CloseHandle(h);
        return "";
    }

    if (size.QuadPart > 200 * 1024 * 1024)
    {
        CloseHandle(h);
        return "";
    }

    std::string data;
    data.resize((size_t)size.QuadPart);

    DWORD readBytes = 0;
    BOOL ok = ReadFile(h, data.data(), (DWORD)data.size(), &readBytes, NULL);
    CloseHandle(h);

    if (!ok)
        return "";

    data.resize(readBytes);
    return data;
}

bool WriteFileBytes(const std::wstring& path, const std::string& text)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    BOOL ok = WriteFile(h, text.data(), (DWORD)text.size(), &written, NULL);
    CloseHandle(h);

    return ok && written == (DWORD)text.size();
}

std::wstring ReadUtf8TextFile(const std::wstring& path)
{
    std::string bytes = ReadFileBytes(path);
    if (bytes.size() >= 3 &&
        (unsigned char)bytes[0] == 0xEF &&
        (unsigned char)bytes[1] == 0xBB &&
        (unsigned char)bytes[2] == 0xBF)
    {
        bytes.erase(0, 3);
    }
    return Utf8ToWide(bytes);
}

bool WriteUtf8TextFile(const std::wstring& path, const std::wstring& text)
{
    std::string bytes = WideToUtf8(text);
    return WriteFileBytes(path, bytes);
}

std::wstring GetEditTextW(HWND hEdit)
{
    int len = GetWindowTextLengthW(hEdit);
    if (len <= 0)
        return L"";

    std::wstring text(len + 1, L'\0');
    GetWindowTextW(hEdit, &text[0], len + 1);
    text.resize(len);
    return text;
}

void SetEditTextW(HWND hEdit, const std::wstring& text)
{
    SetWindowTextW(hEdit, text.c_str());
}

bool MakeBackupIfNeeded(const std::wstring& path)
{
    if (path.empty() || !FileExists(path))
        return false;

    std::string content = ReadFileBytes(path);
    if (content.empty())
        return false;

    std::wstring backupPath = path + L".bak";
    return WriteFileBytes(backupPath, content);
}

void FocusGameWindow()
{
    if (!g_gameWnd || !IsWindow(g_gameWnd))
        return;

    HWND target = g_gameWnd;
    HWND child = GetWindow(g_gameWnd, GW_CHILD);
    if (child && IsWindow(child))
        target = child;

    SetActiveWindow(g_mainWnd);
    SetFocus(target);

    PostMessageW(target, WM_SETFOCUS, 0, 0);
    InvalidateRect(g_gamePanel, NULL, FALSE);
    UpdateWindow(g_gamePanel);

    SetStatus(L"Undertale focused");
}

void ResizeAttachedGame()
{
    if (!g_isAttached || !g_gameWnd || !IsWindow(g_gameWnd))
        return;

    RECT rcPanel;
    RECT rcGame;

    GetClientRect(g_gamePanel, &rcPanel);
    GetWindowRect(g_gameWnd, &rcGame);

    int newW = rcPanel.right - rcPanel.left;
    int newH = rcPanel.bottom - rcPanel.top;
    int curW = rcGame.right - rcGame.left;
    int curH = rcGame.bottom - rcGame.top;

    if (curW != newW || curH != newH)
        MoveWindow(g_gameWnd, 0, 0, newW, newH, TRUE);
}

void DetachGame()
{
    if (!g_gameWnd || !IsWindow(g_gameWnd))
    {
        g_isAttached = false;
        g_gameWnd = NULL;
        return;
    }

    SetParent(g_gameWnd, g_gameOriginalParent);
    SetWindowLongPtrW(g_gameWnd, GWL_STYLE, g_gameOriginalStyle);
    SetWindowLongPtrW(g_gameWnd, GWL_EXSTYLE, g_gameOriginalExStyle);

    ShowWindow(g_gameWnd, SW_SHOW);
    SetWindowPos(
        g_gameWnd,
        NULL,
        100, 100, 640, 480,
        SWP_NOZORDER | SWP_SHOWWINDOW
    );

    g_isAttached = false;
    g_gameWnd = NULL;
}

void TryAttachGame()
{
    if (g_isAttached)
    {
        SetStatus(L"Undertale attached");
        return;
    }

    HWND game = FindWindowW(NULL, L"UNDERTALE");
    if (!game)
        game = FindWindowW(NULL, L"Undertale");

    if (!game)
    {
        SetStatus(L"Waiting for Undertale...");
        return;
    }

    g_gameWnd = game;
    g_gameOriginalParent = GetParent(game);
    g_gameOriginalStyle = GetWindowLongPtrW(game, GWL_STYLE);
    g_gameOriginalExStyle = GetWindowLongPtrW(game, GWL_EXSTYLE);

    LONG_PTR style = g_gameOriginalStyle;
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    style |= WS_CHILD;

    SetWindowLongPtrW(game, GWL_STYLE, style);
    SetParent(game, g_gamePanel);

    RECT rc;
    GetClientRect(g_gamePanel, &rc);
    MoveWindow(game, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    ShowWindow(game, SW_SHOW);

    g_isAttached = true;
    SetStatus(L"Undertale attached");
}

bool StopStartedGame()
{
    if (g_pi.hProcess)
    {
        DWORD code = 0;
        if (GetExitCodeProcess(g_pi.hProcess, &code) && code == STILL_ACTIVE)
        {
            TerminateProcess(g_pi.hProcess, 0);
            WaitForSingleObject(g_pi.hProcess, 3000);
        }

        CloseHandle(g_pi.hProcess);
        g_pi.hProcess = NULL;
    }

    if (g_pi.hThread)
    {
        CloseHandle(g_pi.hThread);
        g_pi.hThread = NULL;
    }

    g_startedGame = false;
    return true;
}

bool StartUndertale()
{
    std::wstring exePath = JoinPath(g_appDir, L"UNDERTALE.exe");
    if (!FileExists(exePath))
        exePath = JoinPath(g_appDir, L"Undertale.exe");

    if (!FileExists(exePath))
    {
        SetStatus(L"UNDERTALE.exe not found");
        return false;
    }

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    ZeroMemory(&g_pi, sizeof(g_pi));

    std::wstring cmd = L"\"" + exePath + L"\"";

    BOOL ok = CreateProcessW(
        NULL,
        cmd.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        g_appDir.c_str(),
        &si,
        &g_pi
    );

    if (!ok)
    {
        SetStatus(L"Failed to start Undertale");
        return false;
    }

    g_startedGame = true;
    SetStatus(L"Undertale started");
    return true;
}

void LoadDefaultJson()
{
    std::wstring path = JoinPath(JoinPath(g_appDir, L"lang"), L"lang_en.json");

    if (!FileExists(path))
    {
        SetStatus(L"lang\\lang_en.json not found");
        return;
    }

    std::wstring content = ReadUtf8TextFile(path);
    SetEditTextW(g_edit, content);
    g_currentFile = path;
    SetStatus(L"Loaded lang\\lang_en.json");
}

void SaveJsonText()
{
    if (g_currentFile.empty())
    {
        SetStatus(L"No file loaded");
        return;
    }

    std::wstring text = GetEditTextW(g_edit);
    MakeBackupIfNeeded(g_currentFile);

    if (WriteUtf8TextFile(g_currentFile, text))
        SetStatus(L"Saved");
    else
        SetStatus(L"Save failed");
}

void RestartUndertale()
{
    if (g_isAttached)
        DetachGame();

    StopStartedGame();

    if (StartUndertale())
    {
        g_attachTryCount = 0;
        SetTimer(g_mainWnd, ID_TIMER_ATTACH, 700, NULL);
    }
}

void SelectEditorRange(int startPos, int endPos)
{
    SendMessageW(g_edit, EM_SETSEL, startPos, endPos);
    SendMessageW(g_edit, EM_SCROLLCARET, 0, 0);
    SetFocus(g_edit);
}

void ResetFindState()
{
    g_lastFindText.clear();
    g_lastFindPos = 0;
}

void DoFind(bool findNext)
{
    std::wstring needle = GetEditTextW(g_findEdit);
    if (needle.empty())
    {
        SetStatus(L"Type something to find");
        return;
    }

    std::wstring text = GetEditTextW(g_edit);

    int startPos = 0;

    if (findNext && needle == g_lastFindText)
        startPos = g_lastFindPos;
    else
        startPos = 0;

    size_t pos = text.find(needle, (size_t)startPos);

    if (pos == std::wstring::npos)
    {
        if (findNext)
            pos = text.find(needle, 0);

        if (pos == std::wstring::npos)
        {
            SetStatus(L"No matches");
            return;
        }
    }

    int s = (int)pos;
    int e = s + (int)needle.size();

    SelectEditorRange(s, e);

    g_lastFindText = needle;
    g_lastFindPos = e;

    SetStatus(L"Found");
}

void CreateFonts()
{
    if (!g_fontUi)
    {
        g_fontUi = CreateFontW(
            -18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
    }

    if (!g_fontEdit)
    {
        g_fontEdit = CreateFontW(
            -24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas"
        );
    }

    if (!g_brushMain) g_brushMain = CreateSolidBrush(g_colBg);
    if (!g_brushEdit) g_brushEdit = CreateSolidBrush(g_colEdit);
}

void ApplyFonts()
{
    SendMessageW(g_edit, WM_SETFONT, (WPARAM)g_fontEdit, TRUE);
    SendMessageW(g_status, WM_SETFONT, (WPARAM)g_fontUi, TRUE);
    SendMessageW(g_btnSave, WM_SETFONT, (WPARAM)g_fontUi, TRUE);
    SendMessageW(g_btnRestart, WM_SETFONT, (WPARAM)g_fontUi, TRUE);
    SendMessageW(g_btnFocusGame, WM_SETFONT, (WPARAM)g_fontUi, TRUE);
    SendMessageW(g_findEdit, WM_SETFONT, (WPARAM)g_fontUi, TRUE);
    SendMessageW(g_btnFind, WM_SETFONT, (WPARAM)g_fontUi, TRUE);
    SendMessageW(g_btnFindNext, WM_SETFONT, (WPARAM)g_fontUi, TRUE);
}

void CreateUi(HWND hwnd)
{
    CreateFonts();

    g_btnSave = CreateWindowExW(
        0, L"BUTTON", L"Save JSON",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 100, 30,
        hwnd, (HMENU)ID_BTN_SAVE, g_hInst, NULL
    );

    g_btnRestart = CreateWindowExW(
        0, L"BUTTON", L"Restart Undertale",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 140, 30,
        hwnd, (HMENU)ID_BTN_RESTART, g_hInst, NULL
    );

    g_btnFocusGame = CreateWindowExW(
        0, L"BUTTON", L"Focus Undertale",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 130, 30,
        hwnd, (HMENU)ID_BTN_FOCUSGAME, g_hInst, NULL
    );

    g_findEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        0, 0, 100, 28,
        hwnd, (HMENU)ID_FIND_EDIT, g_hInst, NULL
    );

    g_btnFind = CreateWindowExW(
        0, L"BUTTON", L"Find",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 70, 28,
        hwnd, (HMENU)ID_BTN_FIND, g_hInst, NULL
    );

    g_btnFindNext = CreateWindowExW(
        0, L"BUTTON", L"Find Next",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 90, 28,
        hwnd, (HMENU)ID_BTN_FINDNEXT, g_hInst, NULL
    );

    g_status = CreateWindowExW(
        0, L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE,
        0, 0, 300, 24,
        hwnd, (HMENU)ID_STATUS, g_hInst, NULL
    );

    g_edit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
        WS_VSCROLL | WS_HSCROLL,
        0, 0, 100, 100,
        hwnd, (HMENU)ID_EDIT, g_hInst, NULL
    );

    SendMessageW(g_edit, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);
    SendMessageW(g_edit, EM_FMTLINES, FALSE, 0);

    g_gamePanel = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 100, 100,
        hwnd, (HMENU)ID_GAME_PANEL, g_hInst, NULL
    );

    ApplyFonts();
}

void LayoutUi(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    int margin = 10;
    int topBarH = 42;
    int rightPanelW = (w * 41) / 100;
    int leftPanelW = w - rightPanelW - margin * 3;

    if (leftPanelW < 260) leftPanelW = 260;
    if (rightPanelW < 260) rightPanelW = 260;

    int xLeft = margin;
    int yTop = margin;
    int xRight = xLeft + leftPanelW + margin;
    int rightW = w - xRight - margin;

    MoveWindow(g_btnSave, xLeft, yTop, 115, 30, TRUE);
    MoveWindow(g_btnRestart, xLeft + 125, yTop, 160, 30, TRUE);
    MoveWindow(g_btnFocusGame, xLeft + 295, yTop, 135, 30, TRUE);
    MoveWindow(g_findEdit, xLeft + 445, yTop, 220, 30, TRUE);
    MoveWindow(g_btnFind, xLeft + 675, yTop, 80, 30, TRUE);
    MoveWindow(g_btnFindNext, xLeft + 765, yTop, 100, 30, TRUE);

    MoveWindow(g_status, xLeft + 875, yTop + 5, w - (xLeft + 875) - margin, 22, TRUE);

    int editorY = yTop + topBarH;
    int editorH = h - editorY - margin;
    MoveWindow(g_edit, xLeft, editorY, leftPanelW, editorH, TRUE);

    MoveWindow(g_gamePanel, xRight, editorY, rightW, editorH, TRUE);

    ResizeAttachedGame();
}

LRESULT HandleCtlColor(HDC hdc, HWND hCtl)
{
    if (hCtl == g_edit)
    {
        SetTextColor(hdc, g_colText);
        SetBkColor(hdc, g_colEdit);
        SetBkMode(hdc, OPAQUE);
        return (LRESULT)g_brushEdit;
    }

    SetTextColor(hdc, g_colText);
    SetBkColor(hdc, g_colBg);
    SetBkMode(hdc, OPAQUE);
    return (LRESULT)g_brushMain;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        g_appDir = GetExeDirectory();
        CreateUi(hwnd);
        LoadDefaultJson();
        StartUndertale();
        g_attachTryCount = 0;
        SetTimer(hwnd, ID_TIMER_ATTACH, 700, NULL);
        return 0;

    case WM_ERASEBKGND:
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, g_brushMain);
        return 1;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER_ATTACH)
        {
            if (!g_isAttached)
            {
                TryAttachGame();
                g_attachTryCount++;
                if (g_isAttached || g_attachTryCount > 20)
                    KillTimer(hwnd, ID_TIMER_ATTACH);
            }
            else
            {
                KillTimer(hwnd, ID_TIMER_ATTACH);
            }
        }
        return 0;

    case WM_SIZE:
        LayoutUi(hwnd);
        return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        return HandleCtlColor((HDC)wParam, (HWND)lParam);

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == ID_BTN_SAVE)
        {
            SaveJsonText();
            return 0;
        }

        if (id == ID_BTN_RESTART)
        {
            RestartUndertale();
            return 0;
        }

        if (id == ID_BTN_FOCUSGAME)
        {
            FocusGameWindow();
            return 0;
        }

        if (id == ID_BTN_FIND)
        {
            DoFind(false);
            return 0;
        }

        if (id == ID_BTN_FINDNEXT)
        {
            DoFind(true);
            return 0;
        }

        if (id == ID_FIND_EDIT && code == EN_CHANGE)
        {
            ResetFindState();
            return 0;
        }

        return 0;
    }

    case WM_CLOSE:
        if (g_isAttached)
            DetachGame();

        StopStartedGame();
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (g_fontUi) DeleteObject(g_fontUi);
        if (g_fontEdit) DeleteObject(g_fontEdit);
        if (g_brushMain) DeleteObject(g_brushMain);
        if (g_brushEdit) DeleteObject(g_brushEdit);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    g_hInst = hInstance;

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"UndertaleTranslatorToolClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (!RegisterClassExW(&wc))
    {
        MessageBoxW(NULL, L"Nem sikerült regisztrálni az ablakosztályt.", L"Hiba", MB_ICONERROR);
        return 1;
    }

    g_mainWnd = CreateWindowExW(
        0,
        L"UndertaleTranslatorToolClass",
        L"Undertale Translation Tool",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1450, 850,
        NULL, NULL,
        hInstance, NULL
    );

    if (!g_mainWnd)
    {
        MessageBoxW(NULL, L"Nem sikerült létrehozni a főablakot.", L"Hiba", MB_ICONERROR);
        return 1;
    }

    ShowWindow(g_mainWnd, nCmdShow);
    UpdateWindow(g_mainWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}