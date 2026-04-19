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

#pragma once
#include <windows.h>

// Globale Konstante für den Dienstnamen
extern const wchar_t g_szServiceName[];

// Prototyp für die grafische Anwendung (aus app.cpp)
int OverlayMain(HINSTANCE hInstance, int nCmdShow);

// Prototypen für den Dienst (aus service.cpp)
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
void WINAPI ServiceCtrlHandler(DWORD);