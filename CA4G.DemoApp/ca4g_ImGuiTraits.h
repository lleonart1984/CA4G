#pragma once



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

	ImGui::DragInt("Material Index", &selectedMaterial, 1, 0, t->scene->getScene()->Materials().Count);

}



template<typename T>
void RenderGUI(gObj<Technique> t) {
	gObj<T> h = t.Dynamic_Cast<T>();
	if (h)
		GuiFor(h);
}