#include "wdpch.h"
#include "ImGuiLayer.h"
#include "imgui.h"
#include "Waldem/Application.h"

Waldem::ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
{
}

Waldem::ImGuiLayer::~ImGuiLayer()
{
}

void Waldem::ImGuiLayer::Begin()
{
    ImGui::NewFrame();
    static bool show = true;
    ImGui::ShowDemoWindow(&show);
}

void Waldem::ImGuiLayer::End()
{
    ImGuiIO& io = ImGui::GetIO();
    Window& window = Application::Instance->GetWindow();
    io.DisplaySize = ImVec2(window.GetWidth(), window.GetHeight());

    // Rendering
    ImGui::Render();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void Waldem::ImGuiLayer::OnAttach()
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
}

void Waldem::ImGuiLayer::OnDetach()
{
    ImGui::DestroyContext();
}

void Waldem::ImGuiLayer::OnEvent(Event& event)
{
    if (m_BlockEvents)
    {
        ImGuiIO& io = ImGui::GetIO();
        event.Handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
        event.Handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
    }
}

void Waldem::ImGuiLayer::OnUIRender()
{
    Layer::OnUIRender();
}
 