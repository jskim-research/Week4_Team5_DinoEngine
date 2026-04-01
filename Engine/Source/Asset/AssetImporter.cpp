#include "AssetImporter.h"
#include <chrono>
#include "Core/Paths.h"
#include <fstream>
#include <istream>
#include <string>
#include "Debug/EngineLog.h"
#include "Renderer/ShaderMap.h"
#include "AssetManager.h"

TMap<FString, FMaterial*> FMaterialImporter::CachedMaterial;
ID3D11Device* FMaterialImporter::Device;

void FMaterialImporter::CleanUp()
{
	for (auto Pair : CachedMaterial)
	{
		if (Pair.second)
			delete Pair.second;
	}

	CachedMaterial.clear();
}

void FMaterialImporter::LoadMaterial(const FString& RelativeFilePath)
{
	FMatInfo MatInfo;

	if (RelativeFilePath.ends_with(".mtl"))
	{
		FMtlInfo MtlInfo;
		if (LoadMtlFile(RelativeFilePath, MtlInfo))
		{
			ConvertMtlToMatInfo(MtlInfo, MatInfo);
			SaveMat(RelativeFilePath.substr(0, RelativeFilePath.length() - 4) + ".mat", MatInfo);
		}
	}
	else if (RelativeFilePath.ends_with(".mat"))
	{
		if (LoadMatFile(RelativeFilePath, MatInfo))
		{

		}
	}
	else
	{
		return;
	}

	CookMaterial(MatInfo);
}

FMaterial* FMaterialImporter::GetMaterialByName(const FString& Name)
{
	auto It = CachedMaterial.find(Name);
	if (It != CachedMaterial.end())
		return It->second;
	return nullptr;
}

const TArray<FMaterial*> FMaterialImporter::GetAllMaterials()
{
	TArray<FMaterial*> Materials;
	for (const auto& Pair : CachedMaterial)
	{
		Materials.push_back(Pair.second);
	}
	return Materials;
}

bool FMaterialImporter::SaveMat(const FString& RelativeSavePath, const FMatInfo& MatInfo)
{
	auto StartTime = std::chrono::high_resolution_clock::now();

	FString AbsolutePath = FPaths::ToAbsolutePath(RelativeSavePath);
	std::ofstream File(FPaths::ToU8String(AbsolutePath));

	if (!File.is_open())
	{
		UE_LOG("Failed to open .mat file for writing: %s", RelativeSavePath.c_str());
		return false;
	}

	for (const auto& Element : MatInfo.Elements)
	{
		// newmtl 라인
		File << "newmtl " << Element.Name << "\n";

		// Vertex / Pixel Shader
		File << "VertexShader " << Element.VertexShader << "\n";
		
		File << "PixelShader " << Element.PixelShader << "\n";

		// Textures
		for (const auto& [Key, Value] : Element.Textures)
		{
			File << "Textures." << Key << " " << Value << "\n";
		}

		// Scalars
		for (const auto& [Key, Value] : Element.Scalars)
		{
			File << "Scalars." << Key << " " << Value << "\n";
		}

		// Vectors
		for (const auto& [Key, Vec] : Element.Vectors)
		{
			File << "Vectors." << Key << " " << Vec.X << " " << Vec.Y << " " << Vec.Z << "\n";
		}

		File << "\n"; // Material 간 구분
	}

	File.close();

	auto EndTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> Elapsed = EndTime - StartTime;

	UE_LOG("[MAT] Save Success: %s\n", RelativeSavePath.c_str());
	UE_LOG("[MAT] Execution Time: %.6f seconds\n", Elapsed.count());

	return true;
}

bool FMaterialImporter::LoadMtlFile(const FString& RelativeFilePath, FMtlInfo& OutMtlInfo)
{
	auto StartTime = std::chrono::high_resolution_clock::now();

	FString AbsolutePath = FPaths::ToAbsolutePath(RelativeFilePath);
	std::filesystem::path path = FPaths::ToU8String(AbsolutePath);
	std::ifstream File(path);
	if (!File.is_open())
	{
		UE_LOG("Failed to load .mtl file (%s)", RelativeFilePath.c_str());
		return {};
	}

	FMtlInfo MtlInfo;
	FMtlInfoElement MtlInfoElement;
	FString Line;
	bool bHasMaterial = false;

	MtlInfo.RelativeParentPath = std::filesystem::path(RelativeFilePath).parent_path().string();

	while (std::getline(File, Line))
	{
		/** Line 별 순차 처리 */
		if (Line.empty())
			continue;
		if (!Line.empty() && Line.back() == '\r')
		{
			Line.pop_back();
		}

		std::stringstream SS(Line);
		FString Type;
		SS >> Type;

		if (Type.empty() || Type[0] == '#')
			continue;
		if (Type == "newmtl")
		{
			if (bHasMaterial)
			{
				MtlInfo.Elements.push_back(MtlInfoElement);
			}
			MtlInfoElement = FMtlInfoElement();
			bHasMaterial = true;
			std::getline(SS >> std::ws, MtlInfoElement.Name);
		}
		else if (Type == "Ka")
		{
			SS >> MtlInfoElement.Ka.X >> MtlInfoElement.Ka.Y >> MtlInfoElement.Ka.Z;
		}
		else if (Type == "Kd")
		{
			SS >> MtlInfoElement.Kd.X >> MtlInfoElement.Kd.Y >> MtlInfoElement.Kd.Z;
		}
		else if (Type == "Ks")
		{
			SS >> MtlInfoElement.Ks.X >> MtlInfoElement.Ks.Y >> MtlInfoElement.Ks.Z;
		}
		else if (Type == "Ke")
		{
			SS >> MtlInfoElement.Ke.X >> MtlInfoElement.Ke.Y >> MtlInfoElement.Ke.Z;
		}
		else if (Type == "Ns")
		{
			SS >> MtlInfoElement.Ns;
		}
		else if (Type == "Ni")
		{
			SS >> MtlInfoElement.Ni;
		}
		else if (Type == "d")
		{
			SS >> MtlInfoElement.D;
		}
		else if (Type == "illum")
		{
			SS >> MtlInfoElement.Illum;
		}
		else if (Type == "map_Kd")
		{
			std::getline(SS >> std::ws, MtlInfoElement.MapKd);
		}
	}
	// 마지막 material은 루프 안에서 push되지 않으므로 여기서 추가
	if (bHasMaterial)
	{
		MtlInfo.Elements.push_back(MtlInfoElement);
	}
	auto EndTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> Elapsed = EndTime - StartTime;

	// [3] 로그 출력 (초 단위)
	UE_LOG("[MTL] Load Success: %s\n", RelativeFilePath.c_str());
	UE_LOG("[MTL] Execution Time: %.6f seconds\n", Elapsed.count());

	OutMtlInfo = MtlInfo;
	return true;
}

bool FMaterialImporter::LoadMatFile(const FString& RelativeFilePath, FMatInfo& OutMatInfo)
{
	auto StartTime = std::chrono::high_resolution_clock::now();

	FString AbsolutePath = FPaths::ToAbsolutePath(RelativeFilePath);
	std::filesystem::path Path = FPaths::ToU8String(AbsolutePath);
	std::ifstream File(Path);

	if (!File.is_open())
	{
		UE_LOG("Failed to load .mat file (%s)", RelativeFilePath.c_str());
		return false;
	}

	FMatInfo MatInfo;
	FMatInfoElement MatInfoElement;
	FString Line;
	bool bProcessingMaterial = false;

	MatInfo.RelativeParentPath = std::filesystem::path(RelativeFilePath).parent_path().string();

	while (std::getline(File, Line))
	{
		if (Line.empty()) continue;
		else
		{
			if (Line.back() == '\r') Line.pop_back();
		}

		std::stringstream SS(Line);
		FString Type;
		FString Key;
		FString Value;

		SS >> Type;

		SIZE_T Index = Type.find('.');

		if (Type.empty() || Type[0] == '#')
			continue;
		else if (Type == "newmtl")
		{
			if (bProcessingMaterial)
			{
				// 이미 처리중인게 있었다면 보관
				MatInfo.Elements.push_back(MatInfoElement);
			}

			MatInfoElement = FMatInfoElement();
			bProcessingMaterial = true;

			std::getline(SS >> std::ws, MatInfoElement.Name);
		}
		else if (Type == "VertexShader")
		{
			SS >> MatInfoElement.VertexShader;
		}
		else if (Type == "PixelShader")
		{
			SS >> MatInfoElement.PixelShader;
		}
		else if (Type.starts_with("Textures"))
		{
			if (Index != std::string::npos)
			{
				Key = Type.substr(Index + 1);
				SS >> Value;
				MatInfoElement.Textures[Key] = Value;
			}
		}
		else if (Type.starts_with("Scalars"))
		{
			if (Index != std::string::npos)
			{
				Key = Type.substr(Index + 1);
				SS >> Value;
				MatInfoElement.Scalars[Key] = std::stof(Value);
			}
		}
		else if (Type.starts_with("Vectors"))
		{
			if (Index != std::string::npos)
			{
				FVector V;
				Key = Type.substr(Index + 1);
				SS >> Value;
				V.X = std::stof(Value);
				SS >> Value;
				V.Y = std::stof(Value);
				SS >> Value;
				V.Z = std::stof(Value);

				MatInfoElement.Vectors[Key] = V;
			}
		}
	}

	if (bProcessingMaterial)
	{
		// 마지막 처리 안된 부분 처리
		MatInfo.Elements.push_back(MatInfoElement);
	}

	auto EndTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> Elapsed = EndTime - StartTime;

	// [3] 로그 출력 (초 단위)
	UE_LOG("[MAT] Load Success: %s\n", RelativeFilePath.c_str());
	UE_LOG("[MAT] Execution Time: %.6f seconds\n", Elapsed.count());
	OutMatInfo = MatInfo;

	return true;
}

bool FMaterialImporter::ConvertMtlToMatInfo(const FMtlInfo& Mtl, FMatInfo& OutMatInfo)
{
	FMatInfo MatInfo;

	MatInfo.RelativeParentPath = Mtl.RelativeParentPath;

	for (const FMtlInfoElement& MtlInfoElement : Mtl.Elements)
	{
		FMatInfoElement MatInfoElement;

		MatInfoElement.Name = MtlInfoElement.Name;
		MatInfoElement.Textures["BaseColor"] = MtlInfoElement.MapKd;
		// Default Shader (.mtl 파일에 없으므로 임의로 지정)
		MatInfoElement.VertexShader = "Engine/Shaders/TextureVertexShader.hlsl";
		MatInfoElement.PixelShader = "Engine/Shaders/TexturePixelShader.hlsl";

		MatInfo.Elements.push_back(MatInfoElement);
	}

	OutMatInfo = MatInfo;


	return true;
}

void FMaterialImporter::CookMaterial(const FMatInfo& MatInfo)
{
	for (const FMatInfoElement& MatInfoElement : MatInfo.Elements)
	{
		// 텍스처 셰이더 경로
		std::filesystem::path Root = FPaths::ProjectRoot();

		std::wstring VSPath = (Root / MatInfoElement.VertexShader).wstring();
		std::wstring PSPath = (Root / MatInfoElement.PixelShader).wstring();

		auto VS = FShaderMap::Get().GetOrCreateVertexShader(Device, VSPath.c_str());
		auto PS = FShaderMap::Get().GetOrCreatePixelShader(Device, PSPath.c_str());

		FMaterial* Mat = new FMaterial();

		Mat->SetOriginName(MatInfoElement.Name);
		Mat->SetVertexShader(VS);
		Mat->SetPixelShader(PS);

		// RasterizerState 명시 설정 (없으면 이전 프레임 상태 상속되는 문제 방지)
		{
			FRasterizerStateOption RSOption;
			RSOption.FillMode = D3D11_FILL_SOLID;
			RSOption.CullMode = D3D11_CULL_NONE;  // blank spots 원인 확인용: culling 완전 비활성화
			RSOption.DepthClipEnable = true;
			auto RS = FRasterizerState::Create(Device, RSOption);
			Mat->SetRasterizerOption(RSOption);
			Mat->SetRasterizerState(RS);

			FDepthStencilStateOption DSOption;
			DSOption.DepthEnable = true;
			DSOption.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			auto DSS = FDepthStencilState::Create(Device, DSOption);
			Mat->SetDepthStencilOption(DSOption);
			Mat->SetDepthStencilState(DSS);
		}

		// b2: ColorTint(VS) + BaseColor(PS) — float4 하나 공유
		int32 SlotIndex = Mat->CreateConstantBuffer(Device, 16);
		if (SlotIndex >= 0)
		{
			Mat->RegisterParameter("BaseColor", SlotIndex, 0, 16);
			// 기본값 흰색
			float White[4] = { 1.f, 1.f, 1.f, 1.f };
			Mat->SetParameterData("BaseColor", White, sizeof(White));
		}

		// Diffuse 텍스처 (MTL 파일과 같은 디렉토리에서 탐색)
		if (MatInfoElement.Textures.contains("BaseColor"))
		{
			FString PathFileName = MatInfo.RelativeParentPath + "/" + MatInfoElement.Textures.find("BaseColor")->second;
			FString RelativePath = FPaths::ToRelativePath(PathFileName);
			std::filesystem::path path = FPaths::ToU8String(RelativePath);
			std::filesystem::path MtlDir = path.parent_path();
			std::filesystem::path TexFullPath = MtlDir / FPaths::ToU8String(PathFileName);
			FTexture* Tex = FAssetManager::LoadTextureAsset(FPaths::ToRelativePath(FPaths::FromPath(TexFullPath)));
			if (Tex)
			{
				Mat->SetMaterialTexture(std::shared_ptr<FTexture>(Tex, [](FTexture*) {}));
			}
		}

		CachedMaterial[MatInfoElement.Name] = Mat;
	}
}
