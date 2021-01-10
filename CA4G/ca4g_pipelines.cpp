#include "ca4g_pipelines.h"
#include "ca4g_collections.h"
#include "private_ca4g_presenter.h"
#include "private_ca4g_definitions.h"

#include "private_ca4g_pipelines.h"

namespace CA4G {

	
#pragma region Binding Methods

	void ComputeBinder::CreateSignature(
		DX_Device device,
		D3D12_ROOT_SIGNATURE_FLAGS flags,
		DX_RootSignature &rootSignature,
		int &rootSignatureSize
		)
	{
		auto bindings = this->__InternalBindingObject->bindings;

		D3D12_ROOT_PARAMETER* parameters = new D3D12_ROOT_PARAMETER[bindings.size()];
		
		rootSignatureSize = 0;
		for (int i = 0; i < bindings.size(); i++)
		{
			parameters[i] = bindings[i].Root_Parameter;
			
			switch (bindings[i].Root_Parameter.ParameterType) {
			case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
				rootSignatureSize += bindings[i].Root_Parameter.Constants.Num32BitValues * 4;
				break;
			default:
				rootSignatureSize += 4 * 4;
				break;
			}
		}

		D3D12_ROOT_SIGNATURE_DESC desc;
		desc.pParameters = parameters;
		desc.NumParameters = bindings.size();
		desc.pStaticSamplers = __InternalBindingObject->Static_Samplers;
		desc.NumStaticSamplers = __InternalBindingObject->StaticSamplerMax;
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

		hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

		if (hr != S_OK)
		{
			throw CA4GException::FromError(CA4G_Errors_BadSignatureConstruction, "Error creating root signature");
		}

		delete[] parameters;
	}

	ComputeBinder::ComputeBinder() :set(new Setting(this)) {
		__InternalBindingObject = new InternalBindings();
		__InternalBindingObject->EngineType = Engine::Compute;
	}

	GraphicsBinder::GraphicsBinder() : ComputeBinder(), set(new Setting(this)) {
		__InternalBindingObject->EngineType = Engine::Direct;
	}

	RaytracingBinder::RaytracingBinder(): ComputeBinder(), set(new Setting(this)) {
		__InternalBindingObject->EngineType = Engine::Compute;
	}

	bool CA4G::ComputeBinder::HasSomeBindings() {
		return 
			__InternalBindingObject->bindings.size() > 0 ||
			__InternalBindingObject->StaticSamplerMax > 0;
	}

	void CA4G::ComputeBinder::Setting::AddConstant(int slot, void* data, int size)
	{
		D3D12_ROOT_PARAMETER p = { };
		p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		p.Constants.Num32BitValues = size;
		p.Constants.RegisterSpace = space;
		p.Constants.ShaderRegister = slot;
		p.ShaderVisibility = visibility;

		SlotBinding b{  };
		b.Root_Parameter = p;
		b.ConstantData.ptrToConstant = data;
		binder->__InternalBindingObject->AddBinding(collectGlobal, b);
	}
	
	void CA4G::ComputeBinder::Setting::AddDescriptorRange(int slot, D3D12_DESCRIPTOR_RANGE_TYPE type, D3D12_RESOURCE_DIMENSION dimension, void* resource)
	{
		D3D12_ROOT_PARAMETER p = { };
		p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		p.DescriptorTable.NumDescriptorRanges = 1;
		p.ShaderVisibility = visibility;

		D3D12_DESCRIPTOR_RANGE range = { };
		range.BaseShaderRegister = slot;
		range.NumDescriptors = 1;
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		range.RangeType = type;
		range.RegisterSpace = space;

		binder->__InternalBindingObject->ranges.add(range);
		p.DescriptorTable.pDescriptorRanges = &((InternalBindings*)binder->__InternalBindingObject)->ranges.last();

		SlotBinding b{  };
		b.Root_Parameter = p;
		b.DescriptorData.Dimension = dimension;
		b.DescriptorData.ptrToResourceViewArray = resource;
		binder->__InternalBindingObject->AddBinding(collectGlobal, b);
	}

	void CA4G::ComputeBinder::Setting::AddDescriptorRange(int initialSlot, D3D12_DESCRIPTOR_RANGE_TYPE type, D3D12_RESOURCE_DIMENSION dimension, void* resourceArray, int* count)
	{
		D3D12_ROOT_PARAMETER p = { };
		p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		p.DescriptorTable.NumDescriptorRanges = 1;
		p.ShaderVisibility = visibility;
		D3D12_DESCRIPTOR_RANGE range = { };
		range.BaseShaderRegister = initialSlot;
		range.NumDescriptors = -1;// undefined this moment
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		range.RangeType = type;
		range.RegisterSpace = space;

		binder->__InternalBindingObject->ranges.add(range);
		p.DescriptorTable.pDescriptorRanges = &((InternalBindings*)binder->__InternalBindingObject)->ranges.last();

		SlotBinding b{ };
		b.Root_Parameter = p;
		b.DescriptorData.Dimension = dimension;
		b.DescriptorData.ptrToResourceViewArray = resourceArray;
		b.DescriptorData.ptrToCount = count;
		binder->__InternalBindingObject->AddBinding(collectGlobal, b);
	}

	void CA4G::ComputeBinder::Setting::AddStaticSampler(int slot, const Sampler& sampler)
	{
		D3D12_STATIC_SAMPLER_DESC desc = { };
		desc.AddressU = sampler.AddressU;
		desc.AddressV = sampler.AddressV;
		desc.AddressW = sampler.AddressW;
		desc.BorderColor =
			!any(sampler.BorderColor - float4(0, 0, 0, 0)) ?
			D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK :
			!any(sampler.BorderColor - float4(0, 0, 0, 1)) ?
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK :
			D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		desc.ComparisonFunc = sampler.ComparisonFunc;
		desc.Filter = sampler.Filter;
		desc.MaxAnisotropy = sampler.MaxAnisotropy;
		desc.MaxLOD = sampler.MaxLOD;
		desc.MinLOD = sampler.MinLOD;
		desc.MipLODBias = sampler.MipLODBias;
		desc.RegisterSpace = space;
		desc.ShaderRegister = slot;
		desc.ShaderVisibility = visibility;

		binder->__InternalBindingObject->Static_Samplers[((InternalBindings*)binder->__InternalBindingObject)->StaticSamplerMax] = desc;
		binder->__InternalBindingObject->StaticSamplerMax++;
	}

	void CA4G::GraphicsBinder::Setting::SetRenderTarget(int slot, gObj<Texture2D>& const resource)
	{
		binder->__InternalBindingObject->RenderTargets[slot] = &resource;
		binder->__InternalBindingObject->RenderTargetMax = max(((InternalBindings*)binder->__InternalBindingObject)->RenderTargetMax, slot + 1);
	}

	void CA4G::GraphicsBinder::Setting::SetDSV(gObj<Texture2D>& const resource)
	{
		binder->__InternalBindingObject->DepthBufferField = &resource;
	}

#pragma endregion

	void StaticPipelineBindingsBase::OnCreate(DX_Wrapper* dxwrapper) {

		// Call setup to set all states of the setting object.
		Setup();

		// When a pipeline is loaded it should collect all bindings for global and local events.
		InternalPipelineWrapper* wrapper = new InternalPipelineWrapper();
		this->__InternalStaticPipelineWrapper = wrapper;
		wrapper->wrapper = dxwrapper;
		wrapper->binder = this->OnCollectBindings();
		InternalBindings* bindings = wrapper->binder->__InternalBindingObject;

		// Create the root signature for both groups together
		wrapper->binder->CreateSignature(
			dxwrapper->device,
			getRootFlags(),
			wrapper->rootSignature,
			wrapper->rootSignatureSize
		);

		// Create the pipeline state object
		#pragma region Create Pipeline State Object

		CompleteStateObject(wrapper->rootSignature);

		const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { (SIZE_T)getSettingObjectSize(), getSettingObject() };
		auto hr = dxwrapper->device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&wrapper->pso));
		if (FAILED(hr)) {
			auto _hr = dxwrapper->device->GetDeviceRemovedReason();
			throw CA4GException::FromError(CA4G_Errors_BadPSOConstruction, nullptr, hr);
		}

		#pragma endregion
	}

	void StaticPipelineBindingsBase::OnSet(ICmdManager* cmdWrapper) {
		// Set the pipeline state object into the cmdList
		InternalPipelineWrapper* wrapper = (InternalPipelineWrapper*)this->__InternalStaticPipelineWrapper;
		// Set pipeline state object
		cmdWrapper->__InternalDXCmdWrapper->cmdList->SetPipelineState(wrapper->pso);
		// Set root signature
		if (this->GetEngine() == Engine::Compute)
			cmdWrapper->__InternalDXCmdWrapper->cmdList->SetComputeRootSignature(wrapper->rootSignature);
		else
			cmdWrapper->__InternalDXCmdWrapper->cmdList->SetGraphicsRootSignature(wrapper->rootSignature);
		// Bind global resources on the root signature
		gObj<ComputeBinder> binder = ((InternalPipelineWrapper*)this->__InternalStaticPipelineWrapper)->binder;
		binder->__InternalBindingObject->BindToGPU(true, wrapper->wrapper, cmdWrapper->__InternalDXCmdWrapper);
	}

	void StaticPipelineBindingsBase::OnDispatch(ICmdManager* cmdWrapper) {
		gObj<ComputeBinder> binder = ((InternalPipelineWrapper*)this->__InternalStaticPipelineWrapper)->binder;
		InternalPipelineWrapper* wrapper = (InternalPipelineWrapper*)this->__InternalStaticPipelineWrapper;
		binder->__InternalBindingObject->BindToGPU(false, wrapper->wrapper, cmdWrapper->__InternalDXCmdWrapper);
	}

	D3D12_SHADER_BYTECODE ShaderLoader::FromFile(const char* bytecodeFilePath) {
		D3D12_SHADER_BYTECODE code;
		FILE* file;
		if (fopen_s(&file, bytecodeFilePath, "rb") != 0)
		{
			throw CA4GException::FromError(CA4G_Errors_ShaderNotFound);
		}
		fseek(file, 0, SEEK_END);
		long long count;
		count = ftell(file);
		fseek(file, 0, SEEK_SET);

		byte* bytecode = new byte[count];
		int offset = 0;
		while (offset < count) {
			offset += fread_s(&bytecode[offset], min(1024, count - offset), sizeof(byte), 1024, file);
		}
		fclose(file);

		code.BytecodeLength = count;
		code.pShaderBytecode = (void*)bytecode;
		return code;
	}

	D3D12_SHADER_BYTECODE ShaderLoader::FromMemory(const byte* bytecodeData, int count) {
		D3D12_SHADER_BYTECODE code;
		code.BytecodeLength = count;
		code.pShaderBytecode = (void*)bytecodeData;
		return code;
	}
}