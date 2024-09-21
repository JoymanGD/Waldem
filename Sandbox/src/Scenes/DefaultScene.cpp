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
	void DefaultScene::Initialize()
	{
		RasterPipeline = new Waldem::Pipeline("RasterPipeline", "Default", true, true, true);
		
		Waldem::ModelImporter importer;
		std::string path = "Content/Models/Sponza/Sponza.gltf";
		TestModel = importer.Import(path, true);
		TestModel->Initialize(RasterPipeline);
		TestModelTransform.Reset();
		TestModelTransform.SetPosition(0, 0, 4);
		
		float screenWidth = Waldem::Application::Instance->GetWindow().GetWidth();
		float screenHeight = Waldem::Application::Instance->GetWindow().GetHeight();
		
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
		if(Waldem::Input::IsKeyPressed(WD_KEY_W))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0, 0, 0.1f * movementSpeed });
		}
		if(Waldem::Input::IsKeyPressed(WD_KEY_S))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0, 0, -0.1f * movementSpeed });
		}
		if(Waldem::Input::IsKeyPressed(WD_KEY_A))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ -0.1f * movementSpeed, 0, 0 });
		}
		if(Waldem::Input::IsKeyPressed(WD_KEY_D))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0.1f * movementSpeed, 0, 0 });
		}
		if(Waldem::Input::IsKeyPressed(WD_KEY_E))
		{
			float movementSpeed = deltaTime * MainCamera.get()->MovementSpeed;
			MainCamera.get()->Move({ 0, 0.1f * movementSpeed, 0 });
		}
		if(Waldem::Input::IsKeyPressed(WD_KEY_Q))
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

	void DefaultScene::Draw(Waldem::Renderer* renderer)
	{
		auto viewProjectionMatrix = MainCamera.get()->GetViewProjectionMatrix();
		RasterPipeline->SetShaderParam(Waldem::ShaderParamType::MAT4, "viewProjection", &viewProjectionMatrix);

		auto testModelWorldMatrix = TestModelTransform.GetMatrix();
		
		RasterPipeline->SetShaderParam(Waldem::ShaderParamType::MAT4, "world", &testModelWorldMatrix);
		uint32_t numLights = Lights.size();
		RasterPipeline->SetShaderParam(Waldem::ShaderParamType::UINT, "NumLights", &numLights);
		RasterPipeline->SetShaderBufferParam("Lights", Lights.data(), Lights.size() * sizeof(Waldem::Light), 0);

		renderer->DrawModel(RasterPipeline, TestModel);
	}
}
