#include <Waldem.h>

class Sandbox : public Waldem::Application
{
public:
	Sandbox()
	{

	}

	~Sandbox()
	{

	}
};

Waldem::Application* Waldem::CreateApplication()
{
	return new Sandbox();
}