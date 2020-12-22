#ifndef CA4G_PIPELINES_H
#define CA4G_PIPELINES_H

#include "ca4g_presenter.h"
#include "ca4g_dxr_support.h"

namespace CA4G {

	// Represents the shader stage a resource is bound to.
	enum class ShaderType : int {
		// Resource is bound to pixel and vertex stages in the same slot.
		Any = D3D12_SHADER_VISIBILITY_ALL,
		// Resource is bound to vertex stage.
		Vertex = D3D12_SHADER_VISIBILITY_VERTEX,
		// Resource is bound to geometry stage.
		Geometry = D3D12_SHADER_VISIBILITY_GEOMETRY,
		// Resource is bound to pixel stage.
		Pixel = D3D12_SHADER_VISIBILITY_PIXEL,
		// Resource is bound to hull stage.
		Hull = D3D12_SHADER_VISIBILITY_HULL,
		// Resource is bound to domain stage.
		Domain = D3D12_SHADER_VISIBILITY_DOMAIN
	};

	// Represents the element type of a vertex field.
	enum class VertexElementType {
		// Each component of this field is a signed integer
		Int = 0,
		// Each component of this field is an unsigned integer
		UInt = 1,
		// Each component of this field is a floating value
		Float = 2
	};

#pragma region Vertex Description

	static DXGI_FORMAT TYPE_VS_COMPONENTS_FORMATS[3][4]{
		/*Int*/{ DXGI_FORMAT_R32_SINT, DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32B32_SINT,DXGI_FORMAT_R32G32B32A32_SINT },
		/*Unt*/{ DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32G32_UINT, DXGI_FORMAT_R32G32B32_UINT,DXGI_FORMAT_R32G32B32A32_UINT },
		/*Float*/{ DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT,DXGI_FORMAT_R32G32B32A32_FLOAT },
	};

	// Basic struct for constructing vertex element descriptions
	struct VertexElement {
		// Type for each field component
		const VertexElementType Type;
		// Number of components
		const int Components;
		// String with the semantic
		LPCSTR const Semantic;
		// Index for indexed semantics
		const int SemanticIndex;
		// Buffer slot this field will be contained.
		const int Slot;
	public:
		constexpr VertexElement(
			VertexElementType Type,
			int Components,
			LPCSTR const Semantic,
			int SemanticIndex = 0,
			int Slot = 0
		) :Type(Type), Components(Components), Semantic(Semantic), SemanticIndex(SemanticIndex), Slot(Slot)
		{
		}
		// Creates a Dx12 description using this information.
		D3D12_INPUT_ELEMENT_DESC createDesc(int offset, int& size) const {
			D3D12_INPUT_ELEMENT_DESC d = {};
			d.AlignedByteOffset = offset;
			d.Format = TYPE_VS_COMPONENTS_FORMATS[(int)Type][Components - 1];
			d.InputSlot = Slot;
			d.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			d.SemanticIndex = SemanticIndex;
			d.SemanticName = Semantic;
			size += 4 * Components;
			return d;
		}
	};

#pragma endregion

#pragma region Pipeline Subobject states
	
	struct DefaultStateValue {

		operator D3D12_RASTERIZER_DESC () {
			D3D12_RASTERIZER_DESC d = {};
			d.FillMode = D3D12_FILL_MODE_SOLID;
			d.CullMode = D3D12_CULL_MODE_NONE;
			d.FrontCounterClockwise = FALSE;
			d.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			d.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			d.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			d.DepthClipEnable = TRUE;
			d.MultisampleEnable = FALSE;
			d.AntialiasedLineEnable = FALSE;
			d.ForcedSampleCount = 0;
			d.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			return d;
		}

		operator D3D12_DEPTH_STENCIL_DESC () {
			D3D12_DEPTH_STENCIL_DESC d = { };

			d.DepthEnable = FALSE;
			d.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			d.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			d.StencilEnable = FALSE;
			d.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			d.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			d.FrontFace = defaultStencilOp;
			d.BackFace = defaultStencilOp;

			return d;
		}

		operator D3D12_BLEND_DESC () {
			D3D12_BLEND_DESC d = { };
			d.AlphaToCoverageEnable = FALSE;
			d.IndependentBlendEnable = FALSE;
			const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
			{
				FALSE,FALSE,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
			};
			for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				d.RenderTarget[i] = defaultRenderTargetBlendDesc;
			return d;
		}

		operator DXGI_SAMPLE_DESC() {
			DXGI_SAMPLE_DESC d = { };
			d.Count = 1;
			d.Quality = 0;
			return d;
		}

		operator D3D12_DEPTH_STENCIL_DESC1 () {
			D3D12_DEPTH_STENCIL_DESC1 _Description = { };
			_Description.DepthEnable = TRUE;
			_Description.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			_Description.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			_Description.StencilEnable = FALSE;
			_Description.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			_Description.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			_Description.FrontFace = defaultStencilOp;
			_Description.BackFace = defaultStencilOp;
			_Description.DepthBoundsTestEnable = false;
			return _Description;
		}
	};

	struct DefaultSampleMask {
		operator UINT()
		{
			return UINT_MAX;
		}
	};

	struct DefaultTopology {
		operator D3D12_PRIMITIVE_TOPOLOGY_TYPE() {
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		}
	};

	// Adapted from d3dx12 class
	template<typename Description, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename DefaultValue = Description>
	struct alignas(void*) PipelineSubobjectState {
		template<typename ...A> friend class PipelineBindings;

		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE _Type;
		Description _Description;

		inline Description& getDescription() {
			return _Description;
		}

		PipelineSubobjectState() noexcept : _Type(Type), _Description(DefaultValue()) {}
		PipelineSubobjectState(Description const& d) : _Type(Type), _Description(d) {}
		PipelineSubobjectState& operator=(Description const& i) { _Description = i; return *this; }
		operator Description() const { return _Description; }
		operator Description& () { return _Description; }

		static const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE PipelineState_Type = Type;
	};

	struct DebugStateManager : public PipelineSubobjectState< D3D12_PIPELINE_STATE_FLAGS, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS> {
		void Debug() { _Description = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG; }
		void NoDebug() { _Description = D3D12_PIPELINE_STATE_FLAG_NONE; }
	};

	struct NodeMaskStateManager : public PipelineSubobjectState< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK> {
		void ExecutionAt(int node) {
			_Description = 1 << node;
		}
		void ExecutionSingleAdapter() {
			_Description = 0;
		}
	};

	struct RootSignatureStateManager : public PipelineSubobjectState< ID3D12RootSignature*, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE> {
		template<typename ...A> friend class StaticPipelineBindings;
	private:
		void SetRootSignature(ID3D12RootSignature* rootSignature) {
			_Description = rootSignature;
		}
	};

	struct InputLayoutStateManager : public PipelineSubobjectState< D3D12_INPUT_LAYOUT_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT> {
		void InputLayout(std::initializer_list<VertexElement> elements) {
			if (_Description.pInputElementDescs != nullptr)
				delete[] _Description.pInputElementDescs;
			auto layout = new D3D12_INPUT_ELEMENT_DESC[elements.size()];
			int offset = 0;
			auto current = elements.begin();
			for (int i = 0; i < elements.size(); i++)
			{
				int size = 0;
				layout[i] = current->createDesc(offset, size);
				offset += size;
				current++;
			}
			_Description.pInputElementDescs = layout;
			_Description.NumElements = elements.size();
		}

		template<int N>
		void InputLayout(VertexElement(&elements)[N]) {
			if (_Description.pInputElementDescs != nullptr)
				delete[] _Description.pInputElementDescs;
			auto layout = new D3D12_INPUT_ELEMENT_DESC[N];
			int offset = 0;
			for (int i = 0; i < N; i++)
			{
				int size = 0;
				layout[i] = elements[i]->createDesc(offset, size);
				offset += size;
			}
			_Description.pInputElementDescs = layout;
			_Description.NumElements = N;
		}
	};

	struct IndexBufferStripStateManager : public PipelineSubobjectState< D3D12_INDEX_BUFFER_STRIP_CUT_VALUE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE> {
		void DisableIndexBufferCut() {
			_Description = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		}
		void IndexBufferCutAt32BitMAX() {
			_Description = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
		}
		void IndexBufferCutAt64BitMAX() {
			_Description = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
		}
	};

	struct PrimitiveTopologyStateManager : public PipelineSubobjectState< D3D12_PRIMITIVE_TOPOLOGY_TYPE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY, DefaultTopology> {
		void CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type) {
			_Description = type;
		}
		void TrianglesTopology() {
			CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		}
		void PointsTopology() {
			CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		}
		void LinesTopology() {
			CustomTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
		}
	};

	template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type>
	struct ShaderStageStateManager : public PipelineSubobjectState< D3D12_SHADER_BYTECODE, Type> {
	protected:
		void FromBytecode(const D3D12_SHADER_BYTECODE& shaderBytecode) {
			this->_Description = shaderBytecode;
		}
	};

	struct VertexShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS> {
		void VertexShader(const D3D12_SHADER_BYTECODE& bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct PixelShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS> {
		void PixelShader(const D3D12_SHADER_BYTECODE& bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct GeometryShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS> {
		void GeometryShader(const D3D12_SHADER_BYTECODE& bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct HullShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS> {
		void HullShader(const D3D12_SHADER_BYTECODE& bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct DomainShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS> {
		void DomainShader(const D3D12_SHADER_BYTECODE& bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct ComputeShaderStageStateManager : public ShaderStageStateManager <D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS> {
		void ComputeShader(const D3D12_SHADER_BYTECODE& bytecode) {
			ShaderStageStateManager::FromBytecode(bytecode);
		}
	};

	struct StreamOutputStateManager : public PipelineSubobjectState< D3D12_STREAM_OUTPUT_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT> {
	};

	struct BlendingStateManager : public PipelineSubobjectState< D3D12_BLEND_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND, DefaultStateValue> {
		void AlphaToCoverage(bool enable = true) {
			_Description.AlphaToCoverageEnable = enable;
		}
		void IndependentBlend(bool enable = true) {
			_Description.IndependentBlendEnable = enable;
		}
		void BlendAtRenderTarget(
			int renderTarget = 0,
			bool enable = true,
			D3D12_BLEND_OP operation = D3D12_BLEND_OP_ADD,
			D3D12_BLEND src = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dst = D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP alphaOperation = D3D12_BLEND_OP_MAX,
			D3D12_BLEND srcAlpha = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dstAlpha = D3D12_BLEND_DEST_ALPHA,
			bool enableLogicOperation = false,
			D3D12_LOGIC_OP logicOperation = D3D12_LOGIC_OP_COPY
		) {
			D3D12_RENDER_TARGET_BLEND_DESC d;
			d.BlendEnable = enable;
			d.BlendOp = operation;
			d.BlendOpAlpha = alphaOperation;
			d.SrcBlend = src;
			d.DestBlend = dst;
			d.SrcBlendAlpha = srcAlpha;
			d.DestBlendAlpha = dstAlpha;
			d.LogicOpEnable = enableLogicOperation;
			d.LogicOp = logicOperation;
			d.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			_Description.RenderTarget[renderTarget] = d;
		}
		void BlendForAllRenderTargets(
			bool enable = true,
			D3D12_BLEND_OP operation = D3D12_BLEND_OP_ADD,
			D3D12_BLEND src = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dst = D3D12_BLEND_INV_SRC_ALPHA,
			D3D12_BLEND_OP alphaOperation = D3D12_BLEND_OP_MAX,
			D3D12_BLEND srcAlpha = D3D12_BLEND_SRC_ALPHA,
			D3D12_BLEND dstAlpha = D3D12_BLEND_DEST_ALPHA,
			bool enableLogicOperation = false,
			D3D12_LOGIC_OP logicOperation = D3D12_LOGIC_OP_COPY
		) {
			D3D12_RENDER_TARGET_BLEND_DESC d;
			d.BlendEnable = enable;
			d.BlendOp = operation;
			d.BlendOpAlpha = alphaOperation;
			d.SrcBlend = src;
			d.DestBlend = dst;
			d.SrcBlendAlpha = srcAlpha;
			d.DestBlendAlpha = dstAlpha;
			d.LogicOpEnable = enableLogicOperation;
			d.LogicOp = logicOperation;
			d.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
				_Description.RenderTarget[i] = d;
		}
		void BlendDisabledAtRenderTarget(int renderTarget) {
			BlendAtRenderTarget(renderTarget, false);
		}
		void BlendDisabled() {
			BlendForAllRenderTargets(false);
		}
	};

	struct DepthStencilStateManager : public PipelineSubobjectState< D3D12_DEPTH_STENCIL_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL, DefaultStateValue> {
		void NoDepthTest() {
			_Description = {};
		}
		void DepthTest(bool enable = true, bool writeDepth = true, D3D12_COMPARISON_FUNC comparison = D3D12_COMPARISON_FUNC_LESS_EQUAL) {
			_Description.DepthEnable = enable;
			_Description.DepthWriteMask = writeDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			_Description.DepthFunc = comparison;
		}
		void StencilTest(bool enable = true, byte readMask = 0xFF, byte writeMask = 0xFF) {
			_Description.StencilEnable = enable;
			_Description.StencilReadMask = readMask;
			_Description.StencilWriteMask = writeMask;
		}
		void StencilOperationAtFront(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.FrontFace.StencilFailOp = fail;
			_Description.FrontFace.StencilDepthFailOp = depthFail;
			_Description.FrontFace.StencilPassOp = pass;
			_Description.FrontFace.StencilFunc = function;
		}
		void StencilOperationAtBack(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.BackFace.StencilFailOp = fail;
			_Description.BackFace.StencilDepthFailOp = depthFail;
			_Description.BackFace.StencilPassOp = pass;
			_Description.BackFace.StencilFunc = function;
		}
	};

	struct DepthStencilWithDepthBoundsStateManager : public PipelineSubobjectState< D3D12_DEPTH_STENCIL_DESC1, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1, DefaultStateValue> {
		void DepthTest(bool enable = true, bool writeDepth = true, D3D12_COMPARISON_FUNC comparison = D3D12_COMPARISON_FUNC_LESS_EQUAL) {
			_Description.DepthEnable = enable;
			_Description.DepthWriteMask = writeDepth ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			_Description.DepthFunc = comparison;
		}
		void StencilTest(bool enable = true, byte readMask = 0xFF, byte writeMask = 0xFF) {
			_Description.StencilEnable = enable;
			_Description.StencilReadMask = readMask;
			_Description.StencilWriteMask = writeMask;
		}
		void StencilOperationAtFront(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.FrontFace.StencilFailOp = fail;
			_Description.FrontFace.StencilDepthFailOp = depthFail;
			_Description.FrontFace.StencilPassOp = pass;
			_Description.FrontFace.StencilFunc = function;
		}
		void StencilOperationAtBack(D3D12_STENCIL_OP fail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP depthFail = D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP pass = D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC function = D3D12_COMPARISON_FUNC_ALWAYS) {
			_Description.BackFace.StencilFailOp = fail;
			_Description.BackFace.StencilDepthFailOp = depthFail;
			_Description.BackFace.StencilPassOp = pass;
			_Description.BackFace.StencilFunc = function;
		}
		void DepthBoundsTest(bool enable) {
			_Description.DepthBoundsTestEnable = enable;
		}
	};

	struct DepthStencilFormatStateManager : public PipelineSubobjectState< DXGI_FORMAT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT> {
		void DepthStencilFormat(DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT) {
			_Description = format;
		}
	};

	struct RasterizerStateManager : public PipelineSubobjectState< D3D12_RASTERIZER_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER, DefaultStateValue> {
		void AntialiasedLine(bool enable = true, bool multisample = true) {
			_Description.AntialiasedLineEnable = false;
			_Description.MultisampleEnable = multisample;
		}
		void ConservativeRasterization(bool enable = true) {
			_Description.ConservativeRaster = enable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		}
		void CullMode(D3D12_CULL_MODE mode = D3D12_CULL_MODE_NONE) {
			_Description.CullMode = mode;
		}
		void FillMode(D3D12_FILL_MODE mode = D3D12_FILL_MODE_SOLID) {
			_Description.FillMode = mode;
		}
		void DepthBias(int depthBias = 1, float slopeScale = 0, float clamp = D3D12_FLOAT32_MAX)
		{
			_Description.DepthBias = depthBias;
			_Description.SlopeScaledDepthBias = slopeScale;
			_Description.DepthBiasClamp = clamp;
		}
		void ForcedSampleCount(UINT value) {
			_Description.ForcedSampleCount = value;
		}
	};

	struct RenderTargetFormatsStateManager : public PipelineSubobjectState< D3D12_RT_FORMAT_ARRAY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS> {
		void RenderTargetFormatAt(int slot, DXGI_FORMAT format) {
			if (slot >= _Description.NumRenderTargets)
				_Description.NumRenderTargets = slot + 1;

			_Description.RTFormats[slot] = format;
		}
		void AllRenderTargets(int numberOfRTs, DXGI_FORMAT format) {
			_Description.NumRenderTargets = numberOfRTs;
			for (int i = 0; i < numberOfRTs; i++)
				_Description.RTFormats[i] = format;
		}
	};

	struct MultisamplingStateManager : public PipelineSubobjectState< DXGI_SAMPLE_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC, DefaultStateValue> {
		void Multisampling(int count = 1, int quality = 0) {
			_Description.Count = count;
			_Description.Quality = quality;
		}
	};

	struct BlendSampleMaskStateManager : public PipelineSubobjectState< UINT, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK, DefaultSampleMask> {
		void BlendSampleMask(UINT value = UINT_MAX) {
			_Description = value;
		}
	};

	struct MultiViewInstancingStateManager : public PipelineSubobjectState< D3D12_VIEW_INSTANCING_DESC, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING, DefaultStateValue> {
		void MultipleViews(std::initializer_list<D3D12_VIEW_INSTANCE_LOCATION> views, D3D12_VIEW_INSTANCING_FLAGS flags = D3D12_VIEW_INSTANCING_FLAG_NONE) {
			_Description.ViewInstanceCount = views.size();
			auto newViews = new D3D12_VIEW_INSTANCE_LOCATION[views.size()];
			auto current = views.begin();
			for (int i = 0; i < views.size(); i++) {
				newViews[i] = *current;
				current++;
			}
			if (_Description.pViewInstanceLocations != nullptr)
				delete[] _Description.pViewInstanceLocations;
			_Description.pViewInstanceLocations = newViews;
		}
	};

	//typedef PipelineSubobjectState< D3D12_CACHED_PIPELINE_STATE, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO>                        CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO;

#pragma endregion

	class ComputeBinder {
		template <typename ...PSS> friend class StaticPipelineBindings;
	protected:
		void* __InternalBindingObject;
	public:
		ComputeBinder();

		class Setting {
			friend ComputeBinder;
		protected:
			ComputeBinder* binder;
			Setting(ComputeBinder* binder) :binder(binder) {}

			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY::D3D12_SHADER_VISIBILITY_ALL;
			int space = 0;

			void AddConstant(int slot, void* data, int size);

			void AddDescriptorRange(int slot, D3D12_DESCRIPTOR_RANGE_TYPE type, void* resource);

			void AddDescriptorRange(int initialSlot, D3D12_DESCRIPTOR_RANGE_TYPE type, void* resourceArray, int* count);

			void AddStaticSampler(int slot, const Sampler& sampler);

		public:
			// Change the space for the next bindings
			void Space(int space) { this->space = space; }

			void CBV(int slot, gObj<Buffer>& const buffer)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, (void*)&buffer);
			}

			template<typename T>
			void CBV(int slot, T& data) {
				AddConstant(slot, (void*)&data, ((sizeof(T) - 1) / 4) + 1);
			}

			void SMP_Static(int slot, const Sampler& sampler) {
				AddStaticSampler(slot, sampler);
			}

			void SMP(int slot, gObj<Sampler>& const sampler) {
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, (void*)&sampler);
			}

			void SRV(int slot, gObj<Buffer>& const buffer)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&buffer);
			}
			void SRV(int slot, gObj<Texture1D>& const texture)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&texture);
			}
			void SRV(int slot, gObj<Texture2D>& const texture)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&texture);
			}
			void SRV(int slot, gObj<Texture3D>& const texture)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&texture);
			}

			void UAV(int slot, gObj<Buffer>& const buffer)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&buffer);
			}
			void UAV(int slot, gObj<Texture1D>& const texture)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&texture);
			}
			void UAV(int slot, gObj<Texture2D>& const texture)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&texture);
			}
			void UAV(int slot, gObj<Texture3D>& const texture)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&texture);
			}

			void SMP_Array(int slot, gObj<Sampler>*& const samplers, int& const count) {
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, (void*)&samplers, &count);
			}

			void SRV_Array(int slot, gObj<Buffer>*& const buffers, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&buffers, &count);
			}
			void SRV_Array(int slot, gObj<Texture1D>*& const textures, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&textures, &count);
			}
			void SRV_Array(int slot, gObj<Texture2D>*& const textures, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&textures, &count);
			}
			void SRV_Array(int slot, gObj<Texture3D>*& const textures, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (void*)&textures, &count);
			}

			void UAV_Array(int slot, gObj<Buffer>*& const buffers, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&buffers, &count);
			}
			void UAV_Array(int slot, gObj<Texture1D>*& const textures, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&textures, &count);
			}
			void UAV_Array(int slot, gObj<Texture2D>*& const textures, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&textures, &count);
			}
			void UAV_Array(int slot, gObj<Texture3D>*& const textures, int& const count)
			{
				AddDescriptorRange(slot, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, (void*)&textures, &count);
			}
		} * const set;
	};

	class GraphicsBinder : public ComputeBinder {
	public:
		GraphicsBinder();

		class Setting : ComputeBinder::Setting {
			friend GraphicsBinder;
			Setting(GraphicsBinder* binder): ComputeBinder::Setting(binder){}

			void SetRenderTarget(int slot, void* resource);
			void SetDSV(void* resource);
		public:

			void AllBindings() {
				this->visibility = D3D12_SHADER_VISIBILITY_ALL;
			}

			void VertexShaderBindings() {
				this->visibility = D3D12_SHADER_VISIBILITY_VERTEX;
			}

			void PixelShaderBindings() {
				this->visibility = D3D12_SHADER_VISIBILITY_PIXEL;
			}

			void GeometryShaderBindings() {
				this->visibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
			}

			void HullShaderBindings() {
				this->visibility = D3D12_SHADER_VISIBILITY_HULL;
			}

			void DomainShaderBindings() {
				this->visibility = D3D12_SHADER_VISIBILITY_DOMAIN;
			}

			void RTV(int slot, gObj<Texture2D>& const texture) {
				SetRenderTarget(slot, (void*)&texture);
			}

			void DSV(gObj<Texture2D>& const texture) {
				SetDSV((void*)&texture);
			}
		} * const set;
	};

	class RaytracingBinder : public ComputeBinder {
	public:
		RaytracingBinder();
		class Setting : ComputeBinder::Setting {
			friend RaytracingBinder;
			Setting(RaytracingBinder* binder) : ComputeBinder::Setting(binder) {}
			void AddADS(int slot, void* resource);
		public:
			void ADS(int slot, gObj<RTScene>& const scene) {
				AddADS(slot, (void*)&scene);
			}
		} * const set;
	};

	template<typename PSS, typename R, typename ...A>
	unsigned long  StreamTypeBits() {
		return (1 << (int)PSS::PipelineState_Type) | StreamTypeBits<R, A...>();
	}

	template<typename PSS>
	unsigned long  StreamTypeBits() {
		return (1 << (int)PSS::PipelineState_Type);// | StreamTypeBits<A...>();
	}

	// -- Abstract pipeline object (can be Graphics or Compute)
	// Allows creation of root signatures and leaves abstract the pipeline state object creation
	template<typename ...PSS>
	class StaticPipelineBindings : public IPipelineBindings {

	private:

		void* __InternalStaticPipelineWrapper;

		Engine GetEngine() {
			if (Using_CS())
				return Engine::Compute;
			return Engine::Direct;
		}

		/// Accumulates the bits of all possible settings in this pipeline object
		unsigned long StreamBits = StreamTypeBits();

		// Gets whenever a specific setting is present in this pipeline.
		bool HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type) {
			return (StreamBits & (1 << (int)type)) != 0;
		}

#pragma region Query to know if some stage is active
		bool Using_VS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS)
				&& ((VertexShaderStageStateManager*)this->set)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_PS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS)
				&& ((PixelShaderStageStateManager*)this->set)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_GS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS)
				&& ((GeometryShaderStageStateManager*)this->set)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_HS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS)
				&& ((HullShaderStageStateManager*)this->set)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_DS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS)
				&& ((DomainShaderStageStateManager*)this->set)->getDescription().pShaderBytecode != nullptr;
		}
		bool Using_CS() {
			return HasSubobjectState(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE::D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS)
				&& ((ComputeShaderStageStateManager*)this->set)->_Description.pShaderBytecode != nullptr;
		}
#pragma endregion

		void OnLoad(IDXWrapper* dxWrapper);

		void OnSet(ICmdWrapper* cmdWrapper);

		void OnDispatch(ICmdWrapper* cmdWrapper);

	protected:

		virtual gObj<ComputeBinder> OnCollectGlobalBindings() = 0;

		virtual gObj<ComputeBinder> OnCollectLocalBindings() = 0;

		// When implemented by users, this method will setup the pipeline object after created
		// Use this method to specify how to load shaders and set other default pipeline settings
		virtual void Setup() = 0;

		// Object used to set all possible states of this pipeline
		// inheriting from different state managers.
		// This object is the pipeline settings object in DX12.
		struct PipelineStateStreamDescription : PSS...{
		} *set;
	};

	// Used as a common compute pipeline binding object
	struct ComputePipelineBindings : public StaticPipelineBindings <
		DebugStateManager,
		ComputeShaderStageStateManager,
		NodeMaskStateManager,
		RootSignatureStateManager
	> {

	private:
		gObj<ComputeBinder> OnCollectGlobalBindings() {
			gObj<ComputeBinder> binder = new ComputeBinder();
			Globals(binder);
			return binder;
		}

		gObj<ComputeBinder> OnCollectLocalBindings() {
			gObj<ComputeBinder> binder = new ComputeBinder();
			Locals(binder);
			return binder;
		}

	protected:
		virtual void Globals(gObj<ComputeBinder> binder) { }
		virtual void Locals(gObj<ComputeBinder> binder) { }
	};

	// Used as a common graphics pipeline binding object
	struct GraphicsPipelineBindings : public StaticPipelineBindings <
		DebugStateManager,
		VertexShaderStageStateManager,
		PixelShaderStageStateManager,
		DomainShaderStageStateManager,
		HullShaderStageStateManager,
		GeometryShaderStageStateManager,
		StreamOutputStateManager,
		BlendingStateManager,
		BlendSampleMaskStateManager,
		RasterizerStateManager,
		DepthStencilStateManager,
		InputLayoutStateManager,
		IndexBufferStripStateManager,
		PrimitiveTopologyStateManager,
		RenderTargetFormatsStateManager,
		DepthStencilFormatStateManager,
		MultisamplingStateManager,
		NodeMaskStateManager,
		RootSignatureStateManager
	> {
	private:
		gObj<ComputeBinder> OnCollectGlobalBindings() {
			gObj<ComputeBinder> binder = new GraphicsBinder();
			Globals(binder);
			return binder;
		}

		gObj<ComputeBinder> OnCollectLocalBindings() {
			gObj<ComputeBinder> binder = new GraphicsBinder();
			Locals(binder);
			return binder;
		}
	protected:

		virtual void Globals(gObj<GraphicsBinder> binder) { }

		virtual void Locals(gObj<GraphicsBinder> binder) { }
	};


}

#endif

