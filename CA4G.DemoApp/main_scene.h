#pragma once

#include "ca4g.h"
#include "shlobj.h"

using namespace CA4G;

//typedef class BunnyScene main_scene;
typedef class LucyAndDrago main_scene;

CA4G::string desktop_directory()
{
	static char path[MAX_PATH + 1];
	SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE);
	return CA4G::string(path);
}

class BunnyScene : public SceneManager {
public:
	BunnyScene() :SceneManager() {
	}
	~BunnyScene() {}

	float4x4* InitialTransforms;

	void SetupScene() {

		camera.Position = float3(0, 0, 2);
		lights[0].Direction = normalize(float3(1, 1, 1));
		lights[0].Intensity = float3(10, 10, 10);

		CA4G::string desktopPath = desktop_directory();
		
		CA4G::string lucyPath = desktopPath + CA4G::string("\\Models\\bunny.obj");

		auto bunnyScene = OBJLoader::Load(lucyPath);
		bunnyScene->Normalize(
			SceneNormalization::Scale |
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
		return;//
		
		scene->Instances().Data[0].Transform =
			mul(InitialTransforms[0], Transforms::RotateY(time));
		OnUpdated(SceneElement::InstanceTransforms);
	}
};

class LucyAndDrago : public SceneManager {
public:
	LucyAndDrago() :SceneManager() {
	}
	~LucyAndDrago() {}

	float4x4* InitialTransforms;

	void SetupScene() {

		camera.Position = float3(0, 0.4, 2);
		camera.Target = float3(0, 0.4, 0);
		lights[0].Direction = normalize(float3(1, 1, 1));
		lights[0].Intensity = float3(10, 10, 10);

		CA4G::string desktopPath = desktop_directory();
		CA4G::string lucyPath = desktopPath + CA4G::string("\\Models\\newLucy.obj");

		auto lucyScene = OBJLoader::Load(lucyPath);
		lucyScene->Normalize(
			SceneNormalization::Scale |
			SceneNormalization::Maximum |
			//SceneNormalization::MinX |
			SceneNormalization::MinY |
			//SceneNormalization::MinZ |
			SceneNormalization::Center
		);
		scene->appendScene(lucyScene);

		CA4G::string dragoPath = desktopPath + CA4G::string("\\Models\\newDragon.obj");
		auto dragoScene = OBJLoader::Load(dragoPath);
		dragoScene->Normalize(
			SceneNormalization::Scale |
			SceneNormalization::Maximum |
			//SceneNormalization::MinX |
			SceneNormalization::MinY |
			//SceneNormalization::MinZ |
			SceneNormalization::Center
		);
		scene->appendScene(dragoScene);

		CA4G::string platePath = desktopPath + CA4G::string("\\Models\\plate.obj");
		auto plateScene = OBJLoader::Load(platePath);
		scene->appendScene(plateScene);

		InitialTransforms = new float4x4[scene->Instances().Count];
		for (int i = 0; i < scene->Instances().Count; i++)
			InitialTransforms[i] = scene->Instances().Data[i].Transform;

		SetTransforms(0);

		SceneManager::SetupScene();
	}

	void SetTransforms(float time) {
		scene->Instances().Data[0].Transform =
			mul(InitialTransforms[0], mul(Transforms::RotateY(time), Transforms::Translate(-0.4, 0, 0)));
		scene->Instances().Data[1].Transform =
			mul(InitialTransforms[1], mul(Transforms::RotateY(time), Transforms::Translate(0.4, 0, 0)));
		OnUpdated(SceneElement::InstanceTransforms);
	}

	virtual void Animate(float time, int frame, SceneElement freeze = SceneElement::None) override {
		return;//

	}
};