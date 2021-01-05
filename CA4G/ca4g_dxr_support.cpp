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

	void RaytracingBinder::Setting::ADS(int slot, gObj<BakedScene>& const scene) {
		D3D12_ROOT_PARAMETER p = { };
		p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		p.Descriptor.ShaderRegister = slot;
		p.Descriptor.RegisterSpace = space;

		SlotBinding b{ };
		b.Root_Parameter = p;
		b.SceneData.ptrToScene = (void*)&scene;
		
		binder->__InternalBindingObject->AddBinding(collectGlobal, b);
	}

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
	void RaytracingManager::Setter::RayGeneration(gObj<RayGenerationHandle> shader) {
		this->wrapper->__InternalDXCmdWrapper->activeProgram->wrapper->
			BindLocal(this->wrapper->__InternalDXCmdWrapper->dxWrapper, shader);
	}
	void RaytracingManager::Setter::Miss(gObj<MissHandle> shader, int index) {
		this->wrapper->__InternalDXCmdWrapper->activeProgram->wrapper->
			BindLocal(this->wrapper->__InternalDXCmdWrapper->dxWrapper, shader, index);
	}
	void RaytracingManager::Setter::HitGroup(gObj<HitGroupHandle> shader, int geometryIndex,
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

	struct DX_GeometryCollectionWrapper {
		gObj<BakedGeometry> reused = nullptr;

		gObj<list<D3D12_RAYTRACING_GEOMETRY_DESC>> geometries = new list<D3D12_RAYTRACING_GEOMETRY_DESC>();
	
		// Triangles definitions
		DX_Resource boundVertices;
		DX_Resource boundIndices;
		DX_Resource boundTransforms;
		int currentVertexOffset = 0;
		DXGI_FORMAT currentVertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		
		// Procedural definitions
		DX_Resource aabbs;
	};

	struct DX_InstanceCollectionWrapper {
		// Used to reuse creational buffers.
		gObj<BakedScene> reusing = nullptr;
		// List with persistent reference to geometries used by the instances in this collection.
		gObj<list<gObj<BakedGeometry>>> usedGeometries = new list<gObj<BakedGeometry>>();
		// Determines if the instances are for a fallback device
		bool FallbackDeviceUsed = false;
		// Instances
		gObj<list<D3D12_RAYTRACING_INSTANCE_DESC>> instances = new list<D3D12_RAYTRACING_INSTANCE_DESC>();
		gObj<list<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC>> fallbackInstances = new list<D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC>();
	};

	

	void TriangleGeometryCollection::Setting::__SetVertices(gObj<Buffer> vertices)
	{
		manager->wrapper->boundVertices = vertices->__InternalDXWrapper->resource;
	}

	void TriangleGeometryCollection::Setting::__SetInputLayout(VertexElement* elements, int count)
	{
		int vertexOffset = 0;
		DXGI_FORMAT vertexFormat = DXGI_FORMAT_UNKNOWN;
		for (int i = 0; i < count; i++)
		{
			// Position was found in layout
			if (elements[i].Semantic == VertexElementSemantic::Position)
			{
				vertexFormat = elements[i].Format();
				break;
			}

			vertexOffset += elements[i].Components * 4;
		}
		manager->wrapper->currentVertexFormat = vertexFormat;
		manager->wrapper->currentVertexOffset = vertexOffset;
	}

	void TriangleGeometryCollection::Setting::Indices(gObj<Buffer> indices)
	{
		manager->wrapper->boundIndices = indices.isNull() ? nullptr : indices->__InternalDXWrapper->resource;
	}

	void TriangleGeometryCollection::Setting::Transforms(gObj<Buffer> transforms)
	{
		manager->wrapper->boundTransforms = transforms.isNull() ? nullptr : transforms->__InternalDXWrapper->resource;
	}

	void ProceduralGeometryCollection::Setting::Boxes(gObj<Buffer> aabbs)
	{
		manager->wrapper->aabbs = aabbs.isNull() ? nullptr : aabbs->__InternalDXWrapper->resource;
	}

	void ProceduralGeometryCollection::Loading::Geometry(int start, int count)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC desc{ };
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE::D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		//desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
		desc.AABBs.AABBCount = count;
		desc.AABBs.AABBs = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE
		{
			manager->wrapper->aabbs->GetGPUVirtualAddress() + (LONG)start * sizeof(D3D12_RAYTRACING_AABB),
			sizeof(D3D12_RAYTRACING_AABB)
		};
		manager->wrapper->geometries->add(desc);
	}

	void FillMat4x3(float(&dst)[3][4], float4x3 transform) {
		dst[0][0] = transform._m00;
		dst[0][1] = transform._m10;
		dst[0][2] = transform._m20;
		dst[0][3] = transform._m30;
		dst[1][0] = transform._m01;
		dst[1][1] = transform._m11;
		dst[1][2] = transform._m21;
		dst[1][3] = transform._m31;
		dst[2][0] = transform._m02;
		dst[2][1] = transform._m12;
		dst[2][2] = transform._m22;
		dst[2][3] = transform._m32;
	}

	void InstanceCollection::Loading::Instance(gObj<BakedGeometry> geometries, UINT mask, int instanceContribution, UINT instanceID, float4x3 transform)
	{
		auto wrapper = manager->wrapper;
		
		wrapper->usedGeometries->add(geometries);

		if (wrapper->FallbackDeviceUsed) {
			D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC d{ };
			FillMat4x3(d.Transform, transform);
			d.InstanceMask = mask;
			d.Flags = D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			//d.Flags = D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
			d.InstanceContributionToHitGroupIndex = instanceContribution;
			d.AccelerationStructure = geometries->internalObject->emulatedPtr;
			int index = wrapper->fallbackInstances->size();
			d.InstanceID = instanceID == INTSAFE_UINT_MAX ? index : instanceID;
			wrapper->fallbackInstances->add(d);
		}
		else {
			D3D12_RAYTRACING_INSTANCE_DESC d{ };
			d.Flags = D3D12_RAYTRACING_INSTANCE_FLAGS::D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			FillMat4x3(d.Transform, transform);
			d.InstanceMask = mask;
			d.InstanceContributionToHitGroupIndex = instanceContribution;
			d.AccelerationStructure = geometries->internalObject->bottomLevelAccDS->GetGPUVirtualAddress();
			int index = wrapper->instances->size();
			d.InstanceID = instanceID == INTSAFE_UINT_MAX ? index : instanceID;
			wrapper->instances->add(d);
		}
	}

	gObj<InstanceCollection> RaytracingManager::Creating::Intances(gObj<BakedScene> forReuse) {
		DX_InstanceCollectionWrapper* wrapper = new DX_InstanceCollectionWrapper();
		wrapper->reusing = forReuse;
		wrapper->FallbackDeviceUsed = manager->__InternalDXCmdWrapper->dxWrapper->fallbackDevice != nullptr;
		InstanceCollection* result = new InstanceCollection();
		result->wrapper = wrapper;
		return result;
	}

	gObj<TriangleGeometryCollection> RaytracingManager::Creating::TriangleGeometries(gObj<BakedGeometry> forReuse)
	{ 
		TriangleGeometryCollection* triangles = new TriangleGeometryCollection();
		triangles->wrapper = new DX_GeometryCollectionWrapper();
		triangles->wrapper->reused = forReuse;
		return triangles;
	}

	gObj<ProceduralGeometryCollection> RaytracingManager::Creating::ProceduralGeometries(gObj<BakedGeometry> forReuse)
	{
		ProceduralGeometryCollection* geometries = new ProceduralGeometryCollection();
		geometries->wrapper = new DX_GeometryCollectionWrapper();
		geometries->wrapper->reused = forReuse;
		return geometries;
	}

	// Create a wrapped pointer for the Fallback Layer path.
	WRAPPED_GPU_POINTER CreateFallbackWrappedPointer(DX_Wrapper* dxWrapper, DX_Resource resource, UINT bufferNumElements)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC rawBufferUavDesc = {};
		rawBufferUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		rawBufferUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		rawBufferUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		rawBufferUavDesc.Buffer.NumElements = bufferNumElements;

		D3D12_CPU_DESCRIPTOR_HANDLE bottomLevelDescriptor;

		// Only compute fallback requires a valid descriptor index when creating a wrapped pointer.
		UINT descriptorHeapIndex = 0;
		if (!dxWrapper->fallbackDevice->UsingRaytracingDriver())
		{
			descriptorHeapIndex = dxWrapper->gpu_csu->AllocatePersistent();
			bottomLevelDescriptor = dxWrapper->gpu_csu->getCPUVersion(descriptorHeapIndex);
			dxWrapper->device->CreateUnorderedAccessView(resource, nullptr, &rawBufferUavDesc, bottomLevelDescriptor);
		}
		return dxWrapper->fallbackDevice->GetWrappedPointerSimple(descriptorHeapIndex, resource->GetGPUVirtualAddress());
	}

	gObj<BakedGeometry> RaytracingManager::Creating::Baked(gObj<GeometryCollection> geometries, bool allowUpdate, bool preferFastRaycasting)
	{
		DX_BakedGeometry* baked = new DX_BakedGeometry();

		auto geometryCollection = geometries->wrapper;
		auto dxWrapper = manager->__InternalDXCmdWrapper->dxWrapper;

		// creates the bottom level acc ds and emulated gpu pointer if necessary
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags =
			(preferFastRaycasting ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD)
			| (allowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE);
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = buildFlags;
		inputs.NumDescs = geometryCollection->geometries->size();
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.pGeometryDescs = &geometryCollection->geometries->first();

		DX_Resource scratchBuffer;
		DX_Resource finalBuffer;
		WRAPPED_GPU_POINTER fallbackEmulatedPtr;

		if (geometryCollection->reused.isNull()) // Create new buffers
		{
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
			D3D12_RESOURCE_STATES initialResourceState;
			if (dxWrapper->fallbackDevice != nullptr)
			{
				dxWrapper->fallbackDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
				initialResourceState = dxWrapper->fallbackDevice->GetAccelerationStructureResourceState();
			}
			else // DirectX Raytracing
			{
				dxWrapper->device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
				initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Width = prebuildInfo.ScratchDataSizeInBytes;
			desc.Height = 1;
			desc.MipLevels = 1;
			desc.DepthOrArraySize = 1;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			desc.SampleDesc = { 1, 0 };
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			auto scraftV = CA4G::Creating::CustomDXResource(dxWrapper, 1, desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			scratchBuffer = scraftV->__InternalDXWrapper->resource;

			desc.Width = prebuildInfo.ResultDataMaxSizeInBytes;

			auto finalV = CA4G::Creating::CustomDXResource(dxWrapper, 1, desc, initialResourceState);
			finalBuffer = finalV->__InternalDXWrapper->resource;

			if (dxWrapper->fallbackDevice != nullptr) {
				// store an emulated gpu pointer via UAV
				UINT numBufferElements = static_cast<UINT>(prebuildInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
				fallbackEmulatedPtr = CreateFallbackWrappedPointer(dxWrapper, finalBuffer, numBufferElements);
			}
		}
		else { // reuse buffers
			scratchBuffer = geometryCollection->reused->internalObject->scratchBottomLevelAccDS;
			finalBuffer = geometryCollection->reused->internalObject->bottomLevelAccDS;
			fallbackEmulatedPtr = geometryCollection->reused->internalObject->emulatedPtr;
		}
		// Bottom Level Acceleration Structure desc
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
		{
			bottomLevelBuildDesc.Inputs = inputs;
			bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
			bottomLevelBuildDesc.DestAccelerationStructureData = finalBuffer->GetGPUVirtualAddress();
		}

		if (dxWrapper->fallbackDevice != nullptr)
		{
			ID3D12DescriptorHeap* pDescriptorHeaps[] = {
				dxWrapper->gpu_csu->getInnerHeap(),
				dxWrapper->gpu_smp->getInnerHeap() };
			manager->__InternalDXCmdWrapper->fallbackCmdList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);
			manager->__InternalDXCmdWrapper->fallbackCmdList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
		}
		else
			manager->__InternalDXCmdWrapper->cmdList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
		
		// Add barrier to wait for ADS construction
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.UAV.pResource = finalBuffer;
		manager->__InternalDXCmdWrapper->cmdList->ResourceBarrier(1, &barrier);

		baked->bottomLevelAccDS = finalBuffer;
		baked->scratchBottomLevelAccDS = scratchBuffer;
		baked->emulatedPtr = fallbackEmulatedPtr;
		baked->geometries = geometryCollection->geometries->clone();

		BakedGeometry* result = new BakedGeometry();
		result->internalObject = baked;
		return result;
	}

	gObj<BakedScene> RaytracingManager::Creating::Baked(gObj<InstanceCollection> instances, bool allowUpdate, bool preferFastRaycasting)
	{
		auto instanceCollection = instances->wrapper;
		auto dxWrapper = manager->__InternalDXCmdWrapper->dxWrapper;

		// Bake scene using instance buffer and generate the top level DS
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags =
			(allowUpdate ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE) |
			(preferFastRaycasting ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD);
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = buildFlags;
		if (dxWrapper->fallbackDevice != nullptr)
			inputs.NumDescs = instanceCollection->fallbackInstances->size();
		else
			inputs.NumDescs = instanceCollection->instances->size();
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		DX_Resource scratchBuffer;
		DX_Resource finalBuffer;
		WRAPPED_GPU_POINTER fallbackEmulatedPtr;
		DX_Resource instanceBuffer;
		void* instanceBufferMap;

		int instanceBufferWidth = (dxWrapper->fallbackDevice) ?
			instanceCollection->fallbackInstances->size() * sizeof(D3D12_RAYTRACING_FALLBACK_INSTANCE_DESC) :
			instanceCollection->instances->size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

		if (instanceCollection->reusing.isNull()) // create new buffers
		{
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
			D3D12_RESOURCE_STATES initialResourceState;

			if (dxWrapper->fallbackDevice != nullptr)
			{
				dxWrapper->fallbackDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
				initialResourceState = dxWrapper->fallbackDevice->GetAccelerationStructureResourceState();
			}
			else // DirectX Raytracing
			{
				dxWrapper->device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);
				initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
			}

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Height = 1;
			desc.MipLevels = 1;
			desc.DepthOrArraySize = 1;
			desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			desc.SampleDesc = { 1, 0 };
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			// Creating scraft memory for ADS construction
			desc.Width = prebuildInfo.ScratchDataSizeInBytes;
			auto scraftV = CA4G::Creating::CustomDXResource(dxWrapper, 1, desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			scratchBuffer = scraftV->__InternalDXWrapper->resource;

			// Creating top level ADS
			desc.Width = prebuildInfo.ResultDataMaxSizeInBytes;
			auto finalV = CA4G::Creating::CustomDXResource(dxWrapper, 1, desc, initialResourceState);
			finalBuffer = finalV->__InternalDXWrapper->resource;

			if (dxWrapper->fallbackDevice != nullptr) {
				// store an emulated gpu pointer via UAV
				UINT numBufferElements = static_cast<UINT>(prebuildInfo.ResultDataMaxSizeInBytes) / sizeof(UINT32);
				fallbackEmulatedPtr = CreateFallbackWrappedPointer(dxWrapper, finalBuffer, numBufferElements);
			}

			// Creating instance buffer
			desc.Width = instanceBufferWidth;
			auto instanceV = CA4G::Creating::CustomDXResource(dxWrapper, 1, desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, CPUAccessibility::Write);
			instanceBuffer = instanceV->__InternalDXWrapper->resource;

			D3D12_RANGE range = {};
			instanceBuffer->Map(0, &range, &instanceBufferMap); // Permanent map of instance buffer
		}
		else {
			scratchBuffer = instanceCollection->reusing->internalObject->scratchBuffer;
			finalBuffer = instanceCollection->reusing->internalObject->topLevelAccDS;
			fallbackEmulatedPtr = instanceCollection->reusing->internalObject->topLevelAccFallbackPtr;
			instanceBuffer = instanceCollection->reusing->internalObject->instancesBuffer;
			instanceBufferMap = instanceCollection->reusing->internalObject->instancesBufferMap;
		}

		// Update instance buffer with instances
		if (dxWrapper->fallbackDevice)
			memcpy(instanceBufferMap, (void*)&instanceCollection->fallbackInstances->first(), instanceBufferWidth);
		else
			memcpy(instanceBufferMap, (void*)&instanceCollection->instances->first(), instanceBufferWidth);

		// Build acc structure

		// Top Level Acceleration Structure desc
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
		{
			inputs.InstanceDescs = instanceBuffer->GetGPUVirtualAddress();
			topLevelBuildDesc.Inputs = inputs;
			topLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
			topLevelBuildDesc.DestAccelerationStructureData = finalBuffer->GetGPUVirtualAddress();
		}

		if (dxWrapper->fallbackDevice)
		{
			manager->__InternalDXCmdWrapper->fallbackCmdList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
		}
		else
			manager->__InternalDXCmdWrapper->cmdList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

		DX_BakedScene* baked = new DX_BakedScene();
		baked->scratchBuffer = scratchBuffer;
		baked->topLevelAccDS = finalBuffer;
		baked->topLevelAccFallbackPtr = fallbackEmulatedPtr;
		baked->instancesBuffer = instanceBuffer;
		baked->instancesBufferMap = instanceBufferMap;
		baked->usedGeometries = instanceCollection->usedGeometries->clone();
		baked->instances = instanceCollection->instances->clone();
		baked->fallbackInstances = instanceCollection->fallbackInstances->clone();
		
		BakedScene* result = new BakedScene();
		result->internalObject = baked;
		return result;
	}

	#pragma endregion
}