// ========================================================================================================================
// @file ImGuiAddon.h
// @brief ImGuiの関数を少し変えたものをおいてる
// 
// ImGuiをもっと使いやすくするというか C++20 にしたら日本語使いにくくなったから作っただけだけど、
// もしかしたらImGuiがもっと便利になるような関数を作っていくかも
// ========================================================================================================================
#pragma once
#include <ImGui/imgui.h>

// char を char8_t に変換するマクロ
#define U8(x) reinterpret_cast<const char*>(u8##x)

// ImGuiの元々の関数を const char8_t* に変換したものを書いてる
namespace ImGui
{
    inline void Text(const char8_t* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(reinterpret_cast<const char*>(fmt), args);
        va_end(args);
    }

    inline bool Button(const char8_t* label, const ImVec2& size = ImVec2(0, 0))
    {
        return ImGui::Button(reinterpret_cast<const char*>(label), size);
    }

    inline bool InputText(const char8_t* label, char* buf, size_t buf_size,
        ImGuiInputTextFlags flags = 0,
        ImGuiInputTextCallback callback = nullptr,
        void* user_data = nullptr) 
    {
        return ImGui::InputText(reinterpret_cast<const char*>(label),
            buf, buf_size, flags, callback, user_data);
    }

    inline bool MenuItem(const char8_t* label, const char* shortcut = nullptr,
        bool selected = false, bool enabled = true) 
    {
        return ImGui::MenuItem(reinterpret_cast<const char*>(label),
            shortcut, selected, enabled);
    }
}