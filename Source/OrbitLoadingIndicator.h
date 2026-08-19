#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
 #include <windows.h>
 #ifdef min
  #undef min
 #endif
 #ifdef max
  #undef max
 #endif
 #include <atomic>
 #include <cmath>
 #include <mutex>
 #include <thread>

class OrbitLoadingIndicator final
{
public:
    OrbitLoadingIndicator() = default;
    ~OrbitLoadingIndicator() { stop(); }

    static OrbitLoadingIndicator& shared()
    {
        static OrbitLoadingIndicator indicator;
        return indicator;
    }

    bool start(const juce::String& statusText,
               const juce::String& detailText = {},
               void* ownerHandle = nullptr)
    {
        if (worker.joinable() && windowHandle.load() != nullptr)
        {
            updateText(statusText, detailText);
            return true;
        }

        stop();
        {
            const std::lock_guard<std::mutex> lock(textMutex);
            status = statusText.toWideCharPointer();
            detail = detailText.toWideCharPointer();
        }
        owner = static_cast<HWND>(ownerHandle);
        readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (readyEvent == nullptr)
            return false;

        worker = std::thread([this] { runWindow(); });
        WaitForSingleObject(readyEvent, 1500);
        return windowHandle.load() != nullptr;
    }

    void updateText(const juce::String& statusText, const juce::String& detailText = {})
    {
        {
            const std::lock_guard<std::mutex> lock(textMutex);
            status = statusText.toWideCharPointer();
            detail = detailText.toWideCharPointer();
        }
        if (auto* window = windowHandle.load())
            InvalidateRect(window, nullptr, FALSE);
    }

    void stop()
    {
        if (!worker.joinable())
            return;

        if (auto* window = windowHandle.load())
            PostMessageW(window, WM_CLOSE, 0, 0);
        else if (const auto id = windowThreadId.load(); id != 0)
            PostThreadMessageW(id, WM_QUIT, 0, 0);

        worker.join();
        windowHandle.store(nullptr);
        windowThreadId.store(0);
        if (readyEvent != nullptr)
        {
            CloseHandle(readyEvent);
            readyEvent = nullptr;
        }
    }

private:
    static constexpr wchar_t windowClassPrefix[] = L"LoopernicoOrbitLoadingIndicator";
    static constexpr int windowWidth = 520;
    static constexpr int windowHeight = 270;

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        OrbitLoadingIndicator* indicator = nullptr;
        if (message == WM_NCCREATE)
        {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            indicator = static_cast<OrbitLoadingIndicator*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(indicator));
        }
        else
            indicator = reinterpret_cast<OrbitLoadingIndicator*>(GetWindowLongPtrW(window, GWLP_USERDATA));

        if (indicator == nullptr)
            return DefWindowProcW(window, message, wParam, lParam);

        switch (message)
        {
            case WM_ERASEBKGND:
                return 1;
            case WM_TIMER:
                indicator->angle += 0.055;
                if (indicator->angle >= juce::MathConstants<double>::twoPi)
                    indicator->angle -= juce::MathConstants<double>::twoPi;
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            case WM_PAINT:
                indicator->paint(window);
                return 0;
            case WM_CLOSE:
                DestroyWindow(window);
                return 0;
            case WM_DESTROY:
                KillTimer(window, 1);
                PostQuitMessage(0);
                return 0;
            default:
                break;
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    static void drawTextLine(HDC dc, const std::wstring& text, RECT bounds,
                             int height, int weight, COLORREF colour)
    {
        if (text.empty())
            return;

        auto* font = CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        auto* oldFont = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, colour);
        DrawTextW(dc, text.c_str(), (int) text.size(), &bounds,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, oldFont);
        DeleteObject(font);
    }

    void paint(HWND window)
    {
        std::wstring currentStatus;
        std::wstring currentDetail;
        {
            const std::lock_guard<std::mutex> lock(textMutex);
            currentStatus = status;
            currentDetail = detail;
        }

        PAINTSTRUCT paintState {};
        auto* target = BeginPaint(window, &paintState);
        RECT client {};
        GetClientRect(window, &client);

        auto* buffer = CreateCompatibleDC(target);
        auto* bitmap = CreateCompatibleBitmap(target, client.right, client.bottom);
        auto* oldBitmap = SelectObject(buffer, bitmap);

        auto* background = CreateSolidBrush(RGB(8, 11, 16));
        FillRect(buffer, &client, background);
        DeleteObject(background);

        auto* borderPen = CreatePen(PS_SOLID, 2, RGB(39, 183, 210));
        auto* oldPen = SelectObject(buffer, borderPen);
        auto* panelBrush = CreateSolidBrush(RGB(12, 16, 22));
        auto* oldBrush = SelectObject(buffer, panelBrush);
        RoundRect(buffer, 1, 1, client.right - 1, client.bottom - 1, 24, 24);
        SelectObject(buffer, oldBrush);
        SelectObject(buffer, oldPen);
        DeleteObject(panelBrush);
        DeleteObject(borderPen);

        const POINT stars[] = {
            { 54, 67 }, { 91, 124 }, { 132, 45 }, { 169, 91 }, { 205, 55 },
            { 321, 51 }, { 356, 105 }, { 402, 61 }, { 447, 126 }, { 474, 78 }
        };
        for (int index = 0; index < (int) juce::numElementsInArray(stars); ++index)
        {
            const int brightness = 82 + (index % 3) * 30;
            auto* star = CreateSolidBrush(RGB(brightness, brightness + 8, brightness + 14));
            const int radius = index % 4 == 0 ? 2 : 1;
            oldBrush = SelectObject(buffer, star);
            oldPen = SelectObject(buffer, GetStockObject(NULL_PEN));
            Ellipse(buffer, stars[index].x - radius, stars[index].y - radius,
                    stars[index].x + radius + 1, stars[index].y + radius + 1);
            SelectObject(buffer, oldPen);
            SelectObject(buffer, oldBrush);
            DeleteObject(star);
        }

        RECT logoBounds { 24, 15, windowWidth - 24, 50 };
        drawTextLine(buffer, L"LOOPERNICO", logoBounds, 28, FW_BOLD, RGB(237, 241, 245));

        constexpr int centreX = windowWidth / 2;
        constexpr int centreY = 123;
        constexpr int orbitRadiusX = 104;
        constexpr int orbitRadiusY = 43;

        auto* orbitPen = CreatePen(PS_SOLID, 1, RGB(35, 74, 88));
        oldPen = SelectObject(buffer, orbitPen);
        auto* hollowBrush = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
        oldBrush = SelectObject(buffer, hollowBrush);
        Ellipse(buffer, centreX - orbitRadiusX, centreY - orbitRadiusY,
                centreX + orbitRadiusX, centreY + orbitRadiusY);
        SelectObject(buffer, oldBrush);
        SelectObject(buffer, oldPen);
        DeleteObject(orbitPen);

        const double depth = std::sin(angle);
        const int moonX = centreX + juce::roundToInt((double) orbitRadiusX * std::cos(angle));
        const int moonY = centreY + juce::roundToInt((double) orbitRadiusY * depth);
        const int moonRadius = juce::roundToInt(8.5 + (depth + 1.0) * 1.5);

        if (depth < 0.0)
            drawMoon(buffer, moonX, moonY, moonRadius, 0.55f);

        for (int radius = 43; radius >= 34; --radius)
        {
            const int fade = juce::jlimit(0, 45, (43 - radius) * 5);
            auto* glow = CreateSolidBrush(RGB(18 + fade / 3, 74 + fade, 96 + fade));
            oldBrush = SelectObject(buffer, glow);
            oldPen = SelectObject(buffer, GetStockObject(NULL_PEN));
            Ellipse(buffer, centreX - radius, centreY - radius,
                    centreX + radius, centreY + radius);
            SelectObject(buffer, oldPen);
            SelectObject(buffer, oldBrush);
            DeleteObject(glow);
        }
        auto* planet = CreateSolidBrush(RGB(38, 137, 164));
        oldBrush = SelectObject(buffer, planet);
        oldPen = SelectObject(buffer, GetStockObject(NULL_PEN));
        Ellipse(buffer, centreX - 32, centreY - 32, centreX + 32, centreY + 32);
        SelectObject(buffer, oldPen);
        SelectObject(buffer, oldBrush);
        DeleteObject(planet);

        auto* shadow = CreateSolidBrush(RGB(12, 47, 64));
        oldBrush = SelectObject(buffer, shadow);
        oldPen = SelectObject(buffer, GetStockObject(NULL_PEN));
        Ellipse(buffer, centreX - 6, centreY - 31, centreX + 32, centreY + 31);
        SelectObject(buffer, oldPen);
        SelectObject(buffer, oldBrush);
        DeleteObject(shadow);

        auto* highlight = CreateSolidBrush(RGB(125, 218, 226));
        oldBrush = SelectObject(buffer, highlight);
        Ellipse(buffer, centreX - 20, centreY - 19, centreX - 8, centreY - 7);
        SelectObject(buffer, oldBrush);
        DeleteObject(highlight);

        if (depth >= 0.0)
            drawMoon(buffer, moonX, moonY, moonRadius, 1.0f);

        RECT statusBounds { 28, 178, windowWidth - 28, 207 };
        drawTextLine(buffer, currentStatus, statusBounds, 15, FW_BOLD, RGB(222, 228, 234));
        RECT detailBounds { 38, 210, windowWidth - 38, 239 };
        drawTextLine(buffer, currentDetail, detailBounds, 13, FW_NORMAL, RGB(154, 169, 181));

        BitBlt(target, 0, 0, client.right, client.bottom, buffer, 0, 0, SRCCOPY);
        SelectObject(buffer, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(buffer);
        EndPaint(window, &paintState);
    }

    static void drawMoon(HDC dc, int x, int y, int radius, float brightness)
    {
        const int base = juce::roundToInt(170.0f * brightness);
        auto* moon = CreateSolidBrush(RGB(base + 30, base + 34, base + 38));
        auto* oldBrush = SelectObject(dc, moon);
        auto* oldPen = SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, x - radius, y - radius, x + radius, y + radius);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(moon);

        auto* crater = CreateSolidBrush(RGB(juce::roundToInt(105.0f * brightness),
                                            juce::roundToInt(112.0f * brightness),
                                            juce::roundToInt(120.0f * brightness)));
        oldBrush = SelectObject(dc, crater);
        Ellipse(dc, x - radius / 2, y - radius / 3,
                x - radius / 7, y + radius / 10);
        SelectObject(dc, oldBrush);
        DeleteObject(crater);
    }

    void runWindow()
    {
        windowThreadId.store(GetCurrentThreadId());
        const auto instance = GetModuleHandleW(nullptr);
        registeredClassName = windowClassPrefix
            + std::to_wstring(reinterpret_cast<std::uintptr_t>(this));
        WNDCLASSEXW windowClass {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        windowClass.lpszClassName = registeredClassName.c_str();
        RegisterClassExW(&windowClass);

        RECT placement {};
        if (owner != nullptr && IsWindow(owner))
            GetWindowRect(owner, &placement);
        else
        {
            const auto monitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO monitorInfo { sizeof(MONITORINFO) };
            GetMonitorInfoW(monitor, &monitorInfo);
            placement = monitorInfo.rcWork;
        }

        const int x = placement.left + ((placement.right - placement.left) - windowWidth) / 2;
        const int y = placement.top + ((placement.bottom - placement.top) - windowHeight) / 2;
        const auto window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                                             registeredClassName.c_str(), L"", WS_POPUP,
                                             x, y, windowWidth, windowHeight,
                                             owner, nullptr, instance, this);
        windowHandle.store(window);
        if (window != nullptr)
        {
            SetWindowRgn(window, CreateRoundRectRgn(0, 0, windowWidth + 1, windowHeight + 1, 24, 24), TRUE);
            SetTimer(window, 1, 33, nullptr);
            ShowWindow(window, SW_SHOWNOACTIVATE);
            UpdateWindow(window);
        }
        SetEvent(readyEvent);

        MSG message {};
        while (window != nullptr && GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        windowHandle.store(nullptr);
        UnregisterClassW(registeredClassName.c_str(), instance);
    }

    std::thread worker;
    std::atomic<HWND> windowHandle { nullptr };
    std::atomic<DWORD> windowThreadId { 0 };
    HANDLE readyEvent = nullptr;
    HWND owner = nullptr;
    std::wstring status;
    std::wstring detail;
    std::wstring registeredClassName;
    std::mutex textMutex;
    double angle = -juce::MathConstants<double>::halfPi;

    JUCE_DECLARE_NON_COPYABLE(OrbitLoadingIndicator)
};

#else

class OrbitLoadingIndicator final
{
public:
    static OrbitLoadingIndicator& shared()
    {
        static OrbitLoadingIndicator indicator;
        return indicator;
    }
    bool start(const juce::String&, const juce::String& = {}, void* = nullptr) { return false; }
    void updateText(const juce::String&, const juce::String& = {}) {}
    void stop() {}
};

#endif
