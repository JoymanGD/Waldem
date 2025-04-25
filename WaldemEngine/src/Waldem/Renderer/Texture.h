#pragma once
#include "TextureFormat.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/DataBuffer.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    class TextureDesc : public Asset
    {
    public:
        TextureDesc() : Asset(AssetType::Texture) {}
        TextureDesc(WString name, int width, int height, TextureFormat format, size_t dataSize, uint8* data) : Name(name), Width(width), Height(height), Format(format), DataSize(dataSize), Data(std::move(data))
        {
            Type = AssetType::Texture;
        }
        
        WString Name;
        int Width;
        int Height;
        TextureFormat Format;
        size_t DataSize;
        uint8* Data;
        
        void Serialize(WDataBuffer& outData) override
        {
            Name.Serialize(outData);
            outData << Width;
            outData << Height;
            outData << Format;
            outData << DataSize;
            outData.Write(Data, DataSize);
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            Name.Deserialize(inData);
            inData >> Width;
            inData >> Height;
            inData >> Format;
            inData >> DataSize;
            Data = new uint8[DataSize];
            inData.Read(Data, DataSize);
            
            // *this = *Renderer::CreateTexture(Name, Width, Height, Format, DataSize, Data);
        }
    };
    
    class Texture2D
    {
    public:
        Texture2D() = default;
        Texture2D(WString name, int width, int height, TextureFormat format, size_t dataSize, uint8* data) : Desc(name, width, height, format, dataSize, data) {}
        Texture2D(TextureDesc desc) : Desc(desc) {}
        virtual ~Texture2D() = default;
        
        virtual WString GetName() { return Desc.Name; }
        virtual void* GetPlatformResource() { return nullptr; }
        virtual void Destroy() {}
        TextureFormat GetFormat() { return Desc.Format; }
        int GetWidth() { return Desc.Width; }
        int GetHeight() { return Desc.Height; }

        TextureDesc Desc;
    };
}
