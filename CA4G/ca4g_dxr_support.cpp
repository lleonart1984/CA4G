#include "ca4g_dxr_support.h"
#include "private_ca4g_definitions.h"
#include "private_ca4g_presenter.h"

namespace CA4G {

	struct DX_LibraryWrapper {
		D3D12_SHADER_BYTECODE bytecode;
		list<D3D12_EXPORT_DESC> exports;
	};

	// Intenal representation of a program.
	// Includes binding objects, signatures and shader tables
	struct DX_RTProgram {
		//! Gets the global bindings for all programs
		gObj<RaytracingBinder> globals;
		//! Gets the locals raygen bindings
		gObj<RaytracingBinder> raygen_locals;
		//! Gets the locals miss bindings
		gObj<RaytracingBinder> miss_locals;
		//! Gets the locals hitgroup bindings
		gObj<RaytracingBinder> hitGroup_locals;
		// Gets the list of all hit groups created in this rt program
		list<gObj<HitGroupHandle>> hitGroups;
		// Gets the list of all associations between shaders and global bindings
		list<LPCWSTR> associationsToGlobal;
		// Gets the list of all associations between shaders and raygen local bindings
		list<LPCWSTR> associationsToRayGenLocals;
		// Gets the list of all associations between shaders and miss local bindings
		list<LPCWSTR> associationsToMissLocals;
		// Gets the list of all associations between shaders and hitgroup local bindings
		list<LPCWSTR> associationsToHitGroupLocals;

		list<gObj<ProgramHandle>> loadedShaderPrograms;

		// Shader table for ray generation shader
		gObj<Buffer> raygen_shaderTable;
		// Shader table for all miss shaders
		gObj<Buffer> miss_shaderTable;
		// Shader table for all hitgroup entries
		gObj<Buffer> group_shaderTable;
		// Gets the attribute size in bytes for this program (normally 2 floats)
		int AttributesSize = 2 * 4;
		// Gets the ray payload size in bytes for this program (normally 3 floats)
		int PayloadSize = 3 * 4;
		// Gets the stack size required for this program
		int StackSize = 1;
		// Gets the maximum number of hit groups that will be setup before any 
		// single dispatch rays
		int MaxGroups = 1024 * 1024;
		// Gets the maximum number of miss programs that will be setup before any
		// single dispatch rays
		int MaxMiss = 10;

		ID3D12RootSignature* globalSignature;
		ID3D12RootSignature* rayGen_Signature;
		ID3D12RootSignature* miss_Signature;
		ID3D12RootSignature* hitGroup_Signature;
	};

	struct RTPipelineWrapper {

		DX_Wrapper* dxWrapper;

		// Collects all libraries used in this pipeline.
		list<DX_LibraryWrapper> libraries = {};
		// Collects all programs used in this pipeline.
		list<gObj<RTProgramBase>> programs = {};
		// Pipeline object state created for this pipeline.
		DX_StateObject stateObject;
		// Pipeline object state created for this pipeline in case of fallback.
		DX_FallbackStateObject fallbackStateObject;
	};



	void RaytracingPipelineBindings::AppendCode(const D3D12_SHADER_BYTECODE &code)
	{
		DX_LibraryWrapper lib;
		lib.bytecode = code;
		lib.exports = {};
		wrapper->libraries.add(lib);
	}

	void RaytracingPipelineBindings::LoadProgram(gObj<RTProgramBase> program) {
		wrapper->programs.add(program);
		program->OnLoad(wrapper->dxWrapper);
	}

	void RaytracingPipelineBindings::DeclareExport(LPCWSTR shader) {
		if (wrapper->libraries.size() == 0)
			throw new CA4G::CA4GException("Load a library code at least first.");

		wrapper->libraries.last().exports.add({ shader });
	}

	gObj<HitGroupHandle> RaytracingPipelineBindings::Creating::HitGroup(gObj<ClosestHitHandle> closest, gObj<AnyHitHandle> anyHit, gObj<IntersectionHandle> intersection)
	{
		return new HitGroupHandle(closest, anyHit, intersection);
	}

	void RTProgramBase::Settings::Payload(int sizeInBytes) {
		manager->wrapper->PayloadSize = sizeInBytes;
	}
	void RTProgramBase::Settings::MaxHitGroupIndex(int index) {
		manager->wrapper->MaxGroups = index + 1;
	}
	void RTProgramBase::Settings::StackSize(int maxDeep) {
		manager->wrapper->StackSize = maxDeep;
	}
	void RTProgramBase::Settings::Attribute(int sizeInBytes) {
		manager->wrapper->AttributesSize = sizeInBytes;
	}

	void RTProgramBase::Loading::Shader(gObj<HitGroupHandle>& handle)
	{
		manager->wrapper->hitGroups.add(handle);
		manager->wrapper->associationsToGlobal.add(handle->shaderHandle);
		manager->wrapper->associationsToHitGroupLocals.add(handle->shaderHandle);

		// load all group shaders
		if (!handle->closestHit.isNull() && !handle->closestHit->IsNull())
		{
			manager->wrapper->associationsToGlobal.add(handle->closestHit->shaderHandle);
			manager->wrapper->associationsToHitGroupLocals.add(handle->closestHit->shaderHandle);
		}
		if (!handle->anyHit.isNull() && !handle->anyHit->IsNull())
		{
			manager->wrapper->associationsToGlobal.add(handle->anyHit->shaderHandle);
			manager->wrapper->associationsToHitGroupLocals.add(handle->anyHit->shaderHandle);
		}
		if (!handle->intersection.isNull() && !handle->intersection->IsNull())
		{
			manager->wrapper->associationsToGlobal.add(handle->intersection->shaderHandle);
			manager->wrapper->associationsToHitGroupLocals.add(handle->intersection->shaderHandle);
		}
		manager->wrapper->loadedShaderPrograms.add(handle);
	}

	void RTProgramBase::Loading::Shader(gObj<RayGenerationHandle>& handle) {
		manager->wrapper->associationsToGlobal.add(handle->shaderHandle);
		manager->wrapper->associationsToRayGenLocals.add(handle->shaderHandle);
		manager->wrapper->loadedShaderPrograms.add(handle);
	}

	void RTProgramBase::Loading::Shader(gObj<MissHandle>& handle) {
		manager->wrapper->associationsToGlobal.add(handle->shaderHandle);
		manager->wrapper->associationsToMissLocals.add(handle->shaderHandle);
		manager->wrapper->loadedShaderPrograms.add(handle);
	}

	// Creates the Pipeline object.
	// This method collects bindings, create signatures and 
	// creates the pipeline state object.
	void RaytracingPipelineBindings::OnCreate(DX_Wrapper* dxWrapper)
	{
		this->wrapper = new RTPipelineWrapper();
		this->wrapper->dxWrapper = dxWrapper;

		// Load Library, programs and setup settings.
		this->Setup();

		// TODO: Create the so
#pragma region counting states
		int count = 0;

		// 1 x each library (D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY)
		count += wrapper->libraries.size();

		int maxAttributes = 2 * 4;
		int maxPayload = 3 * 4;
		int maxStackSize = 1;

		for (int i = 0; i < wrapper->programs.size(); i++)
		{
			gObj<RTProgramBase> program = wrapper->programs[i];

			maxAttributes = max(maxAttributes, program->wrapper->AttributesSize);
			maxPayload = max(maxPayload, program->wrapper->PayloadSize);
			maxStackSize = max(maxStackSize, program->wrapper->StackSize);

			// Global root signature
			if (!program->wrapper->globals.isNull())
				count++;
			// Local raygen root signature
			if (!program->wrapper->raygen_locals.isNull())
				count++;
			// Local miss root signature
			if (!program->wrapper->miss_locals.isNull())
				count++;
			// Local hitgroup root signature
			if (!program->wrapper->hitGroup_locals.isNull())
				count++;

			// Associations to global root signature
			if (program->wrapper->associationsToGlobal.size() > 0)
				count++;
			// Associations to raygen local root signature
			if (program->wrapper->associationsToRayGenLocals.size() > 0)
				count++;
			// Associations to miss local root signature
			if (program->wrapper->associationsToMissLocals.size() > 0)
				count++;
			// Associations to hitgroup local root signature
			if (program->wrapper->associationsToHitGroupLocals.size() > 0)
				count++;
			// 1 x each hit group
			count += program->wrapper->hitGroups.size();
		}

		// 1 x shader config
		count++;
		// 1 x pipeline config
		count++;

#pragma endregion

		AllocateStates(count);

#pragma region Fill States

		int index = 0;
		// 1 x each library (D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY)
		for (int i = 0; i < wrapper->libraries.size(); i++)
			SetDXIL(index++, wrapper->libraries[i].bytecode, wrapper->libraries[i].exports);

		D3D12_STATE_SUBOBJECT* globalRS = nullptr;
		D3D12_STATE_SUBOBJECT* localRayGenRS = nullptr;
		D3D12_STATE_SUBOBJECT* localMissRS = nullptr;
		D3D12_STATE_SUBOBJECT* localHitGroupRS = nullptr;

		for (int i = 0; i < wrapper->programs.size(); i++)
		{
			gObj<RTProgramBase> program = wrapper->programs[i];

			// Global root signature
			if (!program->wrapper->globals.isNull())
			{
				globalRS = &dynamicStates[index];
				SetGlobalRootSignature(index++, program->wrapper->globalSignature);
			}
			// Local raygen root signature
			if (!program->wrapper->raygen_locals.isNull())
			{
				localRayGenRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->wrapper->rayGen_Signature);
			}
			// Local miss root signature
			if (!program->wrapper->miss_locals.isNull())
			{
				localMissRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->wrapper->miss_Signature);
			}
			// Local hitgroup root signature
			if (!program->wrapper->hitGroup_locals.isNull())
			{
				localHitGroupRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->wrapper->hitGroup_Signature);
			}

			for (int j = 0; j < program->wrapper->hitGroups.size(); j++)
			{
				auto hg = program->wrapper->hitGroups[j];
				if (hg->intersection.isNull())
					SetTriangleHitGroup(index++, hg->shaderHandle,
						hg->anyHit.isNull() ? nullptr : hg->anyHit->shaderHandle,
						hg->closestHit.isNull() ? nullptr : hg->closestHit->shaderHandle);
				else
					SetProceduralGeometryHitGroup(index++, hg->shaderHandle,
						hg->anyHit ? hg->anyHit->shaderHandle : nullptr,
						hg->closestHit ? hg->closestHit->shaderHandle : nullptr,
						hg->intersection ? hg->intersection->shaderHandle : nullptr);
			}

			// Associations to global root signature
			if (program->wrapper->associationsToGlobal.size() > 0)
				SetExportsAssociations(index++, globalRS, program->wrapper->associationsToGlobal);
			// Associations to raygen local root signature
			if (!program->wrapper->raygen_locals.isNull() && program->wrapper->associationsToRayGenLocals.size() > 0)
				SetExportsAssociations(index++, localRayGenRS, program->wrapper->associationsToRayGenLocals);
			// Associations to miss local root signature
			if (!program->wrapper->miss_locals.isNull() && program->wrapper->associationsToMissLocals.size() > 0)
				SetExportsAssociations(index++, localMissRS, program->wrapper->associationsToMissLocals);
			// Associations to hitgroup local root signature
			if (!program->wrapper->hitGroup_locals.isNull() && program->wrapper->associationsToHitGroupLocals.size() > 0)
				SetExportsAssociations(index++, localHitGroupRS, program->wrapper->associationsToHitGroupLocals);
		}

		// 1 x shader config
		SetRTSizes(index++, maxAttributes, maxPayload);
		SetMaxRTRecursion(index++, maxStackSize);

#pragma endregion

		// Create so
		D3D12_STATE_OBJECT_DESC soDesc = { };
		soDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		soDesc.NumSubobjects = index;
		soDesc.pSubobjects = this->dynamicStates;

		if (dxWrapper->fallbackDevice != nullptr) // emulating with fallback device
		{
			auto hr = dxWrapper->fallbackDevice->CreateStateObject(&soDesc, IID_PPV_ARGS(&wrapper->fallbackStateObject));
			if (FAILED(hr))
				throw CA4GException::FromError(CA4G_Errors_BadPSOConstruction, nullptr, hr);
		}
		else {
			auto hr = dxWrapper->device->CreateStateObject(&soDesc, IID_PPV_ARGS(&wrapper->stateObject));
			if (FAILED(hr))
				throw CA4GException::FromError(CA4G_Errors_BadPSOConstruction, nullptr, hr);
		}

		// Get All shader identifiers
		for (int i = 0; i < wrapper->programs.size(); i++)
		{
			auto prog = wrapper->programs[i];
			for (int j = 0; j < prog->wrapper->loadedShaderPrograms.size(); j++) {
				auto shaderProgram = prog->wrapper->loadedShaderPrograms[j];

				if (dxWrapper->fallbackDevice != nullptr)
				{
					shaderProgram->cachedShaderIdentifier = wrapper->fallbackStateObject->GetShaderIdentifier(shaderProgram->shaderHandle);
				}
				else // DirectX Raytracing
				{
					CComPtr<ID3D12StateObject> __so = wrapper->stateObject;
					CComPtr<ID3D12StateObjectProperties> __soProp;
					__so->QueryInterface<ID3D12StateObjectProperties>(&__soProp);

					shaderProgram->cachedShaderIdentifier = __soProp->GetShaderIdentifier(shaderProgram->shaderHandle);
				}
			}
		}
	}





	#pragma region Program Implementation

	void RTProgramBase::OnLoad(DX_Wrapper* dxWrapper)
	{
		wrapper = new DX_RTProgram();
		// load shaders and setup program settings
		Setup();

		wrapper->globals = new RaytracingBinder();
		wrapper->raygen_locals = new RaytracingBinder();
		wrapper->miss_locals = new RaytracingBinder();
		wrapper->hitGroup_locals = new RaytracingBinder();
		Bindings(this->wrapper->globals);
		RayGeneration_Bindings(this->wrapper->raygen_locals);
		Miss_Bindings(this->wrapper->miss_locals);
		HitGroup_Bindings(this->wrapper->hitGroup_locals);

		// Create signatures

	}

	#pragma endregion

	#pragma region Raytracing Pipeline Implementation
	
	void CA4G::RaytracingPipelineBindings::OnSet(ICmdManager* cmdWrapper) {
	}

	void CA4G::RaytracingPipelineBindings::OnDispatch(ICmdManager* cmdWrapper) {
	}

	#pragma endregion
}