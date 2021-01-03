//#include "ca4g_dxr_support.h"
//#include "private_ca4g_definitions.h"
//
//namespace CA4G {
//
//	#pragma region Libraries Implementation
//
//	struct DX_LibraryWrapper {
//		D3D12_SHADER_BYTECODE bytecode;
//		list<D3D12_EXPORT_DESC> exports;
//	};
//
//	void CA4G::Library::Loading::Code(D3D12_SHADER_BYTECODE code)
//	{
//		DX_LibraryWrapper lib;
//		lib.bytecode = code;
//		lib.exports = {};
//
//		this->library->__DXLibraries.add(lib);
//	}
//
//	gObj<RayGenerationHandle> CA4G::Library::Creating::RayGenerationShader(LPCWSTR shader)
//	{
//		if (this->library->__DXLibraries.size() == 0)
//			throw new CA4G::CA4GException("Load a library code at least first.");
//		this->library->__DXLibraries.last().exports.add({ shader });
//		return gObj<RayGenerationHandle>(new RayGenerationHandle(shader));
//	}
//
//	gObj<MissHandle> CA4G::Library::Creating::MissShader(LPCWSTR shader)
//	{
//		if (this->library->__DXLibraries.size() == 0)
//			throw new CA4G::CA4GException("Load a library code at least first.");
//		this->library->__DXLibraries.last().exports.add({ shader });
//		return gObj<MissHandle>(new MissHandle(shader));
//	}
//
//	gObj<AnyHitHandle> CA4G::Library::Creating::AnyHit(LPCWSTR shader)
//	{
//		if (this->library->__DXLibraries.size() == 0)
//			throw new CA4G::CA4GException("Load a library code at least first.");
//		this->library->__DXLibraries.last().exports.add({ shader });
//		return gObj<AnyHitHandle>(new AnyHitHandle(shader));
//	}
//
//	gObj<ClosestHitHandle> CA4G::Library::Creating::ClosestHit(LPCWSTR shader)
//	{
//		if (this->library->__DXLibraries.size() == 0)
//			throw new CA4G::CA4GException("Load a library code at least first.");
//		this->library->__DXLibraries.last().exports.add({ shader });
//		return gObj<ClosestHitHandle>(new ClosestHitHandle(shader));
//	}
//
//	gObj<IntersectionHandle> CA4G::Library::Creating::Intersection(LPCWSTR shader)
//	{
//		if (this->library->__DXLibraries.size() == 0)
//			throw new CA4G::CA4GException("Load a library code at least first.");
//		this->library->__DXLibraries.last().exports.add({ shader });
//		return gObj<IntersectionHandle>(new IntersectionHandle(shader));
//	}
//
//	gObj<HitGroupHandle> CA4G::Library::Creating::HitGroup(gObj<ClosestHitHandle> closest, gObj<AnyHitHandle> anyHit, gObj<IntersectionHandle> intersection)
//	{
//		return gObj<HitGroupHandle>(new HitGroupHandle(closest, anyHit, intersection));
//	}
//
//	#pragma endregion
//
//	#pragma region Program Implementation
//
//	struct DX_ProgramWrapper {
//		//! Gets the global bindings for this rt program
//		gObj<RaytracingBinder> globals;
//		//! Gets the locals raygen bindings
//		gObj<RaytracingBinder> raygen_locals;
//		//! Gets the locals miss bindings
//		gObj<RaytracingBinder> miss_locals;
//		//! Gets the locals hitgroup bindings
//		gObj<RaytracingBinder> hitGroup_locals;
//		// Gets the list of all hit groups created in this rt program
//		list<gObj<HitGroupHandle>> hitGroups;
//		// Gets the list of all associations between shaders and global bindings
//		list<LPCWSTR> associationsToGlobal;
//		// Gets the list of all associations between shaders and raygen local bindings
//		list<LPCWSTR> associationsToRayGenLocals;
//		// Gets the list of all associations between shaders and miss local bindings
//		list<LPCWSTR> associationsToMissLocals;
//		// Gets the list of all associations between shaders and hitgroup local bindings
//		list<LPCWSTR> associationsToHitGroupLocals;
//
//		list<gObj<ProgramHandle>> loadedShaderPrograms;
//
//		// Shader table for ray generation shader
//		gObj<Buffer> raygen_shaderTable;
//		// Shader table for all miss shaders
//		gObj<Buffer> miss_shaderTable;
//		// Shader table for all hitgroup entries
//		gObj<Buffer> group_shaderTable;
//		// Gets the attribute size in bytes for this program (normally 2 floats)
//		int AttributesSize = 2 * 4;
//		// Gets the ray payload size in bytes for this program (normally 3 floats)
//		int PayloadSize = 3 * 4;
//		// Gets the stack size required for this program
//		int StackSize = 1;
//		// Gets the maximum number of hit groups that will be setup before any 
//		// single dispatch rays
//		int MaxGroups = 1024 * 1024;
//		// Gets the maximum number of miss programs that will be setup before any
//		// single dispatch rays
//		int MaxMiss = 10;
//	};
//
//	void CA4G::IRTProgram::OnLoad(gObj<Library> library)
//	{
//		this->__DXProgramWrapper = new DX_ProgramWrapper();
//		this->__Library = library;
//		// load shaders and setup program settings
//		Setup();
//		this->__DXProgramWrapper->globals = new RaytracingBinder();
//		this->__DXProgramWrapper->raygen_locals = new RaytracingBinder();
//		this->__DXProgramWrapper->miss_locals = new RaytracingBinder();
//		this->__DXProgramWrapper->hitGroup_locals = new RaytracingBinder();
//		this->Globals(this->__DXProgramWrapper->globals);
//		this->Locals_RayGeneration(this->__DXProgramWrapper->raygen_locals);
//		this->Locals_Miss(this->__DXProgramWrapper->miss_locals);
//		this->Locals_HitGroup(this->__DXProgramWrapper->hitGroup_locals);
//	}
//
//	void IRTProgram::Settings::Payload(int sizeInBytes) {
//		manager->__DXProgramWrapper->PayloadSize = sizeInBytes;
//	}
//	void IRTProgram::Settings::MaxHitGroupIndex(int index) {
//		manager->__DXProgramWrapper->MaxGroups = index + 1;
//	}
//	void IRTProgram::Settings::StackSize(int maxDeep) {
//		manager->__DXProgramWrapper->StackSize = maxDeep;
//	}
//	void IRTProgram::Settings::Attribute(int sizeInBytes) {
//		manager->__DXProgramWrapper->AttributesSize = sizeInBytes;
//	}
//
//	void CA4G::IRTProgram::Loader::Shader(gObj<RayGenerationHandle> handle)
//	{
//		manager->__DXProgramWrapper->associationsToGlobal.add(handle->shaderHandle);
//		manager->__DXProgramWrapper->associationsToRayGenLocals.add(handle->shaderHandle);
//		manager->__DXProgramWrapper->loadedShaderPrograms.add(handle);
//	}
//
//	void CA4G::IRTProgram::Loader::Shader(gObj<MissHandle> handle) {
//		manager->__DXProgramWrapper->associationsToGlobal.add(handle->shaderHandle);
//		manager->__DXProgramWrapper->associationsToMissLocals.add(handle->shaderHandle);
//		manager->__DXProgramWrapper->loadedShaderPrograms.add(handle);
//	}
//
//	void CA4G::IRTProgram::Loader::Shader(gObj<HitGroupHandle> handle)
//	{
//		manager->__DXProgramWrapper->hitGroups.add(handle);
//		manager->__DXProgramWrapper->associationsToGlobal.add(handle->shaderHandle);
//		manager->__DXProgramWrapper->associationsToHitGroupLocals.add(handle->shaderHandle);
//
//		// load all group shaders
//		if (!handle->closestHit.isNull() && !handle->closestHit->IsNull())
//		{
//			manager->__DXProgramWrapper->associationsToGlobal.add(handle->closestHit->shaderHandle);
//			manager->__DXProgramWrapper->associationsToHitGroupLocals.add(handle->closestHit->shaderHandle);
//		}
//		if (!handle->anyHit.isNull() && !handle->anyHit->IsNull())
//		{
//			manager->__DXProgramWrapper->associationsToGlobal.add(handle->anyHit->shaderHandle);
//			manager->__DXProgramWrapper->associationsToHitGroupLocals.add(handle->anyHit->shaderHandle);
//		}
//		if (!handle->intersection.isNull() && !handle->intersection->IsNull())
//		{
//			manager->__DXProgramWrapper->associationsToGlobal.add(handle->intersection->shaderHandle);
//			manager->__DXProgramWrapper->associationsToHitGroupLocals.add(handle->intersection->shaderHandle);
//		}
//		manager->__DXProgramWrapper->loadedShaderPrograms.add(handle);
//	}
//	#pragma endregion
//
//	#pragma region Raytracing Pipeline Implementation
//	
//	struct DX_RTPipelineWrapper {
//		gObj<Library> loadedLibrary;
//		list<gObj<IRTProgram>> loadedPrograms;
//		DX_StateObject so;
//	};
//
//	void CA4G::RaytracingPipelineBindings::OnCreate(DX_Wrapper* dxWrapper)
//	{
//		this->wrapper = new DX_RTPipelineWrapper();
//		// Load Library and programs.
//		this->Setup();
//
//		// TODO: Create the so
//
//#pragma region counting states
//		int count = 0;
//
//		// 1 x each library (D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY)
//		count += wrapper->loadedLibrary->__DXLibraries.size();
//
//		int maxAttributes = 2 * 4;
//		int maxPayload = 3 * 4;
//		int maxStackSize = 1;
//
//		for (int i = 0; i < wrapper->loadedPrograms.size(); i++)
//		{
//			gObj<IRTProgram> program = wrapper->loadedPrograms[i];
//
//			maxAttributes = max(maxAttributes, program->__DXProgramWrapper->AttributesSize);
//			maxPayload = max(maxPayload, program->__DXProgramWrapper->PayloadSize);
//			maxStackSize = max(maxStackSize, program->__DXProgramWrapper->StackSize);
//
//			// Global root signature
//			if (!program->__DXProgramWrapper->globals.isNull())
//				count++;
//			// Local raygen root signature
//			if (!program->__DXProgramWrapper->raygen_locals.isNull())
//				count++;
//			// Local miss root signature
//			if (!program->__DXProgramWrapper->miss_locals.isNull())
//				count++;
//			// Local hitgroup root signature
//			if (!program->__DXProgramWrapper->hitGroup_locals.isNull())
//				count++;
//
//			// Associations to global root signature
//			if (program->__DXProgramWrapper->associationsToGlobal.size() > 0)
//				count++;
//			// Associations to raygen local root signature
//			if (program->__DXProgramWrapper->associationsToRayGenLocals.size() > 0)
//				count++;
//			// Associations to miss local root signature
//			if (program->__DXProgramWrapper->associationsToMissLocals.size() > 0)
//				count++;
//			// Associations to hitgroup local root signature
//			if (program->__DXProgramWrapper->associationsToHitGroupLocals.size() > 0)
//				count++;
//			// 1 x each hit group
//			count += program->__DXProgramWrapper->hitGroups.size();
//		}
//
//		// 1 x shader config
//		count++;
//		// 1 x pipeline config
//		count++;
//
//#pragma endregion
//
//		AllocateStates(count);
//
//#pragma region Fill States
//
//		int index = 0;
//		// 1 x each library (D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY)
//		for (int i = 0; i < wrapper->loadedLibrary->__DXLibraries.size(); i++)
//			SetDXIL(index++, wrapper->loadedLibrary->__DXLibraries[i].bytecode, wrapper->loadedLibrary->__DXLibraries[i].exports);
//
//		D3D12_STATE_SUBOBJECT* globalRS = nullptr;
//		D3D12_STATE_SUBOBJECT* localRayGenRS = nullptr;
//		D3D12_STATE_SUBOBJECT* localMissRS = nullptr;
//		D3D12_STATE_SUBOBJECT* localHitGroupRS = nullptr;
//
//		for (int i = 0; i < wrapper->loadedPrograms.size(); i++)
//		{
//			gObj<IRTProgram> program = wrapper->loadedPrograms[i];
//
//			// Global root signature
//			if (!program->__DXProgramWrapper->globals.isNull())
//			{
//				globalRS = &dynamicStates[index];
//				SetGlobalRootSignature(index++, program->__DXProgramWrapper->globals->rootSignature);
//			}
//			// Local raygen root signature
//			if (!program->__DXProgramWrapper->raygen_locals.isNull())
//			{
//				localRayGenRS = &dynamicStates[index];
//				SetLocalRootSignature(index++, program->__DXProgramWrapper->raygen_locals->rootSignature);
//			}
//			// Local miss root signature
//			if (!program->__DXProgramWrapper->miss_locals.isNull())
//			{
//				localMissRS = &dynamicStates[index];
//				SetLocalRootSignature(index++, program->__DXProgramWrapper->miss_locals->rootSignature);
//			}
//			// Local hitgroup root signature
//			if (!program->__DXProgramWrapper->hitGroup_locals.isNull())
//			{
//				localHitGroupRS = &dynamicStates[index];
//				SetLocalRootSignature(index++, program->__DXProgramWrapper->hitGroup_locals->rootSignature);
//			}
//
//			for (int j = 0; j < program->__DXProgramWrapper->hitGroups.size(); j++)
//			{
//				auto hg = program->__DXProgramWrapper->hitGroups[j];
//				if (hg->intersection.isNull())
//					SetTriangleHitGroup(index++, hg->shaderHandle,
//						hg->anyHit.isNull() ? nullptr : hg->anyHit->shaderHandle,
//						hg->closestHit.isNull() ? nullptr : hg->closestHit->shaderHandle);
//				else
//					SetProceduralGeometryHitGroup(index++, hg->shaderHandle,
//						hg->anyHit ? hg->anyHit->shaderHandle : nullptr,
//						hg->closestHit ? hg->closestHit->shaderHandle : nullptr,
//						hg->intersection ? hg->intersection->shaderHandle : nullptr);
//			}
//
//			// Associations to global root signature
//			if (program->associationsToGlobal.size() > 0)
//				SetExportsAssociations(index++, globalRS, program->associationsToGlobal);
//			// Associations to raygen local root signature
//			if (!program->raygen_locals.isNull() && program->associationsToRayGenLocals.size() > 0)
//				SetExportsAssociations(index++, localRayGenRS, program->associationsToRayGenLocals);
//			// Associations to miss local root signature
//			if (!program->miss_locals.isNull() && program->associationsToMissLocals.size() > 0)
//				SetExportsAssociations(index++, localMissRS, program->associationsToMissLocals);
//			// Associations to hitgroup local root signature
//			if (!program->hitGroup_locals.isNull() && program->associationsToHitGroupLocals.size() > 0)
//				SetExportsAssociations(index++, localHitGroupRS, program->associationsToHitGroupLocals);
//		}
//
//		// 1 x shader config
//		SetRTSizes(index++, maxAttributes, maxPayload);
//		SetMaxRTRecursion(index++, maxStackSize);
//
//#pragma endregion
//
//		// Create so
//		D3D12_STATE_OBJECT_DESC soDesc = { };
//		soDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
//		soDesc.NumSubobjects = index;
//		soDesc.pSubobjects = this->dynamicStates;
//
//		if (manager->fallbackDevice != nullptr) // emulating with fallback device
//		{
//			auto hr = manager->fallbackDevice->CreateStateObject(&soDesc, IID_PPV_ARGS(&fbso));
//			if (FAILED(hr))
//				throw CA4GException::FromError(CA4G_Errors_BadPSOConstruction, nullptr, hr);
//		}
//		else {
//			auto hr = manager->device->CreateStateObject(&soDesc, IID_PPV_ARGS(&so));
//			if (FAILED(hr))
//				throw CA4GException::FromError(CA4G_Errors_BadPSOConstruction, nullptr, hr);
//		}
//
//		// Get All shader identifiers
//		for (int i = 0; i < loadedPrograms.size(); i++)
//		{
//			auto prog = loadedPrograms[i];
//			for (int j = 0; j < prog->loadedShaderPrograms.size(); j++) {
//				auto shaderProgram = prog->loadedShaderPrograms[j];
//
//				if (manager->fallbackDevice != nullptr)
//				{
//					shaderProgram->cachedShaderIdentifier = fbso->GetShaderIdentifier(shaderProgram->shaderHandle);
//				}
//				else // DirectX Raytracing
//				{
//					CComPtr<ID3D12StateObject> __so = so;
//					CComPtr<ID3D12StateObjectProperties> __soProp;
//					__so->QueryInterface<ID3D12StateObjectProperties>(&__soProp);
//
//					shaderProgram->cachedShaderIdentifier = __soProp->GetShaderIdentifier(shaderProgram->shaderHandle);
//				}
//			}
//		}
//	}
//
//	void CA4G::RaytracingPipelineBindings::OnSet(ICmdManager* cmdWrapper) {
//	}
//
//	void CA4G::RaytracingPipelineBindings::OnDispatch(ICmdManager* cmdWrapper) {
//	}
//
//	#pragma endregion
//}