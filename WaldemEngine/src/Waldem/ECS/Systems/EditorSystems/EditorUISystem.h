#pragma once

#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl2.h"
#include "Waldem/ECS/Systems/System.h"
#include "Widgets/HierarchyWidget.h"
#include "Widgets/MainWidgetContainer.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/EntityDetailsWidgetContainer.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/BloomComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/ColliderComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/LightComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/NameWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/OceanComponentWidget.h"
#include "Waldem/ECS/Systems/EditorSystems/Widgets/ComponentWidgets/TransformComponentWidget.h"
#include "Widgets/ContentBrowserWidget.h"

namespace Waldem
{
    class WALDEM_API EditorUISystem : public ISystem
    {
        MainWidgetContainer* MainWidget = nullptr;
        Action OnOpenScene;
        Action OnSaveScene;
        Action OnSaveSceneAs;
        
    public:
        EditorUISystem(ECSManager* eCSManager, const Action& onOpenScene, const Action& onSaveScene, const Action& onSaveSceneAs)
            : ISystem(eCSManager), OnOpenScene(onOpenScene), OnSaveScene(onSaveScene), OnSaveSceneAs(onSaveSceneAs)
        {
           MainWidget = new MainWidgetContainer(eCSManager,
           {
               new HierarchyWidget(eCSManager),
               new EntityDetailsWidgetContainer(eCSManager,
               {
                   //put all component widgets here
                   new NameWidget(eCSManager),
                   new TransformComponentWidget(eCSManager),
                   new ColliderComponentWidget(eCSManager),
                   new LightComponentWidget(eCSManager),
                   new BloomComponentWidget(eCSManager),
                   new OceanComponentWidget(eCSManager),
               }),
               new ContentBrowserWidget(eCSManager),
           });
        }

        void InitializeUI()
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
            // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable docking
            // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable multi-viewport

            SDL_Window* window = static_cast<SDL_Window*>(CWindow::GetNativeWindow());
            ImGui_ImplSDL2_InitForD3D(window);

            Renderer::InitializeUI();
        }

        void DeinitializeUI()
        {
            ImGui_ImplSDL2_Shutdown();
            ImGui_ImplDX12_Shutdown();
            ImGui::DestroyContext();
        }
        
        void Initialize(SceneData* sceneData, InputManager* inputManager, ResourceManager* resourceManager) override
        {
            InitializeUI();
            MainWidget->Initialize(sceneData, inputManager, resourceManager);
        }

        void Deinitialize() override
        {
            DeinitializeUI();
            MainWidget->Deinitialize();
        }

        void Update(float deltaTime) override
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New scene")) {}
                    if (ImGui::MenuItem("Open scene", "Ctrl+O"))
                    {
                        OnOpenScene();
                    }
                    if (ImGui::MenuItem("Save scene"))
                    {
                        OnSaveScene();
                    }
                    if (ImGui::MenuItem("Save scene as..."))
                    {
                        OnSaveSceneAs();
                    }
            
                    ImGui::Separator();
					       
                    if (ImGui::BeginMenu("Options"))
                    {
                        ImGui::EndMenu();
                    }
					       
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            MainWidget->Update(deltaTime);
        }
    };
}
