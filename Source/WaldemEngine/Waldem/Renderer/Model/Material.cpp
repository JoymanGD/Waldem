#include "wdpch.h"
#include "Material.h"

namespace Waldem
{
	void Material::Serialize(WDataBuffer& outData)
	{
		outData << DiffuseRef.Reference;
		outData << NormalRef.Reference;
		outData << MetalRoughnessRef.Reference;
		outData << Albedo;
		outData << Metallic;
		outData << Roughness;
	}

	void Material::Deserialize(WDataBuffer& inData)
	{
		inData >> DiffuseRef.Reference;
		inData >> NormalRef.Reference;
		inData >> MetalRoughnessRef.Reference;
		inData >> Albedo;
		inData >> Metallic;
		inData >> Roughness;

		if(!DiffuseRef.Reference.empty() && DiffuseRef.Reference != "Empty")
		{
			DiffuseRef.LoadAsset();
		}
        else
        {
            DiffuseRef.Texture = nullptr;
        }

		if(!NormalRef.Reference.empty() && NormalRef.Reference != "Empty")
		{
			NormalRef.LoadAsset();
		}
        else
        {
            NormalRef.Texture = nullptr;
        }

		if(!MetalRoughnessRef.Reference.empty() && MetalRoughnessRef.Reference != "Empty")
		{
			MetalRoughnessRef.LoadAsset();
		}
        else
        {
            MetalRoughnessRef.Texture = nullptr;
        }
	}
}
