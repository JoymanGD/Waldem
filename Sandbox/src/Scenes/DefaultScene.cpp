#include <wdpch.h>
#include "DefaultScene.h"

#include "glm/gtc/type_ptr.inl"
#include "Waldem/Application.h"
#include "Waldem/Input.h"
#include "Waldem/KeyCodes.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/World/Camera.h"

namespace Sandbox
{
	void DefaultScene::Initialize(Waldem::SceneData* sceneData)
	{
		TestPixelShader = sceneData->Renderer->LoadShader("Default");
		
		Waldem::ModelImporter importer;
		std::string path = "Content/Models/Sponza/Sponza.gltf";
		TestModel = importer.Import(path, true);
		TestModelTransform.Reset();
		TestModelTransform.SetPosition(0, 0, 4);
		
		float screenWidth = sceneData->Window->GetWidth();
		float screenHeight = sceneData->Window->GetHeight();
		
		MainCamera.reset(new Waldem::Camera(70.0f , screenWidth / screenHeight, .01f, 10000, { 0, 0, 0 }, 100.0f, 1.0f));

		CreateLights();
	}

	void DefaultScene::CreateLights()
	{
		Waldem::Light dirLight;
		dirLight.Color = { .03f, .2f, .3f };
		dirLight.Intensity = 1.0f;
		dirLight.Position = { 0, 0, 0 };
		dirLight.Type = Waldem::LightType::Directional;
		dirLight.Direciton = { 0.33f, -1, 0.33f };
		dirLight.Range = 100.0f;

		Lights.push_back(dirLight);
	}

	void DefaultScene::Update(float deltaTime)
	{
		if(Waldem::Input::IsKeyPressed(W))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0, 0, 0.1f * movementSpeed });
		}
		if(Waldem::Input::IsKeyPressed(S))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0, 0, -0.1f * movementSpeed });
		}
		if(Waldem::Input::IsKeyPressed(A))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ -0.1f * movementSpeed, 0, 0 });
		}
		if(Waldem::Input::IsKeyPressed(D))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0.1f * movementSpeed, 0, 0 });
		}
		if(Waldem::Input::IsKeyPressed(E))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0, 0.1f * movementSpeed, 0 });
		}
		if(Waldem::Input::IsKeyPressed(Q))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0, -0.1f * movementSpeed, 0 });
		}
		static float lastMouseX = 0;
		static float lastMouseY = 0;

		auto [mouseX, mouseY] = Waldem::Input::GetMousePos();

		if (Waldem::Input::IsMouseButtonPressed(WD_MOUSE_BUTTON_RIGHT))
		{
			float deltaX = (mouseX - lastMouseX) * deltaTime;
			float deltaY = (mouseY - lastMouseY) * deltaTime;

			MainCamera.get()->Rotate(deltaX, deltaY, 0);
		}

		lastMouseX = mouseX;
		lastMouseY = mouseY;
	}

	void DefaultScene::Draw(Waldem::SceneData* sceneData)
	{
		auto viewProjectionMatrix = MainCamera.get()->GetViewProjectionMatrix();

		auto testModelWorldMatrix = TestModelTransform.GetMatrix();
		
		uint32_t numLights = Lights.size();

		//pass params to shader

		sceneData->Renderer->Draw(TestModel, TestPixelShader);
	}
}
