#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

// Forward-declare Raylib's handle function to avoid including raylib.h
void *GetWindowHandle(void);

static WNDPROC oldWindowProc = NULL;

static LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_NCHITTEST) {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // Top 42 pixels, excluding the top-right window buttons (right 110px)
        if (pt.y >= 0 && pt.y <= 50 && pt.x < (rcClient.right - (50 * 8. / 9. * 3 + 2))) {
            return HTCAPTION;
        }
    }
    return CallWindowProc(oldWindowProc, hwnd, uMsg, wParam, lParam);
}

void InitWin32Drag(void) {
    HWND hwnd = (HWND)GetWindowHandle();
    if (hwnd) {
        oldWindowProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
    }
}
#endif
