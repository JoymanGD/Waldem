#include "wdpch.h"
#include "ContentManager.h"

bool Waldem::CContentManager::ImportAsset(const Path& path)
{
    std::string extension = path.extension().string();

    Asset* asset = nullptr;

    if (extension == ".png" || extension == ".jpg")
    {
        auto texture = ImageImporter.Import(path);
        asset = (Asset*)&texture->Desc;
    }
    else if (extension == ".gltf" || extension == ".glb")
    {
        asset = (Asset*)ModelImporter.Import(path);
    }
    else if (extension == ".wav")
    {
        asset = (Asset*)AudioImporter.Import(path);
    }

    if(asset)
    {
        WDataBuffer outData;
        asset->Serialize(outData);

        //add header
        uint64 hash = HashFromData(outData.GetData(), outData.GetSize());
        outData.Prepend(&hash, sizeof(uint64));
        outData.Prepend(&asset->Type, sizeof(AssetType));

        //TODO: change to custom path inside Content folder
        auto assetPath = GetPathForAsset(asset->Type);

        assetPath /= path.filename();
        assetPath.replace_extension(".ass");
        
        std::ofstream outFile(assetPath.c_str(), std::ios::binary);
        outFile.write(static_cast<const char*>(outData.GetData()), outData.GetSize());
        outFile.close();
    }
    
    return false;
}
