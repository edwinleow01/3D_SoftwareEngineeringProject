local ok = true
ok = ok and Cook.WindowConfig("Assets/JSON/WindowAsset.json",	"Resource/WindowAsset.cfg",		"Descriptor/WindowAsset.desc")
ok = ok and Cook.Audio       ("Assets/JSON/AudioBank.json",		"Resource/AudioBank.bin",		"Descriptor/AudioBank.desc",		"banks")
ok = ok and Cook.Textures    ("Assets/JSON/TextureAsset.json",	"Resource/TextureAsset.bin",	"Descriptor/TextureAsset.desc",		"textures")
ok = ok and Cook.Meshes      ("Assets/JSON/MeshAsset.json",		"Resource/MeshAsset.bin",		"Descriptor/MeshAsset.desc",		"mesh")
ok = ok and Cook.Prefabs     ("Assets/JSON/PrefabAsset.json",	"Resource/PrefabAsset.bin",		"Descriptor/PrefabAsset.desc",		"prefabs")
ok = ok and Cook.Scenes      ("Assets/JSON/SceneAsset.json",    "Resource/SceneAsset.bin",		"Descriptor/SceneAsset.desc",		"scenes")
ok = ok and Cook.Materials   ("Assets/JSON/MaterialAsset.json", "Resource/MaterialAsset.bin",	"Descriptor/MaterialAsset.desc",	"materials")
print(ok and "[Cook] OK" or "[Cook] FAILED")
return ok