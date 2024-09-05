#include "wdpch.h"
#include "PixelShader.h"

#include <filesystem>
#include <fstream>
#include "glad/glad.h"

namespace Waldem
{
    PixelShader::PixelShader(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
    	InitializeShader(vertexSrc, fragmentSrc);
    }

    std::string GetCurrentFolder()
    {
    	char buffer[MAX_PATH];
    	GetModuleFileNameA(NULL, buffer, MAX_PATH);
    	auto currentPath = std::string(buffer);
    	size_t pos = currentPath.find_last_of("\\/");
    	if (pos != std::string::npos)
    	{
    		currentPath = currentPath.substr(0, pos);
    	}

    	return currentPath;
    }

    PixelShader::PixelShader(const std::string& shaderName)
    {
    	auto currentPath = GetCurrentFolder();
    	
    	size_t lastSlash = shaderName.find_last_of("/\\");
    
    	// Extract the directory part
    	std::string pathToShaders = currentPath + "/Shaders/";
    
    	// Extract the base name
    	std::string baseName = shaderName.substr(lastSlash + 1);
    
    	// Create the shader file names
    	auto vertShaderName = pathToShaders + baseName + ".vert.glsl";
    	auto fragShaderName = pathToShaders + baseName + ".frag.glsl";
    	auto vertexShader = LoadShaderFile(vertShaderName);
    	auto fragmentShader = LoadShaderFile(fragShaderName);

    	InitializeShader(vertexShader, fragmentShader);
    }

    PixelShader::~PixelShader()
    {
    	glDeleteProgram(RendererID);
    }

    void PixelShader::Bind(std::vector<ShaderParam*>& shaderParams)
    {
    	glUseProgram(RendererID);

	    for (int i = 0; i < shaderParams.size(); ++i)
	    {
	    	auto param = shaderParams[i];
	    	GLint glParam = glGetUniformLocation(RendererID, param->Name);

		    switch (param->Type)
		    {
		    case ShaderParamType::MAT4:
	    		glUniformMatrix4fv(glParam, 1, GL_FALSE, (float*)param->Value);
		    	break;
		    case ShaderParamType::FLOAT:
	    		glUniform1f(glParam, *(float*)param->Value);
		    	break;
		    case ShaderParamType::VEC3:
	    		glUniform3fv(glParam, 1, (float*)param->Value);
		    	break;
		    case ShaderParamType::UINT:
	    		glUniform1ui(glParam, *(uint32_t*)param->Value);
		    	break;
		    }
	    }
    }

    void PixelShader::Unbind(std::vector<ShaderParam*>& shaderParams)
    {
    	glUseProgram(0);

    	for (auto element : shaderParams)
    	{
    		delete element;
    	}

    	shaderParams.clear();
    }

    std::string PixelShader::LoadShaderFile(std::string& filename)
    {
    	std::ifstream file(filename);
    	
	    if (!file.is_open())
	    {
    		WD_CORE_INFO("Failed to open file: {0}", filename);
    		return "";
	    }
    	
    	std::stringstream buffer;
    	buffer << file.rdbuf();
    	return buffer.str();
    }

    void PixelShader::InitializeShader(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
		// Create an empty vertex shader handle
		VS = glCreateShader(GL_VERTEX_SHADER);

		// Send the vertex shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		const GLchar *source = vertexSrc.c_str();
		glShaderSource(VS, 1, &source, 0);

		// Compile the vertex shader
		glCompileShader(VS);

		GLint isCompiled = 0;
		glGetShaderiv(VS, GL_COMPILE_STATUS, &isCompiled);
		if(isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(VS, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(VS, maxLength, &maxLength, &infoLog[0]);
			
			// We don't need the shader anymore.
			glDeleteShader(VS);

			// Use the infoLog as you see fit.
			WD_CORE_ERROR("{0}", infoLog.data());
			WD_CORE_ASSERT(false, "Vertex shader compilation failure!")
			
			// In this simple program, we'll just leave
			return;
		}

		// Create an empty fragment shader handle
		PS = glCreateShader(GL_FRAGMENT_SHADER);

		// Send the fragment shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		source = fragmentSrc.c_str();
		glShaderSource(PS, 1, &source, 0);

		// Compile the fragment shader
		glCompileShader(PS);

		glGetShaderiv(PS, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(PS, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetShaderInfoLog(PS, maxLength, &maxLength, &infoLog[0]);
			
			// We don't need the shader anymore.
			glDeleteShader(PS);
			// Either of them. Don't leak shaders.
			glDeleteShader(VS);

			// Use the infoLog as you see fit.
			WD_CORE_ERROR("{0}", infoLog.data());
			WD_CORE_ASSERT(false, "Fragment shader compilation failure!")
			
			// In this simple program, we'll just leave
			return;
		}

		// Vertex and fragment shaders are successfully compiled.
		// Now time to link them together into a program.
		// Get a program object.
		RendererID = glCreateProgram();

		// Attach our shaders to our program
		glAttachShader(RendererID, VS);
		glAttachShader(RendererID, PS);

		// Link our program
		glLinkProgram(RendererID);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(RendererID, GL_LINK_STATUS, (int *)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(RendererID, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(RendererID, maxLength, &maxLength, &infoLog[0]);
			
			// We don't need the program anymore.
			glDeleteProgram(RendererID);
			// Don't leak shaders either.
			glDeleteShader(VS);
			glDeleteShader(PS);

			// Use the infoLog as you see fit.
			WD_CORE_ERROR("{0}", infoLog.data());
			WD_CORE_ASSERT(false, "Shaders linking failure!")
			
			// In this simple program, we'll just leave
			return;
		}

		// Always detach shaders after a successful link.
		glDetachShader(RendererID, VS);
		glDetachShader(RendererID, PS);
    }
}
