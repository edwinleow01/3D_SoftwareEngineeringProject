
#pragma once
#ifndef _MATERIAL_LOADER_H_
#define _MATERIAL_LOADER_H_

#include "AssetManager.h"
#include "TypeRegistry.h"

namespace Framework
{
	std::unique_ptr<MaterialProperties> LoadMaterialAsset(const std::string& path, const AssetHandle& guid);
	void SaveMaterialAsset(const MaterialProperties& mat, const std::string& path, const AssetHandle& guid);
	void RegisterMaterialMetadata(const AssetHandle& guid, const std::string& name, const std::string& path);
	AssetHandle GetDefaultMaterial();
}
#endif // !_MATERIAL_LOADER_H_
