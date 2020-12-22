#include "ca4g_pipelines.h"
#include "ca4g_collections.h"
#include "private_ca4g_presenter.h"
#include "private_ca4g_definitions.h"

namespace CA4G {

	// Represents a binding of a resource (or resource array) to a shader slot.
	struct SlotBinding {
		// Gets or sets the root parameter bound
		D3D12_ROOT_PARAMETER Root_Parameter;

		union {
			struct ConstantData {
				// Gets the pointer to a constant buffer in memory.
				void* ptrToConstant;
			} ConstantData;
			struct DescriptorData
			{
				// Determines the dimension of the bound resource
				D3D12_RESOURCE_DIMENSION Dimension;
				// Gets the pointer to a resource field (pointer to ResourceView)
				// or to the array of resources
				void* ptrToResourceViewArray;
				// Gets the pointer to the number of resources in array
				int* ptrToCount;
			} DescriptorData;
			struct SceneData {
				void* ptrToScene;
			} SceneData;
		};
	};
	
	struct InternalBindings {
		// Used to collect all constant, shader or unordered views bindings
		list<SlotBinding> __CSU;
		// Used to collect all sampler bindings
		list<SlotBinding> __Samplers;

		// Used to collect all render targets bindings
		gObj<Texture2D>* RenderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		// Used to collect all render targets bindings
		D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		// Gets or sets the maxim slot of bound render target
		int RenderTargetMax = 0;
		// Gets the bound depth buffer resource
		gObj<Texture2D>* DepthBufferField = nullptr;

		// Used to collect static samplers
		D3D12_STATIC_SAMPLER_DESC Static_Samplers[32];
		// Gets or sets the maxim sampler slot used
		int StaticSamplerMax = 0;

		Engine EngineType;

		// Table with all descriptor ranges bound by this pipeline bindings object.
		// This list must be on the CPU during bindings.
		list<D3D12_DESCRIPTOR_RANGE> ranges;

		int startRootParameter = 0;

		void BindToGPU(DX_Wrapper* dxwrapper, DX_CmdWrapper* cmdWrapper) {
			auto device = dxwrapper->device;
			auto cmdList = cmdWrapper->cmdList;
			// Foreach bound slot
			for (int i = 0; i < __CSU.size(); i++)
			{
				SlotBinding binding = __CSU[i];

				switch (binding.Root_Parameter.ParameterType)
				{
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					switch (this->EngineType) {
					case Engine::Compute:
					case Engine::Raytracing:
						cmdList->SetComputeRoot32BitConstants(startRootParameter + i, binding.Root_Parameter.Constants.Num32BitValues,
							binding.ConstantData.ptrToConstant, 0);
						break;
					case Engine::Direct:
						cmdList->SetGraphicsRoot32BitConstants(startRootParameter + i, binding.Root_Parameter.Constants.Num32BitValues,
							binding.ConstantData.ptrToConstant, 0);
						break;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
				{
#pragma region DESCRIPTOR TABLE
					// Gets the range length (if bound an array) or 1 if single.
					int count = binding.DescriptorData.ptrToCount == nullptr ? 1 : *binding.DescriptorData.ptrToCount;

					// Gets the bound resource if single
					gObj<ResourceView> resource = binding.DescriptorData.ptrToCount == nullptr ? *((gObj<ResourceView>*)binding.DescriptorData.ptrToResourceViewArray) : nullptr;

					// Gets the bound resources if array or treat the resource as a single array case
					gObj<ResourceView>* resourceArray = binding.DescriptorData.ptrToCount == nullptr ? &resource
						: *((gObj<ResourceView>**)binding.DescriptorData.ptrToResourceViewArray);

					// foreach resource in bound array (or single resource treated as array)
					for (int j = 0; j < count; j++)
					{
						// reference to the j-th resource (or bind null if array is null)
						gObj<ResourceView> resource = resourceArray == nullptr ? nullptr : *(resourceArray + j);

						if (!resource)
							// Grant a resource view to create null descriptor if missing resource.
							resource = DX_Wrapper::ResolveNullView(binding.DescriptorData.Dimension);
						else
						{
							switch (binding.Root_Parameter.DescriptorTable.pDescriptorRanges[0].RangeType)
							{
							case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							{
								D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
								switch (this->EngineType)
								{
								case Engine::Compute:
								case Engine::Raytracing:
									state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
									break;
								case Engine::Direct:
									state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
									break;
								}

								DX_ResourceWrapper* iresource = DX_Wrapper::InternalResource(resource);
								iresource->AddBarrier(cmdList, state);
							}
							break;
							case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
								DX_ResourceWrapper* iresource = DX_Wrapper::InternalResource(resource);
								iresource->AddBarrier(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
								break;
							}
						}
						// Gets the cpu handle at not visible descriptor heap for the resource
						DX_ViewWrapper* vresource = DX_Wrapper::InternalView(resource);
						D3D12_CPU_DESCRIPTOR_HANDLE handle = vresource->GetCPUHandleFor(binding.Root_Parameter.DescriptorTable.pDescriptorRanges[0].RangeType);

						// Adds the handle of the created descriptor into the src list.
						cmdWrapper->srcDescriptors.add(handle);
					}
					// add the descriptors range length
					cmdWrapper->dstDescriptorRangeLengths.add(count);
					int startIndex = dxwrapper->gpu_csu->AllocateInFrame(count);
					cmdWrapper->dstDescriptors.add(dxwrapper->gpu_csu->getCPUVersion(startIndex));

					switch (this->EngineType) {
					case Engine::Compute:
					case Engine::Raytracing:
						cmdList->SetComputeRootDescriptorTable(startRootParameter + i, dxwrapper->gpu_csu->getGPUVersion(startIndex));
						break;
					case Engine::Direct:
						cmdList->SetGraphicsRootDescriptorTable(startRootParameter + i, dxwrapper->gpu_csu->getGPUVersion(startIndex));
						break;
					}
					break; // DESCRIPTOR TABLE
#pragma endregion
				}
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
				{
#pragma region DESCRIPTOR CBV
					// Gets the range length (if bound an array) or 1 if single.
					gObj<ResourceView> resource = *((gObj<ResourceView>*)binding.DescriptorData.ptrToResourceViewArray);
					DX_ResourceWrapper* iresource = DX_Wrapper::InternalResource(resource);

					switch (this->EngineType) {
					case Engine::Compute:
					case Engine::Raytracing:
						cmdList->SetComputeRootConstantBufferView(startRootParameter + i, iresource->resource->GetGPUVirtualAddress());
						break;
					case Engine::Direct:
						cmdList->SetGraphicsRootConstantBufferView(startRootParameter + i, iresource->resource->GetGPUVirtualAddress());
						break;
					}
					break; // DESCRIPTOR CBV
#pragma endregion
				}
				}
			}
		}
	};

	struct InternalStaticPipelineWrapper {
		gObj<ComputeBinder> globalBinder = nullptr;
		gObj<ComputeBinder> localBinder = nullptr;
		DX_Wrapper* wrapper = nullptr;
		ID3D12RootSignature* rootSignature = nullptr;
		ID3D12PipelineState* pso = nullptr;
	};

#pragma region Binding Methods

	ComputeBinder::ComputeBinder() :set(new Setting(this)) {
		__InternalBindingObject = new InternalBindings();
		((InternalBindings*)__InternalBindingObject)->EngineType = Engine::Compute;
	}

	GraphicsBinder::GraphicsBinder() : ComputeBinder(), set(new Setting(this)) {
		((InternalBindings*)__InternalBindingObject)->EngineType = Engine::Direct;
	}

	RaytracingBinder::RaytracingBinder(): ComputeBinder(), set(new Setting(this)) {
		((InternalBindings*)__InternalBindingObject)->EngineType = Engine::Raytracing;
	}

#pragma endregion

	template<typename ...PSS>
	void StaticPipelineBindings<PSS...>::OnLoad(IDXWrapper* dxWrapper) {

		// Call setup to set all states of the setting object.
		Setup();

		// When a pipeline is loaded it should collect all bindings for global and local events.
		InternalStaticPipelineWrapper* wrapper = new InternalStaticPipelineWrapper();
		this->__InternalStaticPipelineWrapper = wrapper;
		DX_Wrapper* dxwrapper = (DX_Wrapper*)dxWrapper->__InternalDXWrapper;
		wrapper->wrapper = dxwrapper;
		wrapper->globalBinder = this->OnCollectGlobalBindings();
		wrapper->localBinder = this->OnCollectLocalBindings();
		InternalBindings* globalBindings = (InternalBindings*)wrapper->globalBinder->__InternalBindingObject;
		InternalBindings* localBindings = (InternalBindings*)wrapper->localBinder->__InternalBindingObject;
		localBindings->startRootParameter = globalBindings->__CSU.size() + globalBindings->__Samplers.size();

		// Create the root signature for both groups together
		#pragma region Creating Root Signature

		D3D12_ROOT_PARAMETER* parameters = new D3D12_ROOT_PARAMETER[globalBindings->__CSU.size() + globalBindings->__Samplers.size() + localBindings->__CSU.size() + localBindings->__Samplers.size()];
		int index = 0;
		for (int i = 0; i < globalBindings->__CSU.size(); i++)
			parameters[index++] = globalBindings->__CSU[i].Root_Parameter;

		for (int i = 0; i < globalBindings->__Samplers.size(); i++)
			parameters[index++] = globalBindings->__Samplers[i].Root_Parameter;

		for (int i = 0; i < localBindings->__CSU.size(); i++)
			parameters[index++] = localBindings->__CSU[i].Root_Parameter;

		for (int i = 0; i < localBindings->__Samplers.size(); i++)
			parameters[index++] = localBindings->__Samplers[i].Root_Parameter;

		D3D12_ROOT_SIGNATURE_DESC desc;
		desc.pParameters = parameters;
		desc.NumParameters = index;
		desc.pStaticSamplers = globalBindings->Static_Samplers;
		desc.NumStaticSamplers = globalBindings->StaticSamplerMax;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		if (!Using_VS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
		if (!Using_PS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
		if (!Using_GS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		if (!Using_HS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		if (!Using_DS()) desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		ID3DBlob* signatureBlob;
		ID3DBlob* signatureErrorBlob;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC d = {};
		d.Desc_1_0 = desc;
		d.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;

		auto hr = D3D12SerializeVersionedRootSignature(&d, &signatureBlob, &signatureErrorBlob);

		if (hr != S_OK)
		{
			throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, "Error serializing root signature");
		}

		hr = dxwrapper->device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&wrapper->rootSignature));

		if (hr != S_OK)
		{
			throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, "Error creating root signature");
		}

		delete[] parameters;

		#pragma endregion

		// Create the pipeline state object
		#pragma region Create Pipeline State Object

		if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE))
		{
			((RootSignatureStateManager*)set)->SetRootSignature(wrapper->rootSignature);
		}
		if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS))
		{
			RenderTargetFormatsStateManager* rtfsm = (RenderTargetFormatsStateManager*)set;
			if (rtfsm->_Description.NumRenderTargets == 0)
				rtfsm->AllRenderTargets(8, DXGI_FORMAT_R8G8B8A8_UNORM);
		}
		if (HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT))
		{
			DepthStencilFormatStateManager* dsfsm = (DepthStencilFormatStateManager*)set;
			dsfsm->_Description = DXGI_FORMAT_D32_FLOAT;
		}
		const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { sizeof(*set), set };
		hr = dxwrapper->device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&wrapper->pso));
		if (FAILED(hr)) {
			auto _hr = dxwrapper->device->GetDeviceRemovedReason();
			throw CA4GException::FromError(CA4G_Errors_BadPSOConstruction, nullptr, hr);
		}

		#pragma endregion
	}

	template<typename ...PSS>
	void StaticPipelineBindings<PSS...>::OnSet(ICmdWrapper* cmdWrapper) {
		gObj<ComputeBinder> binder = ((InternalStaticPipelineWrapper*)this->__InternalStaticPipelineWrapper)->globalBinder;
		InternalStaticPipelineWrapper* wrapper = (InternalStaticPipelineWrapper*)this->__InternalStaticPipelineWrapper;
		((InternalBindings*)binder->__InternalBindingObject)->BindToGPU(wrapper->wrapper, (DX_CmdWrapper*) cmdWrapper->__InternalDXCmdWrapper);
	}

	template<typename ...PSS>
	void StaticPipelineBindings<PSS...>::OnDispatch(ICmdWrapper* cmdWrapper) {
		gObj<ComputeBinder> binder = ((InternalStaticPipelineWrapper*)this->__InternalStaticPipelineWrapper)->localBinder;
		InternalStaticPipelineWrapper* wrapper = (InternalStaticPipelineWrapper*)this->__InternalStaticPipelineWrapper;
		((InternalBindings*)binder->__InternalBindingObject)->BindToGPU(wrapper->wrapper, (DX_CmdWrapper*)cmdWrapper->__InternalDXCmdWrapper);
	}
}