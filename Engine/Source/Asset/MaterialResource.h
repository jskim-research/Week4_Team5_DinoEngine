#pragma once
#include "CoreMinimal.h"

struct FMtlInfoElement
{
	FString Name = "";  // newmtl {Name}
	FVector Ka = FVector(0.0f, 0.0f, 0.0f);
	FVector Kd = FVector(1.0f, 1.0f, 1.0f);
	FVector Ks = FVector(0.0f, 0.0f, 0.0f);
	FVector Ke = FVector(0.0f, 0.0f, 0.0f);
	float Ns = 0.0f;
	float Ni = 1.0f;
	float D = 1.0f;
	int32 Illum = 0;
	FString MapKd = "";
};

struct FMtlInfo
{
	FString RelativeParentPath = "";
	TArray<FMtlInfoElement> Elements;
};

/**
* Textures.BaseColor = dorumon.png
* Textures.Normal = dorumon_n.png
* 
 * Scalars.Roughness = 0.5
 * Scalars.Metallic = 1.0
 * 
 * Vectors.ColorTint = 1.0,1.0,1.0
 */
struct FMatInfoElement
{
	FString Name = "";
	FString VertexShader = "";
	FString PixelShader = "";
	TMap<FString, FString> Textures;  // 경로 저장
	TMap<FString, float> Scalars;
	TMap<FString, FVector> Vectors;
};

struct FMatInfo
{
	FString RelativeParentPath = "";
	FString RelativeMaterialPath = "";
	TArray<FMatInfoElement> Elements;
};

