#include "FileIO.h"
#include <Windows.h>

static std::string WStringToUTF8(const wchar_t* WStr)
{
	if (!WStr || WStr[0] == L'\0')
		return "";

	int Size = WideCharToMultiByte(CP_UTF8, 0, WStr, -1, nullptr, 0, nullptr, nullptr);
	std::string Result(Size - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, WStr, -1, &Result[0], Size, nullptr, nullptr);
	return Result;
}

std::string GetFilePathInternal(EFileDialogType Type, const wchar_t* Filter, const wchar_t* DefaultExt)
{
	wchar_t FileName[MAX_PATH] = L"";
	std::wstring ContentDir = FPaths::ContentDir().wstring();

	OPENFILENAMEW Ofn = {};
	Ofn.lStructSize = sizeof(OPENFILENAMEW);
	Ofn.lpstrFilter = Filter;
	Ofn.lpstrFile = FileName;
	Ofn.nMaxFile = MAX_PATH;
	Ofn.lpstrDefExt = DefaultExt;
	Ofn.lpstrInitialDir = ContentDir.c_str();

	if (Type == EFileDialogType::Save)
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		if (GetSaveFileNameW(&Ofn))
			return WStringToUTF8(FileName);
	}
	else
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileNameW(&Ofn))
			return WStringToUTF8(FileName);
	}

	return "";
}

std::string GetJsonFilePath(EFileDialogType Type)
{
	return GetFilePathInternal(
		Type,
		L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0\0",
		L"json"
	);
}

std::string GetObjFilePath(EFileDialogType Type)
{
	return GetFilePathInternal(
		Type,
		L"OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0\0",
		L"obj"
	);
}