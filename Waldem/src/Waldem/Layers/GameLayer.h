#pragma once
#include "Waldem/Input/InputManager.h"
#include "Waldem/Layers/Layer.h"
#include "Waldem/SceneManagement/Scene.h"
#include "Waldem/Renderer/Renderer.h"

namespace Waldem
{
    class WALDEM_API GameLayer : public Layer
    {
    public:
        GameLayer() : Layer("Example")
        {
            CurrentInputManager = {};
        }

        void OnUpdate(float deltaTime) override
        {
            CurrentScene->Update(deltaTime);

            CurrentScene->Draw(deltaTime);
        }

        void OnEvent(Event& event) override
        {
            auto eventType = event.GetEventType();

            switch (eventType)
            {
            case EventType::KeyPressed:
            case EventType::KeyReleased:
            case EventType::KeyTyped:
            case EventType::MouseButtonPressed:
            case EventType::MouseButtonReleased:
            case EventType::MouseMoved:
            case EventType::MouseScrolled:
                {
                    event.Handled = true;
                    CurrentInputManager.Broadcast(event);
                }
            }
        }

        void OnDrawUI(float deltaTime) override
        {
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
				    ImGui::MenuItem("(demo menu)", NULL, false, false);
				    if (ImGui::MenuItem("New")) {}
				    if (ImGui::MenuItem("Open", "Ctrl+O")) {}
				    if (ImGui::BeginMenu("Open Recent"))
				    {
				        ImGui::MenuItem("fish_hat.c");
				        ImGui::MenuItem("fish_hat.inl");
				        ImGui::MenuItem("fish_hat.h");
				        if (ImGui::BeginMenu("More.."))
				        {
				            ImGui::MenuItem("Hello");
				            ImGui::MenuItem("Sailor");
				        }
				        ImGui::EndMenu();
				    }
				    if (ImGui::MenuItem("Save", "Ctrl+S")) {}
				    if (ImGui::MenuItem("Save As..")) {}

				    ImGui::Separator();
				    if (ImGui::BeginMenu("Options"))
				    {
				        static bool enabled = true;
				        ImGui::MenuItem("Enabled", "", &enabled);
				        ImGui::BeginChild("child", ImVec2(0, 60), ImGuiChildFlags_Borders);
				        for (int i = 0; i < 10; i++)
				            ImGui::Text("Scrolling Text %d", i);
				        ImGui::EndChild();
				        static float f = 0.5f;
				        static int n = 0;
				        ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
				        ImGui::InputFloat("Input", &f, 0.1f);
				        ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
				        ImGui::EndMenu();
				    }

				    if (ImGui::BeginMenu("Colors"))
				    {
				        float sz = ImGui::GetTextLineHeight();
				        for (int i = 0; i < ImGuiCol_COUNT; i++)
				        {
				            const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
				            ImVec2 p = ImGui::GetCursorScreenPos();
				            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32((ImGuiCol)i));
				            ImGui::Dummy(ImVec2(sz, sz));
				            ImGui::SameLine();
				            ImGui::MenuItem(name);
				        }
				        ImGui::EndMenu();
				    }

				    // Here we demonstrate appending again to the "Options" menu (which we already created above)
				    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
				    // In a real code-base using it would make senses to use this feature from very different code locations.
				    if (ImGui::BeginMenu("Options")) // <-- Append!
				    {
				        static bool b = true;
				        ImGui::Checkbox("SomeOption", &b);
				        ImGui::EndMenu();
				    }

				    if (ImGui::BeginMenu("Disabled", false)) // Disabled
				    {
				        IM_ASSERT(0);
				    }
				    if (ImGui::MenuItem("Checked", NULL, true)) {}
				    ImGui::Separator();
				    if (ImGui::MenuItem("Quit", "Alt+F4")) {}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit"))
				{
					if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
					if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
					ImGui::Separator();
					if (ImGui::MenuItem("Cut", "CTRL+X")) {}
					if (ImGui::MenuItem("Copy", "CTRL+C")) {}
					if (ImGui::MenuItem("Paste", "CTRL+V")) {}
					ImGui::EndMenu();
				}
				ImGui::EndMainMenuBar();
			}
        	
            CurrentScene->DrawUI(deltaTime);
        }

        void OpenScene(Scene* scene, SceneData* sceneData)
        {
		    Renderer::Begin();
            scene->Initialize(sceneData, &CurrentInputManager);
		    Renderer::End();
            
            CurrentScene = scene;
        }

        Scene* GetCurrentScene() { return CurrentScene; }
        
        void Begin() override {}
        void End() override {}
        void OnAttach() override {}
        void OnDetach() override{}

    private:
        Scene* CurrentScene = nullptr;
        InputManager CurrentInputManager;
    };
}