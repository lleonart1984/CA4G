#ifndef PRIVATE_PIPELINES_H
#define PRIVATE_PIPELINES_H

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
		// Used to collect all constant, shader, unordered views or sampler bindings
		list<SlotBinding> bindings;

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

		int globalBindings = 0;

		void AddBinding(bool collectingGlobal, const SlotBinding& binding) {
			if (collectingGlobal) {
				bindings.push(binding);
				globalBindings++;
			}
			else
			{
				bindings.add(binding);
			}
		}

		void BindToGPU(bool global, DX_Wrapper* dxwrapper, DX_CmdWrapper* cmdWrapper) {
			auto device = dxwrapper->device;
			auto cmdList = cmdWrapper->cmdList;

			int startRootParameter = global ? 0 : globalBindings;
			int endRootParameter = global ? globalBindings : bindings.size();

			if (global /*Only for global bindings*/)
			{
#pragma region Bind Render Targets and DepthBuffer if any
				if (EngineType == Engine::Direct)
				{
					for (int i = 0; i < RenderTargetMax; i++)
						if (!(*RenderTargets[i]))
							RenderTargetDescriptors[i] = dxwrapper->ResolveNullRTVDescriptor();
						else
						{
							DX_ResourceWrapper* iresource = DX_Wrapper::InternalResource(*RenderTargets[i]);
							DX_ViewWrapper* vresource = DX_Wrapper::InternalView(*RenderTargets[i]);
							iresource->AddBarrier(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

							RenderTargetDescriptors[i] = vresource->getRTVHandle();
						}

					D3D12_CPU_DESCRIPTOR_HANDLE depthHandle;
					if (!(DepthBufferField == nullptr || !(*DepthBufferField)))
					{
						DX_ViewWrapper* vresource = DX_Wrapper::InternalView(*DepthBufferField);
						depthHandle = vresource->getDSVHandle();
					}

					cmdList->OMSetRenderTargets(RenderTargetMax, RenderTargetDescriptors, FALSE, (DepthBufferField == nullptr || !(*DepthBufferField)) ? nullptr : &depthHandle);
				}
				ID3D12DescriptorHeap* heaps[] = { dxwrapper->gpu_csu->getInnerHeap(), dxwrapper->gpu_smp->getInnerHeap() };
				cmdList->SetDescriptorHeaps(2, heaps);
#pragma endregion
			}

			// Foreach bound slot
			for (int i = startRootParameter; i < endRootParameter; i++)
			{
				SlotBinding binding = bindings[i];

				switch (binding.Root_Parameter.ParameterType)
				{
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					switch (this->EngineType) {
					case Engine::Compute:
					case Engine::Raytracing:
						cmdList->SetComputeRoot32BitConstants(i, binding.Root_Parameter.Constants.Num32BitValues,
							binding.ConstantData.ptrToConstant, 0);
						break;
					case Engine::Direct:
						cmdList->SetGraphicsRoot32BitConstants(i, binding.Root_Parameter.Constants.Num32BitValues,
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
							resource = dxwrapper->ResolveNullView(dxwrapper, binding.DescriptorData.Dimension);
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
								iresource->AddBarrier(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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
						cmdList->SetComputeRootDescriptorTable(i, dxwrapper->gpu_csu->getGPUVersion(startIndex));
						break;
					case Engine::Direct:
						cmdList->SetGraphicsRootDescriptorTable(i, dxwrapper->gpu_csu->getGPUVersion(startIndex));
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
						cmdList->SetComputeRootConstantBufferView(i, iresource->resource->GetGPUVirtualAddress());
						break;
					case Engine::Direct:
						cmdList->SetGraphicsRootConstantBufferView(i, iresource->resource->GetGPUVirtualAddress());
						break;
					}
					break; // DESCRIPTOR CBV
#pragma endregion
				}
				}
			}
		}
	};

	struct InternalPipelineWrapper {
		// Gets or sets the binder.
		gObj<ComputeBinder> binder = nullptr;
		// DX12 wrapper.
		DX_Wrapper* wrapper = nullptr;
		// Gets or sets the root signature for this bindings
		ID3D12RootSignature* rootSignature = nullptr;
		// Gets or sets the size of the root signature.
		int rootSignatureSize;
		// Gets or sets the pipeline object built for the pipeline
		ID3D12PipelineState* pso = nullptr;

		// Builds the root signature according to the binder object.
		void BuildRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags) {
#pragma region Creating Root Signature

			auto bindings = binder->__InternalBindingObject->bindings;

			D3D12_ROOT_PARAMETER* parameters = new D3D12_ROOT_PARAMETER[bindings.size()];
			for (int i = 0; i < bindings.size(); i++)
				parameters[i] = bindings[i].Root_Parameter;

			D3D12_ROOT_SIGNATURE_DESC desc;
			desc.pParameters = parameters;
			desc.NumParameters = bindings.size();
			desc.pStaticSamplers = binder->__InternalBindingObject->Static_Samplers;
			desc.NumStaticSamplers = binder->__InternalBindingObject->StaticSamplerMax;
			desc.Flags = flags;

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

			hr = wrapper->device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

			if (hr != S_OK)
			{
				throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, "Error creating root signature");
			}

			delete[] parameters;

#pragma endregion
		}
	};


}

#endif