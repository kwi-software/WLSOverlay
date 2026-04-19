/*
 * Copyright 2026 Karl Wintermann (kwi-software)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Contact: https://github.com/kwi-software | kwi-software(at)proton.me
 */

#include "shared.h"
#include <string>
#include <vector>
#include <algorithm>
#include <objidl.h>
#include <gdiplus.h>

#pragma comment (lib,"Gdiplus.lib")

#define WM_UPDATE_OVERLAY (WM_APP + 1)

std::wstring globalText = L"";
Gdiplus::Image* globalImage = nullptr;

std::wstring globalFontFamily = L"Arial";
std::wstring globalConfigPathText = L"";
std::wstring globalConfigPathImage = L"";
std::wstring globalDirPath = L"";
std::wstring globalOverlayFilesDirPath = L"\\OverlayFiles";

int globalFontSize = 24;
bool globalIsWhiteText = true;
UINT globalTextAlignment = DT_LEFT;
int globalTabWidth = 4;
int globalPosX = 25;
int globalPosY = 25;
int globalAnchor = 0;
int globalOpacity = 100;
int globalOverlayType = 0;

DWORD globalTargetRegRoot = 1;
std::wstring globalTargetRegKey = L"";
std::wstring globalTargetRegValue = L"";

HANDLE hEventTarget = NULL;
HKEY hKeyTarget = NULL;
std::wstring currentWatchedTargetKey = L"";

HKEY ParseRegistryPath(const std::wstring& fullPath, std::wstring& outSubkey) {
    size_t firstSlash = fullPath.find(L'\\');
    std::wstring rootStr;

    if (firstSlash != std::wstring::npos) {
        rootStr = fullPath.substr(0, firstSlash);
        outSubkey = fullPath.substr(firstSlash + 1);
    }
    else {
        rootStr = fullPath;
        outSubkey = L"";
    }

    std::transform(rootStr.begin(), rootStr.end(), rootStr.begin(), ::toupper);

    if (rootStr == L"HKEY_CURRENT_USER" || rootStr == L"HKCU") return HKEY_CURRENT_USER;
    if (rootStr == L"HKEY_LOCAL_MACHINE" || rootStr == L"HKLM") return HKEY_LOCAL_MACHINE;
    if (rootStr == L"HKEY_CLASSES_ROOT" || rootStr == L"HKCR") return HKEY_CLASSES_ROOT;
    if (rootStr == L"HKEY_USERS" || rootStr == L"HKU") return HKEY_USERS;

    return NULL;
}

void SetupPortablePaths() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring fullPath(path);
    size_t lastSlash = fullPath.find_last_of(L"\\");
    globalDirPath = fullPath.substr(0, lastSlash);
    globalConfigPathText = globalDirPath + globalOverlayFilesDirPath + L"\\overlay.txt";
    globalConfigPathImage = globalDirPath + globalOverlayFilesDirPath + L"\\overlay.png";
}

void LoadSettingsFromRegistry() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\KWI-Software\\WLSOverlay", 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    DWORD dataSize, tempVal;
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"OverlayType", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) globalOverlayType = tempVal;
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"Opacity", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) { if (tempVal <= 100) globalOpacity = tempVal; }
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"PosX", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) globalPosX = tempVal;
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"PosY", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) globalPosY = tempVal;
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"Anchor", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) { if (tempVal <= 3) globalAnchor = tempVal; }
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"FontSize", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) { if (tempVal > 5 && tempVal < 150) globalFontSize = tempVal; }
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"IsWhiteText", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) globalIsWhiteText = (tempVal != 0);
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"TextAlignment", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) globalTextAlignment = tempVal;
    dataSize = sizeof(DWORD);
    if (RegGetValueW(hKey, NULL, L"TabWidth", RRF_RT_DWORD, NULL, &tempVal, &dataSize) == ERROR_SUCCESS) { if (tempVal >= 1 && tempVal <= 32) globalTabWidth = (int)tempVal; }

    wchar_t fontBuffer[256] = { 0 };
    dataSize = sizeof(fontBuffer);
    if (RegGetValueW(hKey, NULL, L"FontFamily", RRF_RT_REG_SZ, NULL, fontBuffer, &dataSize) == ERROR_SUCCESS) globalFontFamily = fontBuffer;

    wchar_t strBuf[1024] = { 0 };
    dataSize = sizeof(strBuf);
    if (RegGetValueW(hKey, NULL, L"TargetRegKey", RRF_RT_REG_SZ, NULL, strBuf, &dataSize) == ERROR_SUCCESS) globalTargetRegKey = strBuf;

    dataSize = sizeof(strBuf);
    if (RegGetValueW(hKey, NULL, L"TargetRegValue", RRF_RT_REG_SZ, NULL, strBuf, &dataSize) == ERROR_SUCCESS) globalTargetRegValue = strBuf;

    RegCloseKey(hKey);
}

void UpdateTargetRegistryWatch() {
    if (globalOverlayType != 2 || globalTargetRegKey.empty()) {
        if (hKeyTarget) { RegCloseKey(hKeyTarget); hKeyTarget = NULL; }
        currentWatchedTargetKey = L"";
        return;
    }

    if (globalTargetRegKey != currentWatchedTargetKey || hKeyTarget == NULL) {
        if (hKeyTarget) { RegCloseKey(hKeyTarget); hKeyTarget = NULL; }

        std::wstring subKey;
        HKEY root = ParseRegistryPath(globalTargetRegKey, subKey);

        if (root && !subKey.empty()) {
            if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_NOTIFY, &hKeyTarget) == ERROR_SUCCESS) {
                RegNotifyChangeKeyValue(hKeyTarget, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEventTarget, TRUE);
                currentWatchedTargetKey = globalTargetRegKey;
            }
            else {
                currentWatchedTargetKey = L"";
            }
        }
    }
}

std::wstring LoadTextFromFile(const std::wstring& path) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) return L"";
        return globalText;
    }
    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size) || size.QuadPart > 1024 * 1024 || size.QuadPart == 0) {
        CloseHandle(hFile);
        return size.QuadPart == 0 ? L"" : globalText;
    }

    std::vector<char> buffer((size_t)size.QuadPart);
    DWORD bytesRead;
    if (!ReadFile(hFile, buffer.data(), (DWORD)buffer.size(), &bytesRead, NULL)) {
        CloseHandle(hFile);
        return globalText;
    }
    CloseHandle(hFile);

    int sz = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), (int)bytesRead, NULL, 0);
    std::wstring w(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, buffer.data(), (int)bytesRead, &w[0], sz);
    return w;
}

std::wstring LoadTextFromRegistryTarget() {
    if (globalTargetRegKey.empty() || globalTargetRegValue.empty()) return L"";

    std::wstring subKey;
    HKEY root = ParseRegistryPath(globalTargetRegKey, subKey);

    if (!root || subKey.empty()) return L"";

    HKEY hKey;
    if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) return L"";

    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExW(hKey, globalTargetRegValue.c_str(), NULL, &type, NULL, &size) != ERROR_SUCCESS || size == 0) {
        RegCloseKey(hKey); return L"";
    }

    std::wstring result = L"";
    std::vector<wchar_t> buffer(size / sizeof(wchar_t) + 1);

    if (RegQueryValueExW(hKey, globalTargetRegValue.c_str(), NULL, NULL, (LPBYTE)buffer.data(), &size) == ERROR_SUCCESS) {
        if (type == REG_SZ || type == REG_EXPAND_SZ) {
            result = buffer.data();
        }
        else if (type == REG_MULTI_SZ) {
            wchar_t* str = buffer.data();
            while (*str) {
                result += str;
                result += L"\n";
                str += wcslen(str) + 1;
            }
            if (!result.empty() && result.back() == L'\n') result.pop_back();
        }
    }
    RegCloseKey(hKey);
    return result;
}

Gdiplus::Image* LoadPngImage(const std::wstring& path) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return nullptr;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size) || size.QuadPart < 8) {
        CloseHandle(hFile);
        return nullptr;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size.QuadPart);
    if (!hMem) {
        CloseHandle(hFile);
        return nullptr;
    }

    void* pMem = GlobalLock(hMem);
    if (!pMem) {
        GlobalFree(hMem);
        CloseHandle(hFile);
        return nullptr;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, pMem, (DWORD)size.QuadPart, &bytesRead, NULL)) {
        GlobalUnlock(hMem);
        GlobalFree(hMem);
        CloseHandle(hFile);
        return nullptr;
    }

    unsigned char* pBytes = static_cast<unsigned char*>(pMem);

    // check PNG Header
    if (bytesRead < 8 ||
        pBytes[0] != 0x89 || pBytes[1] != 0x50 || pBytes[2] != 0x4E || pBytes[3] != 0x47 ||
        pBytes[4] != 0x0D || pBytes[5] != 0x0A || pBytes[6] != 0x1A || pBytes[7] != 0x0A) {

        GlobalUnlock(hMem);
        GlobalFree(hMem);
        CloseHandle(hFile);
        return nullptr;
    }

    GlobalUnlock(hMem);
    CloseHandle(hFile);

    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
        GlobalFree(hMem);
        return nullptr;
    }

    Gdiplus::Image* tempImg = Gdiplus::Image::FromStream(pStream);
    if (!tempImg || tempImg->GetLastStatus() != Gdiplus::Ok) {
        if (tempImg) delete tempImg;
        pStream->Release();
        return nullptr;
    }

    Gdiplus::Image* finalImg = tempImg->Clone();
    delete tempImg;
    pStream->Release();
    return finalImg;
}

void CalculateWindowPosition(int width, int height, int padding, int& outX, int& outY) {
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);

    switch (globalAnchor) {
    case 1:
        outX = sw - width - globalPosX + padding;
        outY = globalPosY - padding;
        break;
    case 2:
        outX = globalPosX - padding;
        outY = sh - height - globalPosY + padding;
        break;
    case 3:
        outX = sw - width - globalPosX + padding;
        outY = sh - height - globalPosY + padding;
        break;
    default:
        outX = globalPosX - padding;
        outY = globalPosY - padding;
        break;
    }
}

void RenderAndApplyOverlay(HWND hwnd) {
    LoadSettingsFromRegistry();
    int finalW = 0, finalH = 0;
    bool shouldHide = false;

    if (globalOverlayType == 0 || globalOverlayType == 2) {
        if (globalOverlayType == 0) globalText = LoadTextFromFile(globalConfigPathText);
        else globalText = LoadTextFromRegistryTarget();

        if (globalText.empty()) shouldHide = true;
        else {
            HDC hdc = GetDC(NULL);
            Gdiplus::Graphics g(hdc);
            Gdiplus::Font font(globalFontFamily.c_str(), static_cast<Gdiplus::REAL>(globalFontSize), Gdiplus::FontStyleBold);
            Gdiplus::RectF layoutRect(0, 0, static_cast<Gdiplus::REAL>(GetSystemMetrics(SM_CXSCREEN) - 200), 5000.0f);
            Gdiplus::RectF boundRect;
            Gdiplus::StringFormat format;
            Gdiplus::REAL tabs = static_cast<Gdiplus::REAL>(globalFontSize * globalTabWidth * 0.5f);
            format.SetTabStops(0.0f, 1, &tabs);

            g.MeasureString(globalText.c_str(), -1, &font, layoutRect, &format, &boundRect);
            ReleaseDC(NULL, hdc);

            finalW = (int)boundRect.Width + 50;
            finalH = (int)boundRect.Height + 50;
        }
    }
    else if (globalOverlayType == 1) {
        if (globalImage) { delete globalImage; globalImage = nullptr; }
        globalImage = LoadPngImage(globalConfigPathImage);
        if (!globalImage) shouldHide = true;
        else { finalW = globalImage->GetWidth(); finalH = globalImage->GetHeight(); }
    }

    UpdateTargetRegistryWatch();

    if (shouldHide || finalW <= 0 || finalH <= 0) { ShowWindow(hwnd, SW_HIDE); return; }

    HDC screenDC = GetDC(NULL);
    HDC memDC = CreateCompatibleDC(screenDC);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = finalW; bmi.bmiHeader.biHeight = -finalH;
    bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits;
    HBITMAP hBitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hBitmap) {
        DeleteDC(memDC); ReleaseDC(NULL, screenDC);
        ShowWindow(hwnd, SW_HIDE);
        return;
    }

    HBITMAP hOld = (HBITMAP)SelectObject(memDC, hBitmap);

    {
        Gdiplus::Graphics graphics(memDC);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

        if (globalOverlayType == 1 && globalImage) {
            graphics.DrawImage(globalImage, 0, 0, finalW, finalH);
        }
        else {
            Gdiplus::FontFamily ff(globalFontFamily.c_str());
            Gdiplus::StringFormat sf;
            if (globalTextAlignment == 0) sf.SetAlignment(Gdiplus::StringAlignmentNear);
            else if (globalTextAlignment == 2) sf.SetAlignment(Gdiplus::StringAlignmentFar);
            else sf.SetAlignment(Gdiplus::StringAlignmentCenter);

            Gdiplus::REAL tabs = static_cast<Gdiplus::REAL>(globalFontSize * globalTabWidth * 0.5f);
            sf.SetTabStops(0.0f, 1, &tabs);

            Gdiplus::GraphicsPath path;
            Gdiplus::RectF rect(25.0f, 25.0f, static_cast<Gdiplus::REAL>(finalW - 50), static_cast<Gdiplus::REAL>(finalH - 50));
            path.AddString(globalText.c_str(), -1, &ff, Gdiplus::FontStyleBold, static_cast<Gdiplus::REAL>(globalFontSize), rect, &sf);

            Gdiplus::Color colorMain = globalIsWhiteText ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 0, 0, 0);
            Gdiplus::Color colorBorder = globalIsWhiteText ? Gdiplus::Color(255, 0, 0, 0) : Gdiplus::Color(255, 255, 255, 255);

            Gdiplus::Pen pen(colorBorder, 3.0f);
            pen.SetLineJoin(Gdiplus::LineJoinRound);
            graphics.DrawPath(&pen, &path);
            Gdiplus::SolidBrush brush(colorMain);
            graphics.FillPath(&brush, &path);
        }
    }

    int visualPadding = (globalOverlayType == 1) ? 0 : 25;
    int fx, fy;
    CalculateWindowPosition(finalW, finalH, visualPadding, fx, fy);
    POINT ptD = { fx, fy }, ptS = { 0, 0 }; SIZE sz = { finalW, finalH };

    BYTE windowAlpha = (BYTE)((globalOpacity * 255) / 100);
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, windowAlpha, AC_SRC_ALPHA };

    UpdateLayeredWindow(hwnd, screenDC, &ptD, &sz, memDC, &ptS, 0, &bf, ULW_ALPHA);

    SelectObject(memDC, hOld); DeleteObject(hBitmap); DeleteDC(memDC); ReleaseDC(NULL, screenDC);
    ShowWindow(hwnd, SW_SHOWNA);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg) {
    case WM_TIMER:
        if (wParam == 2) { KillTimer(hwnd, 2); RenderAndApplyOverlay(hwnd); }
        return 0;
    case WM_UPDATE_OVERLAY:
        RenderAndApplyOverlay(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int OverlayMain(HINSTANCE hInstance, int nCmdShow) {
    Gdiplus::GdiplusStartupInput si;
    ULONG_PTR tok;
    Gdiplus::GdiplusStartup(&tok, &si, NULL);

    SetupPortablePaths();

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WLSOverlayClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        L"WLSOverlayClass", L"WLSOverlay", WS_POPUP,
        0, 0, 1, 1, NULL, NULL, hInstance, NULL
    );

    RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_ALT, 'Q');

    hEventTarget = CreateEvent(NULL, FALSE, FALSE, NULL);
    HANDLE hEventSettings = CreateEvent(NULL, FALSE, FALSE, NULL);
    HKEY hKeySettings = NULL;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\KWI-Software\\WLSOverlay", 0, KEY_NOTIFY, &hKeySettings) == ERROR_SUCCESS) {
        RegNotifyChangeKeyValue(hKeySettings, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEventSettings, TRUE);
    }

    SendMessage(hwnd, WM_UPDATE_OVERLAY, 0, 0);

    std::wstring dirWatchPath = globalDirPath + globalOverlayFilesDirPath;
    HANDLE hDirWatch = FindFirstChangeNotificationW(
        dirWatchPath.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME
    );

    MSG msg;
    while (true) {
        HANDLE hEvents[3];
        int evCount = 0;
        if (hDirWatch != INVALID_HANDLE_VALUE && hDirWatch != NULL) hEvents[evCount++] = hDirWatch;
        if (hKeySettings) hEvents[evCount++] = hEventSettings;
        if (hKeyTarget) hEvents[evCount++] = hEventTarget;

        DWORD wait = MsgWaitForMultipleObjects(evCount, hEvents, FALSE, INFINITE, QS_ALLINPUT);

        if (wait >= WAIT_OBJECT_0 && wait < WAIT_OBJECT_0 + evCount) {
            HANDLE triggered = hEvents[wait - WAIT_OBJECT_0];

            if (triggered == hDirWatch && hDirWatch != INVALID_HANDLE_VALUE && hDirWatch != NULL) {
                FindNextChangeNotification(hDirWatch);
            }
            else if (triggered == hEventSettings) RegNotifyChangeKeyValue(hKeySettings, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEventSettings, TRUE);
            else if (triggered == hEventTarget) RegNotifyChangeKeyValue(hKeyTarget, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEventTarget, TRUE);

            SetTimer(hwnd, 2, 150, NULL);
        }
        else if (wait == WAIT_OBJECT_0 + evCount) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) goto end;
                if (msg.message == WM_HOTKEY) PostQuitMessage(0);
                TranslateMessage(&msg); DispatchMessage(&msg);
            }
        }
    }
end:
    if (hKeySettings) RegCloseKey(hKeySettings);
    if (hKeyTarget) RegCloseKey(hKeyTarget);
    if (hDirWatch != INVALID_HANDLE_VALUE && hDirWatch != NULL) FindCloseChangeNotification(hDirWatch);

    Gdiplus::GdiplusShutdown(tok);
    return 0;
}