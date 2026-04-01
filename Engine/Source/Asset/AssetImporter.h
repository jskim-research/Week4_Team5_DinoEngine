#pragma once
#include "MaterialResource.h"
#include "Renderer/Material.h"

struct ENGINE_API FMaterialImporter
{
public:
	// main 끝 부분에서 호출 신경써줘야함
	static void CleanUp();

	static FMaterial* LoadMaterial(const FString& RelativeFilePath);


private:
	static bool SaveMat(const FString& RelativeSavePath);
	static bool LoadMtlFile(const FString& RelativeFilePath, FMtlInfo& OutMtlInfo);
	static bool LoadMatFile(const FString& RelativeFilePath, FMatInfo& OutMatInfo);
	static bool ConvertMtlToMatInfo(const FMtlInfo& Mtl, FMatInfo& OutMatInfo);
	static FMaterial* CookMaterial(const FMatInfo& MatInfo);

	static TMap<FString, FMaterial*> CachedMaterial;
};
