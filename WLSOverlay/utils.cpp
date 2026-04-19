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

#include "utils.h"
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <vector>

#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Advapi32.lib")

static DWORD GetProcessId(const std::wstring& processName, DWORD sessionId)
{
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnapshot, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0)
            {
                DWORD procSessionId = 0;
                if (ProcessIdToSessionId(pe.th32ProcessID, &procSessionId))
                {
                    if (procSessionId == sessionId)
                    {
                        pid = pe.th32ProcessID;
                        break;
                    }
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return pid;
}


bool StartProcess(const std::wstring& applicationName, PROCESS_INFORMATION& procInfo)
{
    DWORD winlogonPid = 0;
    HANDLE hUserTokenDup = NULL;
    HANDLE hPToken = NULL;
    HANDLE hProcess = NULL;

    ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));

    DWORD activeSession = WTSGetActiveConsoleSessionId();
    if (activeSession == 0xFFFFFFFF)
    {
        return false;
    }

    winlogonPid = GetProcessId(L"LogonUI.exe", activeSession);
    if (winlogonPid == 0)
    {
        winlogonPid = GetProcessId(L"winlogon.exe", activeSession);
    }

    if (winlogonPid == 0) return false;

    hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, winlogonPid);
    if (!hProcess) return false;

    if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hPToken))
    {
        CloseHandle(hProcess);
        return false;
    }

    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);

    if (!DuplicateTokenEx(hPToken, MAXIMUM_ALLOWED, &sa, SecurityIdentification, TokenPrimary, &hUserTokenDup))
    {
        CloseHandle(hProcess);
        CloseHandle(hPToken);
        return false;
    }

    STARTUPINFOW si = { 0 };
    si.cb = sizeof(STARTUPINFOW);

    wchar_t desktop[] = L"WinSta0\\Winlogon";
    si.lpDesktop = desktop;

    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS;

    std::vector<wchar_t> cmdLine(applicationName.begin(), applicationName.end());
    cmdLine.push_back(L'\0');

    bool result = CreateProcessAsUserW(
        hUserTokenDup, NULL, cmdLine.data(), &sa, &sa, FALSE,
        dwCreationFlags, NULL, NULL, &si, &procInfo
    );

    if (hProcess != NULL) {
        CloseHandle(hProcess);
    }
    if (hPToken != NULL) {
        CloseHandle(hPToken);
    }
    if (hUserTokenDup != NULL) {
        CloseHandle(hUserTokenDup);
    }

    return result;
}