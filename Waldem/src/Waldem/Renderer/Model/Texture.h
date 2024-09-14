#pragma once
#include "glad/glad.h"

class Texture
{
public:
    int Width;
    int Height;
    int Channels;
    uint32_t ShaderProgramID;

    Texture(int width, int height, int channels, const uint8_t* data)
    {
        Width = width;
        Height = height;
        Channels = channels;

        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int format;

        switch (Channels)
        {
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            format = GL_RGBA;
            break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void Bind()
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TextureID);
    }

    void Unbind()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Destroy()
    {
        glDeleteTextures(1, &TextureID);
    }

    void SetParam(uint32_t shaderProgramID)
    {
        ShaderProgramID = shaderProgramID;
        GLint textureLocation = glGetUniformLocation(ShaderProgramID, "DiffuseTexture");
        glUniform1i(textureLocation, 0);
    }

private:
    GLuint TextureID;
};
