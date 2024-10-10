#include "wdpch.h"
#include "ImGuiLayer.h"
#include "imgui.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "Waldem/Application.h"

#include "GLFW/glfw3.h"

Waldem::ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
{
}

Waldem::ImGuiLayer::~ImGuiLayer()
{
}

void Waldem::ImGuiLayer::Begin()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
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
    
    GLFWwindow* window = static_cast<GLFWwindow*>(Application::Instance->GetWindow().GetNativeWindow());

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void Waldem::ImGuiLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
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
 