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
#include "utils.h"

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    g_StatusHandle = RegisterServiceCtrlHandler(g_szServiceName, ServiceCtrlHandler);
    if (g_StatusHandle == NULL) return;

    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL) {
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        return;
    }

    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    wchar_t myPath[MAX_PATH];
    GetModuleFileNameW(NULL, myPath, MAX_PATH);
    std::wstring appPath(myPath);

    PROCESS_INFORMATION procInfo = { 0 };
    bool bServiceRunning = true;

    while (bServiceRunning) {
        if (procInfo.hProcess == NULL) {
            if (StartProcess(appPath, procInfo)) {
                if (procInfo.hThread) CloseHandle(procInfo.hThread);
            }
            else {
                Sleep(2000);
            }
        }

        HANDLE waitHandles[2];
        waitHandles[0] = g_ServiceStopEvent;
        DWORD numHandles = 1;

        if (procInfo.hProcess != NULL) {
            waitHandles[1] = procInfo.hProcess;
            numHandles = 2;
        }

        DWORD waitResult = WaitForMultipleObjects(numHandles, waitHandles, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0) {
            bServiceRunning = false;
        }
        else if (waitResult == (WAIT_OBJECT_0 + 1)) {
            if (procInfo.hProcess != NULL) {
                CloseHandle(procInfo.hProcess);
                procInfo.hProcess = NULL;
            }
            Sleep(1000);
        }
        else {
            bServiceRunning = false;
        }
    }

    if (procInfo.hProcess != NULL) {
        TerminateProcess(procInfo.hProcess, 0);
        CloseHandle(procInfo.hProcess);
    }

    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

void WINAPI ServiceCtrlHandler(DWORD CtrlCode) {
    if (CtrlCode == SERVICE_CONTROL_STOP) {
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        SetEvent(g_ServiceStopEvent);
    }
}