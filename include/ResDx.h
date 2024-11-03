#pragma once
#pragma once
#include <random>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "ResDxInclude.h"
#include "DirectX/Resource.h"
#include "common/FileSystem.h"
#include "common/JobSystem.h"
#define RES_DX_ID_LEN 16
#define RES_DX_MAX_NUM_TEXTURE 16
#define RES_DX_ANIMATIONTICK_DEFAULT 25.0f
#define RES_DX_SHADER_TOKEN_WORLD_TRANSFORM ("world")
#define RES_DX_SHADER_TOKEN_CAMERA ("camera")
#define RES_DX_SHADER_TOKEN_BONE ("bone")
#define RES_DX_SHADER_TOKEN_MATERIAL ("material")

namespace ResDxCore
{
	using namespace ResDx;
	using job_arg_t = uintptr_t;
	using job_t = Job::generator_t;
	using ResDxJobState = unsigned int;
	using ResDxCommandPipelineFlags = unsigned int;

	/*
	* DirectX12で描画時に要求される制御を高パフォーマンスで実施する
	* 描画における同期、リソース参照の解決、
	* allocatorを複数のコマンドリストから参照するうま味とは
	*
	* cpu側でアニメーションなど計算→　｜gpu側にコピー　→描画｜　deviceでの制御範囲
	*/

	enum ResDxRenderState
	{
		ResDxRenderState_Edit,
		ResDxRenderState_Copy,
		ResDxRenderState_Draw
	};

	enum ResDxCommandListType
	{
		ResDxCommandListType_Copy,
		ResDxCommandListType_Direct,
		ResDxCommandListType_Compute,
		ResDxCommandListType_Bundle,
		ResDxCommandListType_NumType,
	};

	enum ResDxCommandQueueType
	{
		ResDxCommandQueueType_Copy,
		ResDxCommandQueueType_Direct,
		ResDxCommandQueueType_Compute,
		ResDxCommandQueueType_NumType,
	};

	enum ResDxJobState_
	{
		ResDxJobState_Run,
		ResDxJobState_Yield,
		ResDxJobState_End,
		ResDxJobState_TimeOut,
		ResDxJobState_Fail,
	};


	struct ResDxCommandListPacket
	{
		size_type numCommandList;
		ResDxCommandList* commandlist[RES_DX_MAX_NUM_EXECUTE_COMMAND_LISTS];
	};

	class ResDxCommandCounter
	{
	public:

		void Init(ID3D12Device5* device);
		UINT64 RegistExecuteCommandList(ResDxCommandList* list);
		UINT64 RegistExecuteCommandLists(int numCommandList, ResDxCommandList* lists[]);
		queue_t<ResDxCommandListPacket>& GetCommandList();

	private:

		queue_t<ResDxCommandListPacket> executeCommandList;
		ID3D12Device5* device;
		ResDxRenderState state;
	};

	class ResDxCommandQueueDevice
	{
	public:

		void Init(ID3D12Device5* device, int numCommandQueue, D3D12_COMMAND_LIST_TYPE type);
		bool Completed(int index)const;
		bool Empty(int index)const;
		void Execute(int index);
		void Swap(int index, ResDxCommandCounter& counter);
		void WaitCompleted(int index);
		UINT64 CompletedValue(int index)const;
		ID3D12CommandQueue* GetQueue(int index);
		void Release();

	private:
		array_t<queue_t<ResDxCommandListPacket>> executeCommandList;
		array_t<ResDxCommandQueue> commandQueue;
	};

	class ResDxCommandListDevice
	{
	public:

		struct CommandAllocatorData
		{
			ResDxCommandAllocator* allocator[2];
			bool backBufferIndex;
		};

		ResDxCommandList* CreateCommandList(ID3D12Device5* device, D3D12_COMMAND_LIST_TYPE type);
		ResDxCommandListCopy CreateCommandListCopy(ID3D12Device5* device);
		ResDxCommandListDirect CreateCommandListDirect(ID3D12Device5* device);
		ResDxCommandListCompute CreateCommandListCompute(ID3D12Device5* device);
		ResDxCommandListBundle CreateCommandListBundle(ID3D12Device5* device);

		void Reset(ResDxCommandList*);
		void Release(ResDxCommandList*);
		void ResetAllocator();

	private:

		freelist_t<ResDxCommandList> commandListPool;
		freelist_t<ResDxCommandAllocator> commandAllocatorPool;
		map_t<ResDxCommandList*, CommandAllocatorData> allocatorMap;
		queue_t<ResDxCommandAllocator*> resetQueue;
	};

	class ResDxCommandJob
	{
	public:

		void Init(GraphicsDevice* device, ResDxCommandQueueType usecase);
		ResDxCommandListCopy CreateCommandListCopy();
		ResDxCommandListDirect CreateCommandListDirect();
		ResDxCommandListCompute CreateCommandListCompute();
		ResDxCommandListBundle CreateCommandListBundle();
		void RegistCommandListCopy(ResDxCommandListCopy&);
		void RegistCommandListDirect(ResDxCommandListDirect&);
		void RegistCommandListCompute(ResDxCommandListCompute&);
		void ResetCommandList(ResDxCommandList*);
		void ExecuteCommandList(int index);
		void ExecuteCommandListWait(int index);
		void ResetCommandAllocator(int index);

		ResDxJobState GetQueueState(size_type index);

	private:
		void KickJob(int index, job_t(*func)(job_arg_t));

		struct ResDxCommandListArgs
		{
			int index;
			ResDxJobState state;
			ResDxCommandCounter* counter;
			ResDxCommandQueueDevice* queue;
			ResDxCommandListDevice* commandList;
		};

		static job_t ResDxJobExecuteCommandList(job_arg_t);
		static job_t ResDxJobExecuteCommandListWait(job_arg_t);
		static job_t ResDxJobResetCommandList(job_arg_t);

		D3D12_COMMAND_LIST_TYPE GetCommandListTypeFromResDxCommandListType(ResDxCore::ResDxCommandListType type);
		array_t<ResDxCommandQueueDevice> commandQueue;
		array_t<ResDxCommandListArgs> commandListArgs;
		ResDxCommandListDevice commandList;
		ResDxCommandCounter counter[ResDxCommandListType_NumType];
		GraphicsDevice* device;
	};

	class ResDxRendererDevice
	{
	public:

		void ExecuteCommandList(ResDxCommandList*);
		void SetFrameRenderTargetView(ResDxCommandList*);
		void PushCommandList(ResDxCommandList*);
		bool StartFrame();
		void EndFrame();
		ResDxRendererDevice(ResDxCommandListDevice& commandListDevice);
		~ResDxRendererDevice();
	private:

		void SetStartFrameCommandList();
		void SetEndFrameCommandList();

		ResDxCommandQueueDevice commandQueue;
		ResDxCommandList** commandList;
		ResDxCommandCounter counter;
		ResDxCore::ResDxSwapChain swapchain;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[3];
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	};
}

namespace ResDx
{

	enum ResDxResourceTextureType
	{
		ResDxResourceTextureType_DiffuseMap,
		ResDxResourceTextureType_SpecularMap,
		ResDxResourceTextureType_ShininessMap,
		ResDxResourceTextureType_NormalMap,
		ResDxResourceTextureType_HeightMap,
		ResDxResourceTextureType_NumMapType,
		ResDxResourceTextureType_Default,
		ResDxResourceTextureType_SpriteAnimation,
		ResDxResourceTextureType_SpriteAnimationArray
	};

	enum ResDxResourceFlags_
	{
		ResDxResourceMashFlags_None = 0,
		ResDxResourceMeshFlags_VertexType1 = 1 << 0,		//3Dモデル用
		ResDxResourceMeshFlags_VertexType2 = 1 << 1,		//スプライト用
		ResDxResourceMeshFlags_VertexType3 = 1 << 2,		//パーティクル用
		ResDxResourceMeshFlags_IndexType1 = 1 << 3,		//16bit
		ResDxResourceMeshFlags_IndexType2 = 1 << 4,		//32bit
		ResDxResourceMaterialFlags_None = 1 << 5,
		ResDxResourceMaterialFlags_Diffuse_Anbient_Specular = 1 << 6,
		ResDxResourceTextureFlags_UseTexture1 = 1 << 7,
		ResDxResourceTextureFlags_UseTexture2 = 1 << 8,
		ResDxResourceTextureFlags_UseTexture3 = 1 << 9,
		ResDxResourceTextureFlags_UseTexture4 = 1 << 10,
		ResDxResourceTextureFlags_DiffuseMap = 1 << 11,
		ResDxResourceTextureFlags_SpecularMap = 1 << 12,
		ResDxResourceTextureFlags_ShininessMap = 1 << 13,
		ResDxResourceTextureFlags_NormalMap = 1 << 14,
		ResDxResourceTextureFlags_HeightMap = 1 << 15,
		ResDxResourceBoneFlags_None = 1 << 16,
		ResDxResourceBoneFlags_AllowUseBone = 1 << 17,
		ResDxResourceBoneFlags_NotAllowUseBone = 1 << 18,
		ResDxResourceConstantBufferFlags_WorldTransform = 1 << 19,
		ResDxResourceConstantBufferFlags_CameraTransform = 1 << 20,
		ResDxResourceRendererFlags_Triangle = 1 << 21,
	};


	enum ResDxMeshInstanceFlags_
	{
		ResDxMeshInstanceFlags_None,
		ResDxMeshInstanceFlags_BlendAlpha,
	};

	class ResDxID;
	class ResDxTexture;
	class ResDxMesh;
	class ResDxBone;
	class ResDxAnimation;
	class ResDxEffect;
	class ResDxParticle;
	class ResDxDescriptorTableID;

	using id_t = reader_t<ResDxID>;
	using ResDxResourceFlags = uint32_t;
	using ResDxIndex1 = u16_t;
	using ResDxIndex2 = u32_t;
	using ResDxDescriptorHandle = ResDxDescriptorHeap::ResDxDescriptorHandle;
	using ResDxMeshInstanceFlags = size_type;
	extern const ResDxResourceFlags ResDxResourceMeshFlags_VertexTypeMask;
	extern const ResDxResourceFlags ResDxResourceMeshFlags_IndexTypeMask;
	extern const ResDxResourceFlags ResDxResourceRendererFlags_PrimitiveTypeMask;
	extern D3D12_INPUT_ELEMENT_DESC ResDxVertex1Elem[];
	extern D3D12_INPUT_ELEMENT_DESC ResDxVertex2Elem[];
	extern D3D12_INPUT_ELEMENT_DESC ResDxVertex3Elem[];
	extern size_type ResDxVertex1ElemSize;
	extern size_type ResDxVertex2ElemSize;
	extern size_type ResDxVertex3ElemSize;

	struct ResDxColor
	{
		char r;
		char g;
		char b;
		char a;
	};

	struct ResDxVertex1
	{
		ResDxVector3	pos;
		ResDxVector3	normal;
		ResDxVector3	tangent;
		ResDxVector3	binormal;
		ResDxVector2	uv;
		u16_t			boneID[MAX_NUM_BONE_VERTEX];
		float			weight[MAX_NUM_BONE_VERTEX];
	};

	struct ResDxVertex2
	{
		ResDxVector3	pos;
		ResDxVector3	normal;
		ResDxVector2	uv;
	};

	struct ResDxVertex3
	{
		ResDxVector3	pos;
	};

	struct ResDxBoneWeight
	{
		array_t<u16_t> boneID;
		array_t<float> weight;
	};

	class ResDxBoneNode
	{
	public:
		void SetName(const string_t);
		void SetID(const size_type);
		void SetTransform(const ResDxMatrix);
		void AddChild(const ResDxBoneNode&);
		void ReserveChild(const size_type);

		string_t GetName()const;
		size_type GetID()const;
		ResDxMatrix GetTransform()const;
		const array_t<ResDxBoneNode>& GetChild()const;
		ResDxBoneNode& GetChild(size_type idx);
		const ResDxBoneNode& GetChild(size_type idx)const;

		void operator=(const ResDxBoneNode&);
		void operator=(ResDxBoneNode&&);

		ResDxBoneNode() :name(), boneID(), child(), currentChild() {};
		ResDxBoneNode(string_t name, size_type id, size_type numChild) :name(name), boneID(id), child(numChild), currentChild() {};
		ResDxBoneNode(const ResDxBoneNode& node) :name(), boneID(), child(), currentChild() { *this = node; };
		ResDxBoneNode(ResDxBoneNode&& node) :name(), boneID(), child(), currentChild() { *this = std::move(node); }

	private:
		string_t name;
		size_type boneID;
		ResDxMatrix transform;
		array_t<ResDxBoneNode> child;
		size_type currentChild;
	};

	class ResDxID
	{
	public:

		class ResDxIDHandle
		{
		public:

			size_type hash()const;
			bool valid()const;
			operator size_type();
			bool operator==(const ResDxIDHandle& idr)const;
			bool operator!=(const ResDxIDHandle& idr)const;
			void operator=(const ResDxIDHandle& idr);
			ResDxIDHandle():id() {}
			ResDxIDHandle(const ResDxID& id) :id(ResDxID::map[id.hash()]) {};
			ResDxIDHandle(const ResDxIDHandle& idr) :id(idr.id) {};
			~ResDxIDHandle() {};

		private:

			id_t id;
		};

		static ResDxIDHandle create();
		static void destroy(const ResDxIDHandle&);

		size_type hash()const;
		void clear();
		bool valid()const;
		bool same(const ResDxID&)const;
		bool operator==(const ResDxID&)const;
		bool operator!=(const ResDxID&)const;
		operator ResDxIDHandle()const;
		bool operator=(const ResDxID&) = delete;

		ResDxID();
		~ResDxID();
	private:
		ResDxID(const ResDxID&) = delete;
		ResDxID(ResDxID&&);

		void generate();
		char id[RES_DX_ID_LEN + 1];
		bool							generated;
		static std::hash<size_type>		hasher;
		static std::random_device		random;
		static map_t<size_type, id_t>	map;
		static freelist_t<ResDxID>		freelist;
	};



	class ResDxTexture
	{
	public:

		struct data
		{
			string_t					name;
			u32_t						width;
			u32_t						height;
			u32_t						rowPitch;
			u8_t						mip;
			u8_t						ch;
			u16_t						uv_offset;
			array_t<ResDxColor>			pixels;
			ResDxResourceTextureType	type;
		};

		void Init(size_type numTexture);
		void SetTextureData(
			size_type					index,
			string_t					name,
			u32_t						width,
			u32_t						height,
			u32_t						rowPitch,
			u8_t						mip,
			u8_t						ch,
			u16_t						uv_offset,
			char* pixels,
			ResDxResourceTextureType	type
		);
		void SetTextureData(size_type index, data&& src);
		void ClearTextureData(size_type index);
		void ClearAll();

		const data* GetTextureData(size_type index)const;
		size_type GetNumTextureData()const;

		void operator=(const ResDxTexture&) = delete;
		void operator=(ResDxTexture&&);

		ResDxTexture() : textures() {};
		ResDxTexture(const ResDxTexture&) = delete;
		ResDxTexture(ResDxTexture&&);

	private:

		array_t<data> textures;
	};


	class ResDxMeshVertex
	{
	public:
		union ResDxVertex
		{
			ResDxVertex1* vertex1;
			ResDxVertex2* vertex2;
			ResDxVertex3* vertex3;

			ResDxVertex() :vertex1() {}
			~ResDxVertex() {}
		};

		void Init(size_type numVertex, ResDxResourceFlags vertexType);
		void Copy(void* src, size_type num, size_type stride);
		void Copy(void* src);
		void Clear();

		ResDxResourceFlags Type()const;
		ResDxVertex Resource()const;
		size_type NumVertex()const;
		size_type StrideVertex()const;
		bool IsInit()const;

		void operator=(const ResDxMeshVertex&) = delete;
		void operator=(ResDxMeshVertex&&);

		ResDxMeshVertex() :numVertex(), vertexType(), vertexData(), isInit() {};
		ResDxMeshVertex(const ResDxMeshVertex&) = delete;
		ResDxMeshVertex(ResDxMeshVertex&&);
		~ResDxMeshVertex();

	private:

		size_type				numVertex;
		size_type				stride;
		ResDxResourceFlags	vertexType;
		ResDxVertex				vertexData;
		bool					isInit;
	};

	class ResDxMeshIndex
	{
	public:
		union ResDxIndex
		{
			ResDxIndex1* index1;
			ResDxIndex2* index2;

			ResDxIndex() :index2() {}
			~ResDxIndex() {}
		};

		void Init(size_type numIndex, ResDxResourceFlags indexType);
		void Copy(void* src, size_type num, size_type stride);
		void Copy(void* src);
		void Clear();

		ResDxResourceFlags Type()const;
		ResDxIndex Resource()const;
		size_type NumIndex()const;
		size_type StrideIndex()const;
		bool IsInit()const;

		void operator=(const ResDxMeshIndex&) = delete;
		void operator=(ResDxMeshIndex&&);

		ResDxMeshIndex() :numIndex(), stride(), indexData(), indexType(), isInit() {}
		ResDxMeshIndex(const ResDxMeshIndex&) = delete;
		ResDxMeshIndex(ResDxMeshIndex&&);
		~ResDxMeshIndex();

	private:

		size_type				numIndex;
		size_type				stride;
		ResDxIndex				indexData;
		ResDxResourceFlags	indexType;
		bool					isInit;

	};

	class ResDxMesh
	{
	public:

		struct data
		{
			string_t				name;
			ResDxResourceFlags		flags;
			ResDxMeshVertex			vertex;
			ResDxMeshIndex			index;
			size_type				materialIndex;
		};

		class ResDxMeshData
		{
		public:

			inline string_t Name()const { return p->name; }
			inline size_type NumVertices()const { return p->vertex.NumVertex(); };
			inline size_type NumIndices()const { return p->index.NumIndex(); };
			inline size_type StrideVertices()const { return p->vertex.StrideVertex(); };
			inline size_type StrideIndices()const { return p->index.StrideIndex(); };
			inline size_type MaterialIndex()const { return p->materialIndex; };
			inline void* ResourceVertices()const { return p->vertex.Resource().vertex1; };
			inline void* ResourceIndices()const { return p->index.Resource().index1; };

			ResDxMeshData(const data* p) :p(p) {}

		private:

			const data* p;
		};

		void Init(size_type numMesh);
		void SetMeshData(
			size_type				index,
			void* srcVertex,
			size_type				numVertex,
			void* srcIndex,
			size_type				numIndex,
			ResDxResourceFlags	vertexAndIndexType,
			size_type materialIndex,
			string_t				name = ""
		);
		void ClearMeshData(size_type index);
		void ClearAll();

		ResDxMeshData GetMeshData(size_type index)const;
		size_type GetNumMeshData()const;

		void operator=(const ResDxMesh&) = delete;
		void operator=(ResDxMesh&&);

		ResDxMesh() :meshDataList() {};
		ResDxMesh(const ResDxMesh&) = delete;
		ResDxMesh(ResDxMesh&&);
		~ResDxMesh() {}

	private:

		array_t<data> meshDataList;
	};

	class ResDxMaterial
	{
	public:
		struct data
		{
			ResDxVector4 diffuse;
			ResDxVector4 specular;
			float shininess;
			bool useDiffuseMap;
			bool useShininessMap;
			bool useNormalMap;
			bool useSpecularMap;
		};

		void Init(size_type numMaterial);
		void SetMaterial(
			size_type index,
			float shininess,
			ResDxVector3 diffuse,
			ResDxVector3 specular,
			size_type numMaps,
			size_type mapIndex[],
			ResDxResourceTextureType textureTypes[],
			string_t hints[]
		);
		void ClearMaterialData(size_type index);
		void ClearAll();

		const data* GetMaterialData(size_type index)const;
		size_type GetNumMaterialData()const;
		array_t<string_t>& GetHints(int index);

		void operator=(const ResDxMaterial&) = delete;
		void operator=(ResDxMaterial&& src);

		ResDxMaterial() :materialDataList() {};
		ResDxMaterial(const ResDxMaterial&) = delete;
		ResDxMaterial(ResDxMaterial&& src);
		~ResDxMaterial() {};

	private:
		array_t<data> materialDataList;
		array_t<array_t<string_t>> hints;
	};

	class ResDxBone
	{
	public:

		using BoneMap_t = map_t<string_t, size_type>;

		struct data
		{
			array_t<string_t>			name;
			array_t<ResDxMatrix>		offsetMatrix;
			array_t<ResDxMatrix>		finalTransform;
			size_type		numBone;
			ResDxBoneNode rootBoneNode;
			ResDxMatrix		rootBoneInverseTransform;
			BoneMap_t boneMap;
		};


		void Init(size_type numBone);
		void SetBoneData(
			size_type		index,
			size_type		numBone,
			string_t* boneName,
			ResDxMatrix* transform,
			ResDxMatrix* offsetMatrix,
			ResDxBoneNode	rootBoneNode,
			ResDxMatrix		rootNodeInverseTransform
		);
		void ClearBoneData(size_type index);
		void ClearAll();

		void GetBoneTransform(size_type index, ResDxMatrix* dstMat)const;

		data& GetBoneData(size_type index);
		const data& GetBoneData(size_type index)const;
		size_type GetNumBone(size_type index)const;
		size_type GetNumBoneData()const;
		BoneMap_t& GetBoneMap(size_type index);
		ResDx::ResDxBoneNode& GetRootBone(size_type index);

	private:

		void GetBoneTransform(
			const data& data,
			const ResDxBoneNode* node,
			ResDxMatrix& parent,
			ResDxMatrix* dstMat
		)const;

		array_t<data>			boneDataList;
		size_type numBoneData;
	};

	struct ResDxVectorKey
	{
		double time;
		DirectX::XMFLOAT3 value;
	};

	struct ResDxQuartKey
	{
		double time;
		DirectX::XMFLOAT4 value;
	};

	class ResDxAnimation
	{
	public:


		struct data
		{
			struct Channel
			{
				size_type					boneIdx;
				array_t<ResDxVectorKey>		position;
				array_t<ResDxQuartKey>		rotation;
				array_t<ResDxVectorKey>		scale;
			};
			array_t<Channel>			channel;
			double						tickPerSec;
			double						duration;
			ResDxMatrix					globalInverseTransform;
		};

		void Init(size_type numAnimationData);
		void SetAnimationChannelData(
			size_type			index,
			size_type			boneIdx,
			size_type			numPositionKey,
			ResDxVectorKey* positionKey,
			size_type			numRotationKey,
			ResDxQuartKey* rotationKey,
			size_type			numScaleKey,
			ResDxVectorKey* scaleKey
		);
		void SetAnimationGlobalData(
			size_type			index,
			size_type			numChannels,
			ResDxMatrix			globalInverseTransformation,
			double				tickPerSec,
			double				duration
		);
		void GetBoneTransform(
			size_type			index,
			ResDxMatrix			dstTransform[],
			ResDxBone& boneData,
			float				animationTick
		);

		void ClearAnimationData(size_type index);
		void ClearAll();

		data& GetAnimationData(size_type index);

	private:

		void GetBoneTransformNode(
			size_type			index,
			ResDxMatrix			dstTransform[],
			const ResDxBoneNode& node,
			const ResDxBone& bone,
			ResDxMatrix& parent,
			float				animationTick
		);
		ResDxMatrix CalcInterpolatedScaling(float animationTick, data::Channel* channel);
		ResDxMatrix CalcInterpolatedRotation(float animationTick, data::Channel* channel);
		ResDxMatrix CalcInterpolatedPosition(float animationTick, data::Channel* channel);
		array_t<data> animationData;

	};

	//スプライトアニメーション
	class ResDxEffect
	{
	public:

		using texture_data = ResDxTexture::data;
		using ResDxViewpot = float;
		struct data :public texture_data
		{
			ResDxViewpot left;
			ResDxViewpot top;
			ResDxViewpot right;
			ResDxViewpot bot;
			size_type numWidth;
			size_type numHeight;
		};
		void Init(size_type numEffect);
		void SetEffectData(
			size_type					index,
			string_t					name,
			u32_t						width,
			u32_t						height,
			u32_t						rowPitch,
			u8_t						mip,
			u8_t						ch,
			u16_t						uv_offset,
			char* pixels,
			ResDxViewpot				left,
			ResDxViewpot				top,
			ResDxViewpot				right,
			ResDxViewpot				bot,
			size_type					numWidth,
			size_type					numHeight,
			ResDxResourceTextureType	type
		);
		void ClearEffectData(size_type index);

		data& GetEffectData(size_type index);

	private:

		array_t<data> effects;
	};

	//パーティクルアニメーション
	class ResDxParticle
	{

	};

	using ResDxTextureData = typename ResDxTexture::data;
	using ResDxMeshData = typename ResDxMesh::data;
	using ResDxBoneData = typename ResDxBone::data;
	using ResDxMaterialData = typename ResDxMaterial::data;
	using ResDxAnimationData = typename ResDxAnimation::data;
	using ResDxIDHandle = typename ResDxID::ResDxIDHandle;
}

template<>
class std::hash<ResDx::ResDxIDHandle>
{
public:

	size_t operator()(const ResDx::ResDxIDHandle& handle);
};

template<>
class std::hash<ResDx::ResDxDescriptorTableID>
{
public:

	size_t operator()(const ResDx::ResDxDescriptorTableID& handle);
};


namespace ResDxFileStream
{
	class ResDxFileImporter
	{
	public:
		using ResDxMeshVertex = ResDx::ResDxMeshVertex::ResDxVertex;
		using ResDxMeshIndex = ResDx::ResDxMeshIndex::ResDxIndex;
		using ResDxMeshVertexQueryFuncType = void (ResDxFileImporter::*)(ResDxMeshVertex&, aiMesh*);
		using ResDxMeshIndexQueryFuncType = void(ResDxFileImporter::*)(ResDxMeshIndex&, aiMesh*);

		void ImportTexture(int idx,const char*,ResDx::ResDxTexture& dst);
		void Import(const char* file_name);
		void GetTextureData(ResDx::ResDxTexture& dst);
		void GetMeshData(ResDx::ResDxMesh& dst, ResDx::ResDxResourceFlags flags);
		void GetBoneWeight(ResDx::ResDxBoneWeight&, size_type meshIndex);
		void GetMaterialData(ResDx::ResDxMaterial& dst);
		void GetBoneData(ResDx::ResDxBone& dst);
		void GetAnimationData(ResDx::ResDxAnimation& dst, ResDx::ResDxBone& map);
		void GetEffectData(ResDx::ResDxEffect& dst);

	private:

		void CreateMeshVertex1(ResDxMeshVertex&, aiMesh*);
		void CreateMeshVertex2(ResDxMeshVertex&, aiMesh*);
		void CreateMeshVertex3(ResDxMeshVertex&, aiMesh*);

		void CreateMeshIndex1(ResDxMeshIndex&, aiMesh*);
		void CreateMeshIndex2(ResDxMeshIndex&, aiMesh*);

		void QueryMeshVertex(
			aiMesh* src,
			ResDxMeshVertex& dst,
			ResDxMeshVertexQueryFuncType	vertex1,
			ResDxMeshVertexQueryFuncType	vertex2,
			ResDxMeshVertexQueryFuncType	vertex3,
			ResDx::ResDxResourceFlags	flags
		);

		void QueryMeshIndex(
			aiMesh* src,
			ResDxMeshIndex& dst,
			ResDxMeshIndexQueryFuncType		index1,
			ResDxMeshIndexQueryFuncType		index2,
			ResDx::ResDxResourceFlags	flags
		);

		void CreateBoneNode(ResDx::ResDxBoneNode* node, aiNode* aiNode, ResDx::ResDxBone::BoneMap_t& map);
		ResDx::ResDxMatrix TransrateToResDx(aiMatrix4x4);
		void TransrateTextureData(size_type index, ResDx::ResDxTexture& data, aiTexture* texture);
		const aiScene* scene;
		Assimp::Importer importer;
	};

	class ResDxFileExporter
	{
		using file_t = FileSystem::FileManager::FileIO;


	};

}

namespace ResDx
{
	template<class Key, class T>
	using AsyncMap_t = Container::AsyncHashMap<Key, T>;
	template<class T>
	using AsyncVector_t = Container::AsyncVector<T>;
	template<class T>
	using AsyncQueue_t = Container::AsyncQueue<T>;

	using ResDxBufferID = ResDxBuffer*;
	using ResDxDescriptorHeapViewID = ResDxID::ResDxIDHandle;

	class ResDxRendMesh
	{
	public:

		void Init(size_type numMesh, size_type numTextureBuffer, size_type numConstantBuffer);
		void SetMesh(ResDxContext2& context, int index, int numVertex, int strideVertex, void* srcVertex, int numIndex, int strideIndex, void* srcIndex);
		void SetConstant(ResDxContext2& context, int index, int bufferSize, void* src = nullptr);
		void SetTexture(ResDxContext2& context, int index, int width, int height, DXGI_FORMAT format, void* src);

		void CopyConstant(int index, void* src);

		size_type GetNumMesh();
		size_type GetNumTexture();
		size_type GetNumConstant();
		size_type GetNumVertices(int meshIndex);
		size_type GetNumIndices(int meshIndex);

		void Clear();

		ResDxBuffer* Resource(size_type index, ResDxBufferType type);

	private:

		inline size_type OffsetVertexBuffer() { return 0; }
		inline size_type OffsetIndexBuffer() { return numMesh; }
		inline size_type OffsetTextureBuffer() { return numMesh * 2; }
		inline size_type OffsetUploadBuffer() { return numMesh * 2 + numTextureBuffer; }
		inline size_type OffsetConstantBuffer() { return numMesh * 2 + numTextureBuffer * 2; }

		ResDxBuffer* buffers;

		size_type numMesh;
		size_type numTextureBuffer;
		size_type numConstantBuffer;

	};

	class ResDxBufferFactory
	{
	public:
		using buffer_t = reader_t<ResDxBuffer>;
		ResDxBufferID CreateVertexBuffer(ResDxContext2& context, size_type meshIndex, const ResDxMesh& mesh)const;
		ResDxBufferID CreateIndexBuffer(ResDxContext2& context, size_type meshIndex, const ResDxMesh& mesh)const;
		ResDxBufferID CreateConstantBuffer(ResDxContext2& context, const size_type bufferSize)const;
		ResDxBufferID CreateBoneBuffer(ResDxContext2& context, size_type boneIndex, const ResDxBone& boneData)const;
		ResDxBufferID CreateMaterialBuffer(ResDxContext2& context, size_type materialIndex, const ResDxMaterial& material);
		ResDxBufferID CreateTextureBuffer(ResDxContext2& context, size_type textureIndex, const ResDxTexture& texture)const;
		ResDxBufferID CreateUploadBuffer(ResDxContext2& context, const size_type textureIndex, const ResDxTexture& texture)const;
		ResDxBufferID CreateParticleBuffer(ResDxContext2& context, const ResDxParticle& particleData)const;
		ResDxBufferID CreateRenderTaretBuffer(ResDxContext2& context, int width, int height, DXGI_FORMAT format)const;
		ResDxBufferID CreateDepthStencilBuffer(ResDxContext2& context, int width, int height, DXGI_FORMAT format)const;
		ResDxBufferID CreateUnorderedAccessBuffer(ResDxContext2& context, int numResource, int sizeInByte)const;
		ResDxBufferID CreateBuffer(ResDxContext2& context, ID3D12Resource* resource, ResDxBufferInitData initData)const;
		void CreateVertexBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxMesh& mesh)const;
		void CreateIndexBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxMesh& mesh)const;
		void CreateConstantBuffers(ResDxContext2& context, ResDxBufferID* buffers, size_type numBuffer, const size_type bufferSize[])const;
		void CreateBoneBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxBone& boneData)const;
		void CreateMaterialBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxMaterial& material);
		void CreateTextureBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxTexture& texture)const;
		void CreateUploadBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxTexture& texture)const;

		void DeleteBuffer(const ResDxBufferID)const;
		static ResDxBufferFactory& instance();

	private:

		ResDxBufferID CreateBuffer()const;

		void operator=(const ResDxBufferFactory&) = delete;
		void operator=(ResDxBufferFactory&&) = delete;

		ResDxBufferFactory() :bufferPool(new freelist_t<ResDxBuffer>) {};
		ResDxBufferFactory(const ResDxBufferFactory&) = delete;
		ResDxBufferFactory(ResDxBufferFactory&&) = delete;
		~ResDxBufferFactory() { delete bufferPool; }

		freelist_t<ResDxBuffer>* bufferPool;

	};

	class ResDxDescriptorTableID
	{
	public:

		size_t hash()const;

		bool operator==(const ResDxDescriptorTableID&);

		void operator=(const ResDxDescriptorTableID&);
		void operator=(ResDxDescriptorTableID&&);

		ResDxDescriptorTableID() :hashedValue() {};
		ResDxDescriptorTableID(size_type numIDs, ResDxDescriptorHeapViewID ids[]);
		ResDxDescriptorTableID(const ResDxDescriptorTableID&) = default;
		ResDxDescriptorTableID(ResDxDescriptorTableID&&) = default;
		~ResDxDescriptorTableID() {};

	private:

		size_type hashedValue;
	};

	class ResDxGlobalDescriptorHeap
	{
	public:
		using ResDxDescriptorIndex = size_type;

		void Init(ResDxContext2& context);
		void RegistResources(ResDxContext2& context, ResDxDescriptorHeapViewID* dstIDs, ResDxViewType type, size_type numResources, ResDxBufferID resources[]);
		void RemoveResource(ResDxDescriptorHeapViewID id);
		ResDxDescriptorHandle GetDescriptorHandle(ResDxDescriptorHeapViewID);
		void GetDescriptorHandles(size_type numIDs, D3D12_CPU_DESCRIPTOR_HANDLE dstHandles[], ResDxDescriptorHeapViewID ids[]);

		static ResDxGlobalDescriptorHeap& instance(ResDxDescriptorHeapType type);
	private:
		size_type PublishNextViewIndex();
		ResDxDescriptorHandle CreateView(ResDxContext2& context, size_type index, ResDxViewType type, ResDxBuffer* resource);
		void operator=(const ResDxGlobalDescriptorHeap&) = delete;
		void operator=(ResDxGlobalDescriptorHeap&&) = delete;

		ResDxGlobalDescriptorHeap(ResDxDescriptorHeapType type);
		ResDxGlobalDescriptorHeap(const ResDxGlobalDescriptorHeap&) = delete;
		ResDxGlobalDescriptorHeap(ResDxGlobalDescriptorHeap&&) = delete;
		~ResDxGlobalDescriptorHeap() {}
		ResDxDescriptorHeap descHeap;
		ResDxDescriptorHeapType type;
		map_t<ResDxDescriptorHeapViewID, ResDxDescriptorHandle> descHeapViewMap;
		queue_t<ResDxDescriptorHeapViewID> freeList;
		std::atomic<ResDxDescriptorIndex> nextIndex;
	};

	class ResDxDescriptorHeapTable
	{
	public:
		void Init(ResDxContext2& context);
		D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptorGPUHandle(ResDxDescriptorTableID tableIDs);
		const ResDxDescriptorHeap* GetDescriptorHeap(ResDxDescriptorHeapType type)const;
		ResDxDescriptorHandle GetDescriptorHandle(ResDxDescriptorTableID tableID);
		ResDxDescriptorHandle GetDescriptorHandle(size_type numIDs, ResDxDescriptorHeapViewID viewIDs[]);
		void GetDescriptorGPUHandle(ResDxDescriptorTableID tableID, size_type numIDs, D3D12_GPU_DESCRIPTOR_HANDLE dst[]);
		void GetDescriptorHandle(ResDxDescriptorTableID tableID, size_type numDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE dst[]);
		ResDxDescriptorTableID CreateDescriptorsTable(ResDxContext2& context, ResDxViewType viewType, ResDxDescriptorHeapViewID ids[], size_type numID);
		bool FindTableHandle(size_type numID, ResDxDescriptorHeapViewID ids[]);
		static ResDxDescriptorHeapTable& instance();

	private:

		ResDxDescriptorTableID GetTableID(size_type numID, ResDxDescriptorHeapViewID ids[]);
		ResDxDescriptorHandle CopyDescriptors(ResDxContext2& context, ResDxViewType viewType, ResDxDescriptorHeapViewID ids[], size_type numID);

		ResDxDescriptorHeapTable();
		ResDxDescriptorHeapTable(const ResDxDescriptorHeapTable&) = delete;
		ResDxDescriptorHeapTable(ResDxDescriptorHeapTable&&) = delete;
		ResDxDescriptorHeap tableDescHeap[ResDxDescriptorHeapType_NumDescriptorHeapType];
		map_t<ResDxDescriptorTableID, ResDxDescriptorHandle> tableMap;
		size_type nextIndex;
	};

	class ResDxRenderPacket
	{
	public:

		void RegistVertexBuffer(ResDxVertexBufferView view);
		void RegistIndescBuffer(ResDxIndexBufferView view);
		void RegistShaderResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[]);
		void RegistConstantResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[]);
		void RegistUnorderedResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[]);
		void QueryRendererInfo(ResDxContext2& context, ResDxCommandListDirect&, size_type& numIndex);
		vector_t<ResDxDescriptorHeapViewID>& GetBufferViewIDs(ResDxViewType type);
		ResDxVertexBufferView GetVertexBufferView();
		ResDxIndexBufferView GetIndexBufferView();

		void operator=(const ResDxRenderPacket&) = delete;
		void operator=(ResDxRenderPacket&&);

		ResDxRenderPacket();
		ResDxRenderPacket(const ResDxRenderPacket&) = delete;
		ResDxRenderPacket(ResDxRenderPacket&&);
		~ResDxRenderPacket() {}

	private:

		void SetResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[], ResDxViewType type);
		void SetDescriptorTableView(ResDxContext2& context);

		ResDxIDHandle id;
		ResDxVertexBufferView vertex;
		ResDxIndexBufferView index;
		vector_t<ResDxDescriptorHeapViewID> descHeapIDs[ResDxViewType_NumDescriptorHeapViewType];
		D3D12_GPU_DESCRIPTOR_HANDLE descHeapGPUHandle[ResDxViewType_NumDescriptorHeapViewType];
	};

	struct ResDxMeshInstanceTransform
	{
		ResDxMatrix world;
	};

	class ResDxMeshInstances
	{
	public:

		using packet_t = ResDxRenderPacket;

		void SetTransforms(size_type numTransforms, const ResDxTransform transforms[]);
		void SetTransform(size_type index, const ResDxTransform& transform);
		void SetWorldMatrix(size_type index, ResDxMatrix world);
		ResDxMeshInstanceTransform* GetTransforms();
		size_type NumTransforms();
		ResDxDescriptorHeapViewID GetConstantViewID();

		void operator=(const ResDxMeshInstances&) = delete;
		void operator=(ResDxMeshInstances&&);

		ResDxMeshInstances(ResDxContext2& context, size_type numInstance);
		ResDxMeshInstances(const ResDxMeshInstances& instance) = delete;
		ResDxMeshInstances(ResDxMeshInstances&& instance);
		~ResDxMeshInstances() {};

	private:
		ResDxBufferID bufferID;
		ResDxDescriptorHeapViewID worldMatrixConstantBufferView;
		array_t<ResDxMeshInstanceTransform> transform;
	};


	class ResDxSwapchainDevice
	{
	public:

		void Init(GraphicsDevice* device, ResDxCommandQueue* commandQueue, int numFrame, float* clearColor = nullptr, int numRect = 0, ResDxScissorRect* rect = nullptr);
		void Present();
		void QuerySetRenderTagret(int index, ResDxCommandListDirect& commandList);
		void QueryClearRenderTarget(int index, ResDxCommandListDirect& commandList);
		void QueryClearDepthStencil(int index, ResDxCommandListDirect& commandList, D3D12_CLEAR_FLAGS flag);
		UINT GetBackBufferIndex();
		UINT GetCurrentBufferIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetHandle(int index);
		D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilHandle();
		static ResDxSwapchainDevice& instance();
	private:

		struct SwapchainJobArgs
		{
			ResDxSwapChain* swapchain;
			ResDxCommandQueue* queue;
			UINT* currentIndex;
		};

		static ResDxCore::job_t ResDxJobPresent(ResDxCore::job_arg_t);

		ResDxSwapchainDevice() :renderTargetHandle(), depthStencilHandle(), rects(), queue(), swapchain(), numFrame(), clearColor() {};
		ResDxSwapchainDevice(const ResDxSwapchainDevice&) = delete;
		ResDxSwapchainDevice(ResDxSwapchainDevice&&) = delete;
		~ResDxSwapchainDevice() {};

		array_t<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle;
		array_t<D3D12_RECT> rects;
		ResDxSwapChain swapchain;
		SwapchainJobArgs args;
		ResDxCommandQueue* queue;
		int numFrame;
		float clearColor[4];
		UINT currentIndex;
	};

	class ResDxCameraConfig
	{
	public:

		void SwitchFlags(ResDxCameraFlags flag);
		ResDxCameraFlags GetConfig();

	private:

		ResDxCameraFlags flags;
	};

	class ResDxMeshInstanceConfig
	{
	public:

		void SwitchFlags(ResDxMeshInstanceFlags flag);
		ResDxMeshInstanceFlags GetConfig();

	private:
		size_type cameraIndex;
		ResDxMeshInstanceFlags flags;
	};

	class ResDxRendererRootSignature
	{
	public:

		void Init(ResDxContext2& context);
		ID3D12RootSignature* RootSignature();
		static ResDxRendererRootSignature& instance();


	private:
		ResDxRendererRootSignature();

		ResDxRootSignature rootSig;
		ResDxDescriptorRange descRange;
	};


	class ResDxRendererPipeline
	{
	public:

		void VS(ResDxShader* shader);
		void PS(ResDxShader* shader);
		void CS(ResDxShader* shader);

		void SetInputElem(ResDxResourceFlags flag);
		void SetBlend(size_type index, ResDxBlendFlag flag);
		void SetRasterizer(ResDxRasterizerFlags flag);
		void SetDepthStencil(ResDxDepthStencilFlags flag);
		void SetRenderTarget(ResDxRenderTargetFlags flag);
		void SetRenderTarget(size_type numRenderTarget, ResDxRenderTargetFlags flag[]);

		void Create(ResDxContext2&);

		ID3D12PipelineState* GetPipelineState()const;

		ResDxRendererPipeline();
		ResDxRendererPipeline(const ResDxRendererPipeline&) = delete;
		ResDxRendererPipeline(ResDxRendererPipeline&&);
		~ResDxRendererPipeline() {};

	private:

		ResDxShader* vs;
		ResDxShader* ps;
		ResDxShader* cs;

		size_type numInputElem;
		D3D12_INPUT_ELEMENT_DESC* inputElem;
		size_type numRenderTarget;
		ResDxRenderTargetFlags renderTarget[RES_DX_MAX_NUM_RENDER_TARGET];
		ResDxBlendState blend;
		ResDxRasterizer rasterizer;
		ResDxDepthStencil depthStencil;
		ResDxPipelineState pipeline;
	};

	class ResDxRendererRenderTarget
	{
	public:

		void SetRenderTarget(ResDxContext2& context, int index, ResDxRenderTargetFlags usecase);
		DXGI_FORMAT GetRenderTargetFormat(int index);


		ResDxRendererRenderTarget();
		ResDxRendererRenderTarget(const ResDxRendererRenderTarget&) = default;
		ResDxRendererRenderTarget(ResDxRendererRenderTarget&&) = default;
		~ResDxRendererRenderTarget() {};

	private:
		ResDxDescriptorTableID renderTargetView;
		ResDxDescriptorTableID depthStenilView;
		ResDxBufferID renderTargetBuffer[RES_DX_MAX_NUM_RENDER_TARGET];
		DXGI_FORMAT format[RES_DX_MAX_NUM_RENDER_TARGET];
	};

}

namespace ResDx
{

	template<class T>
	using ResDxFactoryObject = reader_t<T>;
	extern const ResDxCameraConfig DefaultCameraConfig;
	extern const ResDxMeshInstanceConfig DefaultInstanceConfig;



	template<class T>
	class ResDxFactory
	{
	public:

		using  ResDxFactoryContainer = freelist_t<T>;
		using object_t = ResDxFactoryObject<T>;

		template<class ...Args>
		object_t Create(Args...args) { return factoryList.Get(args...); }
		void Destory(object_t obj) { factoryList.Release(obj); }
		static ResDxFactory<T>& instance() { static ResDxFactory<T> factory; return factory; }

	private:
		void operator=(const ResDxFactory<T>&) = delete;
		void operator=(ResDxFactory<T>&&) = delete;

		ResDxFactory() {};
		ResDxFactory(const ResDxFactory<T>&) = delete;
		ResDxFactory(ResDxFactory<T>&&) = delete;
		~ResDxFactory() {};

		ResDxFactoryContainer factoryList;
	};


	template<class Ident, class T>
	class ResDxObjectContainer
	{
	public:
		using Factory_t = ResDxFactory<T>;
		using Object_t = ResDxFactoryObject<T>;

		Object_t CreateObject(const Ident&);
		void DestoroyObject(const Ident&);
		const Object_t* Objects()const;
		const Object_t FindObject(const Ident&)const;
		const Ident* Idents()const;
		const bool FindIdent(const Ident&)const;
		const size_type NumObjects()const;
		const size_type NumIdents()const;

		void operator=(const ResDxObjectContainer&) = delete;
		void operator=(ResDxObjectContainer&&) = delete;

		ResDxObjectContainer() :objects(), idents(), map() {};
		ResDxObjectContainer(const ResDxObjectContainer&) = delete;
		ResDxObjectContainer(ResDxObjectContainer&&) = delete;
		~ResDxObjectContainer() {};

	private:

		vector_t<Object_t> objects;
		vector_t<Ident> idents;
		map_t<Ident, Object_t> map;
	};
	using SceneObjectIdent_t = string_t;

	class ResDxSceneObjectRegistory
	{
	public:

		template<class T>
		using Registory_t = ResDxObjectContainer<SceneObjectIdent_t, T>;
		template<class T>
		using Factory_t = ResDxFactory<T>;
		template<class T>
		using Object_t = typename Registory_t<T>::Object_t;

		template<class T, class ...Args>
		Object_t<T> Create(const SceneObjectIdent_t& ident, Args...args);
		template<class T>
		void Destroy(const SceneObjectIdent_t& ident);
		template<class T>
		Object_t<T> FindObject(const SceneObjectIdent_t& ident)const;
		template<class T>
		bool FindIdent(const SceneObjectIdent_t& ident)const;
		template<class T>
		const Object_t<T>* Objects()const;
		template<class T>
		SceneObjectIdent_t* Idents()const;
		template<class T>
		size_type NumObjects()const;
		template<class T>
		size_type NumIdents()const;

		void operator=(const ResDxSceneObjectRegistory&) = delete;
		void operator=(ResDxSceneObjectRegistory&&) = delete;

		ResDxSceneObjectRegistory() :registoryID(ResDxID::create()) {};
		ResDxSceneObjectRegistory(const ResDxSceneObjectRegistory&) = delete;
		ResDxSceneObjectRegistory(ResDxSceneObjectRegistory&&) = delete;
		~ResDxSceneObjectRegistory();

	private:

		struct Deleter
		{
			virtual void Delete(ResDxIDHandle) = 0;
			virtual ~Deleter() {};
		};

		template<class T>
		struct RegistoryMap :public Deleter
		{
			using RegistoryMapValue_t = reader_t<Registory_t<T>>;
			static RegistoryMap<T>& instance() { static RegistoryMap<T> i; return i; }
			map_t<ResDxIDHandle, RegistoryMapValue_t> map;
			freelist_t<Registory_t<T>> registoryList;
			void Delete(ResDxIDHandle id)override { map.erase(id); };
		};
		template<class T>
		Registory_t<T>& registory();
		template<class T>
		Registory_t<T>& registory()const;
		ResDxIDHandle registoryID;
		vector_t<Deleter*> list;
	};

	template<class Ident, class T>
	inline typename ResDxObjectContainer<Ident, T>::Object_t ResDxObjectContainer<Ident, T>::CreateObject(const Ident& ident)
	{
		Factory_t& factory = Factory_t::instance();
		Object_t object = factory.Create();

		objects.push_back(object);
		idents.push_back(ident);
		map[ident] = object;

		return object;
	}
	template<class Ident, class T>
	inline void ResDxObjectContainer<Ident, T>::DestoroyObject(const Ident& ident)
	{
		if (!map.find(ident))
			return;
		Object_t object = map[ident];

		objects.remove(object);
		idents.remove(ident);
		map.erase(ident);
	}
	template<class Ident, class T>
	inline const typename ResDxObjectContainer<Ident, T>::Object_t ResDxObjectContainer<Ident, T>::FindObject(const Ident& ident) const
	{
		auto result = map.find(ident);
		if (!result)
			return Object_t();

		return map.get(ident);
	}
	template<class Ident, class T>
	inline const bool ResDxObjectContainer<Ident, T>::FindIdent(const Ident& ident) const
	{
		return map.find(ident);
	}
	template<class Ident, class T>
	inline const size_type ResDxObjectContainer<Ident, T>::NumObjects() const
	{
		return objects.get_size();
	}
	template<class Ident, class T>
	inline const size_type ResDxObjectContainer<Ident, T>::NumIdents() const
	{
		return idents.get_size();
	}
	template<class Ident, class T>
	inline const typename ResDxObjectContainer<Ident, T>::Object_t* ResDxObjectContainer<Ident, T>::Objects() const
	{
		return &objects[0];
	}
	template<class Ident, class T>
	inline const Ident* ResDxObjectContainer<Ident, T>::Idents() const
	{
		return &idents[0];
	}


	template<class T, class ...Args>
	inline ResDxSceneObjectRegistory::Object_t<T> ResDxSceneObjectRegistory::Create(const SceneObjectIdent_t& ident, Args...args)
	{
		Registory_t<T>& r = registory<T>();
		return r.CreateObject(ident);
	}

	template<class T>
	inline void ResDxSceneObjectRegistory::Destroy(const SceneObjectIdent_t& ident)
	{
		Registory_t<T>& r = registory<T>();
		r.DestoroyObject(ident);
	}

	template<class T>
	inline ResDxSceneObjectRegistory::Object_t<T> ResDxSceneObjectRegistory::FindObject(const SceneObjectIdent_t& ident) const
	{
		Registory_t<T>& r = registory<T>();
		return r.FindObject(ident);
	}

	template<class T>
	inline bool ResDxSceneObjectRegistory::FindIdent(const SceneObjectIdent_t& ident) const
	{
		Registory_t<T>& r = registory<T>();
		return r.FindIdent(ident);
	}

	template<class T>
	inline const ResDxSceneObjectRegistory::Object_t<T>* ResDxSceneObjectRegistory::Objects()const
	{
		const Registory_t<T>& r = registory<T>();
		return r.Objects();
	}

	template<class T>
	inline SceneObjectIdent_t* ResDxSceneObjectRegistory::Idents() const
	{
		Registory_t<T>& r = registory<T>();
		return r.Idents();
	}

	template<class T>
	inline size_type ResDxSceneObjectRegistory::NumObjects() const
	{
		return registory<T>().NumObjects();
	}

	template<class T>
	inline size_type ResDxSceneObjectRegistory::NumIdents() const
	{
		return registory<T>().NumIdents();
	}

	template<class T>
	inline ResDx::ResDxSceneObjectRegistory::Registory_t<T>& ResDxSceneObjectRegistory::registory()const
	{
		static RegistoryMap<T>& r = RegistoryMap<T>::instance();
		assert(!r.map.find(registoryID));
		return *r.map[registoryID];
	}

	template<class T>
	inline ResDxSceneObjectRegistory::Registory_t<T>& ResDxSceneObjectRegistory::registory()
	{
		static RegistoryMap<T>& r = RegistoryMap<T>::instance();
		if (!r.map.find(registoryID))
		{
			this->list.push_back(&r);
			r.map[registoryID] = r.registoryList.Get();
		}
		return *r.map[registoryID];
	}
}

namespace ResDx
{
	D3D12_INPUT_ELEMENT_DESC* GetInputElemFromVertexFlags(ResDxResourceFlags flags);
	D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyFromRendererFlags(ResDxResourceFlags flags);
	D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapTypeFromResDxHeapType(ResDxDescriptorHeapType type);
	ResDxDescriptorHeapType GetResDxDescriptorHeapTypeFromResDxViewType(ResDxViewType type);
	DXGI_FORMAT GetDXGIFormatFromResDxUsecaseRenderTarget(ResDxRenderTargetFlags usecase);
	void GetResolutionFromResDxRenderTargetFlag(int& xResolution, int& yResolution, ResDxRenderTargetFlags flags);
	size_type GetNumInputElemFromVertexFlags(ResDxResourceFlags flags);
	size_type GetSizeIndexFromFormat(DXGI_FORMAT format);

	class ResDxRenderResourceCorrection
	{
	public:

		void Darty();
		ResDxVertexBufferView** GetVertexViews();
		ResDxIndexBufferView** GetindexViews();
		ID3D12DescriptorHeap** GetDescHeaps();

		ResDxRenderResourceCorrection(ResDxSceneObjectRegistory* r) :registory(r), vertices(), indices(), descHeap(), cameras(), darty(true) {};
		ResDxRenderResourceCorrection(const ResDxRenderResourceCorrection&) = delete;
		ResDxRenderResourceCorrection(ResDxRenderResourceCorrection&&) = delete;
		~ResDxRenderResourceCorrection() {}

	private:
		void Update();

		ResDxSceneObjectRegistory* registory;
		vector_t<D3D12_VERTEX_BUFFER_VIEW*> vertices;
		vector_t<D3D12_INDEX_BUFFER_VIEW*> indices;
		vector_t<ID3D12DescriptorHeap*> descHeap;
		vector_t<ResDxCamera*> cameras;
		bool darty;
	};

	struct ResDxCameraTransform
	{
		ResDxMatrix view;
		ResDxMatrix projection;
	};

	class ResDxRendererCamera :public ResDxCamera
	{
	public:
		void UpdateGpuResource();
		ResDxDescriptorHeapViewID GetViewID();
		ResDxRendererCamera(ResDxContext2& context);
		~ResDxRendererCamera();

	private:
		void SetMatrixView(ResDxMatrix view);
		void SetMatrixProjection(ResDxMatrix view);

		ResDxBufferID buffer;
		ResDxDescriptorHeapViewID viewID;
	};

	class ResDxRendererResources
	{
	public:

		void ImportModelFile(ResDxContext2& context, string_t fileName);

	private:

		array_t<ResDxBufferID> vertexBuffer;
		array_t<ResDxBufferID> indexBuffer;
		array_t<ResDxBufferID> wvpConstantBuffer;
		array_t<ResDxBufferID> boneBuffer;
		array_t<ResDxBufferID> shaderResourceBuffer;
	};

	class ResDxRenderDummyObject
	{
	public:

		const ResDxDescriptorHeapViewID GetBoneBufferView();
		const ResDxDescriptorHeapViewID GetMaterialBufferView();
		const array_t<ResDxDescriptorHeapViewID>& GetTextureBufferView();

		ResDxRenderDummyObject();
		static ResDxRenderDummyObject& instance();
	private:
		ResDxDescriptorHeapViewID boneBufferView;
		ResDxDescriptorHeapViewID materialBufferView;
		array_t<ResDxDescriptorHeapViewID> textureBufferView;
		ResDxBufferID uploadBuffer;
	};

	class ResDxTextureSet
	{
	public:

		void ImportTextrueDirectory(ResDxContext2& context,string_t dir);
		ResDxDescriptorHeapViewID GetView(ResDx::string_t name)const;
		ResDxDescriptorHeapViewID GetView(int idx)const;
		int GetNumTexture()const;

	private:

		ResDxTexture texture;
		array_t<string_t> names;
		array_t<ResDxBufferID> buffers;
		array_t<ResDxBufferID> upload;
		array_t<ResDxDescriptorHeapViewID> viewIDs;
	};

	class ResDxRendererObject
	{
	public:
		void ImportModelFile(ResDxContext2& context, string_t filePath);
		void ImportPictureFile(ResDxContext2& context, string_t filePath);
		void ImportTextureSet(ResDxTextureSet&);
		void QueryCopyCommand(ResDxCommandListDirect& direct);
		void QueryCopyCommand(ResDxCommandListCopy& copy);
		void QueryResourceBarrierCommand(ResDxCommandListDirect& direct);
		const ResDxVertexBufferView GetVertexBufferView(int meshIndex);
		const ResDxIndexBufferView GetIndexBufferView(int meshIndex);
		const array_t<ResDxDescriptorHeapViewID>& GetShaderResourceViews(int meshIndex);
		const ResDxDescriptorHeapViewID GetBoneBufferView(int index);
		const ResDxDescriptorHeapViewID GetMaterialBufferView(int index);
		const array_t<ResDxMatrix>& GetBoneMatrix(int index, float animationTick);
		const ResDxMaterial& GetMaterialData();
		const ResDxBone& GetBoneData();
		const ResDxAnimation& GetAnimationData();
		const ResDxTexture& GetTextureData();
		size_type GetNumBoneData();
		size_type GetNumMesh();
		size_type GetNumMaterial();
		size_type GetNumTexture();
		size_type GetNumIndices(int meshIndex);
		size_type GetMaterialIndex(int meshIndex);
		string_t GetMeshName(int meshIndex);
		string_t GetObjectName();
		ResDxRendererObject() {};
		ResDxRendererObject(const ResDxRendererObject&) = delete;
		ResDxRendererObject(ResDxRendererObject&&) = default;
		~ResDxRendererObject() {};

	private:

		string_t name;
		array_t<string_t> meshName;
		array_t<ResDxBufferID> vertexBuffer;
		array_t<ResDxBufferID> indexBuffer;
		array_t<ResDxBufferID> wvpConstantBuffer;
		array_t<ResDxBufferID> shaderResourceBuffer;
		array_t<ResDxBufferID> uploadBuffer;
		array_t<ResDxBufferID> boneBuffer;
		array_t<ResDxRenderPacket> renderPacketList;
		array_t<ResDxBufferID> materialBuffer;
		array_t<array_t<ResDxDescriptorHeapViewID>> shaderResourceViewID;
		array_t<ResDxDescriptorHeapViewID> boneViewID;
		array_t<ResDxDescriptorHeapViewID> materialViewID;
		array_t<array_t<ResDxMatrix>> boneMatrix;
		array_t<size_type> numIndices;
		array_t<size_type> materialIndex;
		ResDxTexture texture;
		ResDxMaterial material;
		ResDxBone bone;
		ResDxAnimation animation;
		FileSystem::path_t path;
	};
	class ResDxRenderObjectBundle
	{
	public:

		void SetDrawBundle(ResDxContext2& context,ResDxCommandList* list,ResDxRendererObject& obj,ResDxMeshInstances& meshInstance, ResDxRendererCamera& camera);
		void SetResourceBundle(int meshIndex,ResDxContext2& context, ResDxCommandList* list, ResDxRendererObject& obj, ResDxMeshInstances& meshInstance, ResDxRendererCamera& camera);
		void SetMeshBundle(int meshIndex, ResDxCommandList* list, ResDxRendererObject& obj);
		
	};


	class ResDxRenderShaderBundle
	{
	public:

		void SetDefault();

		void SetVS(string_t path);
		void SetPS(string_t path);
		void CommitPipelineGraphics(ResDxContext2& context);
		void SetShaderBundle(ResDxCommandList*);
	private:

		ResDxShader vs;
		ResDxShader ps;
		ResDxRendererPipeline pipeline;
		ResDxResourceFlags meshFlags;
		ResDxRenderTargetFlags rtvFlags;
		ResDxDepthStencilFlags dsvFlags;
		ResDxRasterizerFlags rasFlags;
	};

	class ResDxShaderBinder
	{
	public:

		void Commit(ResDxContext2&);
		void SetShader(ResDxShader*);
		bool BindFromIndex(int index, ResDxDescriptorHeapViewID id, ResDxViewType type,size_type size);
		UINT GetNumRegister();
		UINT GetNumRegister(ResDxViewType type);
		int GetIndexBindNext(ResDxViewType type, int index = 0);
		string_t GetNameRegister(UINT index);
		ResDxDescriptorTableID GetTableID(ResDxViewType type);
		ResDxDescriptorHeapViewID GetViewID(UINT index);
		vector_t<ResDxDescriptorHeapViewID>& GetViewID(ResDxViewType type);
		void operator+=(ResDxShaderBinder&);
		ResDxShaderBinder() :nextBinder(), reflection() {}

	private:
		ResDxViewType TransrateViewTypeFromSIT(D3D_SHADER_INPUT_TYPE type);
		D3D_SHADER_INPUT_TYPE TransrateSITFromViewType(ResDxViewType type);
		bool IsBound(ResDxViewType type, UINT bindPoint);
		void PushDescriptorViewID(ResDxViewType type, UINT bindPoint, ResDxDescriptorHeapViewID id);
		bool CheckConstantBufferSize(LPCSTR name,size_type size);
		ResDxShaderBinder* nextBinder;
		ID3D12ShaderReflection* reflection;
		ResDxDescriptorTableID tableID[ResDxViewType_NumDescriptorHeapViewType];
		vector_t<ResDxDescriptorHeapViewID> viewID[ResDxViewType_NumDescriptorHeapViewType];
		vector_t<size_type> size[ResDxViewType_NumDescriptorHeapViewType];
	};

	class ResDxResourceView
	{
	public:
		void RegistTableID(int numIDs, ResDxDescriptorHeapViewID ids[]);
		void RegistTableID(ResDxDescriptorHeapViewID id);
		void ClearTableID();
		void Commit(ResDxContext2& context);
		ResDxDescriptorTableID GetTableID();
		ResDxResourceView(ResDxViewType type) :type(type) {};

	private:

		ResDxViewType type;
		ResDxDescriptorTableID tableID;
		vector_t<ResDxDescriptorHeapViewID> ids;
	};

	class ResDxRendererScene
	{
	public:
		template<class T>
		using Object_t = ResDxFactoryObject<T>;
		using CameraObject_t = Object_t<ResDxCamera>;
		using MeshInstance_t = Object_t<ResDxMeshInstances>;
		using ResDxSceneIdent = string_t;

		void RenderingScene(ResDxContext2&);

		ResDxRendererScene(ResDxSceneIdent ident);
		ResDxRendererScene(const ResDxRendererScene&) = delete;
		ResDxRendererScene(ResDxRendererScene&&) = delete;
		~ResDxRendererScene() {}

	private:

		template<class T>
		SceneObjectIdent_t GetIdentAddNumber(const SceneObjectIdent_t& ident);

		ResDxSceneIdent ident;
		ResDxSceneObjectRegistory registory;
	};

	void ResDxInit();
	void ResDxEnd();
	void ResDxMessage(string_t);

	template<class T>
	inline SceneObjectIdent_t ResDxRendererScene::GetIdentAddNumber(const SceneObjectIdent_t& ident)
	{
		size_type counter = 0;
		SceneObjectIdent_t newIdent;
		do
		{
			++counter;
			newIdent = ident + counter;
		} while (registory.FindIdent<T>(newIdent));
		return newIdent;
	}
}


inline size_t std::hash<ResDx::ResDxIDHandle>::operator()(const ResDx::ResDxIDHandle& handle)
{
	return handle.hash();
}

inline size_t std::hash<ResDx::ResDxDescriptorTableID>::operator()(const ResDx::ResDxDescriptorTableID& handle)
{
	return handle.hash();
}