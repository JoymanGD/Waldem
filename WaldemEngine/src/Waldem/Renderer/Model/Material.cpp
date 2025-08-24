#include "wdpch.h"
#include "Waldem/Engine.h"

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

		if(DiffuseRef.Reference != "Empty")
		{
			DiffuseRef.LoadAsset();
		}

		if(NormalRef.Reference != "Empty")
		{
			NormalRef.LoadAsset();
		}

		if(MetalRoughnessRef.Reference != "Empty")
		{
			MetalRoughnessRef.LoadAsset();
		}
	}
}
