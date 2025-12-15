#pragma once

#define NOMINMAX
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>

#define FATAL_ERROR(error) MessageBoxW(nullptr, L##error L".", __FUNCTIONW__, MB_ICONERROR); std::abort()

LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
