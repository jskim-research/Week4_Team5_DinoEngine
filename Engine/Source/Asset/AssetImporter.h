#pragma once
#include "MaterialResource.h"
#include "Renderer/Material.h"

struct ENGINE_API FMaterialImporter
{
public:
	// main 끝 부분에서 호출 신경써줘야함
	static void CleanUp();

	// Material load 후 Cache 에 넣어둠
	static void LoadMaterial(const FString& RelativeFilePath);

	// OverrideMaterial 정보를 바탕으로 파일에서 Texture 정보만 바꿀 때 사용
	// static void LoadMaterial(const FString& RelativeFilePath, const FString& TexturePath);
	
	// RelativeFilePath 를 Cache 의 Key 로 사용
	static FMaterial* GetMaterialByName(const FString& RelativeFilePath);
	static const TArray<FMaterial*> GetAllMaterials();

	static void SetDevice(ID3D11Device* InDevice) { Device = InDevice; }

private:
	static bool SaveMat(const FString& RelativeSavePath, const FMatInfo& MatInfo);
	static bool LoadMtlFile(const FString& RelativeFilePath, FMtlInfo& OutMtlInfo);
	static bool LoadMatFile(const FString& RelativeFilePath, FMatInfo& OutMatInfo);
	static bool ConvertMtlToMatInfo(const FMtlInfo& Mtl, FMatInfo& OutMatInfo);
	static void CookMaterial(const FMatInfo& MatInfo);

private:
	static TMap<FString, FMaterial*> CachedMaterial;
	static ID3D11Device* Device;
};
