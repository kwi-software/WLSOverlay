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
#include <tchar.h>

const wchar_t g_szServiceName[] = L"WLSOverlayService";

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR lpCmdLine,
    _In_ int nCmdShow) {

    UNREFERENCED_PARAMETER(hPrevInstance);

    if (_tcsstr(lpCmdLine, _T("/s")) != NULL) {
        SERVICE_TABLE_ENTRY ServiceTable[] = {
            {(TCHAR*)g_szServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
            {NULL, NULL}
        };

        if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
            return GetLastError();
        }
    }
    else {
        return OverlayMain(hInstance, nCmdShow);
    }
    return 0;
}