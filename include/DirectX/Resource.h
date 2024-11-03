#pragma once
#include <string>
#include <vector>
#include <map>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <dxgi.h>
#include <memory>
#include <DirectXMath.h>
#include <chrono>
#include <atomic>
#include "Input.h"
#include "Graphics.h"


#define MAX_NUM_BONE_VERTEX 16
#define SIZE_BONE_TRANSFORM sizeof(DirectX::XMMATRIX);
#define MAX_NUM_BONE 320
#define MAX_NUM_DESCRIPTOR 64
#define PI 3.1415f
#define RES_DX_MAX_NUM_DESCRIPTOR 10000 
#define RES_DX_MAX_NUM_SAMPLER 2048
#define RES_DX_MAX_NUM_COPY_DESCRIPTOR 100
#define RES_DX_MAX_NUM_TEMP_BUFFER 10
#define RES_DX_BUFFER_COUNT 2
#define RES_DX_MAX_NUM_RENDER_TARGET 8
#define RES_DX_MAX_NUM_EXECUTE_COMMAND_LISTS 8

enum ResType_
{
	ResType_Texture,
	ResType_Material,
	ResType_Mesh,
	ResType_Bone,
	ResType_Animation
};


class ResUTF8
{

};

class ResString
{

};

struct ResTexture
{
	std::string name;
	uint32_t width;
	uint32_t height;
	uint32_t rowPitch;
	uint32_t depth;
	uint16_t mip;
	uint8_t ch;
	uint32_t sizeOfByte;
	DXGI_FORMAT format;
	D3D12_RESOURCE_DIMENSION dimension;
	uint8_t* pixels;
};

struct ResMaterial
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	float alpha;
	float shininess;
	std::vector<ResTexture> texture;
	std::string diffuseMap;
	std::string specularMap;
	std::string shininessMap;
	std::string normalMap;
};

struct ResVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 tangent;
	DirectX::XMFLOAT3 binormal;
	DirectX::XMFLOAT2 uv;
};

struct ResMesh
{
	std::string name;
	std::vector<ResVertex> vertices;
	std::vector<uint32_t> indices;
	uint32_t materialIdx;
};

struct BoneNode
{
	std::string boneName;
	int boneID;
	DirectX::XMMATRIX transform;//親を基準とした変換
	std::vector<BoneNode*> child;
};

struct BoneData
{
	struct BoneInfo
	{
		DirectX::XMMATRIX boneOffsetTransform;
		DirectX::XMMATRIX finalTransform;
	};

	struct Weight
	{
		uint16_t boneID[MAX_NUM_BONE_VERTEX];
		float weight[MAX_NUM_BONE_VERTEX];
	};
	std::map<std::string, size_t> boneIdxMap;
	std::vector<BoneInfo> boneInfo;
	std::vector<Weight> boneWeight;
	BoneNode* rootNode;
	DirectX::XMMATRIX rootNodeInverseTransformation;
	uint16_t meshIdx;
};

struct AnimationData
{
	struct VectorKey
	{
		double time;
		DirectX::XMFLOAT3 value;
	};

	struct QuartKey
	{
		double time;
		DirectX::XMFLOAT4 value;
	};

	struct Channel
	{
		std::string boneName;
		std::vector<VectorKey> position;
		std::vector<QuartKey> rotation;
		std::vector<VectorKey> scale;
	};

	std::string name;
	std::vector<Channel> channels;
	double tickPerSec;
	double duration;
	DirectX::XMMATRIX globalInverseTransform;
};


struct LoadedResourceBuffer
{
	std::vector<std::unique_ptr<ResMesh>> meshes;
	std::vector<std::unique_ptr<ResMaterial>> materials;
	std::vector<std::unique_ptr<BoneData>> bones;
	std::vector<std::unique_ptr<AnimationData>> animations;
};



class ResourceLoader
{
public:
	static void LoadTexture(const char*, std::unique_ptr<ResTexture>&);
	static LoadedResourceBuffer LoadModel(const char*);
	static void DeleteLoadData(LoadedResourceBuffer&);
	static void DeleteLoadData(ResTexture*);
private:
};

namespace ResDx
{
	enum ResDxBufferType
	{
		ResDxBufferType_None = 0,
		ResDxBufferType_Vertex,
		ResDxBufferType_Index,
		ResDxBufferType_Texture,
		ResDxBufferType_Constant,
		ResDxBufferType_Upload,
		ResDxBufferType_Structured,
		ResDxBufferType_RWStructured,
		ResDxBufferType_RenderTarget,
		ResDxBufferType_DepthStencil
	};

	enum ResDxViewType
	{
		ResDxViewType_ConstantBufferView,
		ResDxViewType_ShaderResourceView,
		ResDxViewType_UnorderedResourceView,
		ResDxViewType_NumDescriptorHeapViewType,
		ResDxViewType_RenderTargetView,
		ResDxViewType_DepthStencilView,
		ResDxViewType_None,
	};

	enum ResDxDescriptorHeapType
	{
		ResDxDescriptorHeapType_CBV_SRV_UAV,
		ResDxDescriptorHeapType_RTV,
		ResDxDescriptorHeapType_DSV,
		ResDxDescriptorHeapType_Sampler,
		ResDxDescriptorHeapType_NumDescriptorHeapType
	};

	enum ResDxInputElementFlags_
	{
		ResDxInputElementFlags_None = 0,
		ResDxInputElementFlags_SV_POSITION = 1 << 0,
		ResDxInputElementFlags_POSITION = 1 << 1,
		ResDxInputElementFlags_NORMAL = 1 << 2,
		ResDxInputElementFlags_BINORMAL = 1 << 3,
		ResDxInputElementFlags_TANGENT = 1 << 4,
		ResDxInputElementFlags_TEXCOORD = 1 << 5,
		ResDxInputElementFlags_BONENO = 1 << 6,
		ResDxInputElementFlags_BONE_WEIGHT = 1 << 7,
	};

	enum ResDxCameraFlags_
	{
		ResDxCameraFlags_None = 0,		//全ての移動と回転を許可し入力値はカメラのローカル座標変換として扱う
		ResDxCameraFlags_CalcrateUpVector = 1 << 0,	//ターゲットの変動に際して上ベクトルを計算する
		ResDxCameraFlags_FrameAspectRaito = 1 << 1,	//フレームバッファのアスペクト比を使用する
		ResDxCameraFlags_AllowMoveEyePosX = 1 << 2,	//カメラ座標のX軸の移動を許可する
		ResDxCameraFlags_AllowMoveEyePosY = 1 << 3,	//カメラ座標のY軸の移動を許可する
		ResDxCameraFlags_AllowMoveEyePosZ = 1 << 4,	//カメラ座標のZ軸の移動を許可する
		ResDxCameraFlags_AllowMoveTargetPosX = 1 << 5,	//注視点のX軸の移動を許可する
		ResDxCameraFlags_AllowMoveTargetPosY = 1 << 6,	//注視点のY軸の移動を許可する
		ResDxCameraFlags_AllowMoveTargetPosZ = 1 << 7,	//注視点のZ軸の移動を許可する
		ResDxCameraFlags_AllowRotationX = 1 << 8,	//カメラのX軸の角度変換を許可する
		ResDxCameraFlags_AllowRotationY = 1 << 9,	//カメラのY軸の角度変換を許可する
		ResDxCameraFlags_AllowRotationZ = 1 << 10,	//カメラのZ軸の角度変換を許可する
		ResDxCameraFlags_NotAllowMoveEyePos = 1 << 11,	//カメラの移動を許可しない
		ResDxCameraFlags_NotAllowMoveTargetPos = 1 << 12,	//注視点の移動を許可しない
		ResDxCameraFlags_NotAllowRotation = 1 << 13,	//カメラの回転を許可しない
		ResDxCameraFlags_WorldCoordinate = 1 << 14,	//入力された値をワールド座標変換として扱う
		ResDxCameraFlags_LocalCoordinate = 1 << 15,	//入力された値をカメラのローカル座標変換として扱う
		ResDxCameraFlags_RotateLocalFromEyePos = 1 << 16,	//カメラの座標を中心に回転
		ResDxCameraFlags_RotateLocalFromTargetPos = 1 << 17,	//カメラの注視点を中心に回転
	};

	enum ResDxTransformFlags_
	{
		ResDxTransformFlags_None = 0,
		ResDxTransformFlags_Location = 1 << 0,
		ResDxTransformFlags_Rotation = 1 << 1,
		ResDxTransformFlags_Scale = 1 << 2,
		ResDxTransformFlags_OperateStore = 1 << 3,
		ResDxTransformFlags_OperateClear = 1 << 4,
		ResDxTransformflags_OperateNormalize = 1 << 5,
		ResDxTransformFlags_OperateCalc = 1 << 5,
		ResDxTransformFlags_CalcrateAdd = 1 << 6,
		ResDxTransformFlags_CalcrateSub = 1 << 7,
		ResDxTransformFlags_CalcrateMul = 1 << 8,
		ResDxTransformFlags_CalcrateDiv = 1 << 9,
	};

	enum ResDxRenderTargetFlags_
	{
		ResDxRenderTargetFlags_None = 0,
		ResDxRenderTargetFlags_OutputColor = 1 << 0,
		RedDxRenderTargetFlags_OutputWorldPos = 1 << 1,
		ResDxRenderTargetFlags_OutputNormal = 1 << 2,
		ResDxRenderTargetFlags_OutputBinormal = 1 << 3,
		ResDxRenderTargetFlags_OutputTangent = 1 << 4,
		ResDxRenderTargetFlags_OutputUV = 1 << 5,
		ResDxRenderTargetFlags_OutputDepth = 1 << 6,
		ResDxRenderTargetFlags_ResolutionFrameBuffer = 1 << 7,
		ResDxRenderTargetFlags_Resolution1920 = 1 << 8,
		ResDxRenderTargetFlags_Resolution1280 = 1 << 9,
		ResDxRenderTargetFlags_Resolution720 = 1 << 10,
		ResDxRenderTargetFlags_Resolution480 = 1 << 11,
		ResDxRenderTargetFlags_RateResolutionFrameBuffer = 1 << 12,
		ResDxRenderTargetFlags_RateResolution4x3 = 1 << 13,
		ResDxRenderTargetFlags_RateResolution16x9 = 1 << 14,
		ResDxRenderTargetFlags_RateResolution16x10 = 1 << 15,
	};

	enum ResDxDepthStencilFlags_
	{
		ResDxDepthStencilFlags_None = 0,
		ResDxDepthStencilFlags_DepthEnable = 1 << 0,
		ResDxDepthStencilFlags_StencilEnable = 1 << 1,
		ResDxDepthStencilFlags_FormatD32_FLOAT = 1 << 2,
		ResDxDepthStencilFlags_FormatD32_FLOAT_S824_UINT = 1 << 3,
		ResDxDepthStencilFlags_D16_UNORM = 1 << 4,
		ResDxDepthStencilFlags_D24_UNORM_S8_UINT = 1 << 5,
		ResDxDepthStencilFlags_DepthComparisonLess=1<<6,
		ResDxDepthStencilFlags_DepthComparisonGreater = 1 << 7,
	};

	enum ResDxBlendFlag
	{
		ResDxRendererBlendFlag_None = 0,
		ResDxRendererBlendFlga_Alpha,
		ResDxRendererBlendFlga_Alpha_Add,
		ResDxRendererBlendFlga_Add,
		ResDxRendererBlendFlga_Sub,
	};

	enum ResDxRasterizeFlags_
	{
		ResDxRasterizerFlags_None = 0,
		ResDxRasterizerFlags_FillFrame = 1 << 0,//メッシュフレームの描画
		ResDxRasterizerFlags_FillSolid = 1 << 1,//メッシュの描画
		ResDxRasterizerFlags_CullNone = 1 << 2,//メッシュの両面描画
		ResDxRasterizerFlags_CullFront = 1 << 3,//メッシュの表面だけ描画
		ResDxRasterizerFlags_CullBack = 1 << 4,//メッシュの裏面だけ描画
		ResDxRasterizerFlags_DepthBiasNone = 1 << 5,
		ResDxRasterizerFlags_DepthBiasMax = 1 << 6,
		ResDxRasterizerFlags_DepthBiasMin = 1 << 7,
		ResDxRasterizerFlags_DepthEnable = 1 << 8,
		ResDxRasterizerFlags_MultisampleEnable = 1 << 9,
		ResDxRasterizerFlags_AntialiasedLineEnable = 1 << 10,
	};

	class ResDxBuffer;
	class ResDxContext;
	class ResDxContext2;
	class ResDxIO;
	class ResDxCamera;
	struct ResDxVector4;
	struct ResDxVector4x4;
	struct ResDxQuaternion;

	typedef unsigned int ResDxInputElementFlags;
	typedef unsigned int ResDxCameraFlags;
	typedef unsigned int ResDxTransformFlags;
	typedef unsigned int ResDxRasterizerFlags;
	typedef unsigned int ResDxRenderTargetFlags;
	typedef unsigned int ResDxDepthStencilFlags;
	typedef DirectX::XMFLOAT2 ResDxVector2;
	typedef DirectX::XMFLOAT3 ResDxVector3;
	typedef ResDxVector4 ResDxVector;
	typedef ResDxVector4x4 ResDxMatrix;
	typedef ResDxVector ResDxLocation;
	typedef ResDxQuaternion ResDxRotation;
	typedef ResDxVector ResDxScale;
	typedef D3D12_VERTEX_BUFFER_VIEW ResDxVertexBufferView;
	typedef D3D12_INDEX_BUFFER_VIEW ResDxIndexBufferView;


	struct ResDxBufferVertex
	{
		int bufferSize;
		int stride;
		int numVertex;
		ResDxVertexBufferView view;
		void* map;
	};

	struct ResDxBufferIndex
	{
		int bufferSize;
		int stride;
		int numIndex;
		ResDxIndexBufferView view;
		void* map;
	};

	struct ResDxBufferTexture
	{
		int width;
		int height;
		DXGI_FORMAT format;
	};

	struct ResDxBufferConstant
	{
		UINT size;
		void* map;
	};

	struct ResDxBufferUpload
	{
		int width;
		int height;
		int depth;
		UINT64 uploadSize;
		DXGI_FORMAT format;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		void* map;
	};

	struct ResDxBufferStructured
	{
		int sizeOfElem;
		int numOfElem;
		void* map;
	};

	struct ResDxBufferRWStructured
	{
		int sizeOfElem;
		int numOfElem;
		void* map;
		ID3D12Resource* counter;
	};

	using ResDxBufferRenderTarget = ResDxBufferTexture;
	using ResDxBufferDepthStencil = ResDxBufferTexture;
	template<auto type>struct ResDxType;

	template<>struct ResDxType<ResDxBufferType_Vertex> { typedef ResDxBufferVertex				type; };
	template<>struct ResDxType<ResDxBufferType_Index> { typedef ResDxBufferIndex				type; };
	template<>struct ResDxType<ResDxBufferType_Texture> { typedef ResDxBufferTexture			type; };
	template<>struct ResDxType<ResDxBufferType_Constant> { typedef ResDxBufferConstant			type; };
	template<>struct ResDxType<ResDxBufferType_Upload> { typedef ResDxBufferUpload				type; };
	template<>struct ResDxType<ResDxBufferType_Structured> { typedef ResDxBufferStructured		type; };
	template<>struct ResDxType<ResDxBufferType_RWStructured> { typedef ResDxBufferRWStructured	type; };
	template<>struct ResDxType<ResDxBufferType_RenderTarget> { typedef ResDxBufferRenderTarget	type; };
	template<>struct ResDxType <ResDxBufferType_DepthStencil> { typedef ResDxBufferDepthStencil	type; };


	struct InitDxVertex { int bufferSize, stride;														InitDxVertex(int bufferSize, int stride) :bufferSize(bufferSize), stride(stride) {}; };
	struct InitDxIndex { int bufferSize, stride;														InitDxIndex(int bufferSize, int stride) :bufferSize(bufferSize), stride(stride) {}; };
	struct InitDxTexture { int width, height; DXGI_FORMAT format; D3D12_RESOURCE_FLAGS flags;			InitDxTexture(int width, int height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) :width(width), height(height), format(format), flags(flags) {}; };
	struct InitDxConstant { int bufferSize;																InitDxConstant(int bufferSize) :bufferSize(bufferSize) {}; };
	struct InitDxUpload { int width, height; DXGI_FORMAT format;										InitDxUpload(int width, int height, DXGI_FORMAT format) :width(width), height(height), format(format) {}; };
	struct InitDxStructured { int sizeOfElem, numOfElem;												InitDxStructured(int sizeOfElem, int numOfElem) :sizeOfElem(sizeOfElem), numOfElem(numOfElem) {}; };
	struct InitDxRWStructured { int sizeOfElem, numOfElem;												InitDxRWStructured(int sizeOfElem, int numOfElem) :sizeOfElem(sizeOfElem), numOfElem(numOfElem) {}; };
	struct InitDxRenderTarget { int width, height; DXGI_FORMAT format;									InitDxRenderTarget(int width, int height, DXGI_FORMAT format) :width(width), height(height), format(format) {}; };
	struct InitDxDepthStencil { int width, height; DXGI_FORMAT format;									InitDxDepthStencil(int width, int height, DXGI_FORMAT format) :width(width), height(height), format(format) {}; };

	class ResDxBufferInitData
	{
	public:

		ResDxBufferInitData(InitDxVertex res) :data(res), type(ResDxBufferType_Vertex) {};
		ResDxBufferInitData(InitDxIndex res) :data(res), type(ResDxBufferType_Index) {};
		ResDxBufferInitData(InitDxTexture res) :data(res), type(ResDxBufferType_Texture) {};
		ResDxBufferInitData(InitDxConstant res) :data(res), type(ResDxBufferType_Constant) {};
		ResDxBufferInitData(InitDxUpload res) :data(res), type(ResDxBufferType_Upload) {};
		ResDxBufferInitData(InitDxStructured res) :data(res), type(ResDxBufferType_Structured) {};
		ResDxBufferInitData(InitDxRWStructured res) :data(res), type(ResDxBufferType_RWStructured) {};
		ResDxBufferInitData(InitDxRenderTarget res) :data(res), type(ResDxBufferType_RenderTarget) {};
		ResDxBufferInitData(InitDxDepthStencil res) :data(res), type(ResDxBufferType_DepthStencil) {};

	private:


		union InitDxData
		{
			InitDxVertex			vertex;
			InitDxIndex				index;
			InitDxTexture			texture;
			InitDxConstant			constant;
			InitDxUpload			upload;
			InitDxStructured		structured;
			InitDxRWStructured		rwstructured;
			InitDxRenderTarget		rendertarget;
			InitDxDepthStencil		depthstencil;

			InitDxData(InitDxVertex res) :vertex(res) {}
			InitDxData(InitDxIndex res) :index(res) {}
			InitDxData(InitDxTexture res) :texture(res) {}
			InitDxData(InitDxConstant res) :constant(res) {}
			InitDxData(InitDxUpload res) :upload(res) {}
			InitDxData(InitDxStructured res) :structured(res) {}
			InitDxData(InitDxRWStructured res) :rwstructured(res) {}
			InitDxData(InitDxRenderTarget res) :rendertarget(res) {}
			InitDxData(InitDxDepthStencil res) :depthstencil(res) {}
		};
		InitDxData data;
		ResDxBufferType	type;
		friend ResDxBuffer;
	};


	class ResDxBuffer
	{
	public:
		void Init(ResDxContext2& context,const ResDxBufferInitData);
		void Init(ResDxContext2& context, const ResDxBufferInitData initData, ID3D12Resource* resource);
		void CopyCpuResource(const void* src, int offsetByteSize = 0, int srcByteSize = 0);
		//GPUコピー処理を削除したので後ほど追加する
		void Clear();
		bool IsInit()const;
		ID3D12Resource* Resource()const;
		ResDxBufferType Type()const;
		template<ResDxBufferType type>
		typename ResDxType<type>::type* Data()const { assert(type == this->type); return static_cast<typename ResDxType<type>::type*> (this->data); };

		void operator=(const ResDxBuffer&) = delete;
		void operator=(ResDxBuffer&&);
		ResDxBuffer() :data(), dataSize(), resource(), type(), isInit(),isMine() {}
		ResDxBuffer(const ResDxBuffer&) = delete;
		ResDxBuffer(ResDxBuffer&&);
		~ResDxBuffer();
	private:

		void CreateDataVertex( const ResDxBufferInitData initData);
		void CreateDataIndex(const ResDxBufferInitData initData);
		void CreateDataTexture(const ResDxBufferInitData initData);
		void CreateDataConstant(const ResDxBufferInitData initData);
		void CreateDataUpload(const ResDxBufferInitData initData, UINT uploadSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint);
		void CreateDataStructured(const ResDxBufferInitData initData);
		void CreateDataRWStructured(const ResDxBufferInitData initData,ID3D12Resource* counter=nullptr);
		void CreateDataRenderTarget(const ResDxBufferInitData initData);
		void CreateDataDepthStencil(const ResDxBufferInitData initData);


		void* data;
		UINT dataSize;
		ID3D12Resource* resource;
		ResDxBufferType type;
		bool isInit;
		bool isMine;
	};

	class ResDxDescriptorHeap
	{
	public:
		struct ResDxDescriptorHandle
		{
			int index;
			int numDescriptor;
			size_t descriptorSize;
			ResDxViewType type;
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			ID3D12Resource* resource;
		};

		void Init(ResDxContext2& context, int numDescriptor);
		void Init(ResDxContext2& context,int numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE type,D3D12_DESCRIPTOR_HEAP_FLAGS flags);
		void Clear();
		ResDxDescriptorHandle CreateShaderResourceView(ResDxContext2& context, int index, ID3D12Resource* resource,DXGI_FORMAT format);
		ResDxDescriptorHandle CreateConstantBufferView(ResDxContext2& context, int index, ID3D12Resource* resource);
		ResDxDescriptorHandle CreateUnorderedBufferView(ResDxContext2& context, int index, ID3D12Resource* resource,ID3D12Resource* count,int numElements,int sizeOfelem);
		ResDxDescriptorHandle CreateRenderTargetView(ResDxContext2& context,int index, ID3D12Resource* resource);
		ResDxDescriptorHandle CreateDepthStencilView(ResDxContext2& context,int index, ID3D12Resource* resource);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(int index);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(int index);

		ID3D12DescriptorHeap* GetDescriptorHeap()const;

		void operator=(const ResDxDescriptorHeap&) = delete;
		void operator=(ResDxDescriptorHeap&&);
		ResDxDescriptorHeap();
		ResDxDescriptorHeap(const ResDxDescriptorHeap&) = delete;
		ResDxDescriptorHeap(ResDxDescriptorHeap&&);
		~ResDxDescriptorHeap();

	private:
		ID3D12DescriptorHeap* descriptorHeap;
		size_t descriptorSize;
		int numDescriptor;
		bool isInit;

	};

	class ResDxSampler
	{
	public:

		void Init(int numSampler, D3D12_STATIC_SAMPLER_DESC* desc);
		void Init(int numSampler);
		void Init(
			D3D12_FILTER					samplerFilter,
			D3D12_TEXTURE_ADDRESS_MODE		addressU,
			D3D12_TEXTURE_ADDRESS_MODE		addressV,
			D3D12_TEXTURE_ADDRESS_MODE		addressW
		);
		void Init();

		void SetSamplerDesc(
			int								index,
			D3D12_FILTER					filter,
			D3D12_TEXTURE_ADDRESS_MODE		addressU,
			D3D12_TEXTURE_ADDRESS_MODE		addressV,
			D3D12_TEXTURE_ADDRESS_MODE		addressW,
			UINT							shaderRegister = (UINT)-1,
			UINT							registerSpace = 0,
			D3D12_COMPARISON_FUNC			comparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			D3D12_STATIC_BORDER_COLOR		borderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
			FLOAT							minLOD = 0.0f,
			FLOAT							maxLOD = D3D12_FLOAT32_MAX,
			D3D12_SHADER_VISIBILITY			shaderVisiblity = D3D12_SHADER_VISIBILITY_PIXEL
		);

		int GetNumSampler();
		D3D12_STATIC_SAMPLER_DESC* GetSampler();
		bool IsInit();
		void Clear();

		void operator=(const ResDxSampler&) = delete;
		void operator=(ResDxSampler&&);

		ResDxSampler() :samplerDesc(), numSampler(), isInit() {}
		ResDxSampler(const ResDxSampler&) = delete;
		ResDxSampler(ResDxSampler&&);
		~ResDxSampler();

	private:
		D3D12_STATIC_SAMPLER_DESC* samplerDesc;
		int numSampler;
		bool isInit;
	};

	class ResDxDescriptorRange
	{
	public:

		struct ResDxDescriptorRangeHandle
		{
			D3D12_DESCRIPTOR_RANGE* descriptorRange;
			D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle;
		};

		void Init(int numDescriptorRange);
		void Clear();
		void SetDescriptorRange(int index, D3D12_DESCRIPTOR_RANGE_TYPE type, int numDescriptor, int baseRegister, int registerSpace);
		void SetDescriptorRange(int index, ResDxViewType type, int numDescriptor, int baseRegister, int registerSpace);
		ResDxDescriptorRangeHandle SetDescriptorRange(int index, ResDxDescriptorHeap::ResDxDescriptorHandle descriptorHandle, int baseRegister, int registerSpace);
		D3D12_DESCRIPTOR_RANGE* GetDescriptorRange(int index = 0);
		bool IsInit();

		void operator=(const ResDxDescriptorRange&) = delete;
		void operator=(ResDxDescriptorRange&&);

		ResDxDescriptorRange() :descriptorRange(), numDescriptorRange(), isInit() {};
		ResDxDescriptorRange(const ResDxDescriptorRange&) = delete;
		ResDxDescriptorRange(ResDxDescriptorRange&&);
		~ResDxDescriptorRange();

	private:
		D3D12_DESCRIPTOR_RANGE* descriptorRange;
		int numDescriptorRange;
		bool isInit;
	};

	class ResDxRootSignature
	{
	public:
		struct RootDescriptorTableHandle
		{
			int rootParameterIndex;
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		};

		void Init();
		void Init(int numRootParameter);
		void Init(ResDxSampler& sampler, int numRootParameter);
		void SetUsecaseConstant(int index, uint32_t num32BitValues, int shaderRegister, int registerSpace);
		void SetUsecaseDescriptor(int index, D3D12_ROOT_PARAMETER_TYPE descriptorType, int shaderRegister, int registerSpace);
		void SetUsecaseDescriptorTable(int index, D3D12_DESCRIPTOR_RANGE* descriptorRange, int numDescriptorRange, int shaderRegister, int registerSpace);
		RootDescriptorTableHandle SetUsecaseDescriptorTable(int index, ResDxDescriptorRange::ResDxDescriptorRangeHandle descriptorRangeHandle, int shaderRegister, int registerSpace);
		void Create(ResDxContext2& context);
		void Clear();
		ID3D12RootSignature* GetRootSignature();
		bool IsInit();

		void operator=(const ResDxRootSignature&) = delete;
		void operator=(ResDxRootSignature&&);

		ResDxRootSignature();
		ResDxRootSignature(const ResDxRootSignature&) = delete;
		ResDxRootSignature(ResDxRootSignature&&);
		~ResDxRootSignature();

	private:
		ResDxSampler sampler;
		ID3D12RootSignature* rootSignature;
		D3D12_ROOT_SIGNATURE_DESC desc;
		D3D12_ROOT_PARAMETER* rootParameter;
		int numRootParameter;
		bool isInit;
	};

	class ResDxShader
	{
	public:

		void LoadPS(const wchar_t* filePath, const char* entryPointName);
		void LoadVS(const wchar_t* filePath, const char* entryPointName);
		void LoadCS(const wchar_t* filePath, const char* entryPointName);

		void LoadPS(const char* filePath, const char* entryPointName);
		void LoadVS(const char* filePath, const char* entryPointName);
		void LoadCS(const char* filePath, const char* entryPointName);

		ID3DBlob* GetCompileBlob();
		ID3D12ShaderReflection* GetReflection();
		bool IsInited();
		void Clear();

		void operator=(const ResDxShader&) = delete;
		void operator=(ResDxShader&&);

		ResDxShader() :blob(), isInit() {};
		ResDxShader(const ResDxShader&) = delete;
		ResDxShader(ResDxShader&&);
		~ResDxShader();

	private:
		void Load(const wchar_t* filePath, const char* entryPoint, const char* compileShaderModel);
		ID3DBlob* blob;
		ID3D12ShaderReflection* reflection;
		bool isInit;
	};

	class ResDxBlendState
	{
	public:
		void Init();
		void SetBlendState(int index, ResDxBlendFlag);
		D3D12_BLEND_DESC GetBlend();

		void operator=(const ResDxBlendState&);
		void operator=(ResDxBlendState&&);

		ResDxBlendState() : blend() {};
		ResDxBlendState(const ResDxBlendState&) = default;
		ResDxBlendState(ResDxBlendState&&) = default;
		~ResDxBlendState() {}

	private:

		D3D12_BLEND_DESC blend;
	};

	class ResDxRasterizer
	{
	public:

		void Init();
		void SetRasterizerState(ResDxRasterizerFlags);
		D3D12_RASTERIZER_DESC GetRasterizer();
		void operator=(const ResDxRasterizer&);
		void operator=(ResDxRasterizer&&);
		ResDxRasterizer() :rasterizer() {};
		ResDxRasterizer(const ResDxRasterizer&) = default;
		ResDxRasterizer(ResDxRasterizer&&) = default;
		~ResDxRasterizer() {}

	private:
		D3D12_RASTERIZER_DESC rasterizer;
	};


	class ResDxDepthStencil
	{
	public:
		void Init();
		void SetDepthStencil(ResDxDepthStencilFlags flags);
		DXGI_FORMAT GetFormat();
		D3D12_DEPTH_STENCIL_DESC GetDepthStencil();

		void operator=(const ResDxDepthStencil&);
		void operator=(ResDxDepthStencil&&);

		ResDxDepthStencil() :format(), desc() {};
		ResDxDepthStencil(const ResDxDepthStencil&) = default;
		ResDxDepthStencil(ResDxDepthStencil&&) = default;
		~ResDxDepthStencil() {};


	private:

		DXGI_FORMAT format;
		D3D12_DEPTH_STENCIL_DESC desc;
	};

	class ResDxScissorRect
	{
	public:
		void Init();
		void SetRect(LONG left, LONG top, LONG right, LONG bottom);
		void SetRectDefault();

		operator D3D12_RECT*();
		void operator=(const ResDxScissorRect&);
		void operator=(ResDxScissorRect&&);

		ResDxScissorRect():rect() {};
		ResDxScissorRect(const ResDxScissorRect&) = default;
		ResDxScissorRect(ResDxScissorRect&&) = default;
		~ResDxScissorRect() {};

	private:

		D3D12_RECT rect;
	};

	class ResDxViewport
	{
	public:
		void Init();
		void SetViewort(FLOAT left,FLOAT top,FLOAT width,FLOAT height,FLOAT maxDepth,FLOAT minDepth);
		void SetViewportDefault();

		operator D3D12_VIEWPORT*();
		void operator=(const ResDxViewport&);
		void operator=(ResDxViewport&&);

		ResDxViewport() :viewport() {};
		ResDxViewport(const ResDxViewport&) = default;
		ResDxViewport(ResDxViewport&&) = default;
		~ResDxViewport() {};

	private:

		D3D12_VIEWPORT viewport;
	};

	class ResDxPipelineState
	{
	public:

		void Init(ResDxContext2& context,D3D12_GRAPHICS_PIPELINE_STATE_DESC);
		void Init(ResDxContext2& context, D3D12_COMPUTE_PIPELINE_STATE_DESC);
		void Init(
			ResDxContext2&					context,
			ID3D12RootSignature*			rootSignature,
			UINT							numLayout,
			D3D12_INPUT_ELEMENT_DESC*		elementDesc,
			ResDxShader&					vs,
			ResDxShader&					ps,
			D3D12_BLEND_DESC				blend,
			D3D12_DEPTH_STENCIL_DESC		depthStencil,
			D3D12_RASTERIZER_DESC			rasterizer,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE	topology,
			UINT							numRenderTarget,
			DXGI_FORMAT						RTVFormats[],
			DXGI_FORMAT						DSVFormat
		);
		void Init(
			ResDxContext2&					context,
			ID3D12RootSignature*			rootSignature,
			UINT							numLayout,
			D3D12_INPUT_ELEMENT_DESC*		elementDesc,
			ResDxShader&					vs,
			ResDxShader&					ps,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE	topology,
			UINT							numRenderTarget,
			DXGI_FORMAT						RTVFormats[],
			DXGI_FORMAT						DSVFormat
		);

		void Init(
			ResDxContext2&					context,
			ID3D12RootSignature*			rootSignature,
			ResDxInputElementFlags			inputElem,
			ResDxShader&					vs,
			ResDxShader&					ps,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE	topology,
			UINT							numRenderTarget,
			DXGI_FORMAT						RTVFormats[],
			DXGI_FORMAT						DSVFormat
		);

		void Init(
			ResDxContext2&					context,
			ID3D12RootSignature*			rootSignature,
			ResDxShader&					cs,
			D3D12_PIPELINE_STATE_FLAGS		flags,
			UINT							nodeMask
		);

		bool IsInit();
		void Clear();

		ID3D12PipelineState* GetPipelineState()const;

		void operator=(const ResDxPipelineState&) = delete;
		void operator=(ResDxPipelineState&&);

		ResDxPipelineState();
		ResDxPipelineState(const ResDxPipelineState&) = delete;
		ResDxPipelineState(ResDxPipelineState&&);
		~ResDxPipelineState();

	private:

		ID3D12PipelineState* pipeline;
		bool isInit;

	};
	/*
		・commandlist
		open()
		条件：登録しているアロケータが他のコマンドから編集できる状態でないこと
		
		close()
		条件：とくになし

		
		・commandallocator
		reset()
		条件：参照しているコマンドリストのgpu処理が全て終了していること、複数スレッドから同時に呼び出してはいけない
		
		・commandqueue
		execute()
		条件：コマンドリストがクローズしてること、対象コマンドリストのgpu実行が終了してること
		*/

	class ResDxCommandList
	{
	public:

		void Init(ID3D12Device5* device,ID3D12CommandAllocator* allocator,D3D12_COMMAND_LIST_TYPE type);
		void Reset(ID3D12CommandAllocator* allocator);
		void Close();
		void Release();
		ID3D12GraphicsCommandList4* Get();
		D3D12_COMMAND_LIST_TYPE Type();

		void operator=(const ResDxCommandList&) = delete;
		void operator=(ResDxCommandList&&);

		ResDxCommandList() :type(), commandList() {};
		ResDxCommandList(const ResDxCommandList&) = delete;
		ResDxCommandList(ResDxCommandList&&);
		~ResDxCommandList();
	private:
		D3D12_COMMAND_LIST_TYPE type;
		ID3D12GraphicsCommandList4* commandList;
	};

	class ResDxCommandListCopy
	{
	public:

		void CopyGpuResource(ID3D12Resource* dstResource, ID3D12Resource* srcResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint);
		void ExecuteBundle(ID3D12GraphicsCommandList*);

		operator ResDx::ResDxCommandList *()const;
		void operator=(const ResDxCommandListCopy&);
		void operator=(ResDxCommandListCopy&&);
		
		ResDxCommandListCopy(ResDxCommandList* commandList) :resDxCommandList(commandList),commandList(commandList->Get()) {};
		ResDxCommandListCopy(const ResDxCommandListCopy&) = default;
		ResDxCommandListCopy(ResDxCommandListCopy&&) = default;
		~ResDxCommandListCopy() {};
	private:
		ResDxCommandList* resDxCommandList;
		ID3D12GraphicsCommandList4* commandList;
	};

	class ResDxCommandListDirect
	{
	public:
		void SetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW* vertexBufferView);
		void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView);
		void SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap);
		void SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps);
		void SetGraphicsDescriptorTable(UINT indexRootParameter, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle);
		void SetGraphicsRootSignature(ID3D12RootSignature* rootSignature);
		void SetComputeRootSignature(ID3D12RootSignature* rootSignature);
		void SetGraphicsRoot32BitConstant(UINT rootParameter, UINT src, UINT dstOffset);
		void SetGraphicsRoot32BitConstants(UINT rootParameter, UINT num32BitValue, const void* src, UINT dstOffset);
		void SetPipelineState(ID3D12PipelineState* pipelineState);
		void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV);
		void SetRenderTargets(UINT numDescriptorRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV);
		void SetScissorRect(D3D12_RECT* rect);
		void SetViewport(D3D12_VIEWPORT* viewport);
		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

		void ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
		void ResourceBarriers(UINT numResource, ID3D12Resource** resources, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

		void DrawInstanced(UINT numVertex);
		void DrawIndexedInstanced(UINT numIndex);
		void DrawIndexedInstanced(UINT numIndex, UINT numInstance);
		void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);
		void ExecuteBundle(ID3D12GraphicsCommandList*);

		void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE view,const FLOAT* color,const UINT numRect,const D3D12_RECT* rects);
		void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE view, D3D12_CLEAR_FLAGS flags,FLOAT depth,UINT stencil, const UINT numRect, const D3D12_RECT* rects);

		void CopyGpuResource(ID3D12Resource* dstResource, ID3D12Resource* srcResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint);
		ResDx::ResDxCommandList* Get();
		operator ResDx::ResDxCommandList *()const;
		void operator=(const ResDxCommandListDirect&);
		void operator=(ResDxCommandListDirect&&);

		ResDxCommandListDirect(ResDxCommandList* commandList) :resDxCommandList(commandList), commandList(commandList->Get()) {};
		ResDxCommandListDirect(const ResDxCommandListDirect&) = default;
		ResDxCommandListDirect(ResDxCommandListDirect&&) = default;
		~ResDxCommandListDirect() {};

	private:
		ResDxCommandList* resDxCommandList;
		ID3D12GraphicsCommandList4* commandList;
	};

	class ResDxCommandListCompute
	{
	public:
		void SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap);
		void SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps);
		void SetComputeRootSignature(ID3D12RootSignature* rootSignature); 
		void SetPipelineState(ID3D12PipelineState* pipelineState);
		
		void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);
		void ExecuteBundle(ID3D12GraphicsCommandList*);

		operator ResDx::ResDxCommandList *()const;
		void operator=(const ResDxCommandListCompute&);
		void operator=(ResDxCommandListCompute&&);

		ResDxCommandListCompute(ResDxCommandList* commandList) :resDxCommandList(commandList), commandList(commandList->Get()) {};
		ResDxCommandListCompute(const ResDxCommandListCompute&) = default;
		ResDxCommandListCompute(ResDxCommandListCompute&&) = default;
		~ResDxCommandListCompute() {};

	private:

		ResDxCommandList* resDxCommandList;
		ID3D12GraphicsCommandList4* commandList;
	};

	class ResDxCommandListBundle
	{
	public:

		void SetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW* vertexBufferView);
		void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView);
		void SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap);
		void SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps);
		void SetGraphicsDescriptorTable(UINT indexRootParameter, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle);
		void SetGraphicsRootSignature(ID3D12RootSignature* rootSignature);
		void SetComputeRootSignature(ID3D12RootSignature* rootSignature);
		void SetGraphicsRoot32BitConstant(UINT rootParameter, UINT src, UINT dstOffset);
		void SetGraphicsRoot32BitConstants(UINT rootParameter, UINT num32BitValue, const void* src, UINT dstOffset);
		void SetPipelineState(ID3D12PipelineState* pipelineState);
		void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV);
		void SetRenderTargets(UINT numDescriptorRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV);
		void SetScissorRect(D3D12_RECT* rect);
		void SetViewport(D3D12_VIEWPORT* viewport);
		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);

		void DrawInstanced(UINT numVertex);
		void DrawIndexedInstanced(UINT numIndex);
		void DrawIndexedInstanced(UINT numIndex, UINT numInstance);
		void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);

		operator ResDx::ResDxCommandList *()const;
		void operator=(const ResDxCommandListBundle&);
		void operator=(ResDxCommandListBundle&&);

		ResDxCommandListBundle(ResDxCommandList* commandList) :resDxCommandList(commandList), commandList(commandList->Get()) {};
		ResDxCommandListBundle(const ResDxCommandListBundle&) = default;
		ResDxCommandListBundle(ResDxCommandListBundle&&) = default;
		~ResDxCommandListBundle() {};

	private:

		ResDxCommandList* resDxCommandList;
		ID3D12GraphicsCommandList4* commandList;
	};

	class ResDxCommandAllocator
	{
	public:

		void Init(ID3D12Device5* device, D3D12_COMMAND_LIST_TYPE type);
		void Reset();
		void Release();
		ID3D12CommandAllocator* Get();

		void operator=(const ResDxCommandAllocator&) = delete;
		void operator=(ResDxCommandAllocator&&);

		ResDxCommandAllocator() :type(), allocator() {};
		ResDxCommandAllocator(const ResDxCommandAllocator&) = delete;
		ResDxCommandAllocator(ResDxCommandAllocator&&);
		~ResDxCommandAllocator();

	private:
		D3D12_COMMAND_LIST_TYPE type;
		ID3D12CommandAllocator* allocator;
	};

	class ResDxCommandQueue
	{
	public:

		void Init(ID3D12Device5* device, D3D12_COMMAND_LIST_TYPE type);
		void WaitCompleteExecute();
		void Release();
		UINT64 Execute(ResDxCommandList* commandList);
		UINT64 Execute(int numCommandList, ResDxCommandList* commandList[]);
		UINT64 FenceCompletedValue()const;
		bool Completed()const;
		UINT64 FenceValue()const;
		void ClearFenceValue();
		ID3D12CommandQueue* GetQueue();

		void operator=(const ResDxCommandQueue&) = delete;
		void operator=(ResDxCommandQueue&&);

		ResDxCommandQueue():commandQueue(),fence(),fenceValue(){}
		ResDxCommandQueue(const ResDxCommandQueue&) = delete;
		ResDxCommandQueue(ResDxCommandQueue&&);
		~ResDxCommandQueue();

	private:

		ID3D12CommandQueue* commandQueue;
		ID3D12Fence* fence;
		std::atomic_ullong fenceValue;
	};

	class ResDxSwapChain
	{
	public:

		void Init(GraphicsDevice* device,ID3D12CommandQueue* commandQueue, int numFrame, DXGI_FORMAT rtvFormat);
		void Present();
		void Release();
		ID3D12Resource* GetFrameRenderTargetBuffer(UINT index);
		const int GetFrameBufferWidth();
		const int GetFrameBufferHight();
		const UINT GetBackBufferIndex();

		void operator=(const ResDxSwapChain&) = delete;
		void operator=(ResDxSwapChain&&);

		ResDxSwapChain():swapchain() {};
		ResDxSwapChain(const ResDxSwapChain&) = delete;
		ResDxSwapChain(ResDxSwapChain&&);
		~ResDxSwapChain();

	private:

		IDXGISwapChain3* swapchain;
		ID3D12Resource** resources;
		int numFrame;
		int width;
		int height;
	};

	class ResDxContext2
	{
	public:
		void CreateVertexBuffer(int bufferSize, int stride, ID3D12Resource** buffer);
		void CreateIndexBuffer(int bufferSize, int stride,ID3D12Resource** buffer);
		void CreateTextureBuffer(int width, int height, DXGI_FORMAT, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Resource** buffer);
		void CreateRenderTargetBuffer(int width, int height, DXGI_FORMAT format,ID3D12Resource** buffer);
		void CreateDepthStencilBuffer(int width, int height, DXGI_FORMAT format,ID3D12Resource** buffer);
		void CreateConstantBuffer(int bufferSize, ID3D12Resource** buffer);
		void CreateUploadBuffer(int width, int height, DXGI_FORMAT, ID3D12Resource** buffer, UINT64* uploadSize = nullptr, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* footprint = nullptr);
		void CreateStructuredBuffer(int sizeOfElem, int numOfElem, ID3D12Resource** buffer);
		void CreateRWStructuredBuffer(int sizeOfElem, int numOfElem, ID3D12Resource** buffer,ID3D12Resource** counter);
		void CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC& desc, ID3D12DescriptorHeap**);
		void CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateUnorderedAccessView(ID3D12Resource* resource, ID3D12Resource* counter, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateRootSignature(UINT nodeMask, const void* blobWithRootSignature, UINT blobLenghInBytes, ID3D12RootSignature**);
		void CreateGraphicsPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState);
		void CreateComputePipelineState(D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState);
		
		void CopyDescriptorSimple(UINT numDescriptor, ResDxDescriptorHeap* dst, UINT dstStartIndex, ResDxDescriptorHeap* src, UINT srcStartIndex, D3D12_DESCRIPTOR_HEAP_TYPE type);
		void CopyDescriptors(const UINT numDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE dstHandles[], const D3D12_CPU_DESCRIPTOR_HANDLE srcHandles[], const UINT numDescriptors[], D3D12_DESCRIPTOR_HEAP_TYPE type);

		UINT GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type);
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT GetPlacedFootprint(ID3D12Resource*, UINT64& uploadSize, UINT64& totalByte);

		void operator=(const ResDxContext2&);
		void operator= (ResDxContext2&&);

		ResDxContext2(GraphicsDevice* device) :device(device->GetDevice()) {};
		ResDxContext2(const ResDxContext2&) = default;
		ResDxContext2(ResDxContext2&&) = default;
		~ResDxContext2() {};

	private:
		ID3D12Device5* device;
	};

	//スーパークラスなので分割しよう
	class ResDxContext
	{
	public:
		void CreateVertexBuffer(int bufferSize, int stride, ResDxBufferVertex& data, ID3D12Resource** buffer);
		void CreateIndexBuffer(int bufferSize, int stride, ResDxBufferIndex& data, ID3D12Resource** buffer);
		void CreateTextureBuffer(int width, int height, DXGI_FORMAT, D3D12_RESOURCE_FLAGS flags, ResDxBufferTexture& data, ID3D12Resource** buffer);
		void CreateConstantBuffer(int bufferSize, ResDxBufferConstant& data, ID3D12Resource** buffer);
		void CreateUploadBuffer(int width, int height, DXGI_FORMAT, ResDxBufferUpload& data, ID3D12Resource** buffer);
		void CreateStructuredBuffer(int sizeOfElem, int numOfElem, ResDxBufferStructured& data, ID3D12Resource** buffer);
		void CreateRWStructuredBuffer(int sizeOfElem, int numOfElem, ResDxBufferRWStructured& data, ID3D12Resource** buffer);
		void CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC& desc, ID3D12DescriptorHeap**);
		void CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateUnorderedAccessView(ID3D12Resource* resource, ID3D12Resource* counter, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateRootSignature(UINT nodeMask, const void* blobWithRootSignature, UINT blobLenghInBytes, ID3D12RootSignature**);
		void CreateGraphicsPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState);
		void CreateComputePipelineState(D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState);

		void ReleaseResource(ID3D12Resource* resource);
		void ReleaseDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap);
		void ReleaseRootSignature(ID3D12RootSignature* rootSignature);
		void ReleasePipelineState(ID3D12PipelineState* pipelineState);
		void ReleaseBlob(ID3DBlob* blob);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT GetPlacedFootprint(ID3D12Resource*);
		DXGI_FORMAT GetFrameRTVFormat();
		DXGI_FORMAT GetFrameDSVFormat();
		UINT GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type);
		UINT GetBackBufferIndex();
		int GetFrameRenderTargetWidth();
		int GetFrameRenderTargetHeight();
		float GetFrameAspect();

		void SetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW* vertexBufferView);
		void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView);
		void SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap);
		void SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps);
		void SetGraphicsDescriptorTable(UINT indexRootParameter, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle);
		void SetGraphicsRootSignature(ID3D12RootSignature* rootSignature);
		void SetComputeRootSignature(ID3D12RootSignature* rootSignature);
		void SetGraphicsRoot32BitConstant(UINT rootParameter, UINT src, UINT dstOffset);
		void SetGraphicsRoot32BitConstants(UINT rootParameter, UINT num32BitValue, const void* src, UINT dstOffset);
		void SetPipelineState(ID3D12PipelineState* pipelineState);
		void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV);
		void SetRenderTargets(UINT numDescriptorRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV);
		void SetScissorRect(D3D12_RECT* rect);
		void SetViewport(D3D12_VIEWPORT* viewport);
		void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
		void SetFrameScissorRect();
		void SetFrameViewport();
		void SetFrameRenderTarget();
		void SetDevice(ID3D12Device* device, ID3D12GraphicsCommandList* command);

		void ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
		void ResourceBarriers(UINT numResource, ID3D12Resource** resources, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

		void DrawInstanced(UINT numVertex);
		void DrawIndexedInstanced(UINT numIndex);
		void DrawIndexedInstanced(UINT numIndex, UINT numInstance);
		void Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ);

		void CopyGpuResource(ID3D12Resource* dstResource, ID3D12Resource* srcResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint);
		void CopyGpuResourceFromTempUploadBuffer(ID3D12Resource* dstResource, ResDxBuffer&& tempUploadBuffer, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint);
		void CopyDescriptorSimple(UINT numDescriptor, ResDxDescriptorHeap* dst, UINT dstStartIndex, ResDxDescriptorHeap* src, UINT srcStartIndex, D3D12_DESCRIPTOR_HEAP_TYPE type);
		void CopyDescriptors(const UINT numDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE dstHandles[], const D3D12_CPU_DESCRIPTOR_HANDLE srcHandles[], const UINT numDescriptors[], D3D12_DESCRIPTOR_HEAP_TYPE type);

		void BeginRender();
		void EndRender();

		static ResDxContext& instance();
	private:

		void ClearTempBuffer(size_t index);

		ResDxContext();
		ResDxContext(const ResDxContext&) = delete;
		void operator=(const ResDxContext&) = delete;
		ID3D12Device* device;
		ID3D12GraphicsCommandList* commandList;
		ResDxBuffer temp[RES_DX_BUFFER_COUNT][RES_DX_MAX_NUM_TEMP_BUFFER];
		size_t tempBufferCount;
	};

	class ResDxIO
	{
	public:
		virtual void ResDxIO_Camera(ResDxCamera*) {};

		ResDxIO() :mouse(&g_mouse), key(&g_keybord) {}
		ResDxIO(const ResDxIO&) = default;
		virtual ~ResDxIO() {}

	protected:
		Mouse* mouse;
		KeyBord* key;
	};

	class ResDxIODefault :public ResDxIO
	{
	public:
		void ResDxIO_Camera(ResDxCamera*)override;
		static ResDxIODefault& instance();
	};

	struct ResDxVector4
	{
		operator DirectX::XMVECTOR()const;
		operator DirectX::XMVECTOR* ();
		operator DirectX::XMFLOAT4()const;
		operator DirectX::XMFLOAT3()const;
		operator DirectX::XMFLOAT2()const;

		void operator=(const DirectX::XMVECTOR&);
		void operator=(const DirectX::XMFLOAT2&);
		void operator=(const DirectX::XMFLOAT3&);
		void operator=(const DirectX::XMFLOAT4&);

		void operator=(const ResDxVector4&);
		void operator=(ResDxVector4&&);

		ResDxVector4 operator*(const ResDxVector4&);
		ResDxVector4 operator+(const ResDxVector4&);
		ResDxVector4 operator-(const ResDxVector4&);

		ResDxVector4 operator*(float);

		float x();
		float y();
		float z();
		float w();

		ResDxVector4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f);
		ResDxVector4(DirectX::XMVECTOR src) :value() { *this = src; };
		ResDxVector4(DirectX::XMFLOAT2 src) :value() { *this = src; };
		ResDxVector4(DirectX::XMFLOAT3 src) :value() { *this = src; };
		ResDxVector4(DirectX::XMFLOAT4 src) :value() { *this = src; };
		ResDxVector4(const ResDxVector4& src) :value() { *this = src; };
		ResDxVector4(ResDxVector4&& src) noexcept:value() { *this = std::move(src); };
		DirectX::XMVECTOR value;
	};

	struct ResDxVector4x4
	{
		operator DirectX::XMMATRIX()const;
		operator DirectX::XMMATRIX* ();

		void operator=(const DirectX::XMMATRIX&);
		void operator=(DirectX::XMMATRIX&&);

		void operator=(const ResDxVector4x4&);
		void operator=(ResDxVector4x4&&);

		ResDxVector4x4 operator*(const ResDxVector4x4)const;

		ResDxMatrix Inverse()const;

		ResDxVector4x4() :matrix() {};
		ResDxVector4x4(DirectX::XMMATRIX matrix) :matrix(matrix) {}
		ResDxVector4x4(const ResDxVector4x4& matrix) :matrix() { *this = matrix; }
		ResDxVector4x4(ResDxVector4x4&& matrix)noexcept :matrix() { *this = matrix; }

	private:

		DirectX::XMMATRIX matrix;
	};

	struct ResDxQuaternion
	{
		void SetQuaternionRotationEuler(float pitch, float yaw, float roll);
		void SetQuaternionRotationEuler(const ResDxVector rotate);
		void SetQuaternionRotationAxis(const ResDxVector axis, const float angle);
		void SetQuaternionSlep(const ResDxQuaternion q1, const ResDxQuaternion q2, float t);
		void SetQuaternionRotationMatrix(const ResDxMatrix rotation);

		ResDxMatrix GetRotationMatrix();
		ResDxQuaternion GetQuaternionInverse();

		ResDxQuaternion operator*(const ResDxQuaternion&);
		operator DirectX::XMVECTOR()const;
		operator DirectX::XMFLOAT4()const;

		void operator=(const DirectX::XMVECTOR&);
		void operator=(const DirectX::XMFLOAT4&);

		void operator=(const ResDxQuaternion&);
		void operator=(ResDxQuaternion&&);
		ResDxQuaternion(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) :value() {};
		ResDxQuaternion(DirectX::XMVECTOR src) :value() { *this = src; };
		ResDxQuaternion(DirectX::XMFLOAT4 src) :value() { *this = src; };
		ResDxQuaternion(const ResDxQuaternion& src) :value() { *this = src; };
		ResDxQuaternion(ResDxQuaternion&& src)noexcept :value() { *this = std::move(src); };
		DirectX::XMVECTOR value;
	};

	class ResDxTransform
	{
	public:

		using location_t = ResDxLocation;
		using rotation_t = ResDxRotation;
		using scale_t = ResDxScale;
		using matrix_t = ResDxMatrix;

		void SetLocation(location_t vector);
		void SetRotation(rotation_t quaternion);
		void SetRotationSlep(rotation_t quaternion, float t);
		void SetRotationEuler(float pitch, float yaw, float roll);
		void SetScale(scale_t vector);

		void AddLocation(location_t vector);
		void AddRotation(rotation_t);
		void AddRotationEuler(float pitch, float yaw, float roll);
		void AddScale(scale_t vector);

		matrix_t GetTransform()const;
		location_t GetLocation()const;
		rotation_t GetRotation()const;
		scale_t GetScale()const;

		void operator=(const ResDxTransform&);
		void operator=(ResDxTransform&&);
		ResDxTransform() :loc(), rot(), scl() {};
		ResDxTransform(const ResDxTransform&);
		ResDxTransform(ResDxTransform&&);

	public:

		//void QueryCalcrateTransform(const ResDxVector src, const ResDxTransformFlags flags);

		location_t loc;
		rotation_t rot;
		scale_t scl;
	};

	class ResDxAngle
	{
	public:
		using angle_t = ResDxVector3;

		void SetAngleEuler(float x, float y, float z);
		void SetAngleEuler(ResDxVector);
		void SetAngleRadian(float x, float y, float z);
		void SetAngleRadian(ResDxVector);

		angle_t GetAngleEuler();
		angle_t GetAngleRadian();
		ResDxQuaternion GetAngleQuaternion();

		float RadianX();
		float RadianY();
		float RadianZ();

		operator angle_t();
		void operator=(const ResDxAngle&);
		void operator=(ResDxAngle&&);

		ResDxAngle() :value() {};
		ResDxAngle(const ResDxAngle& angle) :value() { *this = angle; };
		ResDxAngle(ResDxAngle&& angle)noexcept :value() { *this = std::move(angle); };

	private:

		angle_t TranslateEulerToRadian(angle_t);
		angle_t TranslateRadianToEuler(angle_t);

		angle_t value;
	};

	struct ResDxPointLight
	{
		ResDxVector pos;
		float color[4];
		float brightness;
	};

	class ResDxCamera
	{
	public:

		void Init(
			ResDxVector pos,
			ResDxVector target,
			ResDxVector up,
			float fovY = DirectX::XM_PIDIV2,
			float aspect = (float)DEFAULT_WINDOW_WIDTH / (float)DEFAULT_WINDOW_HEIGHT,
			float nearZ = 0.5f,
			float farZ = 500.0f,
			ResDxCameraFlags flags = ResDxCameraFlags_None,
			ResDxIO* io = &ResDxIODefault::instance()
		);

		void Update();

		void MoveEyePos(ResDxVector vector);
		void MoveTargetPos(ResDxVector vector);
		void MoveForward(float quantity);
		void MoveRight(float quantity);
		void MoveUp(float quantity);
		void Rotation(ResDxVector rotation);

		void LocalMoveEyePos(ResDxVector vector);
		void LocalMoveTargetPos(ResDxVector vecotr);
		void LocalRotation(ResDxVector rotation);

		void WorldMoveEyePos(ResDxVector vector);
		void WorldMoveTargetPos(ResDxVector vector);
		void WorldRotation(ResDxVector rotation);

		ResDxCameraFlags GetCameraFlags();
		void SetCameraFlags(ResDxCameraFlags flags);

		ResDxMatrix GetViewMatrix();
		ResDxMatrix GetProjectionMatrix();

		void SetIO(ResDxIO*);

		void operator=(const ResDxCamera&);
		ResDxCamera();
		ResDxCamera(const ResDxCamera&) = default;
		~ResDxCamera() {}

	private:

		typedef void(ResDxCamera::* ResDxCameraQueryFunctionType)(ResDxVector);

		void QueryCameraTransform(
			ResDxCameraQueryFunctionType	local,
			ResDxCameraQueryFunctionType	world,
			ResDxVector						vector,
			ResDxCameraFlags				coordinateFlagsX,
			ResDxCameraFlags				coordinateFlagsY,
			ResDxCameraFlags				coordinateFlagsZ
		);

		ResDxMatrix GetMatrixLocalRotation(ResDxVector rotation);
		ResDxMatrix GetMatrixWorldRotation(ResDxVector rotation);

		ResDxVector eyePos;
		ResDxVector targetPos;
		ResDxVector upVector;
		ResDxVector forwardVector;
		ResDxVector rightVector;

		float fovY;
		float aspect;
		float nearZ;
		float farZ;

		ResDxMatrix viewMatrix;
		ResDxMatrix projectionMatrix;

		ResDxCameraFlags flags;

		ResDxIO* io;
	};


	class ResDxTimer
	{
	public:


	private:
	};
}