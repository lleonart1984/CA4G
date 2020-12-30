#ifndef GUITRAITS_H
#define GUITRAITS_H

#include "ca4g_scene.h"

namespace CA4G {

	class IManageScene {
	protected:
		gObj<SceneManager> scene = nullptr;
		SceneVersion sceneVersion;
	public:
		virtual void SetSceneManager(gObj<SceneManager> scene) {
			this->scene = scene;
		}
	};
}

#endif