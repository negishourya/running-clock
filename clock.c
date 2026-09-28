/**
 * ============================================================================
 * Application: Running Digital Clock (Win32 GUI in Pure C)
 * Author: Windows Win32 Systems Development
 *
 * Description:
 *   A lightweight, flicker-free, real-time digital clock written in pure C
 *   using the native Windows API (Win32 GDI).
 *
 * Features:
 *   - Real-time updates via SetTimer (every 1000 ms)
 *   - Current time queried using GetLocalTime()
 *   - Double-buffered GDI rendering to eliminate flickering
 *   - Background erasing suppressed in WM_ERASEBKGND
 *   - Dynamic font scaling using Consolas when the window resizes
 *   - Centered horizontally and vertically (DT_CENTER | DT_VCENTER)
 *   - Clean shutdown with KillTimer() and PostQuitMessage() in WM_DESTROY
 *   - Wide-character (W) API functions paired with standard WinMain entry point
 * ============================================================================
 */

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

/* Identifier for the 1-second interval timer */
#define IDT_TIMER_CLOCK 1001

/* Forward declaration of the window procedure */
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**
 * Entry Point: WinMain
 * Uses standard WinMain signature with wide-character (W) API functions,
 * allowing effortless compilation on both MinGW-w64 and MSVC without
 * requiring special entry-point linker flags such as -municode.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"RunningDigitalClockWindowClass";
    const wchar_t WINDOW_TITLE[] = L"Running Digital Clock";

    /* Register the window class */
    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    /* hbrBackground is NULL because we draw the entire surface in WM_PAINT */
    wc.hbrBackground = NULL;

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Fatal Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Create the window with standard resizable borders and title bar */
    HWND hwnd = CreateWindowExW(
        0,                                  /* Optional extended window styles */
        CLASS_NAME,                         /* Window class name */
        WINDOW_TITLE,                       /* Window title text */
        WS_OVERLAPPEDWINDOW,                /* Standard window style (title, resize, sysmenu) */
        CW_USEDEFAULT, CW_USEDEFAULT,       /* Initial X, Y position */
        640, 360,                           /* Initial width, height (16:9 ratio) */
        NULL,                               /* Parent window */
        NULL,                               /* Menu */
        hInstance,                          /* Instance handle */
        NULL                                /* Additional application data */
    );

    if (hwnd == NULL)
    {
        MessageBoxW(NULL, L"Window Creation Failed!", L"Fatal Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    /* Display the window on screen */
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    /* Main Message Loop */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

/**
 * Window Procedure: Handles messages sent to the clock window.
 */
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            /* Start a high-level periodic timer ticking every 1000 milliseconds (1 second) */
            if (SetTimer(hwnd, IDT_TIMER_CLOCK, 1000, NULL) == 0)
            {
                MessageBoxW(hwnd, L"Failed to create timer!", L"Warning", MB_ICONWARNING | MB_OK);
            }
            return 0;
        }

        case WM_TIMER:
        {
            if (wParam == IDT_TIMER_CLOCK)
            {
                /*
                 * Invalidate the client area to schedule a WM_PAINT message.
                 * Passing FALSE for bErase prevents Windows from erasing the
                 * background before WM_PAINT, eliminating white flashes.
                 */
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_ERASEBKGND:
        {
            /*
             * Crucial for flicker-free rendering:
             * Returning a non-zero value signals to Windows that the application
             * handled erasing the background. This prevents the default erase cycle.
             */
            return 1;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;

            /* Guard against minimized or zero-size window */
            if (width <= 0 || height <= 0)
            {
                EndPaint(hwnd, &ps);
                return 0;
            }

            /* --- 1. DOUBLE BUFFERING SETUP --- */
            /* Create an off-screen memory DC compatible with the screen DC */
            HDC memDC = CreateCompatibleDC(hdc);
            /* Create a compatible bitmap to act as the off-screen canvas */
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            /* --- 2. DRAW DARK BACKGROUND --- */
            /* Deep, sleek dark background RGB(18, 20, 26) */
            HBRUSH bgBrush = CreateSolidBrush(RGB(18, 20, 26));
            FillRect(memDC, &rc, bgBrush);
            DeleteObject(bgBrush);

            /* --- 3. GET CURRENT LOCAL TIME --- */
            SYSTEMTIME st;
            GetLocalTime(&st);

            /* Format time string as HH:MM:SS */
            wchar_t timeString[32];
            swprintf(timeString, sizeof(timeString) / sizeof(wchar_t),
                     L"%02d:%02d:%02d",
                     st.wHour, st.wMinute, st.wSecond);

            /* --- 4. DYNAMIC FONT SCALING --- */
            /*
             * Calculate optimal font height based on current window dimensions:
             * "HH:MM:SS" is 8 characters long.
             * Consolas glyphs have an aspect ratio of roughly 0.55 to 0.60 width/height.
             * 8 characters at width W requires: 8 * (0.6 * H) <= width * 0.85
             * => H <= (width * 0.85) / 4.8 ~= width / 5.6
             * Also limit height so it does not exceed 45% of total window height.
             */
            int fontHeightByWidth = (width * 85) / 480;
            int fontHeightByHeight = (height * 45) / 100;
            int fontHeight = (fontHeightByWidth < fontHeightByHeight)
                                ? fontHeightByWidth
                                : fontHeightByHeight;

            /* Set a readable minimum font size */
            if (fontHeight < 16)
            {
                fontHeight = 16;
            }

            /* Create the monospaced Consolas font */
            HFONT hFont = CreateFontW(
                fontHeight,                 /* Height of font in logical units */
                0,                          /* Average character width (0 = auto) */
                0,                          /* Angle of escapement */
                0,                          /* Base-line orientation angle */
                FW_BOLD,                    /* Font weight (Bold for high legibility) */
                FALSE,                      /* Italic attribute */
                FALSE,                      /* Underline attribute */
                FALSE,                      /* Strikeout attribute */
                DEFAULT_CHARSET,            /* Character set identifier */
                OUT_OUTLINE_PRECIS,         /* Output precision */
                CLIP_DEFAULT_PRECIS,        /* Clipping precision */
                CLEARTYPE_QUALITY,          /* Output quality (ClearType for smooth edges) */
                FIXED_PITCH | FF_MODERN,    /* Pitch and family (Monospaced font) */
                L"Consolas"                 /* Typeface name */
            );

            HFONT oldFont = (HFONT)SelectObject(memDC, hFont);

            /* Set text colors: Clean electric cyan text on transparent background */
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, RGB(56, 189, 248)); /* Electric Sky Cyan */

            /* --- 5. RENDER CENTERED TEXT --- */
            DrawTextW(memDC, timeString, -1, &rc,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

            /* Clean up custom font */
            SelectObject(memDC, oldFont);
            DeleteObject(hFont);

            /* --- 6. BLIT OFF-SCREEN BUFFER TO SCREEN --- */
            BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

            /* --- 7. CLEAN UP DOUBLE BUFFER GDI OBJECTS --- */
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
        {
            /* Clean up the timer to free system resources */
            KillTimer(hwnd, IDT_TIMER_CLOCK);

            /* Post WM_QUIT to terminate the message loop cleanly */
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}
