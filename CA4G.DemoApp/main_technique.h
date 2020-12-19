#pragma once

#include "ca4g.h"

using namespace CA4G;

typedef class MyTechnique : public Technique {
public:

	~MyTechnique() {}

	// Inherited via Technique
	void OnLoad() override {
	}

	void OnDispatch() override {
		__dispatch member_collector(ClearRT);
	}

	void ClearRT(gObj<GraphicsManager> manager) {
		manager _clear RT(render_target, float4(1, 0, 1, 1));
	}

} main_technique;

