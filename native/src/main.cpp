// PES 2021 Workbench Native - Pure C++20 + Win32 + DirectX 11 + Dear ImGui.
// Custom frameless Windows 11/10 studio window (DWM rounded corners, custom dark titlebar,
// native Aero Snap, embedded multi-res icon), single-instance mutex, 0%-idle GPU/CPU loop.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>

#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/imgui_impl_win32.h"
#include "../third_party/imgui/imgui_impl_dx11.h"

#include "engine/pes_engine.hpp"
#include "ui/theme.hpp"
#include "ui/workbench_ui.hpp"

#include <atomic>
#include <cstdlib>
#ifdef PES_QA_DUMP
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

constexpr wchar_t kWndClass[] = L"PES2021WorkbenchNativeClass";
constexpr wchar_t kMutexName[] = L"Local\\PES2021_Workbench_Native_SingleInstance_v12";
constexpr UINT kWmStatusUpdated = WM_APP + 1;
constexpr int kAppIconId = 101;        // logo Lettuce (icone do executavel e da janela)

ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
UINT                    g_resize_w = 0;
UINT                    g_resize_h = 0;
int                     g_dirty_frames = 6;
float                   g_dpi_scale = 1.0f;

pes::PESEngine*         g_engine_ptr = nullptr;
pes::ui::WorkbenchApp*  g_app_ptr = nullptr;

void emergency_shutdown() {
    if (g_engine_ptr) {
        g_engine_ptr->shutdown();
    }
}

LONG WINAPI crash_handler(EXCEPTION_POINTERS*) {
    emergency_shutdown();
    return EXCEPTION_CONTINUE_SEARCH;
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    if (SUCCEEDED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) && pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
    );
    if (FAILED(res)) {
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
        );
    }
    if (res == DXGI_ERROR_UNSUPPORTED) {
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext
        );
    }
    if (FAILED(res)) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void configure_custom_window_frame(HWND hwnd) {
    BOOL dark = TRUE;
    // DWMWA_USE_IMMERSIVE_DARK_MODE (20)
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    // DWMWA_WINDOW_CORNER_PREFERENCE (33) = DWMWCP_ROUND (2) on Windows 11
    DWORD corner = 2;
    DwmSetWindowAttribute(hwnd, 33, &corner, sizeof(corner));
    // DWMWA_BORDER_COLOR (34) = subtle studio slate-blue border #223452
    COLORREF border_col = RGB(static_cast<BYTE>(pes::ui::col::Border.x * 255.0f + 0.5f), static_cast<BYTE>(pes::ui::col::Border.y * 255.0f + 0.5f), static_cast<BYTE>(pes::ui::col::Border.z * 255.0f + 0.5f));
    DwmSetWindowAttribute(hwnd, 34, &border_col, sizeof(border_col));
    // Extend 1px into client area to preserve native DWM drop shadow on frameless window
    MARGINS margins = {0, 0, 0, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

#ifndef PES_QA_DUMP
// Grava posicao, tamanho e estado maximizado. Precisa rodar em WM_CLOSE: depois do DestroyWindow o handle ja nao existe.
void save_window_state(HWND hwnd) {
    WINDOWPLACEMENT wp{};
    wp.length = sizeof(wp);
    if (!GetWindowPlacement(hwnd, &wp)) return;
    pes::ui::AppSettings cur = pes::ui::load_settings();
    cur.win_x = wp.rcNormalPosition.left;
    cur.win_y = wp.rcNormalPosition.top;
    cur.win_w = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
    cur.win_h = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
    cur.win_max = (wp.showCmd == SW_SHOWMAXIMIZED) || ((wp.flags & WPF_RESTORETOMAXIMIZED) != 0);
    pes::ui::save_settings(cur);
}
#endif

COLORREF to_colorref(const ImVec4& c) {
    auto b = [](float v) { return static_cast<BYTE>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
    return RGB(b(c.x), b(c.y), b(c.z));
}

// A borda DWM acompanha a paleta do tema ativo (nome e icone sao sempre os do Lettuce)
void apply_window_identity(HWND hwnd) {
    COLORREF border_col = to_colorref(pes::ui::col::Border);
    DwmSetWindowAttribute(hwnd, 34, &border_col, sizeof(border_col));
}

#include "ui/logo_data.inc"

ID3D11ShaderResourceView* g_logo_srv = nullptr;
HBRUSH g_bg_brush = nullptr;

void create_logo_texture() {
    if (!g_pd3dDevice || !g_pd3dDeviceContext) return;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = kLogoSize;
    td.Height = kLogoSize;
    td.MipLevels = 0; // cadeia completa de mipmaps (o logo aparece em 20 a 60 px)
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(g_pd3dDevice->CreateTexture2D(&td, nullptr, &tex)) || !tex) return;
    g_pd3dDeviceContext->UpdateSubresource(tex, 0, nullptr, kLogoRgba, kLogoSize * 4, 0);
    D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = td.Format;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = static_cast<UINT>(-1);
    if (SUCCEEDED(g_pd3dDevice->CreateShaderResourceView(tex, &sd, &g_logo_srv)) && g_logo_srv) {
        g_pd3dDeviceContext->GenerateMips(g_logo_srv);
        pes::ui::set_logo_texture(reinterpret_cast<ImTextureID>(g_logo_srv));
    }
    tex->Release();
}

void release_logo_texture() {
    pes::ui::set_logo_texture(static_cast<ImTextureID>(0));
    if (g_logo_srv) { g_logo_srv->Release(); g_logo_srv = nullptr; }
}

bool g_dpi_changed = false;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        g_dirty_frames = 5;
        return 1;
    }

    switch (msg) {
        case WM_NCCALCSIZE: {
            if (wParam == TRUE) {
                auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                if (IsZoomed(hWnd)) {
                    int pad = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
                    params->rgrc[0].left   += pad;
                    params->rgrc[0].top    += pad;
                    params->rgrc[0].right  -= pad;
                    params->rgrc[0].bottom -= pad;
                }
                return 0;
            }
            break;
        }

        case WM_NCACTIVATE:
            g_dirty_frames = 4;
            return TRUE; // suppress legacy non-client redraw flicker

        case WM_NCHITTEST: {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT rc{};
            GetWindowRect(hWnd, &rc);
            const bool zoomed = IsZoomed(hWnd) != FALSE;
            const int border = zoomed ? 0 : static_cast<int>(6.0f * g_dpi_scale);

            if (!zoomed) {
                bool left   = pt.x < rc.left + border;
                bool right  = pt.x >= rc.right - border;
                bool top    = pt.y < rc.top + border;
                bool bottom = pt.y >= rc.bottom - border;
                if (top && left) return HTTOPLEFT;
                if (top && right) return HTTOPRIGHT;
                if (bottom && left) return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;
                if (left) return HTLEFT;
                if (right) return HTRIGHT;
                if (top) return HTTOP;
                if (bottom) return HTBOTTOM;
            }

            POINT cpt = pt;
            ScreenToClient(hWnd, &cpt);
            if (g_app_ptr && g_app_ptr->is_point_in_titlebar_caption(cpt.x, cpt.y)) {
                return HTCAPTION;
            }
            return HTCLIENT;
        }

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_SETFOCUS:
        case kWmStatusUpdated:
            g_dirty_frames = 5;
            break;

        case WM_ACTIVATE:
            g_dirty_frames = 5;
            if (LOWORD(wParam) != WA_INACTIVE && g_app_ptr) {
                g_app_ptr->wake_poller();
            }
            break;

        case WM_XBUTTONDOWN:
            if (HIWORD(wParam) == XBUTTON1 && g_app_ptr) {
                g_app_ptr->trigger_back();
                g_dirty_frames = 5;
                return TRUE;
            }
            break;

        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            mmi->ptMinTrackSize.x = static_cast<LONG>(640 * g_dpi_scale);
            mmi->ptMinTrackSize.y = static_cast<LONG>(480 * g_dpi_scale);
            return 0;
        }

        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) {
                if (g_app_ptr) g_app_ptr->set_minimized(true);
                return 0;
            }
            if (g_app_ptr) g_app_ptr->set_minimized(false);
            g_resize_w = static_cast<UINT>(LOWORD(lParam));
            g_resize_h = static_cast<UINT>(HIWORD(lParam));
            g_dirty_frames = 6;
            // Wake GetMessageW immediately because WM_SIZE is sent synchronously (non-queued)
            PostMessageW(hWnd, kWmStatusUpdated, 0, 0);
            return 0;

        case WM_EXITSIZEMOVE:
            g_dirty_frames = 6;
            PostMessageW(hWnd, kWmStatusUpdated, 0, 0);
            return 0;

        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) return 0; // disable Alt menu beep
            break;

        case WM_DPICHANGED: {
            // Janela arrastada para um monitor com outra escala: aplica o retangulo sugerido e reconstroi fontes e estilo
            const RECT* sugg = reinterpret_cast<const RECT*>(lParam);
            if (sugg) {
                SetWindowPos(hWnd, nullptr, sugg->left, sugg->top, sugg->right - sugg->left, sugg->bottom - sugg->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            g_dpi_changed = true;
            g_dirty_frames = 6;
            PostMessageW(hWnd, kWmStatusUpdated, 0, 0);
            return 0;
        }

        case WM_CLOSE:
#ifndef PES_QA_DUMP
            save_window_state(hWnd);
#endif
            break; // DefWindowProc destroi a janela

        case WM_ENDSESSION:
#ifndef PES_QA_DUMP
            if (wParam) save_window_state(hWnd);
#endif
            if (wParam) emergency_shutdown();
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    // 1. Single-instance protection
#ifdef PES_QA_DUMP
    HANDLE hMutex = nullptr; // build de QA nao disputa a instancia unica com o app aberto
    if (false) {
#else
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
#endif
        if (HWND existing = FindWindowW(kWndClass, nullptr)) {
            if (IsIconic(existing)) ShowWindow(existing, SW_RESTORE);
            SetForegroundWindow(existing);
        }
        CloseHandle(hMutex);
        return 0;
    }

    // 2. Per-Monitor V2 DPI Awareness
    ImGui_ImplWin32_EnableDpiAwareness();

    // Preferencias visuais precisam estar prontas antes do icone, da janela e do pincel de fundo
    pes::ui::AppSettings cfg = pes::ui::load_settings();
#ifdef PES_QA_DUMP
    if (const char* qt = std::getenv("PES_QA_THEME")) {  // QA: forca o tema sem tocar no settings.ini do usuario
        cfg.theme_idx = std::clamp(std::atoi(qt), 0, pes::ui::kThemeCount - 1);
        cfg.accent_idx = pes::ui::theme_default_accent(cfg.theme_idx);
    }
#endif
    pes::ui::apply_visual_theme(cfg.theme_idx);
    pes::ui::set_accent_theme(cfg.accent_idx);

    const int start_icon_id = kAppIconId;
    HICON hIconBig = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(start_icon_id), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
    HICON hIconSmall = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(start_icon_id), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = hIconBig ? hIconBig : LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = hIconSmall ? hIconSmall : wc.hIcon;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    g_bg_brush = CreateSolidBrush(RGB(static_cast<BYTE>(pes::ui::col::BgCanvas.x * 255.0f + 0.5f), static_cast<BYTE>(pes::ui::col::BgCanvas.y * 255.0f + 0.5f), static_cast<BYTE>(pes::ui::col::BgCanvas.z * 255.0f + 0.5f)));
    wc.hbrBackground = g_bg_brush;
    wc.lpszClassName = kWndClass;
    RegisterClassExW(&wc);

    // Center window comfortably on primary work area
    RECT wa{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    int wa_w = wa.right - wa.left;
    int wa_h = wa.bottom - wa.top;
    int win_w = std::min(1340, std::max(960, wa_w - 80));
    int win_h = std::min(840, std::max(640, wa_h - 60));
    int pos_x = wa.left + (wa_w - win_w) / 2;
    int pos_y = wa.top + (wa_h - win_h) / 2;

    HWND hwnd = CreateWindowExW(
        0, kWndClass, L"Lettuce Live Editor",
        WS_OVERLAPPEDWINDOW,
        pos_x, pos_y, win_w, win_h,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!hwnd) {
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    float dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    if (dpi < 0.75f || dpi > 3.0f) dpi = 1.0f;
    g_dpi_scale = dpi;

    // Resize cleanly for monitor DPI while staying comfortably inside the work area
    int scaled_w = std::min(static_cast<int>(1280.0f * dpi), std::max(720, wa_w - 48));
    int scaled_h = std::min(static_cast<int>(800.0f * dpi), std::max(540, wa_h - 48));
    int scaled_x = wa.left + (wa_w - scaled_w) / 2;
    int scaled_y = wa.top + (wa_h - scaled_h) / 2;
    bool restored_geom = false;
    if (cfg.win_w >= 640 && cfg.win_h >= 480) {
        // rcNormalPosition usa coordenadas da area de trabalho (origem no canto da area de trabalho primaria)
        RECT sr{cfg.win_x + wa.left, cfg.win_y + wa.top, cfg.win_x + wa.left + cfg.win_w, cfg.win_y + wa.top + cfg.win_h};
        // A faixa da barra de titulo precisa cair sobre algum monitor, senao a janela ficaria inalcancavel
        RECT band{sr.left + 40, sr.top, std::max<LONG>(sr.left + 41, sr.right - 40), sr.top + 40};
        if (HMONITOR mon = MonitorFromRect(&band, MONITOR_DEFAULTTONULL)) {
            MONITORINFO mi{};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(mon, &mi) && mi.rcWork.right > mi.rcWork.left && mi.rcWork.bottom > mi.rcWork.top) {
            const int mw = mi.rcWork.right - mi.rcWork.left;
            const int mh = mi.rcWork.bottom - mi.rcWork.top;
            const int w = std::min(cfg.win_w, mw);
            const int h = std::min(cfg.win_h, mh);
            const int x = std::clamp(static_cast<int>(sr.left), static_cast<int>(mi.rcWork.left), static_cast<int>(mi.rcWork.right) - w);
            const int y = std::clamp(static_cast<int>(sr.top), static_cast<int>(mi.rcWork.top), static_cast<int>(mi.rcWork.bottom) - h);
            WINDOWPLACEMENT wp{};
            wp.length = sizeof(wp);
            wp.showCmd = SW_HIDE; // so posiciona: a janela aparece depois, em ShowWindow(nCmdShow)
            wp.rcNormalPosition = RECT{x - wa.left, y - wa.top, x - wa.left + w, y - wa.top + h};
            SetWindowPlacement(hwnd, &wp);
            restored_geom = true;
            }
        }
    }
    if (!restored_geom) {
        SetWindowPos(hwnd, nullptr, scaled_x, scaled_y, scaled_w, scaled_h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (cfg.topmost) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (restored_geom && cfg.win_max) nCmdShow = SW_SHOWMAXIMIZED;

    // A janela pode ter ido parar em um monitor com outro DPI: mede de novo antes de montar fontes e estilo
    dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    if (dpi < 0.75f || dpi > 3.0f) dpi = 1.0f;
    g_dpi_scale = dpi;

    configure_custom_window_frame(hwnd);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClassW(kWndClass, hInstance);
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 3. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
#ifdef PES_QA_DUMP
    io.ConfigErrorRecoveryEnableAssert = true; // QA: Push/Pop desbalanceado vira IM_ASSERT registrado em arquivo
#endif
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    const float eff_dpi = dpi * pes::ui::kUiScales[std::clamp(cfg.ui_scale_idx, 0, 4)];
    pes::ui::apply_studio_theme(eff_dpi);
    pes::ui::FontBank fonts;
    pes::ui::build_font_bank(fonts, eff_dpi);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    create_logo_texture();

    // 4. Initialize engine & crash-safe teardown hooks
    pes::PESEngine engine;
    g_engine_ptr = &engine;
    std::atexit(emergency_shutdown);
    SetUnhandledExceptionFilter(crash_handler);

    pes::ui::WorkbenchApp app(engine);
    app.set_window_handle(hwnd, eff_dpi);
    g_app_ptr = &app;

#ifdef PES_QA_DUMP
    {
        auto save_backbuffer_bmp = [&](const std::string& path) {
            ID3D11Texture2D* bb = nullptr;
            if (FAILED(g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&bb))) || !bb) return;
            D3D11_TEXTURE2D_DESC desc{};
            bb->GetDesc(&desc);
            D3D11_TEXTURE2D_DESC sd = desc;
            sd.Usage = D3D11_USAGE_STAGING;
            sd.BindFlags = 0;
            sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            sd.MiscFlags = 0;
            ID3D11Texture2D* staging = nullptr;
            if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&sd, nullptr, &staging)) && staging) {
                g_pd3dDeviceContext->CopyResource(staging, bb);
                D3D11_MAPPED_SUBRESOURCE mapped{};
                if (SUCCEEDED(g_pd3dDeviceContext->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
                    const uint32_t w = desc.Width, h = desc.Height;
                    const uint32_t row_stride = (w * 3 + 3) & ~3u;
                    const uint32_t img_sz = row_stride * h;
                    std::vector<uint8_t> out(54 + img_sz, 0);
                    out[0] = 'B'; out[1] = 'M';
                    uint32_t fsz = 54 + img_sz;
                    std::memcpy(&out[2], &fsz, 4);
                    uint32_t off = 54; std::memcpy(&out[10], &off, 4);
                    uint32_t ih = 40; std::memcpy(&out[14], &ih, 4);
                    std::memcpy(&out[18], &w, 4);
                    int32_t nh = -static_cast<int32_t>(h); std::memcpy(&out[22], &nh, 4);
                    uint16_t pl = 1, bpp = 24;
                    std::memcpy(&out[26], &pl, 2); std::memcpy(&out[28], &bpp, 2);
                    std::memcpy(&out[34], &img_sz, 4);
                    const uint8_t* src_base = static_cast<const uint8_t*>(mapped.pData);
                    for (uint32_t y = 0; y < h; ++y) {
                        const uint8_t* srow = src_base + y * mapped.RowPitch;
                        uint8_t* drow = out.data() + 54 + y * row_stride;
                        for (uint32_t x = 0; x < w; ++x) {
                            // Backbuffer is RGBA8 -> BMP is BGR8
                            drow[x * 3 + 0] = srow[x * 4 + 2];
                            drow[x * 3 + 1] = srow[x * 4 + 1];
                            drow[x * 3 + 2] = srow[x * 4 + 0];
                        }
                    }
                    g_pd3dDeviceContext->Unmap(staging, 0);
                    std::ofstream ofs(path, std::ios::binary);
                    ofs.write(reinterpret_cast<const char*>(out.data()), out.size());
                }
                staging->Release();
            }
            bb->Release();
        };

        const char* env_out = std::getenv("PES_QA_OUT");
        std::string out_dir = (env_out && env_out[0]) ? std::string(env_out) : std::string("qa_out");
        std::filesystem::create_directories(out_dir);
        app.qa_populate_fixture();
        const float qa_clear[4] = {0.031f, 0.047f, 0.078f, 1.0f};

        struct Shot { const char* name; int w; int h; int view; int ptab; int modal; };
        const Shot shots[] = {
            {"01_wide_dashboard",      1920, 1200, 0, 0, 0},
            {"02_wide_squad",          1920, 1200, 1, 0, 0},
            {"03_wide_finances",       1920, 1200, 2, 0, 0},
            {"04_wide_player_bio",     1920, 1200, 3, 0, 0},
            {"05_wide_player_stats",   1920, 1200, 3, 1, 0},
            {"06_wide_player_poslab",  1920, 1200, 3, 2, 0},
            {"07_wide_player_skills",  1920, 1200, 3, 3, 0},
            {"08_wide_player_role",    1920, 1200, 3, 4, 0},
            {"09_wide_search",         1920, 1200, 4, 0, 0},
            {"10_wide_scouts",         1920, 1200, 5, 0, 0},
            {"11_wide_match",          1920, 1200, 6, 0, 0},
            {"12_wide_training",       1920, 1200, 7, 0, 0},
            {"13_wide_cmdpalette",     1920, 1200, 0, 0, 1},
            {"14_wide_jsonmodal",      1920, 1200, 3, 0, 2},
            {"23_wide_packstudio",     1920, 1200, 1, 0, 3},
            {"15_med_player_bio",      1320,  860, 3, 0, 0},
            {"16_med_player_poslab",   1320,  860, 3, 2, 0},
            {"17_narrow_dashboard",     980, 1080, 0, 0, 0},
            {"18_narrow_squad",         980, 1080, 1, 0, 0},
            {"19_narrow_player_bio",    980, 1080, 3, 0, 0},
            {"20_narrow_player_stats",  980, 1080, 3, 1, 0},
            {"21_narrow_player_poslab", 980, 1080, 3, 2, 0},
            {"22_narrow_match",         980, 1080, 6, 0, 0},
            {"24_narrow_packstudio",    980, 1080, 1, 0, 3},
            {"25_wide_prefs",          1920, 1200, 0, 0, 5},
            {"26_narrow_prefs",         980, 1080, 0, 0, 5},
            {"36_wide_proc_picker",    1920, 1200, 0, 0, 10},
            {"37_narrow_proc_picker",   980, 1080, 0, 0, 10},
            {"34_wide_prefs_services", 1920, 1200, 0, 0, 9},
            {"35_narrow_prefs_services",980, 1080, 0, 0, 9},
            {"27_tall_player_bio",     1920, 2300, 3, 0, 0},
            {"28_tall_player_bio_lock",1920, 2300, 3, 0, 8},
            {"29_wide_offline_dash",   1920, 1200, 0, 0, 7},
            {"32_wide_update_banner",  1920, 1200, 0, 0, 6},
            {"33_narrow_update_banner", 980, 1080, 0, 0, 6},
        };
        int last_w = -1, last_h = -1;
        for (const auto& s : shots) {
            if (s.w != last_w || s.h != last_h) {
                SetWindowPos(hwnd, nullptr, 40, 20, s.w, s.h, SWP_NOZORDER | SWP_NOACTIVATE);
                MSG m{};
                while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&m); DispatchMessageW(&m); }
                RECT cr{}; GetClientRect(hwnd, &cr);
                UINT cw = static_cast<UINT>(cr.right - cr.left);
                UINT ch = static_cast<UINT>(cr.bottom - cr.top);
                if (cw > 0 && ch > 0) {
                    g_pd3dDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
                    CleanupRenderTarget();
                    g_pd3dDeviceContext->Flush();
                    g_pSwapChain->ResizeBuffers(0, cw, ch, DXGI_FORMAT_UNKNOWN, 0);
                    CreateRenderTarget();
                }
                g_resize_w = g_resize_h = 0;
                last_w = s.w; last_h = s.h;
            }
            app.qa_select_screen(s.view, s.ptab, s.modal);
            for (int f = 0; f < 4; ++f) {
                ImGui_ImplDX11_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
                app.render_frame(fonts);
                ImGui::Render();
                g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
                g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, qa_clear);
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                if (f == 3) save_backbuffer_bmp(out_dir + "\\" + s.name + ".bmp");
                g_pSwapChain->Present(0, 0);
            }
            // Close any open popup before next shot
            if (s.modal != 0) {
                ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
                ImGui::EndFrame();
            }
        }
        g_app_ptr = nullptr;
        g_engine_ptr = nullptr;
        release_logo_texture();
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClassW(kWndClass, hInstance);
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }
#endif

    app.start_background_poller(hwnd);

    // 5. Event-Driven Zero-Idle-CPU/GPU Main Loop
    bool done = false;
    bool keep_animating = true;

    while (!done) {
        MSG msg;
        if (!keep_animating && g_dirty_frames <= 0) {
            if (GetMessageW(&msg, nullptr, 0, 0) <= 0) {
                done = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            g_dirty_frames = 5;
        }

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (IsIconic(hwnd)) {
            g_dirty_frames = 0;
            keep_animating = false;
            continue;
        }

        if (g_resize_w != 0 && g_resize_h != 0) {
            g_pd3dDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
            CleanupRenderTarget();
            g_pd3dDeviceContext->Flush();
            g_pSwapChain->ResizeBuffers(0, g_resize_w, g_resize_h, DXGI_FORMAT_UNKNOWN, 0);
            g_resize_w = g_resize_h = 0;
            CreateRenderTarget();
        }

        const bool dpi_moved = g_dpi_changed;
        g_dpi_changed = false;
        if (dpi_moved) {
            dpi = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
            if (dpi < 0.75f || dpi > 3.0f) dpi = 1.0f;
            g_dpi_scale = dpi;
        }
        if (app.take_theme_rebuild() || dpi_moved) {
            // A paleta so muda aqui, entre quadros: nenhum quadro mistura cores velhas e novas
            pes::ui::apply_visual_theme(app.settings().theme_idx);
            pes::ui::set_accent_theme(app.settings().accent_idx);
            if (g_bg_brush) {
                HBRUSH nb = CreateSolidBrush(to_colorref(pes::ui::col::BgCanvas));
                SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(nb));
                DeleteObject(g_bg_brush);
                g_bg_brush = nb;
            }
            // Escala da UI ou cor de destaque mudou: reconstroi atlas de fontes e estilo entre frames
            const float new_dpi = dpi * pes::ui::kUiScales[std::clamp(app.settings().ui_scale_idx, 0, 4)];
            ImGui_ImplDX11_InvalidateDeviceObjects();
            io.Fonts->Clear();
            fonts = pes::ui::FontBank{};
            pes::ui::build_font_bank(fonts, new_dpi);
            pes::ui::apply_studio_theme(new_dpi);
            app.set_window_handle(hwnd, new_dpi);
            apply_window_identity(hwnd);
            g_dirty_frames = 6;
        }

        // Passe de assentamento: o ImGui calcula larguras de coluna, altura de cards e presenca de barra de rolagem
        // a partir do quadro anterior. Rodar um quadro oculto antes do visivel faz o layout ja nascer no estado final
        // (sem o "pulo" de um quadro apos navegar, trocar tema, redimensionar ou receber dados novos).
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app.render_frame(fonts);
        ImGui::EndFrame();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        keep_animating = app.render_frame(fonts);

        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        const float clear_col[4] = {pes::ui::col::BgCanvas.x, pes::ui::col::BgCanvas.y, pes::ui::col::BgCanvas.z, 1.0f};
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_col);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
        if (g_dirty_frames > 0) --g_dirty_frames;
    }

    // 6. Clean Shutdown
    app.stop_background_poller();
    g_app_ptr = nullptr;
    engine.shutdown();
    g_engine_ptr = nullptr;

    release_logo_texture();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(kWndClass, hInstance);
    if (hMutex) CloseHandle(hMutex);
    return 0;
}
