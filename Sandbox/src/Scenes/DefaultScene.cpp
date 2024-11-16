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
		
		float screenWidth = sceneData->Window->GetWidth();
		float screenHeight = sceneData->Window->GetHeight();
		
		MainCamera = new Waldem::Camera(70.0f , screenWidth / screenHeight, .01f, 10000, { 0, 0, 0 }, 100.0f, 30.0f);
		
		std::vector<Waldem::Resource> resources;

		// Waldem::Matrix4 projection = glm::perspective(70 * glm::pi<float>() / 180.0f, 1.0f, .01f, 10000.0f);
		// Waldem::Matrix4 ZFlipMatrix = glm::scale(Waldem::Matrix4(1.0f), Waldem::Vector3(1.0f, 1.0f, -1.0f));
		Waldem::Matrix4 cbv[3] = { TestModelTransform.GetMatrix(), Lights[0].Data.View, Lights[0].Data.Projection };
		resources.push_back(Waldem::Resource("MyConstantBuffer", Waldem::ResourceType::RTYPE_ConstantBuffer, cbv, sizeof(Waldem::Matrix4), 3 * sizeof(Waldem::Matrix4), 0));
		TestShadowmapShader = sceneData->Renderer->LoadPixelShader("Shadowmap", resources, Lights[0].Shadowmap);

		resources.clear();
		Waldem::Matrix4 matrices[3]  = { TestModelTransform.GetMatrix(), MainCamera->GetViewMatrix(), MainCamera->GetProjectionMatrix() };
		resources.push_back(Waldem::Resource("MyConstantBuffer", Waldem::ResourceType::RTYPE_ConstantBuffer, matrices, sizeof(Waldem::Matrix4), 3 * sizeof(Waldem::Matrix4), 0));
		auto allLightsData = GetLightsData();
		resources.push_back(Waldem::Resource("LightsBuffer", Waldem::ResourceType::RTYPE_Buffer, allLightsData.data(), sizeof(Waldem::LightData), (uint32_t)Lights.size() * sizeof(Waldem::LightData), 0));
		resources.push_back(Waldem::Resource("Shadowmap", Lights[0].Shadowmap, 1));
		resources.push_back(Waldem::Resource("TestTextures", TestModel->GetTextures(), 2));
		resources.push_back(Waldem::Resource("ComparisonSampler", { Waldem::Sampler( Waldem::COMPARISON_MIN_MAG_MIP_LINEAR, Waldem::WRAP, Waldem::WRAP, Waldem::WRAP, Waldem::LESS_EQUAL) }, 1));
		TestPixelShader = sceneData->Renderer->LoadPixelShader("Default", resources);

		TestRenderTarget = sceneData->Renderer->CreateRenderTarget("TestRenderTarget", 1024, 1024, Waldem::TextureFormat::TEXTURE_FORMAT_R32_FLOAT);
		resources.clear();
		resources.push_back(Waldem::Resource("TestResource", TestRenderTarget, 0));
		TestComputeShader = sceneData->Renderer->LoadComputeShader("PostProcess", resources);
	}

	void DefaultScene::CreateLights(Waldem::Renderer* Renderer)
	{
		TestDirLightTransform.Reset();
		TestDirLightTransform.SetPosition(0, 0, 0);
		TestDirLightTransform.SetEuler(90, 0, 0);
		
		Waldem::Light dirLight;
		dirLight.Data.Color = { 1, 1, 1 };
		dirLight.Data.Intensity = 1.0f;
		dirLight.Data.Type = Waldem::LightType::Directional;
		dirLight.Data.Range = 100.0f;
		dirLight.Data.World = TestDirLightTransform;
		dirLight.Data.View = glm::scale(Waldem::Matrix4(1.0f), Waldem::Vector3(1.0f, 1.0f, -1.0f)) * TestDirLightTransform.Inverse();
		dirLight.Data.Projection = glm::orthoZO(-20.0f, 20.0f, -20.0f, 20.0f, -200.f, 200.0f);
		// dirLight.Data.Projection = glm::perspectiveZO(70 * glm::pi<float>() / 180.0f, 1.0f, .1f, 100.0f);
		dirLight.Shadowmap = Renderer->CreateRenderTarget("ShadowmapRT", 2048, 2048, Waldem::TextureFormat::TEXTURE_FORMAT_D32_FLOAT);

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
		
		if (Waldem::Input::IsMouseButtonPressed(WD_MOUSE_BUTTON_RIGHT))
		{
			float deltaX = (mouseX - lastMouseX) * deltaTime;
			float deltaY = (mouseY - lastMouseY) * deltaTime;

			TestDirLightTransform.Rotate(0, deltaY, deltaX);
		}

		lastMouseX = mouseX;
		lastMouseY = mouseY;

		Lights[0].Data.World = TestDirLightTransform.GetMatrix();
		Lights[0].Data.View = glm::scale(Waldem::Matrix4(1.0f), Waldem::Vector3(1.0f, 1.0f, -1.0f)) * TestDirLightTransform.Inverse();
	}

	void DefaultScene::Draw(Waldem::SceneData* sceneData)
	{
		sceneData->Renderer->Draw(TestModel, TestShadowmapShader);

		auto viewMatrix = MainCamera->GetViewMatrix();
		auto ProjectionMatrix = MainCamera->GetProjectionMatrix();
		auto testModelWorldMatrix = TestModelTransform.GetMatrix();
		Waldem::Matrix4 matrices[3] = { testModelWorldMatrix, viewMatrix, ProjectionMatrix };
		TestPixelShader->UpdateResourceData("MyConstantBuffer", matrices);
		
		auto allLightsData = GetLightsData();
		TestPixelShader->UpdateResourceData("LightsBuffer", allLightsData.data());
		
		Waldem::Matrix4 cbv[3] = { TestModelTransform.GetMatrix(), Lights[0].Data.View, Lights[0].Data.Projection };
		TestShadowmapShader->UpdateResourceData("MyConstantBuffer", cbv);

		sceneData->Renderer->Draw(TestModel, TestPixelShader);
		
		Waldem::Point3 numThreads = sceneData->Renderer->GetNumThreadsPerGroup(TestComputeShader);
		sceneData->Renderer->Compute(TestComputeShader, Waldem::Point3((TestRenderTarget->GetWidth() + numThreads.x - 1) / numThreads.x, (TestRenderTarget->GetWidth() + numThreads.y - 1) / numThreads.y, 1));
	}
}
