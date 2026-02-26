#pragma once
#include "GraphicResource.h"
#include "TextureFormat.h"
#include "Waldem/Serialization/Asset.h"
#include "Waldem/Types/DataBuffer.h"
#include "Waldem/Types/String.h"
#include "Waldem/Utils/ImageUtils.h"

namespace Waldem
{
    class TextureDesc : public Asset
    {
    public:
        TextureDesc() : Asset(AssetType::Texture) {}
        TextureDesc(WString name) : Asset(name, AssetType::Texture) {}
        TextureDesc(WString name, int width, int height, int depth, TextureFormat format, uint8* data = nullptr) : Width(width), Height(height), Depth(depth), Format(format), Data(data)
        {
            Type = AssetType::Texture;
            Name = name;
        }
        
        int Width;
        int Height;
        int Depth;
        TextureFormat Format;
        uint8* Data;
        
        void Serialize(WDataBuffer& outData) override
        {
            int DataSize = CALCULATE_IMAGE_DATA_SIZE(Width, Height, Depth, Format);
            outData << Width;
            outData << Height;
            outData << Depth;
            outData << Format;
            outData << DataSize;
            outData.Write(Data, DataSize);
        }
        
        void Deserialize(WDataBuffer& inData) override
        {
            int DataSize;
            inData >> Width;
            inData >> Height;
            inData >> Depth;
            inData >> Format;
            inData >> DataSize;
            Data = new uint8[DataSize];
            inData.Read(Data, DataSize);
        }
    };
    
    class Texture2D : public GraphicResource
    {
    public:
        Texture2D() { SetType(RTYPE_Texture); }
        Texture2D(WString name, int width, int height, TextureFormat format, uint8* data = nullptr) : Desc(name, width, height, 1, format, data) { SetType(RTYPE_Texture); }
        
        virtual ~Texture2D() = default;
        
        virtual WString GetName() { return Desc.Name; }
        TextureFormat GetFormat() { return Desc.Format; }
        int GetWidth() { return Desc.Width; }
        int GetHeight() { return Desc.Height; }

        TextureDesc Desc;
    };
    
    class Texture3D : public GraphicResource
    {
    public:
        Texture3D() { SetType(RTYPE_Texture); }
        Texture3D(WString name, int width, int height, int depth, TextureFormat format, uint8* data = nullptr) : Desc(name, width, height, depth, format, data) { SetType(RTYPE_Texture); }
        
        virtual ~Texture3D() = default;
        
        virtual WString GetName() { return Desc.Name; }
        TextureFormat GetFormat() { return Desc.Format; }
        int GetWidth() { return Desc.Width; }
        int GetHeight() { return Desc.Height; }
        int GetDepth() { return Desc.Depth; }

        TextureDesc Desc;
    };
}
