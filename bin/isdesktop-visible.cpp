#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class DesktopMonitor {
public:
    // Initialize, cleanup, and run
    static bool Initialize();
    static void Cleanup();
    static void RunMessageLoop();

    // Helper to get formatted timestamp
    static std::wstring GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm timeInfo;
        localtime_s(&timeInfo, &in_time_t);
        std::wstringstream ss;
        ss << L"[" << std::put_time(&timeInfo, L"%Y-%m-%d %H:%M:%S") << L"] ";
        return ss.str();
    }

private:
    static bool c_ShowDesktop;
    static HWINEVENTHOOK c_WinEventHook;
    static HWND c_Window;
    static HWND c_LastForegroundWindow;

    // Timer IDs
    static const UINT TIMER_SHOWDESKTOP = 1;
    // Timer interval (ms)
    static const UINT INTERVAL_CHECK = 100;

    static bool CheckDesktopState();
    static bool IsDesktopVisible();
    static bool IsShowDesktopActive();
    static HWND GetDesktopWindow();
    static HWND GetShellTrayWindow();

    static void CALLBACK MyWinEventProc(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};

// Static members
bool DesktopMonitor::c_ShowDesktop = false;
HWINEVENTHOOK DesktopMonitor::c_WinEventHook = nullptr;
HWND DesktopMonitor::c_Window = nullptr;
HWND DesktopMonitor::c_LastForegroundWindow = nullptr;

bool DesktopMonitor::Initialize() {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), 0, WndProc, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DesktopMonitorClass", nullptr };
    if (!RegisterClassEx(&wc)) {
        std::wcout << GetTimestamp() << L"Failed to register window class" << std::endl;
        return false;
    }

    c_Window = CreateWindowEx(0, L"DesktopMonitorClass", L"Desktop Monitor", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!c_Window) {
        std::wcout << GetTimestamp() << L"Failed to create hidden window" << std::endl;
        return false;
    }

    c_WinEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, MyWinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    if (!c_WinEventHook) {
        std::wcout << GetTimestamp() << L"Failed to set WinEvent hook" << std::endl;
        return false;
    }

    c_ShowDesktop = IsDesktopVisible();
    c_LastForegroundWindow = GetForegroundWindow();
    SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_CHECK, nullptr);

    std::wcout << GetTimestamp() << L"Desktop Monitor initialized successfully" << std::endl;
    std::wcout << GetTimestamp() << L"Initial state: " << (c_ShowDesktop ? L"DESKTOP SHOWN" : L"DESKTOP HIDDEN") << std::endl;
    std::wcout << GetTimestamp() << L"Press Ctrl+C to exit" << std::endl;
    return true;
}

void DesktopMonitor::Cleanup() {
    if (c_WinEventHook) {
        UnhookWinEvent(c_WinEventHook);
        c_WinEventHook = nullptr;
    }
    if (c_Window) {
        KillTimer(c_Window, TIMER_SHOWDESKTOP);
        DestroyWindow(c_Window);
        c_Window = nullptr;
    }
    UnregisterClass(L"DesktopMonitorClass", GetModuleHandle(nullptr));
}

void DesktopMonitor::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool DesktopMonitor::IsDesktopVisible() {
    return IsShowDesktopActive();
}

bool DesktopMonitor::IsShowDesktopActive() {
    bool allMinimized = true;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        bool* pAll = (bool*)lParam;
        if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
            WCHAR cls[256];
            if (GetClassName(hwnd, cls, 256) > 0) {
                if (wcscmp(cls, L"Shell_TrayWnd") != 0 && wcscmp(cls, L"WorkerW") != 0 && wcscmp(cls, L"Progman") != 0 &&
                    wcscmp(cls, L"DV2ControlHost") != 0 && wcscmp(cls, L"ApplicationFrameWindow") != 0) {
                    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
                    LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                    if (!(ex & WS_EX_TOOLWINDOW) && (style & WS_CAPTION)) {
                        WCHAR title[256];
                        if (GetWindowText(hwnd, title, 256) > 0) {
                            DWORD pid; GetWindowThreadProcessId(hwnd, &pid);
                            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
                            if (hProc) {
                                WCHAR path[MAX_PATH]; DWORD size = MAX_PATH;
                                if (QueryFullProcessImageName(hProc, 0, path, &size)) {
                                    WCHAR* bn = wcsrchr(path, L'\\'); bn = bn ? bn + 1 : path;
                                    if (_wcsicmp(bn, L"explorer.exe") != 0) { *pAll = false; CloseHandle(hProc); return FALSE; }
                                }
                                CloseHandle(hProc);
                            }
                        }
                    }
                }
            }
        }
        return TRUE;
        }, (LPARAM)&allMinimized);
    return allMinimized;
}

bool DesktopMonitor::CheckDesktopState() {
    bool current = IsDesktopVisible();
    if (current != c_ShowDesktop) {
        c_ShowDesktop = current;
        std::wcout << GetTimestamp() << L"Desktop state changed: " << (c_ShowDesktop ? L"SHOWN" : L"HIDDEN") << std::endl;
        HWND fg = GetForegroundWindow();
        if (fg) {
            WCHAR cls[256], title[256] = {};
            GetClassName(fg, cls, 256);
            GetWindowText(fg, title, 256);
            std::wcout << GetTimestamp() << L" Foreground: " << cls;
            if (wcslen(title)) std::wcout << L" (" << title << L")";
            std::wcout << std::endl;
        }
    }
    return false;
}

// Provide definitions for declared but unused methods
HWND DesktopMonitor::GetDesktopWindow() { return ::GetDesktopWindow(); }
HWND DesktopMonitor::GetShellTrayWindow() { return FindWindow(L"Shell_TrayWnd", nullptr); }

void CALLBACK DesktopMonitor::MyWinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG, LONG, DWORD, DWORD) {
    if (event == EVENT_SYSTEM_FOREGROUND) {
        c_LastForegroundWindow = hwnd;
        CheckDesktopState();
    }
}

LRESULT CALLBACK DesktopMonitor::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_TIMER && wp == TIMER_SHOWDESKTOP) {
        CheckDesktopState();
        return 0;
    }
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wp, lp);
}

// Clean exit on Ctrl+C without extra logs
BOOL WINAPI ConsoleCtrlHandler(DWORD ctrl) {
    if (ctrl == CTRL_C_EVENT || ctrl == CTRL_CLOSE_EVENT) {
        DesktopMonitor::Cleanup();
        ExitProcess(0);
    }
    return FALSE;
}

int main() {
    std::wcout << DesktopMonitor::GetTimestamp() << L"Desktop State Monitor v2.0" << std::endl;
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    if (!DesktopMonitor::Initialize()) return 1;
    DesktopMonitor::RunMessageLoop();
    return 0;
}
 