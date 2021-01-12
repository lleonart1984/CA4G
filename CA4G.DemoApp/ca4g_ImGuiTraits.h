#pragma once

void GuiFor(gObj<CA4G::IShowComplexity> t) {
	ImGui::Checkbox("View Complexity", &t->ShowComplexity);
	ImGui::SliderFloat("Pathtracing", &t->PathtracingRatio, 0, 1);
}

int selectedMaterial = 0;

void GuiFor(gObj<CA4G::IManageScene> t) {

#pragma region camera setup

	Camera camera = t->scene->getCamera();
	if (
		ImGui::InputFloat3("View position", (float*)&camera.Position) |
		ImGui::InputFloat3("Target position", (float*)&camera.Target))
		t->scene->setCamera(camera);

#pragma endregion

#pragma region lighting setup

	LightSource l = t->scene->getMainLight();

	float alpha = atan2f(l.Direction.z, l.Direction.x);
	float beta = asinf(l.Direction.y);
	bool changeDirection = ImGui::SliderFloat("Light Direction Alpha", &alpha, 0, 3.141596 * 2);
	changeDirection |= ImGui::SliderFloat("Light Direction Beta", &beta, -3.141596 / 2, 3.141596 / 2);
	if (changeDirection |
		ImGui::InputFloat3("Light intensity", (float*)&l.Intensity)) {
		l.Direction = float3(cosf(alpha) * cosf(beta), sinf(beta), sinf(alpha) * cosf(beta));
		t->scene->setMainLightSource(l);
	}

#pragma endregion

	ImGui::SliderInt("Material Index", &selectedMaterial, -1, t->scene->getScene()->Materials().Count);

#pragma region Modifying Material

	if (selectedMaterial >= 0)
	{
		bool materialModified = false;

		materialModified |= ImGui::SliderFloat3(
			"Diffuse",
			(float*)&t->scene->getScene()->Materials().Data[selectedMaterial].Diffuse,
			0.0f,
			1.0f,
			"%.3f",
			ImGuiSliderFlags_Logarithmic
		);

		materialModified |= ImGui::SliderFloat3(
			"Specular",
			(float*)&t->scene->getScene()->Materials().Data[selectedMaterial].Specular,
			0.0f,
			1.0f,
			"%.3f",
			ImGuiSliderFlags_Logarithmic
		);

		materialModified |= ImGui::SliderFloat(
			"Refraction Index",
			(float*)&t->scene->getScene()->Materials().Data[selectedMaterial].RefractionIndex,
			0.0f,
			1.0f,
			"%.3f",
			ImGuiSliderFlags_Logarithmic
		);

		if (ImGui::SliderFloat4(
			"Models",
			(float*)&t->scene->getScene()->Materials().Data[selectedMaterial].Roulette,
			0.0f,
			1.0f,
			"%.3f",
			ImGuiSliderFlags_Logarithmic
		)) {
			// Normalize roulette values
			auto r = t->scene->getScene()->Materials().Data[selectedMaterial].Roulette;
			r = r / maxf(0.001, r.x + r.y + r.z + r.w);
			t->scene->getScene()->Materials().Data[selectedMaterial].Roulette = r;
			materialModified = true;
		}

		VolumeMaterial& vol = t->scene->getScene()->VolumeMaterials().Data[selectedMaterial];

		float3 extinction = vol.Extinction;
		float size = max (0.001, max(extinction.x, max(extinction.y, extinction.z)));
		extinction = maxf(0.0001, extinction * 1.0 / size);
		float3 absorption = 1 - vol.ScatteringAlbedo;

		if (
			ImGui::SliderFloat("Size", (float*)&size, 0.01, 1000, "%.3f", ImGuiSliderFlags_Logarithmic) |
			ImGui::SliderFloat3("Extinction", (float*)&extinction, 0.0, 1.0) |
			ImGui::SliderFloat3("Absorption", (float*)&absorption, 0.0f, 1.0f, "%.5f", ImGuiSliderFlags_Logarithmic) |
			ImGui::SliderFloat3("G", (float*)&vol.G, -0.999, 0.999)
			) {
			vol.Extinction = extinction * size;
			vol.ScatteringAlbedo = 1 - absorption;
			materialModified = true;
		}


		if (materialModified)
			t->scene->MakeDirty(SceneElement::Materials);
	}

#pragma endregion
}



template<typename T>
void RenderGUI(gObj<Technique> t) {
	gObj<T> h = t.Dynamic_Cast<T>();
	if (h)
		GuiFor(h);
}