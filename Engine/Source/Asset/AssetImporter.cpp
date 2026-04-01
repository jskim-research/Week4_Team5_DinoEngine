#include "AssetImporter.h"
#include <chrono>
#include "Core/Paths.h"
#include <fstream>
#include <istream>
#include <string>
#include "Debug/EngineLog.h"

void FMaterialImporter::CleanUp()
{
	for (auto Pair : CachedMaterial)
	{
		if (Pair.second)
			delete Pair.second;
	}
}

FMaterial* FMaterialImporter::LoadMaterial(const FString& RelativeFilePath)
{
	FMatInfo MatInfo;

	if (RelativeFilePath.ends_with(".mtl"))
	{
		FMtlInfo MtlInfo;
		if (LoadMtlFile(RelativeFilePath, MtlInfo))
		{

		}
		else
			return nullptr;
	}
	else if (RelativeFilePath.ends_with(".mat"))
	{
		if (LoadMatFile(RelativeFilePath, MatInfo))
		{

		}
		else
			return nullptr;
	}
	else
	{
		return nullptr;
	}

	return nullptr;
}

bool FMaterialImporter::SaveMat(const FString& RelativeSavePath)
{
	return false;
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
		else if (Type == "Shader")
		{
			SS >> MatInfoElement.Shader;
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

	for (const FMtlInfoElement& MtlInfoElement : Mtl.Elements)
	{
		FMatInfoElement MatInfoElement;

		MatInfoElement.Name = MtlInfoElement.Name;
		MatInfoElement.Textures["BaseColor"] = MtlInfoElement.MapKd;
		MatInfoElement.Shader = "Engine/Shaders/TextureVertexShader.hlsl";
	}
	return true;
}

FMaterial* FMaterialImporter::CookMaterial(const FMatInfo& MatInfo)
{
	return nullptr;
}

TMap<FString, FMaterial*> FMaterialImporter::CachedMaterial;