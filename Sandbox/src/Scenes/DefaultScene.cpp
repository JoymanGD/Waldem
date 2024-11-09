#include <wdpch.h>
#include "DefaultScene.h"
#include "Waldem/Application.h"
#include "Waldem/Input.h"
#include "Waldem/KeyCodes.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/Import/ModelImporter.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/Renderer/Resource.h"
#include "Waldem/World/Camera.h"

namespace Sandbox
{
	void DefaultScene::Initialize(Waldem::SceneData* sceneData)
	{
		CreateLights(sceneData->Renderer);

		Waldem::ModelImporter importer;
		Waldem::String path = "Content/Models/Sponza/Sponza.gltf";
		TestModel = importer.Import(path, true);
		TestModelTransform.Reset();
		TestModelTransform.SetPosition(0, 0, 0);
		
		TestDirLightTransform.Reset();
		TestDirLightTransform.SetPosition(0, 0, 0);
		
		float screenWidth = sceneData->Window->GetWidth();
		float screenHeight = sceneData->Window->GetHeight();
		
		MainCamera = new Waldem::Camera(70.0f , screenWidth / screenHeight, .01f, 10000, { 0, 0, 0 }, 100.0f, 30.0f);
		
		std::vector<Waldem::Resource> resources;

		Waldem::Matrix4 viewProjection = glm::perspective(70 * glm::pi<float>() / 180.0f, 1.0f, .01f, 10000.0f);
		// Waldem::Matrix4 viewProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, 0.01f, 10000.0f);
		
		Waldem::Matrix4 cbv[2] = { TestDirLightTransform.GetMatrix(), viewProjection };
		resources.push_back(Waldem::Resource("MyConstantBuffer", Waldem::ResourceType::RTYPE_ConstantBuffer, cbv, sizeof(Waldem::Matrix4), 2 * sizeof(Waldem::Matrix4), 0));
		TestShadowmapShader = sceneData->Renderer->LoadShader("Shadowmap", resources, Lights[0].Shadowmap);

		resources.clear();
		Waldem::Matrix4 matrices[2]  = { MainCamera->GetViewProjectionMatrix(), TestModelTransform.GetMatrix() };
		resources.push_back(Waldem::Resource("MyConstantBuffer", Waldem::ResourceType::RTYPE_ConstantBuffer, matrices, sizeof(Waldem::Matrix4), 2 * sizeof(Waldem::Matrix4), 0));
		resources.push_back(Waldem::Resource("LightsBuffer", Waldem::ResourceType::RTYPE_Buffer, GetLightsData().data(), sizeof(Waldem::LightData), (uint32_t)Lights.size() * sizeof(Waldem::LightData), 0));
		resources.push_back(Waldem::Resource("Shadowmap", Lights[0].Shadowmap, 1));
		resources.push_back(Waldem::Resource("TestTextures", TestModel->GetTextures(), 2));
		
		TestPixelShader = sceneData->Renderer->LoadShader("Default", resources);

		ConstantBuffer = { MainCamera->GetViewProjectionMatrix(), TestModelTransform.GetMatrix(), (uint32_t)Lights.size() };
	}

	void DefaultScene::CreateLights(Waldem::Renderer* Renderer)
	{
		Waldem::Light dirLight;
		dirLight.Data.Color = { .03f, .2f, .3f };
		dirLight.Data.Intensity = 1.0f;
		dirLight.Data.Position = { 0, 0, 0 };
		dirLight.Data.Type = Waldem::LightType::Directional;
		dirLight.Data.Direction = normalize(Waldem::Vector3(0, -1, 0));
		dirLight.Data.Range = 100.0f;
		dirLight.Shadowmap = Renderer->CreateRenderTarget("ShadowmapRT", 1024, 1024, Waldem::TextureFormat::TEXTURE_FORMAT_R32_FLOAT);

		Lights.push_back(dirLight);
	}

	std::vector<Waldem::LightData> DefaultScene::GetLightsData()
	{
		std::vector<Waldem::LightData> lightsData;

		for (auto& light : Lights)
		{
			lightsData.push_back(light.Data);
		}
		
		return lightsData;
	}

	void DefaultScene::Update(float deltaTime)
	{
		if(Waldem::Input::IsKeyPressed(W))
		{
			float movementSpeed = deltaTime * MainCamera->MovementSpeed;
			MainCamera->Move({ 0, 0, 0.1f * movementSpeed });
		}
		if(Waldem::Input::IsKeyPressed(S))
		{
			float movementSpeed = deltaTime * MainCamera->MovementSpeed;
			MainCamera->Move({ 0, 0, -0.1f * movementSpeed });
		}
		if(Waldem::Input::IsKeyPressed(A))
		{
			float movementSpeed = deltaTime * MainCamera->MovementSpeed;
			MainCamera->Move({ -0.1f * movementSpeed, 0, 0 });
		}
		if(Waldem::Input::IsKeyPressed(D))
		{
			float movementSpeed = deltaTime * MainCamera->MovementSpeed;
			MainCamera->Move({ 0.1f * movementSpeed, 0, 0 });
		}
		if(Waldem::Input::IsKeyPressed(E))
		{
			float movementSpeed = deltaTime * MainCamera->MovementSpeed;
			MainCamera->Move({ 0, 0.1f * movementSpeed, 0 });
		}
		if(Waldem::Input::IsKeyPressed(Q))
		{
			float movementSpeed = deltaTime * MainCamera->MovementSpeed;
			MainCamera->Move({ 0, -0.1f * movementSpeed, 0 });
		}
		static float lastMouseX = 0;
		static float lastMouseY = 0;

		auto [mouseX, mouseY] = Waldem::Input::GetMousePos();

		if (Waldem::Input::IsMouseButtonPressed(WD_MOUSE_BUTTON_LEFT))
		{
			float deltaX = (mouseX - lastMouseX) * deltaTime;
			float deltaY = (mouseY - lastMouseY) * deltaTime;

			MainCamera->Rotate(deltaX, deltaY, 0);
		}

		lastMouseX = mouseX;
		lastMouseY = mouseY;
	}

	void DefaultScene::Draw(Waldem::SceneData* sceneData)
	{
		sceneData->Renderer->Draw(TestModel, TestShadowmapShader);

		auto viewProjectionMatrix = MainCamera->GetViewProjectionMatrix();
		auto testModelWorldMatrix = TestModelTransform.GetMatrix();
		Waldem::Matrix4 matrices[2] = { viewProjectionMatrix, testModelWorldMatrix };
		TestPixelShader->UpdateResourceData("MyConstantBuffer", matrices);

		sceneData->Renderer->Draw(TestModel, TestPixelShader);
	}
}
