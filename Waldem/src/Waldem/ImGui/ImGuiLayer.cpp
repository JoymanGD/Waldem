#include "wdpch.h"
#include "ImGuiLayer.h"
#include "imgui.h"
#include "Platform/OpenGL/ImGuiOpenGLRenderer.h"
#include "GLFW/glfw3.h"

Waldem::ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
{
}

Waldem::ImGuiLayer::~ImGuiLayer()
{
}

void Waldem::ImGuiLayer::OnAttach()
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    // io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    // io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

}

void Waldem::ImGuiLayer::OnDetach()
{
}

void Waldem::ImGuiLayer::OnUpdate()
{
}

void Waldem::ImGuiLayer::OnEvent(Event& event)
{
}
 