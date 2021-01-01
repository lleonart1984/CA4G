#pragma once

#include "ca4g.h"
#include "shlobj.h"

using namespace CA4G;

typedef class BunnyScene main_scene;

LPSTR desktop_directory()
{
	static char path[MAX_PATH + 1];
	SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);
	return path;
}

class BunnyScene : public SceneManager {
public:
	BunnyScene() :SceneManager() {
	}
	~BunnyScene() {}

	float4x4* InitialTransforms;

	void SetupScene() {

		camera.Position = float3(0, 0, 4);

		char* filePath = desktop_directory();
		strcat_s(filePath, MAX_PATH, "\\Models\\bunny.obj");

		auto bunnyScene = OBJLoader::Load(filePath);
		bunnyScene->Normalize(
			SceneNormalization::Maximum | 
			//SceneNormalization::MinX |
			//SceneNormalization::MinY |
			//SceneNormalization::MinZ |
			SceneNormalization::Center
		);

		scene->appendScene(bunnyScene);

		InitialTransforms = new float4x4[scene->Instances().Count];
		for (int i = 0; i < scene->Instances().Count; i++)
			InitialTransforms[i] = scene->Instances().Data[i].Transform;

		SceneManager::SetupScene();
	}

	virtual void Animate(float time, int frame, SceneElement freeze = SceneElement::None) override {
		scene->Instances().Data[0].Transform =
			mul(Transforms::RotateY(time), InitialTransforms[0]);
		OnUpdated(SceneElement::InstanceTransforms);
	}
};