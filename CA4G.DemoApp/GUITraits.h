#ifndef GUITRAITS_H
#define GUITRAITS_H

#include "ca4g_scene.h"

namespace CA4G {

	struct IManageScene {
		gObj<SceneManager> scene = nullptr;
		SceneVersion sceneVersion;
		virtual void SetSceneManager(gObj<SceneManager> scene) {
			this->scene = scene;
		}
	};

	struct IShowComplexity {
		bool ShowComplexity;
	};
}

#endif