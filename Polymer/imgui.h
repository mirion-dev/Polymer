#pragma once

#define NOMINMAX
#include <Windows.h>

#define FATAL_ERROR(error) MessageBoxW(nullptr, L##error L".", __FUNCTIONW__, MB_ICONERROR); std::abort()

LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
