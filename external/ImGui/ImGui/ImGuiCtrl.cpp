//-----< include >------------------------
#include "ImGuiCtrl.h"

//=======================================
// 
//      ImGuiCtrl
// 
//=======================================
#if USE_IMGUI

namespace ImGuiCtrl
{
    ImFont* fontMeiryo = nullptr;
    ImFont* fontKyoukasho = nullptr;

    void Initialize(HWND windowHandle, ID3D11Device* device, ID3D11DeviceContext* deviceContext)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiStyle& style = ImGui::GetStyle();
        ImGuiIO& io = ImGui::GetIO();

#ifdef _DEBUG
        io.IniFilename = "./Config/imgui.ini";
#else
        io.IniFilename = nullptr;
#endif

        /// フォント変更
        fontMeiryo = io.Fonts->AddFontFromFileTTF(
            "./EngineResource/Font/Noto_Sans_JP/static/NotoSansJP-Regular.ttf",
            18.0f, nullptr, glyphRangesJapanese
        );
        io.FontDefault = fontMeiryo;

        ImGui_ImplWin32_Init(windowHandle);
        ImGui_ImplDX11_Init(device, deviceContext);
        ImGui::StyleColorsDark();
        /// ImGuiの背景色の変更
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
        /// ImGuiのチェックボックス色変更
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.16f, 0.16f, 1));
        /// ImGuiのボタン色の変更
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive | ImGuiCol_TitleBg, ImVec4(0, 0, 0, 1.0f));
        /// ImGuiチェックマーク色の変更
        style.Colors[ImGuiCol_CheckMark] = ImVec4(1, 1, 1, 1);
        /// ボタンのホバー時の色変更
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.27f, 0.27f, 0.27f, 1);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(1, 1, 1, 1);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.23f, 0.23f, 0.23f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.23f, 0.23f, 0.23f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1, 1, 1, 1);

        // ドッキングできるようにしてる
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    void ClearFrame()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void Display()
    {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Uninitialize()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}
#endif // USE_IMGUI