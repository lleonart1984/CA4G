#ifndef DXR_SUPPORT_H
#define DXR_SUPPORT_H

#include "ca4g_techniques.h"
#include "ca4g_pipelines.h"

#include <wchar.h>

namespace CA4G {

	struct RTPipelineWrapper;
	struct DX_RTProgram;
	class RTProgramBase;

#pragma region Shader handles

	class ProgramHandle {
		friend RaytracingPipelineBindings;
		friend RTProgramBase;

		LPCWSTR shaderHandle;
		void* cachedShaderIdentifier = nullptr;
	protected:
		ProgramHandle(LPCWSTR handle) :shaderHandle(handle) {}
	public:
		ProgramHandle() :shaderHandle(nullptr) {}
		ProgramHandle(const ProgramHandle& other) {
			this->shaderHandle = other.shaderHandle;
		}
		inline bool IsNull() { return shaderHandle == nullptr; }
	};

	class MissHandle : public ProgramHandle {
		friend RaytracingPipelineBindings;
		friend RTProgramBase;

		MissHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) { }
	public:
		MissHandle() : ProgramHandle() {}
		MissHandle(const MissHandle& other) : ProgramHandle(other) { }
	};

	class RayGenerationHandle : public ProgramHandle {
		friend RaytracingPipelineBindings;
		friend RTProgramBase;

		RayGenerationHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}
	public:
		RayGenerationHandle() : ProgramHandle() { }
		RayGenerationHandle(const RayGenerationHandle& other) : ProgramHandle(other) { }
	};

	class AnyHitHandle : public ProgramHandle {
		friend RaytracingPipelineBindings;
		friend RTProgramBase;

		AnyHitHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}
	public:
		AnyHitHandle() : ProgramHandle() { }
		AnyHitHandle(const AnyHitHandle& other) : ProgramHandle(other) { }
	};

	class ClosestHitHandle : public ProgramHandle {
		friend RaytracingPipelineBindings;
		friend RTProgramBase;

		ClosestHitHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}
	public:
		ClosestHitHandle() : ProgramHandle() { }
		ClosestHitHandle(const ClosestHitHandle& other) : ProgramHandle(other) { }
	};

	class IntersectionHandle : public ProgramHandle {
		friend RaytracingPipelineBindings;
		friend RTProgramBase;

		IntersectionHandle(LPCWSTR shaderHandle) : ProgramHandle(shaderHandle) {}
	public:
		IntersectionHandle() : ProgramHandle() { }
		IntersectionHandle(const IntersectionHandle& other) : ProgramHandle(other) { }
	};

	class HitGroupHandle : public ProgramHandle {
		friend RaytracingPipelineBindings;
		friend RTProgramBase;

		static LPCWSTR CreateAutogeneratedName() {
			static int ID = 0;

			wchar_t* label = new wchar_t[100];
			label[0] = 0;
			wcscat_s(label, 100, TEXT("HITGROUP"));
			wsprintf(&label[8], L"%d",
				ID);
			ID++;

			return label;
		}

		gObj<AnyHitHandle> anyHit;
		gObj<ClosestHitHandle> closestHit;
		gObj<IntersectionHandle> intersection;

		HitGroupHandle(gObj<ClosestHitHandle> closestHit, gObj<AnyHitHandle> anyHit, gObj<IntersectionHandle> intersection)
			: ProgramHandle(CreateAutogeneratedName()), anyHit(anyHit), closestHit(closestHit), intersection(intersection) {}
	public:
		HitGroupHandle() :ProgramHandle() {}
		HitGroupHandle(const HitGroupHandle& other) :ProgramHandle(other) {
			this->anyHit = other.anyHit;
			this->closestHit = other.closestHit;
			this->intersection = other.intersection;
		}
	};

#pragma endregion

	// Represents a raytracing program.
	// Internally represents a set of bindings for programs
	// and shader tables to represent accessible programs during tracing.
	class RTProgramBase {
		friend RaytracingPipelineBindings;

		DX_RTProgram* wrapper;

		// Collect bindings, create signatures and shader tables.
		void OnLoad(DX_Wrapper* dxWrapper);

	protected:
		virtual void Setup() = 0;

		// Collects global bindings for all programs.
		virtual void Bindings(gObj<RaytracingBinder> binder) {
		}

		// Collects local bindings for raygeneration program.
		virtual void RayGeneration_Bindings(gObj<RaytracingBinder> binder) {
		}

		// Collects local bindings for miss programs.
		virtual void Miss_Bindings(gObj<RaytracingBinder> binder) {
		}

		// Collects local bindings for hit group programs.
		virtual void HitGroup_Bindings(gObj<RaytracingBinder> binder) {
		}

	public:
		class Settings {
			friend RTProgramBase;
			RTProgramBase* manager;
			Settings(RTProgramBase* manager) :manager(manager) {}
		public:
			// Defines the maximum payload size for all raytracing recursive programs.
			void Payload(int sizeInBytes);
			// Defines the size of the shader table used to handle hit groups
			void MaxHitGroupIndex(int index);
			// Defines the maximum stack size for recursive shaders.
			void StackSize(int maxDeep);
			// Defines the attribute size for intersection feed back.
			void Attribute(int sizeInBytes);
		}* const set;

		class Loading {
			friend RTProgramBase;
			RTProgramBase* manager;
			Loading(RTProgramBase* manager) :manager(manager) {}
		public:
			void Shader(gObj<RayGenerationHandle> &shader);
			void Shader(gObj<MissHandle> &shader);
			void Shader(gObj<HitGroupHandle> &shader);
		} * const load;
	};

	template <typename C>
	class RTProgram : public RTProgramBase{
		friend RaytracingPipelineBindings;

		gObj<C> __Pipeline;
	protected:
		RTProgram() {}
	public:
		inline const gObj<C>& Context() {
			return __Pipeline;
		}
	};

	class RTScene {
		void* __InternalRTWrapper;
	};

	class DynamicStateBindings {

		friend RaytracingPipelineBindings;
		template<typename S, D3D12_STATE_SUBOBJECT_TYPE Type> friend class DynamicStateBindingOf;

		D3D12_STATE_SUBOBJECT* dynamicStates;

		void AllocateStates(int length) {
			dynamicStates = new D3D12_STATE_SUBOBJECT[length];
		}

		template<typename D>
		D* GetState(int index) {
			return (D*)dynamicStates[index].pDesc;
		}

		template<typename D>
		void SetState(int index, D3D12_STATE_SUBOBJECT_TYPE type, D* state) {
			dynamicStates[index].Type = type;
			dynamicStates[index].pDesc = state;
		}

	public:
		DynamicStateBindings() {}
	};

	template<typename S, D3D12_STATE_SUBOBJECT_TYPE Type>
	class DynamicStateBindingOf : public virtual DynamicStateBindings {
	protected:
		S* GetState(int index) {
			return GetState<S>(index);
		}

		S* GetAfterCreate(int index) {
			S* state = new S{ };
			SetState(index, Type, state);
			return state;
		}
	public:
	};

#pragma region Dynamic States

	struct GlobalRootSignatureManager : public DynamicStateBindingOf<D3D12_GLOBAL_ROOT_SIGNATURE, D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE> {
		void SetGlobalRootSignature(int index, ID3D12RootSignature* rootSignature) {
			auto state = GetAfterCreate(index);
			state->pGlobalRootSignature = rootSignature;
		}
	};

	struct LocalRootSignatureManager : public DynamicStateBindingOf<D3D12_LOCAL_ROOT_SIGNATURE, D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE> {
		void SetLocalRootSignature(int index, ID3D12RootSignature* rootSignature) {
			auto state = GetAfterCreate(index);
			state->pLocalRootSignature = rootSignature;
		}
	};

	struct HitGroupManager : public virtual DynamicStateBindingOf<D3D12_HIT_GROUP_DESC, D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP> {
		void SetTriangleHitGroup(int index, LPCWSTR exportName, LPCWSTR anyHit, LPCWSTR closestHit) {
			auto hg = GetAfterCreate(index);
			hg->AnyHitShaderImport = anyHit;
			hg->ClosestHitShaderImport = closestHit;
			hg->IntersectionShaderImport = nullptr;
			hg->Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
			hg->HitGroupExport = exportName;
		}
		void SetProceduralGeometryHitGroup(int index, LPCWSTR exportName, LPCWSTR anyHit, LPCWSTR closestHit, LPCWSTR intersection) {
			auto hg = GetAfterCreate(index);
			hg->AnyHitShaderImport = anyHit;
			hg->ClosestHitShaderImport = closestHit;
			hg->IntersectionShaderImport = intersection;
			hg->Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
			hg->HitGroupExport = exportName;
		}
	};

	struct DXILManager : public virtual DynamicStateBindingOf<D3D12_DXIL_LIBRARY_DESC, D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY> {
		void SetDXIL(int index, D3D12_SHADER_BYTECODE bytecode, list<D3D12_EXPORT_DESC>& exports) {
			auto dxil = GetAfterCreate(index);
			dxil->DXILLibrary = bytecode;
			dxil->NumExports = exports.size();
			dxil->pExports = &exports.first();
		}
	};

	struct RTShaderConfig : public virtual DynamicStateBindingOf<D3D12_RAYTRACING_SHADER_CONFIG, D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG>
	{
		void SetRTSizes(int index, int maxAttributeSize, int maxPayloadSize) {
			auto state = GetAfterCreate(index);
			state->MaxAttributeSizeInBytes = maxAttributeSize;
			state->MaxPayloadSizeInBytes = maxPayloadSize;
		}
	};

	struct RTPipelineConfig : public virtual DynamicStateBindingOf<D3D12_RAYTRACING_PIPELINE_CONFIG, D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG>
	{
		void SetMaxRTRecursion(int index, int maxRecursion) {
			auto state = GetAfterCreate(index);
			state->MaxTraceRecursionDepth = maxRecursion;
		}
	};

	struct ExportsManager : public virtual DynamicStateBindingOf<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION, D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION>
	{
		void SetExportsAssociations(int index, D3D12_STATE_SUBOBJECT* ptrToSubobject, const list<LPCWSTR>& exports) {
			auto state = GetAfterCreate(index);
			state->pSubobjectToAssociate = ptrToSubobject;
			state->NumExports = exports.size();
			state->pExports = &exports.first();
		}
	};

#pragma endregion

	struct RaytracingPipelineBindings : public virtual DynamicStateBindings,
		DXILManager,
		GlobalRootSignatureManager,
		LocalRootSignatureManager,
		HitGroupManager,
		RTShaderConfig,
		RTPipelineConfig,
		ExportsManager
	{
		friend RaytracingManager;
		friend Loading;
	private:

		RTPipelineWrapper* wrapper;

		// Will be executed when this pipeline manager were loaded
		void OnCreate(DX_Wrapper* dxWrapper);

		// Called when a pipeline is active into the pipeline
		void OnSet(ICmdManager* cmdWrapper);

		// Called when a pipeline is used to gen rays.
		void OnDispatch(ICmdManager* cmdWrapper);

		void AppendCode(const D3D12_SHADER_BYTECODE& code);
		void LoadProgram(gObj<RTProgramBase> program);
		void DeclareExport(LPCWSTR shader);

	protected:

		class Loading {
			RaytracingPipelineBindings* manager;
		public:
			Loading(RaytracingPipelineBindings* manager) :manager(manager) {
			}

			// Loads the code of a dxil library.
			// After this several shaders can be created for handling programs.
			// Use __create Handle<>(...) to export the shader handler used by your programs. 
			// This method can be called several times to load different library files but the shaders should be created inbetween.
			void Code(const D3D12_SHADER_BYTECODE& code) {
				manager->AppendCode(code);
			}

			// Loads a specific program into a field.
			// This method ask the program to setup and create shader tables, signatures, etc.
			template<typename P>
			void Program(gObj<P>& program) {
				program = new P();
				program->__Pipeline = manager;
				manager->LoadProgram(program);
			}
		}* const load;

		class Creating {
			RaytracingPipelineBindings* manager;
		public:
			Creating(RaytracingPipelineBindings* manager) :manager(manager) {
			}

			template<typename S>
			gObj<S> Shader(LPCWSTR shader) {
				gObj<S> handle = new S(shader);
				manager->DeclareExport(shader);
				return handle;
			}
			// Creates a handler to a Hit Group of shaders
			gObj<HitGroupHandle> HitGroup(gObj<ClosestHitHandle> closest, gObj<AnyHitHandle> anyHit, gObj<IntersectionHandle> intersection);
		}* const create;

		// When implemented, configures the pipeline object loading libraries,
		// creating shader handles and defining program settings.
		virtual void Setup() = 0;

		RaytracingPipelineBindings() : 
			create(new Creating(this)), 
			load(new Loading(this))
		{
		}
	};

}

#endif