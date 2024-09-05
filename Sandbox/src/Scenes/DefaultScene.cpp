#include <wdpch.h>
#include "DefaultScene.h"

#include "glm/gtc/type_ptr.inl"
#include "Waldem/Application.h"
#include "Waldem/Input.h"
#include "Waldem/KeyCodes.h"
#include "Waldem/MouseButtonCodes.h"
#include "Waldem/Renderer/Renderer.h"

namespace Sandbox
{
	void DefaultScene::Initialize()
	{
		float screenWidth = Waldem::Application::Instance->GetWindow().GetWidth();
		float screenHeight = Waldem::Application::Instance->GetWindow().GetHeight();
		
		MainCamera.reset(new Waldem::Camera(70.0f , screenWidth / screenHeight, .01f, 100, { 0, 0, 0 }, 100.0f, 1.0f));
		
		auto defaultPixelShader = new Waldem::PixelShader("Default");

		Waldem::BufferLayout bufferLayout = {
			{ Waldem::ShaderDataType::Float3, "Position", false },
			{ Waldem::ShaderDataType::Float4, "Color", false },
		};
		
		//tris vertex buffer
		float vertices[3*7] = {
			-.5f, -.5f, .0f, 1.0f, 0.0f, 1.0f, 1.0f,
			.5f, -.5f, .0f, 0.0f, 1.0f, 1.0f, 1.0f,
			0.f, 0.5f, .0f, 1.0f, 1.0f, 0.0f, 1.0f,
		};
		
		uint32_t indices[3] = { 0, 1, 2 };
		
		TrisMesh.reset(new Waldem::Mesh(vertices, sizeof(vertices), indices, sizeof(indices)/sizeof(uint32_t), defaultPixelShader, bufferLayout));
		TrisMesh->WorldTransform.SetPosition({ 0, 0, 10 });

		//square vertex buffer
		float squareVertices[4*7] = {
			-.5f, -.5f, .0f, 1.0f, 0.0f, 1.0f, 1.0f,
			.5f, -.5f, .0f, 0.0f, 1.0f, 1.0f, 1.0f,
			.5f, .5f, .0f, 1.0f, 1.0f, 0.0f, 1.0f,
			-.5f, .5f, .0f, 1.0f, 1.0f, 1.0f, 1.0f,
		};

		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };

		QuadMesh.reset(new Waldem::Mesh(squareVertices, sizeof(squareVertices), squareIndices, sizeof(squareIndices)/sizeof(uint32_t), defaultPixelShader, bufferLayout));
		QuadMesh->WorldTransform.SetPosition({ 0, 10, 10 });
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

		auto trisWorldMatrix = TrisMesh.get()->WorldTransform.GetMatrix();
		TrisMesh->SetShaderParam(Waldem::ShaderParamType::MAT4, "world", &trisWorldMatrix);
		TrisMesh->SetShaderParam(Waldem::ShaderParamType::MAT4, "viewProjection", &viewProjectionMatrix);

		auto quadWorldMatrix = QuadMesh.get()->WorldTransform.GetMatrix();
		QuadMesh->SetShaderParam(Waldem::ShaderParamType::MAT4, "world", &quadWorldMatrix);
		QuadMesh->SetShaderParam(Waldem::ShaderParamType::MAT4, "viewProjection", &viewProjectionMatrix);
		
		Waldem::Renderer::DrawMesh(QuadMesh.get());
		Waldem::Renderer::DrawMesh(TrisMesh.get());
	}
}
