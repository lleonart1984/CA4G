#include "ca4g_dxr_support.h"
#include "private_ca4g_definitions.h"
#include "private_ca4g_presenter.h"
#include "private_ca4g_pipelines.h"

namespace CA4G {

	struct DX_LibraryWrapper {
		D3D12_SHADER_BYTECODE bytecode;
		list<D3D12_EXPORT_DESC> exports;
	};

	struct DX_ShaderTable {
		DX_Resource Buffer;
		void* MappedData;
		int ShaderIDSize;
		int ShaderRecordSize;
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
		DX_ShaderTable raygen_shaderTable;
		// Shader table for all miss shaders
		DX_ShaderTable miss_shaderTable;
		// Shader table for all hitgroup entries
		DX_ShaderTable group_shaderTable;
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

		DX_RootSignature globalSignature = nullptr;
		DX_RootSignature rayGen_Signature = nullptr;
		DX_RootSignature miss_Signature = nullptr;
		DX_RootSignature hitGroup_Signature = nullptr;

		int globalSignatureSize = 0;
		int rayGen_SignatureSize = 0;
		int miss_SignatureSize = 0;
		int hitGroup_SignatureSize = 0;

		void BindLocalsOnShaderTable(gObj<RaytracingBinder> binder, byte* shaderRecordData) {
			
			list<SlotBinding> bindings = binder->__InternalBindingObject->bindings;
			
			for (int i = 0; i < bindings.size(); i++)
			{
				auto binding = bindings[i];

				switch (binding.Root_Parameter.ParameterType)
				{
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					memcpy(shaderRecordData, binding.ConstantData.ptrToConstant, binding.Root_Parameter.Constants.Num32BitValues * 4);
					shaderRecordData += binding.Root_Parameter.Constants.Num32BitValues * 4;
					break;
				default:
					shaderRecordData += 4 * 4;
				}
			}
		}

		void BindLocal(
			DX_Wrapper* dxWrapper, 
			DX_ShaderTable &shadertable, 
			gObj<ProgramHandle> shader, 
			int index, 
			gObj<RaytracingBinder> binder)
		{
			byte* shaderRecordStart = (byte*)shadertable.MappedData + shadertable.ShaderRecordSize * index;
			memcpy(shaderRecordStart, shader->cachedShaderIdentifier, shadertable.ShaderIDSize);
			if (binder->HasSomeBindings())
				BindLocalsOnShaderTable(miss_locals, shaderRecordStart + shadertable.ShaderIDSize);
		}
		void BindLocal(DX_Wrapper* dxWrapper, gObj<RayGenerationHandle> shader) {
			BindLocal(dxWrapper,
				raygen_shaderTable,
				shader, 0, raygen_locals);
		}

		void BindLocal(DX_Wrapper* dxWrapper, gObj<MissHandle> shader, int index) {
			BindLocal(dxWrapper, 
				miss_shaderTable,
				shader, index, miss_locals);
		}

		void BindLocal(DX_Wrapper* dxWrapper, gObj<HitGroupHandle> shader, int index) {
			BindLocal(dxWrapper,
				group_shaderTable, 
				shader, index, hitGroup_locals);
		}
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
		DX_LibraryWrapper lib = { };
		lib.bytecode = code;
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
			if (program->wrapper->raygen_locals->HasSomeBindings())
				count++;
			// Local miss root signature
			if (program->wrapper->miss_locals->HasSomeBindings())
				count++;
			// Local hitgroup root signature
			if (program->wrapper->hitGroup_locals->HasSomeBindings())
				count++;

			// Associations to global root signature
			if (program->wrapper->associationsToGlobal.size() > 0)
				count++;
			// Associations to raygen local root signature
			if (program->wrapper->raygen_locals->HasSomeBindings() 
				&& program->wrapper->associationsToRayGenLocals.size() > 0)
				count++;
			// Associations to miss local root signature
			if (program->wrapper->miss_locals->HasSomeBindings()
				&& program->wrapper->associationsToMissLocals.size() > 0)
				count++;
			// Associations to hitgroup local root signature
			if (program->wrapper->hitGroup_locals->HasSomeBindings() 
				&& program->wrapper->associationsToHitGroupLocals.size() > 0)
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
			if (program->wrapper->raygen_locals->HasSomeBindings())
			{
				localRayGenRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->wrapper->rayGen_Signature);
			}
			// Local miss root signature
			if (program->wrapper->miss_locals->HasSomeBindings())
			{
				localMissRS = &dynamicStates[index];
				SetLocalRootSignature(index++, program->wrapper->miss_Signature);
			}
			// Local hitgroup root signature
			if (program->wrapper->hitGroup_locals->HasSomeBindings())
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
			if (program->wrapper->raygen_locals->HasSomeBindings() && program->wrapper->associationsToRayGenLocals.size() > 0)
				SetExportsAssociations(index++, localRayGenRS, program->wrapper->associationsToRayGenLocals);
			// Associations to miss local root signature
			if (program->wrapper->miss_locals->HasSomeBindings() && program->wrapper->associationsToMissLocals.size() > 0)
				SetExportsAssociations(index++, localMissRS, program->wrapper->associationsToMissLocals);
			// Associations to hitgroup local root signature
			if (program->wrapper->hitGroup_locals->HasSomeBindings() && program->wrapper->associationsToHitGroupLocals.size() > 0)
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

	// Creates a shader table buffer
	DX_ShaderTable ShaderTable(DX_Wrapper* dxWrapper, int idSize, int stride, int count) {

		DX_ShaderTable result = { };

		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;

		stride = (stride + (D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1)) & ~(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT - 1);

		D3D12_RESOURCE_DESC desc = { };
		desc.Width = count * stride;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Height = 1;
		desc.Alignment = 0;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.SampleDesc = { 1, 0 };

		D3D12_HEAP_PROPERTIES uploadProp = {};
		uploadProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		uploadProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		uploadProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		uploadProp.VisibleNodeMask = 1;
		uploadProp.CreationNodeMask = 1;

		dxWrapper->device->CreateCommittedResource(&uploadProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			state,
			nullptr,
			IID_PPV_ARGS(&result.Buffer));

		D3D12_RANGE read = {};
		result.Buffer->Map(0, &read, &result.MappedData);
		result.ShaderRecordSize = stride;
		result.ShaderIDSize = idSize;
		return result;
	}

	void RTProgramBase::OnLoad(DX_Wrapper* dxWrapper)
	{
		wrapper = new DX_RTProgram();
		wrapper->globals = new RaytracingBinder();
		wrapper->raygen_locals = new RaytracingBinder();
		wrapper->miss_locals = new RaytracingBinder();
		wrapper->hitGroup_locals = new RaytracingBinder();

		// load shaders and setup program settings
		Setup();

		Bindings(this->wrapper->globals);
		RayGeneration_Bindings(this->wrapper->raygen_locals);
		Miss_Bindings(this->wrapper->miss_locals);
		HitGroup_Bindings(this->wrapper->hitGroup_locals);

		// Create signatures
		wrapper->globals->CreateSignature(
			dxWrapper->device,
			D3D12_ROOT_SIGNATURE_FLAG_NONE,
			wrapper->globalSignature, wrapper->globalSignatureSize);

		if (wrapper->raygen_locals->HasSomeBindings())
			wrapper->raygen_locals->CreateSignature(
				dxWrapper->device,
				D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE,
				wrapper->rayGen_Signature, wrapper->rayGen_SignatureSize);

		if (wrapper->miss_locals->HasSomeBindings())
			wrapper->miss_locals->CreateSignature(
				dxWrapper->device,
				D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE,
				wrapper->miss_Signature, wrapper->miss_SignatureSize);

		if (wrapper->hitGroup_locals->HasSomeBindings())
			wrapper->hitGroup_locals->CreateSignature(
				dxWrapper->device,
				D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE,
				wrapper->hitGroup_Signature, wrapper->hitGroup_SignatureSize);

		// Create Shader Tables
		UINT shaderIdentifierSize;
		if (dxWrapper->fallbackDevice != nullptr)
			shaderIdentifierSize = dxWrapper->fallbackDevice->GetShaderIdentifierSize();
		else // DirectX Raytracing
			shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		wrapper->raygen_shaderTable = ShaderTable(dxWrapper, shaderIdentifierSize, shaderIdentifierSize + wrapper->rayGen_SignatureSize, 1);
		wrapper->miss_shaderTable = ShaderTable(dxWrapper, shaderIdentifierSize, shaderIdentifierSize + wrapper->miss_SignatureSize, wrapper->MaxMiss);
		wrapper->group_shaderTable = ShaderTable(dxWrapper, shaderIdentifierSize, shaderIdentifierSize + wrapper->hitGroup_SignatureSize, wrapper->MaxGroups);
	}

	#pragma endregion

	#pragma region Raytracing Pipeline Implementation
	
	void CA4G::RaytracingPipelineBindings::OnSet(ICmdManager* cmdWrapper) {
		// Set the pipeline state object into the cmdList
		RTPipelineWrapper* wrapper = this->wrapper;

		ID3D12DescriptorHeap* heaps[] = {
			wrapper->dxWrapper->gpu_csu->getInnerHeap(),
			wrapper->dxWrapper->gpu_smp->getInnerHeap()
		};

		if (wrapper->dxWrapper->fallbackDevice)
			cmdWrapper->__InternalDXCmdWrapper->fallbackCmdList->SetDescriptorHeaps(2, heaps);
		else
			cmdWrapper->__InternalDXCmdWrapper->cmdList->SetDescriptorHeaps(2, heaps);
	}

	void CA4G::RaytracingPipelineBindings::OnDispatch(ICmdManager* cmdWrapper) {
	}

	#pragma endregion

	#pragma region RaytracingManager implementation

	void RaytracingManager::Setter::Pipeline(gObj<RaytracingPipelineBindings> pipeline) {
		DX_CmdWrapper* cmdWrapper = this->wrapper->__InternalDXCmdWrapper;
		cmdWrapper->currentPipeline = pipeline;

		if (this->wrapper->__InternalDXCmdWrapper->fallbackCmdList != nullptr)
			this->wrapper->__InternalDXCmdWrapper->fallbackCmdList->SetPipelineState1(pipeline->wrapper->fallbackStateObject);
		else
			this->wrapper->__InternalDXCmdWrapper->cmdList->SetPipelineState1(pipeline->wrapper->stateObject);

		pipeline->OnSet(this->wrapper);
	}

	void RaytracingManager::Setter::Program(gObj<RTProgramBase> program) {
		wrapper->__InternalDXCmdWrapper->activeProgram = program;
		wrapper->__InternalDXCmdWrapper->cmdList->SetComputeRootSignature(program->wrapper->globalSignature);
		program->wrapper->globals->__InternalBindingObject->BindToGPU(
			true,
			this->wrapper->__InternalDXCmdWrapper->dxWrapper,
			this->wrapper->__InternalDXCmdWrapper
		);
	}

	void CA4G::RaytracingManager::Setter::RayGeneration(gObj<RayGenerationHandle> shader) {
		this->wrapper->__InternalDXCmdWrapper->activeProgram->wrapper->
			BindLocal(this->wrapper->__InternalDXCmdWrapper->dxWrapper, shader);
	}
	void CA4G::RaytracingManager::Setter::Miss(gObj<MissHandle> shader, int index) {
		this->wrapper->__InternalDXCmdWrapper->activeProgram->wrapper->
			BindLocal(this->wrapper->__InternalDXCmdWrapper->dxWrapper, shader, index);
	}
	void CA4G::RaytracingManager::Setter::HitGroup(gObj<HitGroupHandle> shader, int geometryIndex,
		int rayContribution, int multiplier, int instanceContribution) {
		int index = rayContribution + (geometryIndex * multiplier) + instanceContribution;
		this->wrapper->__InternalDXCmdWrapper->activeProgram->wrapper->
			BindLocal(this->wrapper->__InternalDXCmdWrapper->dxWrapper, shader, index);
	}

	void RaytracingManager::Dispatcher::Rays(int width, int height, int depth) {
		auto currentBindings = manager->__InternalDXCmdWrapper->currentPipeline.Dynamic_Cast<RaytracingPipelineBindings>();
		auto currentProgram = manager->__InternalDXCmdWrapper->activeProgram;

		if (!currentBindings)
			return; // Exception?

		D3D12_DISPATCH_RAYS_DESC d;
		auto rtRayGenShaderTable = currentProgram->wrapper->raygen_shaderTable;
		auto rtMissShaderTable = currentProgram->wrapper->miss_shaderTable;
		auto rtHGShaderTable = currentProgram->wrapper->group_shaderTable;

		auto DispatchRays = [&](auto commandList, auto stateObject, auto* dispatchDesc)
		{
			dispatchDesc->HitGroupTable.StartAddress = rtHGShaderTable.Buffer->GetGPUVirtualAddress();
			dispatchDesc->HitGroupTable.SizeInBytes = rtHGShaderTable.Buffer->GetDesc().Width;
			dispatchDesc->HitGroupTable.StrideInBytes = rtHGShaderTable.ShaderRecordSize;

			dispatchDesc->MissShaderTable.StartAddress = rtMissShaderTable.Buffer->GetGPUVirtualAddress();
			dispatchDesc->MissShaderTable.SizeInBytes = rtMissShaderTable.Buffer->GetDesc().Width;
			dispatchDesc->MissShaderTable.StrideInBytes = rtMissShaderTable.ShaderRecordSize;

			dispatchDesc->RayGenerationShaderRecord.StartAddress = rtRayGenShaderTable.Buffer->GetGPUVirtualAddress();
			dispatchDesc->RayGenerationShaderRecord.SizeInBytes = rtRayGenShaderTable.Buffer->GetDesc().Width;

			dispatchDesc->Width = width;
			dispatchDesc->Height = height;
			dispatchDesc->Depth = depth;
			commandList->DispatchRays(dispatchDesc);
		};

		D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
		if (manager->__InternalDXCmdWrapper->fallbackCmdList)
		{
			DispatchRays(manager->__InternalDXCmdWrapper->fallbackCmdList, currentBindings->wrapper->fallbackStateObject, &dispatchDesc);
		}
		else // DirectX Raytracing
		{
			DispatchRays(manager->__InternalDXCmdWrapper->cmdList, currentBindings->wrapper->stateObject, &dispatchDesc);
		}

		D3D12_RESOURCE_BARRIER b = { };
		b.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		b.UAV.pResource = nullptr;
		manager->__InternalDXCmdWrapper->cmdList->ResourceBarrier(1, &b);
	}

	#pragma endregion
}