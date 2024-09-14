#include <wdpch.h>
#include "DefaultScene.h"

#include "glm/gtc/type_ptr.inl"
#include "Waldem/Application.h"
#include "Waldem/Input.h"
#include "Waldem/KeyCodes.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/Geometry/Import/FBXImporter.h"
#include "Waldem/Renderer/Renderer.h"
#include "Waldem/World/Camera.h"

namespace Sandbox
{
	void DefaultScene::Initialize()
	{
		RasterPipeline = new Waldem::Pipeline("RasterPipeline", "Default", true, true, true);
		RasterPipeline->AddShaderParam(Waldem::ShaderParamType::MAT4, "world");
		RasterPipeline->AddShaderParam(Waldem::ShaderParamType::MAT4, "viewProjection");
		
		Waldem::FBXImporter importer;
		std::string path = "Content/Models/Mage.fbx";
		TestModel = importer.Import(path, true);
		TestModel->Initialize(RasterPipeline);
		TestModelTransform.Reset();
		TestModelTransform.SetPosition(0, 0, 4);
		
		float screenWidth = Waldem::Application::Instance->GetWindow().GetWidth();
		float screenHeight = Waldem::Application::Instance->GetWindow().GetHeight();
		
		MainCamera.reset(new Waldem::Camera(70.0f , screenWidth / screenHeight, .01f, 10000, { 0, 0, 0 }, 100.0f, 1.0f));
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

	void DefaultScene::Draw()
	{
		auto viewProjectionMatrix = MainCamera.get()->GetViewProjectionMatrix();
		RasterPipeline->SetShaderParam("viewProjection", &viewProjectionMatrix);

		auto trisWorldMatrix = TestModelTransform.GetMatrix();
		RasterPipeline->SetShaderParam("world", &trisWorldMatrix);

		Waldem::Renderer::DrawModel(RasterPipeline, TestModel);
	}
}
