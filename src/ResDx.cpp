#include "ResDx.h"
#define MESSAGE_RESDX "message ResDx"

std::hash<size_t> ResDx::ResDxID::hasher;
std::random_device ResDx::ResDxID::random;
ResDx::map_t<size_t, ResDx::id_t> ResDx::ResDxID::map;
ResDx::freelist_t<ResDx::ResDxID> ResDx::ResDxID::freelist;

const ResDx::ResDxResourceFlags ResDx::ResDxResourceMeshFlags_VertexTypeMask
=
ResDx::ResDxResourceMeshFlags_VertexType1 |
ResDx::ResDxResourceMeshFlags_VertexType2 |
ResDx::ResDxResourceMeshFlags_VertexType3;

const ResDx::ResDxResourceFlags ResDx::ResDxResourceMeshFlags_IndexTypeMask
=
ResDx::ResDxResourceMeshFlags_IndexType1 |
ResDx::ResDxResourceMeshFlags_IndexType2;

const ResDx::ResDxResourceFlags ResDx::ResDxResourceRendererFlags_PrimitiveTypeMask
=
ResDx::ResDxResourceRendererFlags_Triangle;

D3D12_INPUT_ELEMENT_DESC ResDx::ResDxVertex1Elem[] =
{
		{ "POSITION"	,0 ,DXGI_FORMAT_R32G32B32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL"		,0 ,DXGI_FORMAT_R32G32B32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TANGENT"		,0 ,DXGI_FORMAT_R32G32B32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BINORMAL"	,0 ,DXGI_FORMAT_R32G32B32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD"	,0 ,DXGI_FORMAT_R32G32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONENO"		,0 ,DXGI_FORMAT_R16G16B16A16_UINT	,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONENO"		,1 ,DXGI_FORMAT_R16G16B16A16_UINT	,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "WEIGHT"		,0 ,DXGI_FORMAT_R32G32B32A32_FLOAT	,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "WEIGHT"		,1 ,DXGI_FORMAT_R32G32B32A32_FLOAT	,0 ,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
};

D3D12_INPUT_ELEMENT_DESC ResDx::ResDxVertex2Elem[] =
{

		{ "POSITION"	,0 ,DXGI_FORMAT_R32G32B32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL"		,0 ,DXGI_FORMAT_R32G32B32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD"	,0 ,DXGI_FORMAT_R32G32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};
D3D12_INPUT_ELEMENT_DESC ResDx::ResDxVertex3Elem[] =
{
		{ "POSITION"	,0 ,DXGI_FORMAT_R32G32B32_FLOAT		,0 ,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};

size_type ResDx::ResDxVertex1ElemSize = sizeof(ResDx::ResDxVertex1Elem) / sizeof(ResDx::ResDxVertex1Elem[0]);
size_type ResDx::ResDxVertex2ElemSize = sizeof(ResDx::ResDxVertex2Elem) / sizeof(ResDx::ResDxVertex2Elem[0]);
size_type ResDx::ResDxVertex3ElemSize = sizeof(ResDx::ResDxVertex3Elem) / sizeof(ResDx::ResDxVertex3Elem[0]);

ResDx::ResDxID::ResDxID(ResDxID&& id)
{
	memcpy_s(this->id, RES_DX_ID_LEN + 1, id.id, RES_DX_ID_LEN);
	generated = true;

	memset(id.id, 0, RES_DX_ID_LEN);
	id.generated = false;
}

ResDx::ResDxID::ResDxID() :id(), generated()
{
}

ResDx::ResDxID::~ResDxID()
{
	clear();
}

void ResDx::ResDxID::generate()
{
	do
	{
		for (UINT i = 0; i < _countof(id); i += sizeof(UINT16))
		{
			UINT32 v = (UINT32)random() % RAND_MAX;
			memcpy(&id[i], &v, sizeof(UINT16));
		}

	} while (map.find(hash()));

	generated = true;
}

void ResDx::ResDxID::clear()
{
	if (!valid())
		return;
	freelist.Release(map[hash()]);
	map.erase(hash());
	for (int i = 0; i < _countof(id); ++i)
		id[i] = 0;
	generated = false;
}

size_type ResDx::ResDxID::hash() const
{
	size_type result = 0;
	for (int i = 0; i < RES_DX_ID_LEN / sizeof(UINT); ++i)
	{
		result = (result << 8) ^ ((hasher(*(UINT*)&id[i * sizeof(UINT)])) >> 24);
	}

	return result;
}

bool ResDx::ResDxID::same(const ResDxID& id) const
{
	for (UINT i = 0; i < _countof(this->id); ++i)
		if (this->id[i] != id.id[i])
			return false;

	return true;
}

bool ResDx::ResDxID::valid() const
{
	return generated;
}

bool ResDx::ResDxID::operator==(const ResDxID& id) const
{
	return same(id);
}

bool ResDx::ResDxID::operator!=(const ResDxID& id) const
{
	return !same(id);
}

ResDx::ResDxID::operator ResDxIDHandle() const
{
	return ResDxIDHandle(*this);
}


ResDx::ResDxID::ResDxIDHandle ResDx::ResDxID::create()
{
	auto obj = freelist.Get();
	obj->generate();
	map[obj->hash()] = obj;
	return ResDxIDHandle(*obj);
}

void ResDx::ResDxID::destroy(const ResDxIDHandle& id)
{
	map[id.hash()]->clear();
}


void ResDx::ResDxRendMesh::Init(size_type numMesh, size_type numTextureBuffer, size_type numConstantBuffer)
{
	UINT numBuffers = numMesh * 2 + numTextureBuffer * 2 + numConstantBuffer;
	buffers = new ResDxBuffer[numBuffers]();

	this->numMesh = numMesh;
	this->numTextureBuffer = numTextureBuffer;
	this->numConstantBuffer = numConstantBuffer;

}

void ResDx::ResDxRendMesh::SetMesh(ResDxContext2& context, int index, int numVertex, int strideVertex, void* srcVertex, int numIndex, int strideIndex, void* srcIndex)
{
	assert((UINT)index < numMesh);

	UINT verticesIdx = index + OffsetVertexBuffer();
	UINT indicesIdx = index + OffsetIndexBuffer();

	buffers[verticesIdx].Init(context, InitDxVertex(numVertex * strideVertex, strideVertex));
	buffers[indicesIdx].Init(context, InitDxIndex(numIndex * strideIndex, strideIndex));

	buffers[verticesIdx].CopyCpuResource(srcVertex);
	buffers[indicesIdx].CopyCpuResource(srcIndex);
}

void ResDx::ResDxRendMesh::SetTexture(ResDxContext2& context, int index, int width, int height, DXGI_FORMAT format, void* src)
{
	assert((UINT)index < numTextureBuffer);

	UINT textureIdx = index + OffsetTextureBuffer();
	UINT uploadIdx = index + OffsetUploadBuffer();

	buffers[textureIdx].Init(context, InitDxTexture(width, height, format));
	buffers[uploadIdx].Init(context, InitDxUpload(width, height, format));

	buffers[uploadIdx].CopyCpuResource(src);

}

void ResDx::ResDxRendMesh::CopyConstant(int index, void* src)
{
	assert((UINT)index < numConstantBuffer);
	buffers[index + OffsetConstantBuffer()].CopyCpuResource(src);
}


void ResDx::ResDxRendMesh::SetConstant(ResDxContext2& context, int index, int bufferSize, void* src)
{
	assert((UINT)index < numConstantBuffer);

	UINT indexConstantBuffer = index + OffsetConstantBuffer();

	buffers[indexConstantBuffer].Init(context, InitDxConstant(bufferSize));
	buffers[indexConstantBuffer].CopyCpuResource(src);
}

size_type ResDx::ResDxRendMesh::GetNumMesh()
{
	return numMesh;
}

size_type ResDx::ResDxRendMesh::GetNumTexture()
{
	return numTextureBuffer;
}

size_type ResDx::ResDxRendMesh::GetNumConstant()
{
	return numConstantBuffer;
}

size_type ResDx::ResDxRendMesh::GetNumVertices(int meshIndex)
{
	assert((size_type)meshIndex < numMesh);
	return buffers[meshIndex + OffsetVertexBuffer()].Data<ResDxBufferType_Vertex>()->numVertex;
}

size_type ResDx::ResDxRendMesh::GetNumIndices(int meshIndex)
{
	assert((size_type)meshIndex < numMesh);
	return buffers[meshIndex + OffsetIndexBuffer()].Data<ResDxBufferType_Index>()->numIndex;
}

void ResDx::ResDxRendMesh::Clear()
{
	if (numMesh > 0)
		for (size_type i = 0; i < numMesh; ++i)
		{
			buffers[i + OffsetVertexBuffer()].Clear();
			buffers[i + OffsetIndexBuffer()].Clear();
		}
	if (numTextureBuffer > 0)
		for (size_type i = 0; i < numTextureBuffer; ++i)
		{
			buffers[i + OffsetTextureBuffer()].Clear();
			buffers[i + OffsetUploadBuffer()].Clear();
		}
	if (numConstantBuffer)
		for (size_type i = 0; i < numConstantBuffer; ++i)
			buffers[i + OffsetConstantBuffer()].Clear();

	delete[] buffers;
	buffers = nullptr;

	numMesh = 0;
	numTextureBuffer = 0;
	numConstantBuffer = 0;
}

ResDx::ResDxBuffer* ResDx::ResDxRendMesh::Resource(size_type index, ResDxBufferType type)
{
	switch (type)
	{
	case ResDxBufferType_None:
		break;
	case ResDxBufferType_Vertex:
		return &buffers[OffsetVertexBuffer() + index];
		break;
	case ResDxBufferType_Index:
		return &buffers[OffsetIndexBuffer() + index];
		break;
	case ResDxBufferType_Texture:
		return &buffers[OffsetTextureBuffer() + index];
		break;
	case ResDxBufferType_Constant:
		return &buffers[OffsetConstantBuffer() + index];
		break;
	case ResDxBufferType_Upload:
		return &buffers[OffsetUploadBuffer() + index];
		break;
	case ResDxBufferType_Structured:
		assert(0);
		break;
	case ResDxBufferType_RWStructured:
		assert(0);
		break;
	default:
		assert(0);
		break;
	}
	return nullptr;
}




void ResDx::ResDxMeshVertex::Init(size_type numVertex, ResDxResourceFlags vertexType)
{
	ResDxResourceFlags type = vertexType & ResDxResourceMeshFlags_VertexTypeMask;

	switch (type)
	{
	case ResDxResourceMeshFlags_VertexType1:
		vertexData.vertex1 = new ResDxVertex1[numVertex];
		stride = sizeof(ResDxVertex1);
		break;
	case ResDxResourceMeshFlags_VertexType2:
		vertexData.vertex2 = new ResDxVertex2[numVertex];
		stride = sizeof(ResDxVertex2);
		break;
	case ResDxResourceMeshFlags_VertexType3:
		vertexData.vertex3 = new ResDxVertex3[numVertex];
		stride = sizeof(ResDxVertex3);
		break;

	default:
		assert(false);
		break;
	}

	this->numVertex = numVertex;
	this->vertexType = type;
	isInit = true;
}

void ResDx::ResDxMeshVertex::Copy(void* src, size_type num, size_type stride)
{
	assert(num <= numVertex);
	assert(stride == this->stride);

	memcpy_s(
		vertexData.vertex1,
		numVertex * stride,
		src,
		num * stride
	);
}

void ResDx::ResDxMeshVertex::Copy(void* src)
{
	Copy(src, numVertex, stride);
}

void ResDx::ResDxMeshVertex::Clear()
{
	if (!isInit)
		return;
	switch (vertexType)
	{
	case ResDxResourceMeshFlags_VertexType1:
		delete[] vertexData.vertex1;
		break;
	case ResDxResourceMeshFlags_VertexType2:
		delete[] vertexData.vertex2;
		break;
	case ResDxResourceMeshFlags_VertexType3:
		delete[] vertexData.vertex3;
		break;
	default:
		assert(false);
		break;
	}
}

ResDx::ResDxMeshVertex::ResDxVertex ResDx::ResDxMeshVertex::Resource() const
{
	return vertexData;
}

ResDx::ResDxResourceFlags ResDx::ResDxMeshVertex::Type() const
{
	return vertexType;
}

size_type ResDx::ResDxMeshVertex::NumVertex() const
{
	return numVertex;
}

size_type ResDx::ResDxMeshVertex::StrideVertex() const
{
	return stride;
}

bool ResDx::ResDxMeshVertex::IsInit() const
{
	return isInit;
}

void ResDx::ResDxMeshVertex::operator=(ResDxMeshVertex&& src)
{
	numVertex = src.numVertex;
	stride = src.stride;
	vertexType = src.vertexType;
	vertexData.vertex1 = src.vertexData.vertex1;
	isInit = true;

	src.numVertex = 0;
	src.stride = 0;
	src.vertexType = 0;
	src.vertexData.vertex1 = nullptr;
	src.isInit = false;

}

ResDx::ResDxMeshVertex::ResDxMeshVertex(ResDxMeshVertex&& src)
{
	*this = std::move(src);
}

ResDx::ResDxMeshVertex::~ResDxMeshVertex()
{
	Clear();
}

void ResDx::ResDxMeshIndex::Init(size_type numIndex, ResDxResourceFlags indexType)
{
	ResDxResourceFlags type = indexType & ResDxResourceMeshFlags_IndexTypeMask;

	switch (type)
	{
	case ResDxResourceMeshFlags_IndexType1:
		indexData.index1 = new ResDxIndex1[numIndex];
		stride = sizeof(ResDxIndex1);
		break;

	case ResDxResourceMeshFlags_IndexType2:
		indexData.index2 = new ResDxIndex2[numIndex];
		stride = sizeof(ResDxIndex2);
		break;

	default:
		assert(false);
		break;

	}

	this->numIndex = numIndex;
	indexType = type;
	isInit = true;
}

void ResDx::ResDxMeshIndex::Copy(void* src, size_type num, size_type stride)
{
	assert(num <= numIndex);
	assert(stride == this->stride);

	memcpy_s(indexData.index1, stride * numIndex, src, num * stride);
}

void ResDx::ResDxMeshIndex::Copy(void* src)
{
	Copy(src, numIndex, stride);
}

void ResDx::ResDxMeshIndex::Clear()
{
	if (isInit)
		return;


	switch (indexType)
	{
	case ResDxResourceMeshFlags_IndexType1:
		delete[] indexData.index1;
		break;

	case ResDxResourceMeshFlags_IndexType2:
		delete[] indexData.index2;
		break;

	default:
		assert(false);
		break;

	}

	numIndex = 0;
	stride = 0;
	indexData.index1 = nullptr;
	indexType = 0;
	isInit = false;


}

ResDx::ResDxMeshIndex::ResDxIndex ResDx::ResDxMeshIndex::Resource() const
{
	return indexData;
}

ResDx::ResDxResourceFlags ResDx::ResDxMeshIndex::Type() const
{
	return indexType;
}

size_type ResDx::ResDxMeshIndex::NumIndex() const
{
	return numIndex;
}

size_type ResDx::ResDxMeshIndex::StrideIndex() const
{
	return stride;
}

bool ResDx::ResDxMeshIndex::IsInit() const
{
	return isInit;
}

void ResDx::ResDxMeshIndex::operator=(ResDxMeshIndex&& src)
{
	numIndex = src.numIndex;
	stride = src.stride;
	indexData.index1 = src.indexData.index1;
	indexType = src.indexType;
	isInit = src.isInit;

	src.numIndex = 0;
	src.stride = 0;
	src.indexData.index1 = nullptr;
	src.indexType = 0;
	src.isInit = false;
}

ResDx::ResDxMeshIndex::ResDxMeshIndex(ResDxMeshIndex&& src)
{
	*this = std::move(src);
}

ResDx::ResDxMeshIndex::~ResDxMeshIndex()
{
	Clear();
}

void ResDx::ResDxMesh::Init(size_type numMesh)
{
	meshDataList.reserve(numMesh);
}

void ResDx::ResDxMesh::SetMeshData(size_type index, void* srcVertex, size_type numVertex, void* srcIndex, size_type numIndex, ResDxResourceFlags vertexAndIndexType, size_type materialIndex, string_t name)
{
	assert(index < meshDataList.size());

	meshDataList[index].name = name;
	meshDataList[index].vertex.Init(numVertex, vertexAndIndexType);
	meshDataList[index].index.Init(numIndex, vertexAndIndexType);
	meshDataList[index].vertex.Copy(srcVertex);
	meshDataList[index].index.Copy(srcIndex);
	meshDataList[index].materialIndex = materialIndex;
	meshDataList[index].flags = vertexAndIndexType;
}

void ResDx::ResDxMesh::ClearMeshData(size_type index)
{
	assert(index < meshDataList.size());
	meshDataList[index].flags = 0;
	meshDataList[index].name = "";
	meshDataList[index].vertex.Clear();
	meshDataList[index].index.Clear();
}

void ResDx::ResDxMesh::ClearAll()
{
	size_type numMesh = meshDataList.size();
	for (size_type i = 0; i < numMesh; ++i)
		ClearMeshData(i);
}

ResDx::ResDxMesh::ResDxMeshData ResDx::ResDxMesh::GetMeshData(size_type index)const
{
	assert(index < meshDataList.size());

	return { &meshDataList[index] };
}

size_type ResDx::ResDxMesh::GetNumMeshData() const
{
	return meshDataList.size();
}

void ResDx::ResDxMesh::operator=(ResDxMesh&& mesh)
{
	meshDataList = std::move(mesh.meshDataList);
}

ResDx::ResDxMesh::ResDxMesh(ResDxMesh&& mesh)
{
	*this = std::move(mesh);
}

void ResDx::ResDxBone::Init(size_type numBone)
{
	boneDataList.reserve(numBone);
}

void ResDx::ResDxBone::SetBoneData(
	size_type		index,
	size_type		numBone,
	string_t* boneName,
	ResDxMatrix* transform,
	ResDxMatrix* offsetMatrix,
	ResDxBoneNode	rootBoneNode,
	ResDxMatrix		rootNodeInverseTransform)
{
	assert(index < boneDataList.size());

	data* boneData = &boneDataList[index];
	BoneMap_t& map = boneData->boneMap;

	if (numBone == 0)
		return;

	boneData->name.reserve(numBone);
	boneData->offsetMatrix.reserve(numBone);
	boneData->finalTransform.reserve(numBone);
	boneDataList[index].numBone = numBone;

	for (size_type i = 0; i < numBone; ++i)
	{
		boneData->name[i] = boneName[i];
		boneData->offsetMatrix[i] = offsetMatrix[i];
		if (!map.find(boneName[i]))
			map[boneName[i]] = i;
	}

	boneDataList[index].rootBoneNode = rootBoneNode;
	boneDataList[index].rootBoneInverseTransform = rootNodeInverseTransform;
	numBoneData++;
}

void ResDx::ResDxBone::ClearBoneData(size_type index)
{
	data* boneData = &boneDataList[index];
	size_type numBone = boneDataList[index].numBone;

	boneData->name.clear();
	boneData->offsetMatrix.clear();
	boneData->finalTransform.clear();
}

void ResDx::ResDxBone::ClearAll()
{
	size_type numBoneData = boneDataList.size();
	for (size_type i = 0; i < numBoneData; ++i)
		ClearBoneData(i);
}

void ResDx::ResDxBone::GetBoneTransform(size_type index, ResDxMatrix* dstMat)const
{
	ResDxMatrix mat = DirectX::XMMatrixIdentity();
	GetBoneTransform(boneDataList[index], &boneDataList[index].rootBoneNode, mat, dstMat);
}

ResDx::ResDxBone::data& ResDx::ResDxBone::GetBoneData(size_type index)
{
	assert(index < boneDataList.size());
	return boneDataList[index];
}

const ResDx::ResDxBone::data& ResDx::ResDxBone::GetBoneData(size_type index) const
{
	assert(index < boneDataList.size());
	return boneDataList[index];
}

size_type ResDx::ResDxBone::GetNumBone(size_type index)const
{
	return boneDataList[index].numBone;
}

size_type ResDx::ResDxBone::GetNumBoneData() const
{
	return boneDataList.size();
}

ResDx::ResDxBoneNode& ResDx::ResDxBone::GetRootBone(size_type index)
{
	assert(index < boneDataList.size());
	return boneDataList[index].rootBoneNode;
}

void ResDx::ResDxBone::GetBoneTransform(const data& data, const ResDxBoneNode* node, ResDxMatrix& parent, ResDxMatrix* dstMat)const
{
	auto& child = node->GetChild();
	size_type boneIdx = node->GetID();
	ResDxMatrix globalTransformation = node->GetTransform() * parent;
	ResDxMatrix gTransformInverse = globalTransformation.Inverse();
	if (boneIdx != -1)
		dstMat[boneIdx] = data.offsetMatrix[boneIdx] * globalTransformation;

	for (size_type i = 0; i < child.size(); ++i)
		GetBoneTransform(data, &child[i], globalTransformation, dstMat);
}

ResDx::ResDxBone::BoneMap_t& ResDx::ResDxBone::GetBoneMap(size_type index)
{
	assert(index < boneDataList.size());
	return boneDataList[index].boneMap;
}

void ResDx::ResDxAnimation::Init(size_type numAnimationData)
{
	ClearAll();
	animationData.reserve(numAnimationData);
}

void ResDx::ResDxAnimation::SetAnimationChannelData(size_type index, size_type boneIdx, size_type numPositionKey, ResDxVectorKey* positionKey, size_type numRotationKey, ResDxQuartKey* rotationKey, size_type numScaleKey, ResDxVectorKey* scaleKey)
{
	assert(index < animationData.size());
	data* animation = &animationData[index];
	data::Channel channel;

	channel.boneIdx = boneIdx;
	channel.position.reserve(numPositionKey);
	channel.rotation.reserve(numRotationKey);
	channel.scale.reserve(numScaleKey);

	for (size_type i = 0; i < numPositionKey; ++i)
		channel.position[i] = positionKey[i];
	for (size_type i = 0; i < numRotationKey; ++i)
		channel.rotation[i] = rotationKey[i];
	for (size_type i = 0; i < numScaleKey; ++i)
		channel.scale[i] = scaleKey[i];

	animation->channel[boneIdx] = (channel);
}

void ResDx::ResDxAnimation::SetAnimationGlobalData(size_type index, size_type numChannels, ResDxMatrix globalInverseTransformation, double tickPerSec, double duration)
{
	data* animation = &animationData[index];

	animation->channel.reserve(numChannels);
	animation->globalInverseTransform = globalInverseTransformation;
	tickPerSec = tickPerSec;
	duration = duration;
}

void ResDx::ResDxAnimation::ClearAnimationData(size_type index)
{
	assert(index < animationData.size());
	data* animation = &animationData[index];

	if (!animation)
		return;
	animation->channel.clear();
	animation->tickPerSec = 0;
	animation->duration;
	animation->globalInverseTransform = ResDxMatrix();
}

void ResDx::ResDxAnimation::ClearAll()
{
	size_type num = animationData.size();
	for (size_type i = 0; i < num; ++i)
		ClearAnimationData(i);
}

void ResDx::ResDxAnimation::GetBoneTransform(size_type index, ResDxMatrix dstTransform[], ResDxBone& boneData, float animationTick)
{
	auto& animation = GetAnimationData(index);
	auto& bone = boneData.GetBoneData(index);
	float tickPerSecond = (float)animation.tickPerSec != 0 ? animation.tickPerSec : RES_DX_ANIMATIONTICK_DEFAULT;
	float timeInTicks = animationTick * tickPerSecond;
	float animationTimeTicks = fmod(timeInTicks, (float)animation.duration - 1.0f);
	ResDxMatrix mat = DirectX::XMMatrixIdentity();
	GetBoneTransformNode(index, dstTransform, bone.rootBoneNode, boneData, mat, animationTick);
}

ResDx::ResDxAnimation::data& ResDx::ResDxAnimation::GetAnimationData(size_type index)
{
	assert(index < animationData.size());
	return animationData[index];
}

void ResDx::ResDxAnimation::GetBoneTransformNode(size_type index, ResDxMatrix dstTransform[], const ResDxBoneNode& node, const ResDxBone& boneData, ResDxMatrix& parent, float animationTick)
{
	auto& animation = GetAnimationData(index);
	auto& bone = boneData.GetBoneData(index);
	auto& child = node.GetChild();
	data::Channel* channel = nullptr;
	size_type boneIdx = node.GetID();
	ResDxMatrix nodeTransformation = node.GetTransform();
	ResDxMatrix globalTransform = DirectX::XMMatrixIdentity();


	if (boneIdx != -1)
		channel = &animation.channel[boneIdx];

	if (channel)
	{
		ResDxMatrix scale = CalcInterpolatedScaling(animationTick, channel);
		ResDxMatrix rotation = CalcInterpolatedRotation(animationTick, channel);
		ResDxMatrix position = CalcInterpolatedPosition(animationTick, channel);

		nodeTransformation = scale * rotation * position;
	}

	globalTransform = nodeTransformation * parent;

	if (boneIdx != -1)
		dstTransform[boneIdx] = bone.offsetMatrix[boneIdx] * globalTransform * animation.globalInverseTransform;

	for (size_type i = 0; i < child.size(); ++i)
		GetBoneTransformNode(index, dstTransform, child[i], boneData, globalTransform, animationTick);
}

ResDx::ResDxMatrix ResDx::ResDxAnimation::CalcInterpolatedScaling(float animationTick, data::Channel* channel)
{
	ResDxMatrix scalingMat = {};
	ResDxVector3 scaling = {};
	size_type scalingIndex = 0;
	size_type nextScalingIndex = 0;
	size_type numChannel = channel->scale.size();
	if (numChannel == 0)
		return DirectX::XMMatrixIdentity();

	if (numChannel == 1)
	{
		scaling = channel->scale[0].value;
		return DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z);
	}

	for (size_type i = 0; i < numChannel; ++i)
	{
		if (channel->scale[i].time >= animationTick)
		{
			scalingIndex = i;

			nextScalingIndex = scalingIndex + 1;
			break;
		}
	}
	assert(nextScalingIndex < numChannel);
	float t1 = (float)channel->scale[scalingIndex].time;
	float t2 = (float)channel->scale[nextScalingIndex].time;
	float deltaTime = t2 - t1;
	float factor = ((float)t1 - animationTick) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);
	ResDxVector start = channel->scale[scalingIndex].value;
	ResDxVector end = channel->scale[nextScalingIndex].value;
	ResDxVector delta = DirectX::XMVectorSubtract(end, start);
	ResDxVector ret = start + end * factor;
	return DirectX::XMMatrixScaling(ret.x(), ret.y(), ret.z());
}

ResDx::ResDxMatrix ResDx::ResDxAnimation::CalcInterpolatedRotation(float animationTick, data::Channel* channel)
{
	size_type rotationIdx = 0;
	size_type nextRotationIdx = 0;
	size_type numChannel = channel->rotation.size();

	if (numChannel == 0)
		return DirectX::XMMatrixIdentity();

	if (numChannel == 1)
	{
		return DirectX::XMMatrixRotationQuaternion(ResDxVector(channel->rotation[0].value));
	}

	for (size_type i = 0; i < numChannel; ++i)
	{
		if (channel->rotation[i].time >= animationTick)
		{
			rotationIdx = i;
			nextRotationIdx = rotationIdx + 1;
			break;
		}
	}
	assert(nextRotationIdx < numChannel);
	float t1 = (float)channel->rotation[rotationIdx].time;
	float t2 = (float)channel->rotation[nextRotationIdx].time;
	float deltaTime = t2 - t1;
	float factor = (t1 - (float)animationTick) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);
	ResDxVector start = channel->rotation[rotationIdx].value;
	ResDxVector end = channel->rotation[nextRotationIdx].value;
	ResDxVector ret = DirectX::XMQuaternionSlerp(start, end, factor);

	return DirectX::XMMatrixRotationQuaternion(
		DirectX::XMQuaternionNormalize(ret)
	);
}

ResDx::ResDxMatrix ResDx::ResDxAnimation::CalcInterpolatedPosition(float animationTick, data::Channel* channel)
{
	ResDxVector3 position;
	size_type positionIdx = 0;
	size_type nextPositionIdx = 0;
	size_type numChannel = channel->position.size();

	if (numChannel == 0)
		return DirectX::XMMatrixIdentity();

	if (numChannel == 1)
	{
		position = channel->position[0].value;
		return DirectX::XMMatrixTranslation(position.x, position.y, position.z);
	}

	for (size_type i = 0; i < numChannel; ++i)
	{
		if (channel->position[i].time >= animationTick)
		{
			positionIdx = i;
			nextPositionIdx = positionIdx + 1;
			break;
		}
	}

	assert(nextPositionIdx < numChannel);
	float t1 = (float)channel->position[positionIdx].time;
	float t2 = (float)channel->position[nextPositionIdx].time;
	float deltaTime = t2 - t1;
	float factor = (t1 - (float)animationTick) / deltaTime;
	assert(factor >= 0.0f && factor <= 1.0f);
	ResDxVector start = channel->position[positionIdx].value;
	ResDxVector end = channel->position[nextPositionIdx].value;
	auto delta = end - start;
	auto ret = start + delta * factor;
	return DirectX::XMMatrixTranslation(ret.x(), ret.y(), ret.z());
}

void ResDx::ResDxTexture::Init(size_type numTexture)
{
	textures.reserve(numTexture);
}

void ResDx::ResDxTexture::SetTextureData(size_type index, string_t name, u32_t width, u32_t height, u32_t rowPitch, u8_t mip, u8_t ch, u16_t uv_offset, char* pixels, ResDxResourceTextureType type)
{
	assert(index < textures.size());

	data* texture = &textures[index];
	texture->name = name;
	texture->width = width;
	texture->height = height;
	texture->rowPitch = rowPitch;
	texture->ch = ch;
	texture->uv_offset = uv_offset;
	texture->type = type;
	texture->pixels.reserve(width * height);

	if (ch < 4)
	{
		Jpeg::BitmapData map{ (unsigned char*)pixels,width,height,ch };
		Jpeg::ConvertToChannel(&map, 4);
	}
	memcpy_s(&texture->pixels[0], ch * width * height + 1, pixels, ch * width * height);
}

void ResDx::ResDxTexture::SetTextureData(size_type index, data&& src)
{
	assert(index < textures.size());
	textures[index] = std::move(src);
}

void ResDx::ResDxTexture::ClearTextureData(size_type index)
{
	assert(index < textures.size());

	data* texture = &textures[index];
	texture->name = "";
	texture->width = 0;
	texture->height = 0;
	texture->rowPitch = 0;
	texture->mip = 0;
	texture->ch = 0;
	texture->uv_offset = 0;
	texture->pixels.clear();
	texture->type = ResDxResourceTextureType_Default;
}

void ResDx::ResDxTexture::ClearAll()
{
	size_type num = textures.size();
	for (size_type i = 0; i < num; ++i)
		ClearTextureData(i);
}

const ResDx::ResDxTexture::data* ResDx::ResDxTexture::GetTextureData(size_type index)const
{
	assert(index < textures.size());
	return &textures[index];
}

size_type ResDx::ResDxTexture::GetNumTextureData() const
{
	return textures.size();
}

void ResDx::ResDxTexture::operator=(ResDxTexture&& src)
{
	textures = std::move(src.textures);
}

ResDx::ResDxTexture::ResDxTexture(ResDxTexture&& src)
{
	*this = std::move(src);
}

void ResDx::ResDxEffect::Init(size_type numEffect)
{
	effects.reserve(numEffect);
}

void ResDx::ResDxEffect::SetEffectData(size_type index, string_t name, u32_t width, u32_t height, u32_t rowPitch, u8_t mip, u8_t ch, u16_t uv_offset, char* pixels, ResDxViewpot left, ResDxViewpot top, ResDxViewpot right, ResDxViewpot bot, size_type numWidth, size_type numHeight, ResDxResourceTextureType type)
{
	assert(index < effects.size());
	data* effect = &effects[index];

	effect->name = name;
	effect->width = width;
	effect->height = height;
	effect->rowPitch = rowPitch;
	effect->mip = mip;
	effect->ch = ch;
	effect->uv_offset = uv_offset;
	effect->left = left;
	effect->right = right;
	effect->top = top;
	effect->bot = bot;
	effect->numWidth = numWidth;
	effect->numHeight = numHeight;
	effect->type = type;
	effect->pixels.reserve(width * rowPitch);
	memcpy_s(&effect->pixels[0], width * rowPitch + 1, pixels, width * rowPitch);
}

void ResDx::ResDxEffect::ClearEffectData(size_type index)
{
	effects.clear();
}

ResDx::ResDxEffect::data& ResDx::ResDxEffect::GetEffectData(size_type index)
{
	assert(index < effects.size());
	return effects[index];
}

void ResDxFileStream::ResDxFileImporter::ImportTexture(int idx,const char*fileName, ResDx::ResDxTexture& dst)
{
	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage image = {};
	FileSystem::path_t fileNameWstr = fileName;

	auto result = DirectX::LoadFromWICFile(fileNameWstr.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);
	if (FAILED(result))
	{
		MessageBoxA(nullptr, "‰æ‘œƒtƒ@ƒCƒ‹‚Ì“Ç‚Ýž‚Ý‚ÉŽ¸”s‚µ‚Ü‚µ‚½", "error", MB_OK);
		return;
	}

	auto img = image.GetImage(0, 0, 0);
	auto resTexture = ResDx::ResDxTextureData();
	resTexture.name = fileName;
	resTexture.width = img->width;
	resTexture.height = img->height;
	resTexture.rowPitch = img->rowPitch;
	resTexture.mip = metadata.mipLevels;
	resTexture.pixels.reserve(img->rowPitch * img->height);
	memcpy(resTexture.pixels.data(), img->pixels, img->rowPitch * img->height);
	dst.SetTextureData(idx,std::move(resTexture));
}

void ResDxFileStream::ResDxFileImporter::Import(const char* file_name)
{
	int flag = 0;
	flag |= aiProcess_Triangulate;
	flag |= aiProcess_CalcTangentSpace;
	flag |= aiProcess_GenSmoothNormals;
	flag |= aiProcess_GenUVCoords;
	flag |= aiProcess_OptimizeMeshes;
	flag |= aiProcess_OptimizeGraph;
	flag |= aiProcess_JoinIdenticalVertices;
	flag |= aiProcess_FlipUVs;
	//flag |= aiProcess_LimitBoneWeights;

	scene = importer.ReadFile(file_name, flag);
	if (!scene)
	{
		ResDx::string_t errorStr = importer.GetErrorString();
		MessageBoxA(NULL,errorStr.C_Str(),"assimp_error",MB_OK);
	}
}

void ResDxFileStream::ResDxFileImporter::GetTextureData(ResDx::ResDxTexture& dst)
{
	ResDx::ResDxTexture texture;
	if (!scene->HasTextures())
	{
		ResDx::ResDxMessage("file has no textures");
		return;
	}

	texture.Init(scene->mNumTextures);
	for (size_type i = 0; i < scene->mNumTextures; ++i)
	{
		auto p = scene->mTextures[i];
		TransrateTextureData(i, texture, p);
	}

	dst = std::move(texture);
}

void ResDxFileStream::ResDxFileImporter::GetMeshData(ResDx::ResDxMesh& dst, ResDx::ResDxResourceFlags flags)
{

	if (!scene->HasMeshes())
		ResDx::ResDxMessage("file has no meshes");

	ResDx::ResDxMesh mesh;
	mesh.Init(scene->mNumMeshes);

	for (size_type i = 0; i < scene->mNumMeshes; ++i)
	{
		auto p = scene->mMeshes[i];
		auto vertexType = flags & ResDx::ResDxResourceMeshFlags_VertexTypeMask;
		auto indexType = flags & ResDx::ResDxResourceMeshFlags_IndexTypeMask;
		aiVector3D zero3D(0.0f, 0.0f, 0.0f);
		ResDxMeshVertex vertices;
		ResDxMeshIndex indices;
		QueryMeshVertex(
			p,
			vertices,
			&ResDxFileImporter::CreateMeshVertex1,
			&ResDxFileImporter::CreateMeshVertex2,
			&ResDxFileImporter::CreateMeshVertex3,
			vertexType
		);
		QueryMeshIndex(
			p,
			indices,
			&ResDxFileImporter::CreateMeshIndex1,
			&ResDxFileImporter::CreateMeshIndex2,
			indexType
		);

		mesh.SetMeshData(
			i,
			(void*)vertices.vertex1,
			p->mNumVertices,
			(void*)indices.index1,
			p->mNumFaces * 3,
			flags,
			p->mMaterialIndex,
			FileSystem::path_t(p->mName.C_Str()).stem().string().c_str()
		);

		delete[](void*)vertices.vertex1;
		delete[](void*)indices.index1;
	}

	dst = std::move(mesh);
}

void ResDxFileStream::ResDxFileImporter::GetBoneWeight(ResDx::ResDxBoneWeight& data, size_type meshIndex)
{
	auto mesh = scene->mMeshes[meshIndex];
	if (!mesh->HasBones())
		return;

	ResDx::map_t<ResDx::string_t, size_type> boneMap;
	size_type count = 0;
	size_type numWeights = 0;
	for (size_type i = 0; i < mesh->mNumBones; ++i)
	{
		aiBone* bone = mesh->mBones[i];
		size_type boneID = 0;
		if (!boneMap.find(bone->mName.C_Str()))
		{
			boneMap[bone->mName.C_Str()] = count;
			count++;
		}
		boneID = boneMap[bone->mName.C_Str()];

		for (size_type j = 0; j < bone->mNumWeights; ++j)
		{
			for (size_type k = 0; k < MAX_NUM_BONE_VERTEX; ++k)
			{
				if (data.weight[k] == 0.0f)
				{
					data.boneID[k] = boneID;
					data.weight[k] = bone->mWeights[j].mWeight;
					break;
				}
				if (k == MAX_NUM_BONE_VERTEX - 1)
					assert(0);
			}
		}
	}
}

void ResDxFileStream::ResDxFileImporter::GetMaterialData(ResDx::ResDxMaterial& dst)
{
	ResDx::ResDxMaterial material;
	material.Init(scene->mNumMaterials);
	
	for (size_type i = 0; i < scene->mNumMaterials; ++i)
	{
		const aiMaterial* paiMaterial = scene->mMaterials[i];
		paiMaterial->mProperties;
		aiString path;
		float shininess = 0.0f;
		aiColor3D diffuseColor(0.0f, 0.0f, 0.0f);
		aiColor3D specularColor(0.0f, 0.0f, 0.0f);
		size_type numAmbient = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_AMBIENT);
		size_type numDiffuseMap = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_DIFFUSE);
		size_type numShiniessMap = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_SHININESS);
		size_type numSpeculerMap = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_SPECULAR);
		size_type numNormalMap = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_NORMALS);
		size_type numHeightMap = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_HEIGHT);
		size_type numOpacityMap = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_OPACITY);
		size_type numDisplacementMap = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_DISPLACEMENT);
		size_type numUnknown = paiMaterial->GetTextureCount(aiTextureType::aiTextureType_UNKNOWN);
		size_type numMaps = numDiffuseMap + numShiniessMap + numSpeculerMap + numNormalMap + numHeightMap;
		size_type mapOffset = 0;
		ResDx::array_t<size_type> mapList;
		ResDx::array_t<ResDx::ResDxResourceTextureType> mapTypeList;
		ResDx::array_t<ResDx::string_t> hints;
		auto QueryGetMaterialMap = [](
			const aiMaterial* material,
			ResDx::array_t<size_type>& mapList,
			ResDx::array_t<ResDx::ResDxResourceTextureType>& mapTypeList,
			ResDx::array_t<ResDx::string_t>& hints,
			aiTextureType aiMapType,
			ResDx::ResDxResourceTextureType mapType,
			size_type numMaps,
			size_type& mapOffset
			)
		{
			aiString path;

			for (size_type j = mapOffset; j < numMaps + mapOffset; ++j)
			{

				material->Get(AI_MATKEY_TEXTURE(aiMapType, j), path);

				mapTypeList[j] = mapType;
				hints[j] = path.C_Str();
			}
			mapOffset += numMaps;
		};

		mapList.reserve(numMaps);
		mapTypeList.reserve(numMaps);
		hints.reserve(numMaps);

		//ŠgŽU”½ŽË
		if (paiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) != AI_SUCCESS)
			diffuseColor = aiColor3D(0.5f, 0.5f, 0.5f);
		//‹¾–Ê”½ŽË
		if (paiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) != AI_SUCCESS)
			specularColor = aiColor3D(0.0f, 0.0f, 0.0f);
		//‹¾–Ê”½ŽË‹­“x
		if (paiMaterial->Get(AI_MATKEY_SHININESS, shininess) != AI_SUCCESS)
			shininess = 0.0f;

		QueryGetMaterialMap(
			paiMaterial,
			mapList,
			mapTypeList,
			hints,
			aiTextureType_DIFFUSE,
			ResDx::ResDxResourceTextureType_DiffuseMap,
			numDiffuseMap,
			mapOffset
		);

		QueryGetMaterialMap(
			paiMaterial,
			mapList,
			mapTypeList,
			hints,
			aiTextureType_SPECULAR,
			ResDx::ResDxResourceTextureType_SpecularMap,
			numSpeculerMap,
			mapOffset
		);

		QueryGetMaterialMap(
			paiMaterial,
			mapList,
			mapTypeList,
			hints,
			aiTextureType_SHININESS,
			ResDx::ResDxResourceTextureType_ShininessMap,
			numShiniessMap,
			mapOffset
		);

		QueryGetMaterialMap(
			paiMaterial,
			mapList,
			mapTypeList,
			hints,
			aiTextureType_NORMALS,
			ResDx::ResDxResourceTextureType_NormalMap,
			numNormalMap,
			mapOffset
		);

		QueryGetMaterialMap(
			paiMaterial,
			mapList,
			mapTypeList,
			hints,
			aiTextureType_HEIGHT,
			ResDx::ResDxResourceTextureType_HeightMap,
			numHeightMap,
			mapOffset
		);

		if (mapList.size())
			material.SetMaterial(
				i,
				shininess,
				{ diffuseColor.r,diffuseColor.g,diffuseColor.b },
				{ specularColor.r,specularColor.g,specularColor.b },
				mapList.size(),
				mapList.data(),
				mapTypeList.data(),
				hints.data()
			);
		else
			material.SetMaterial(
				i,
				shininess,
				{ diffuseColor.r,diffuseColor.g,diffuseColor.b },
				{ specularColor.r,specularColor.g,specularColor.b },
				mapList.size(),
				nullptr,
				nullptr,
				nullptr
			);
	}
	dst = std::move(material);
}


void ResDxFileStream::ResDxFileImporter::GetBoneData(ResDx::ResDxBone& dst)
{
	ResDx::ResDxBone bone;
	bone.Init(scene->mNumMeshes);
	for (size_type i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[i];
		ResDx::map_t<ResDx::string_t, size_type> boneMap;
		ResDx::array_t<ResDx::string_t> name;
		ResDx::array_t<ResDx::ResDxMatrix> offset;
		ResDx::array_t<ResDx::ResDxMatrix> transform;
		size_type count = 0;

		if (!mesh->HasBones())
			continue;
		name.reserve(mesh->mNumBones);
		offset.reserve(mesh->mNumBones);
		transform.reserve(mesh->mNumBones);

		for (size_type j = 0; j < mesh->mNumBones; ++j)
		{
			aiBone* paiBone = mesh->mBones[j];
			if (!boneMap.find(paiBone->mName.C_Str()))
			{
				boneMap[paiBone->mName.C_Str()] = count;
				name[count] = paiBone->mName.C_Str();
				offset[count] = TransrateToResDx(paiBone->mOffsetMatrix);
				count++;
			}


		}

		aiNode* paiNode = scene->mRootNode;
		ResDx::ResDxBoneNode node;
		ResDx::ResDxMatrix globalInverseTransformation = DirectX::XMMatrixIdentity();
		ResDx::ResDxMatrix parentMat = DirectX::XMMatrixIdentity();
		ResDx::string_t rootBoneName = "";
		//AssimpRootBoneInverseMatix(scene->mRootNode, boneMap, parentMat, globalInverseTransformation);
		//AssimpRootBoneName(scene->mRootNode, boneMap, rootBoneName);

		//paiNode=scene->mRootNode->FindNode(rootBoneName.C_Str());
		CreateBoneNode(&node, paiNode, boneMap);

		bone.SetBoneData(i, mesh->mNumBones, &name[0], &transform[0], &offset[0], node, DirectX::XMMatrixIdentity());
	}

	dst = std::move(bone);
}

void ResDxFileStream::ResDxFileImporter::GetAnimationData(ResDx::ResDxAnimation& dst, ResDx::ResDxBone& bone)
{
	ResDx::ResDxAnimation animation;
	ResDx::ResDxMatrix globalInverseTransformation = TransrateToResDx(scene->mRootNode->mTransformation.Inverse());
	animation.Init(scene->mNumAnimations);

	for (size_type i = 0; i < scene->mNumAnimations; ++i)
	{
		auto& map = bone.GetBoneMap(i);
		aiAnimation* aiAnim = scene->mAnimations[i];
		ResDx::array_t<ResDx::ResDxVectorKey> position;
		ResDx::array_t<ResDx::ResDxQuartKey> rotation;
		ResDx::array_t<ResDx::ResDxVectorKey> scale;

		animation.SetAnimationGlobalData(
			i,
			aiAnim->mNumChannels,
			globalInverseTransformation,
			aiAnim->mTicksPerSecond,
			aiAnim->mDuration
		);

		for (size_type j = 0; j < aiAnim->mNumChannels; ++j)
		{
			aiNodeAnim* aiChannel = aiAnim->mChannels[j];
			aiNode* boneNode = scene->mRootNode->FindNode(aiChannel->mNodeName);
			size_type numPosKey = aiChannel->mNumPositionKeys;
			size_type numRotKey = aiChannel->mNumRotationKeys;
			size_type numSclKey = aiChannel->mNumScalingKeys;

			if (!boneNode)
				continue;

			position.reserve(numPosKey);
			rotation.reserve(numRotKey);
			scale.reserve(numSclKey);

			for (size_type k = 0; k < numPosKey; ++k)
			{
				aiVectorKey key = aiChannel->mPositionKeys[k];
				position[k].time = key.mTime;
				position[k].value = { key.mValue.x,key.mValue.y ,key.mValue.z };
			}

			for (size_type k = 0; k < numRotKey; ++k)
			{
				aiQuatKey key = aiChannel->mRotationKeys[k];
				rotation[k].time = key.mTime;
				rotation[k].value = { key.mValue.x,key.mValue.y ,key.mValue.z,key.mValue.w };
			}

			for (size_type k = 0; k < numSclKey; ++k)
			{
				aiVectorKey key = aiChannel->mScalingKeys[k];
				scale[k].time = key.mTime;
				scale[k].value = { key.mValue.x,key.mValue.y ,key.mValue.z };
			}

			animation.SetAnimationChannelData(
				i,
				map[boneNode->mName.C_Str()],
				aiChannel->mNumPositionKeys,
				&position[0],
				aiChannel->mNumRotationKeys,
				&rotation[0],
				aiChannel->mNumScalingKeys,
				&scale[0]
			);
		}
	}
	dst = std::move(animation);
}

void ResDxFileStream::ResDxFileImporter::QueryMeshVertex(
	aiMesh* mesh,
	ResDxMeshVertex& dst,
	ResDxMeshVertexQueryFuncType	vertex1,
	ResDxMeshVertexQueryFuncType	vertex2,
	ResDxMeshVertexQueryFuncType	vertex3,
	ResDx::ResDxResourceFlags	flags
)
{
	switch (flags & ResDx::ResDxResourceMeshFlags_VertexTypeMask)
	{
	case ResDx::ResDxResourceMeshFlags_VertexType1:
		(this->*vertex1)(dst, mesh);
		break;
	case ResDx::ResDxResourceMeshFlags_VertexType2:
		(this->*vertex2)(dst, mesh);
		break;
	case ResDx::ResDxResourceMeshFlags_VertexType3:
		(this->*vertex3)(dst, mesh);
		break;
	default:
		assert(0);
		break;
	}
}

void ResDxFileStream::ResDxFileImporter::QueryMeshIndex(
	aiMesh* src,
	ResDxMeshIndex& dst,
	ResDxMeshIndexQueryFuncType index1,
	ResDxMeshIndexQueryFuncType index2,
	ResDx::ResDxResourceFlags flags
)
{
	switch (flags & ResDx::ResDxResourceMeshFlags_IndexTypeMask)
	{
	case ResDx::ResDxResourceMeshFlags_IndexType1:
		(this->*index1)(dst, src);
		break;
	case ResDx::ResDxResourceMeshFlags_IndexType2:
		(this->*index2)(dst, src);
		break;
	default:
		assert(0);
		break;
	}
}

void ResDxFileStream::ResDxFileImporter::CreateMeshVertex1(ResDxMeshVertex& vertex, aiMesh* mesh)
{
	vertex.vertex1 = new ResDx::ResDxVertex1[mesh->mNumVertices];
	aiVector3D zero3D(0.0f, 0.0f, 0.0f);
	for (size_type i = 0; i < mesh->mNumVertices; ++i)
	{
		auto pos = &(mesh->mVertices[i]);
		auto normal = mesh->HasNormals()? &(mesh->mNormals[i]):&zero3D;
		auto tex = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][i]) : &zero3D;
		auto tangent = mesh->HasTangentsAndBitangents() ? &(mesh->mTangents[i]) : &zero3D;
		auto binormal = mesh->HasTangentsAndBitangents() ? &(mesh->mBitangents[i]) : &zero3D;
		vertex.vertex1[i] =
		{
			ResDx::ResDxVector3{pos->x,pos->y,pos->z},
			ResDx::ResDxVector3{normal->x,normal->y,normal->z},
			ResDx::ResDxVector3{tangent->x,tangent->y,tangent->z},
			ResDx::ResDxVector3{binormal->x,binormal->y,binormal->z},
			ResDx::ResDxVector2{tex->x,tex->y},
			{0},
			{1.0f}
		};
	}

	if (!mesh->HasBones())
		return;

	ResDx::map_t<ResDx::string_t, size_type> boneMap;
	size_type count = 0;
	for (size_type i = 0; i < mesh->mNumBones; ++i)
	{
		aiBone* bone = mesh->mBones[i];
		//aiBone* bone = &obone;
		size_type boneID = 0;
		if (!boneMap.find(bone->mName.C_Str()))
		{
			boneMap[bone->mName.C_Str()] = count;
			count++;
		}
		boneID = boneMap[bone->mName.C_Str()];

		for (size_type j = 0; j < bone->mNumWeights; ++j)
		{
			ResDx::ResDxVertex1* p = &vertex.vertex1[bone->mWeights[j].mVertexId];

			for (size_type k = 0; k < MAX_NUM_BONE_VERTEX; ++k)
			{
				if (p->weight[k] == 0.0f)
				{
					p->boneID[k] = boneID;
					p->weight[k] = bone->mWeights[j].mWeight;
					break;
				}
				if (k == MAX_NUM_BONE_VERTEX - 1)
					assert(0);
			}
		}
	}
}

void ResDxFileStream::ResDxFileImporter::CreateMeshVertex2(ResDxMeshVertex& vertex, aiMesh* mesh)
{
	vertex.vertex2 = new ResDx::ResDxVertex2[mesh->mNumVertices];

	aiVector3D zero3D(0.0f, 0.0f, 0.0f);
	for (size_type i = 0; i < mesh->mNumVertices; ++i)
	{
		auto pos = &(mesh->mVertices[i]);
		auto normal = &(mesh->mNormals[i]);
		auto tex = mesh->HasTextureCoords(0) ? &(mesh->mTextureCoords[0][i]) : &zero3D;
		vertex.vertex2[i] =
		{
			ResDx::ResDxVector3{pos->x,pos->y,pos->z},
			ResDx::ResDxVector3{normal->x,normal->y,normal->z},
			ResDx::ResDxVector2{tex->x,tex->y}
		};
	}
}

void ResDxFileStream::ResDxFileImporter::CreateMeshVertex3(ResDxMeshVertex& vertex, aiMesh* mesh)
{
	vertex.vertex3 = new ResDx::ResDxVertex3[mesh->mNumVertices];
	aiVector3D zero3D(0.0f, 0.0f, 0.0f);
	for (size_type i = 0; i < mesh->mNumVertices; ++i)
	{
		auto pos = &(mesh->mVertices[i]);
		vertex.vertex3[i] =
		{
			ResDx::ResDxVector3{pos->x,pos->y,pos->z},
		};
	}
}

void ResDxFileStream::ResDxFileImporter::CreateMeshIndex1(ResDxMeshIndex& index, aiMesh* mesh)
{
	index.index1 = new ResDx::ResDxIndex1[mesh->mNumFaces * 3];

	for (auto i = 0u; i < mesh->mNumFaces; ++i)
	{
		const auto& face = mesh->mFaces[i];
		assert(face.mNumIndices == 3u);

		index.index1[i * 3 + 0] = face.mIndices[0];
		index.index1[i * 3 + 1] = face.mIndices[1];
		index.index1[i * 3 + 2] = face.mIndices[2];

	}
}

void ResDxFileStream::ResDxFileImporter::CreateMeshIndex2(ResDxMeshIndex& index, aiMesh* mesh)
{
	index.index2 = new ResDx::ResDxIndex2[mesh->mNumFaces * 3];

	for (auto i = 0u; i < mesh->mNumFaces; ++i)
	{
		const auto& face = mesh->mFaces[i];
		const auto indices = face.mNumIndices;

		for(auto j=0u;j<indices;++j)
			index.index2[i * indices + j] = face.mIndices[j];
	}
}

void ResDxFileStream::ResDxFileImporter::CreateBoneNode(ResDx::ResDxBoneNode* node, aiNode* paiNode, ResDx::ResDxBone::BoneMap_t& map)
{
	size_type boneID = -1;
	ResDx::ResDxMatrix nodeTransform = TransrateToResDx(paiNode->mTransformation);

	if (map.find(paiNode->mName.C_Str()))
	{
		boneID = map[paiNode->mName.C_Str()];
	}
	node->SetID(boneID);
	node->SetName(paiNode->mName.C_Str());
	node->SetTransform(nodeTransform);
	node->ReserveChild(paiNode->mNumChildren);

	for (size_type i = 0; i < paiNode->mNumChildren; ++i)
		CreateBoneNode(&node->GetChild(i), paiNode->mChildren[i], map);


}

ResDx::ResDxMatrix ResDxFileStream::ResDxFileImporter::TransrateToResDx(aiMatrix4x4 matrix)
{
	DirectX::XMMATRIX mat =
	{
		matrix.a1,matrix.a2,matrix.a3,matrix.a4,
		matrix.b1,matrix.b2,matrix.b3,matrix.b4,
		matrix.c1,matrix.c2,matrix.c3,matrix.c4,
		matrix.d1,matrix.d2,matrix.d3,matrix.d4,
	};
	mat = DirectX::XMMatrixTranspose(mat);
	return mat;
}

void ResDxFileStream::ResDxFileImporter::TransrateTextureData(size_type index, ResDx::ResDxTexture& texture, aiTexture* p)
{
	ResDx::ResDxTextureData data;

	if (p->mHeight == 0)
	{
		ResDx::string_t hint = p->achFormatHint;
		Jpeg::BitmapData map;
		if (hint == "jpg" || hint == "jpeg")
			Jpeg::JpegDecompress(&map, (unsigned char*)p->pcData, 4, p->mWidth, Jpeg::SAMPLING_OPTION_INV_Y);
		data.name = hint;
		data.width = map.width;
		data.height = map.height;
		data.ch = 4;
		data.rowPitch = map.width;
		data.uv_offset = 0;
		data.mip = 0;
		data.pixels.reserve(data.width * data.height);
		data.pixels.copy(map.data, data.width * data.height);
	}
	else
	{
		data.name = p->achFormatHint;
		data.width = p->mWidth;
		data.height = p->mHeight;
		data.rowPitch = p->mWidth;
		data.mip = 0;
		data.ch = 4;
		data.uv_offset = 0;
		data.pixels.reserve(data.width * data.height);
		data.pixels.copy(p->pcData, data.width * data.height);
	}

	texture.SetTextureData(
		index,
		p->achFormatHint,
		data.width,
		data.height,
		data.width,
		data.mip,
		data.ch,
		data.uv_offset,
		(char*)&data.pixels[0],
		ResDx::ResDxResourceTextureType_Default
	);

}

void ResDx::ResDxMaterial::Init(size_type numMaterial)
{
	materialDataList.reserve(numMaterial);
	hints.reserve(numMaterial);
}

void ResDx::ResDxMaterial::SetMaterial(size_type index, float shininess, ResDxVector3 diffuse, ResDxVector3 specular, size_type numMaps, size_type mapIndex[], ResDxResourceTextureType textureTypes[],string_t hints[])
{
	assert(index < materialDataList.size());

	data* material = &materialDataList[index];
	material->shininess = shininess;
	material->diffuse = diffuse;
	material->specular = specular;
	this->hints[index].reserve(numMaps);
	for (size_type i = 0; i < numMaps; ++i)
	{
		auto type = textureTypes[i];
		switch (type)
		{
		case ResDx::ResDxResourceTextureType_Default:
			break;
		case ResDx::ResDxResourceTextureType_DiffuseMap:
			material->useDiffuseMap = true;
			break;
		case ResDx::ResDxResourceTextureType_SpecularMap:
			material->useSpecularMap = true;
			break;
		case ResDx::ResDxResourceTextureType_ShininessMap:
			material->useShininessMap = true;
			break;
		case ResDx::ResDxResourceTextureType_NormalMap:
			material->useNormalMap = true;
			break;
		default:
			break;
		}
		this->hints[index][i] = hints[i];
	}
}

void ResDx::ResDxMaterial::ClearMaterialData(size_type index)
{
	assert(index < materialDataList.size());
	materialDataList[index].diffuse = ResDxVector3();
	materialDataList[index].specular = ResDxVector3();
	materialDataList[index].shininess = 0.0f;
}

void ResDx::ResDxMaterial::ClearAll()
{
	for (size_type i = 0; i < materialDataList.size(); ++i)
		ClearMaterialData(i);
}

const ResDx::ResDxMaterial::data* ResDx::ResDxMaterial::GetMaterialData(size_type index)const
{
	assert(index < materialDataList.size());
	return &materialDataList[index];
}

size_type ResDx::ResDxMaterial::GetNumMaterialData()const
{
	return materialDataList.size();
}

ResDx::array_t<ResDx::string_t>& ResDx::ResDxMaterial::GetHints(int index)
{
	return hints[index];
}

void ResDx::ResDxMaterial::operator=(ResDxMaterial&& src)
{
	this->materialDataList = std::move(src.materialDataList);
	this->hints = std::move(src.hints);
}

ResDx::ResDxMaterial::ResDxMaterial(ResDxMaterial&& src)
{
	*this = std::move(src);
}

void ResDx::ResDxBoneNode::SetName(const string_t name)
{
	this->name = name;
}

void ResDx::ResDxBoneNode::SetID(const size_type id)
{
	boneID = id;
}

void ResDx::ResDxBoneNode::SetTransform(const ResDxMatrix matrix)
{
	transform = matrix;
}

void ResDx::ResDxBoneNode::AddChild(const ResDxBoneNode& node)
{
	child[currentChild] = node;
	currentChild++;
}

void ResDx::ResDxBoneNode::ReserveChild(const size_type size)
{
	child.reserve(size);
}

ResDx::string_t ResDx::ResDxBoneNode::GetName()const
{
	return name;
}

size_type ResDx::ResDxBoneNode::GetID()const
{
	return boneID;
}

ResDx::ResDxMatrix ResDx::ResDxBoneNode::GetTransform()const
{
	return transform;
}

const ResDx::array_t<ResDx::ResDxBoneNode>& ResDx::ResDxBoneNode::GetChild()const
{
	return child;
}

ResDx::ResDxBoneNode& ResDx::ResDxBoneNode::GetChild(size_type idx)
{
	return child[idx];
}

const ResDx::ResDxBoneNode& ResDx::ResDxBoneNode::GetChild(size_type idx)const
{
	return child[idx];
}

void ResDx::ResDxBoneNode::operator=(const ResDxBoneNode& node)
{
	name = node.name;
	boneID = node.boneID;
	transform = node.transform;
	child = node.child;
	currentChild = node.currentChild;
}

void ResDx::ResDxBoneNode::operator=(ResDxBoneNode&& node)
{
	name = std::move(node.name);
	boneID = std::move(node.boneID);
	transform = std::move(transform);
	child = std::move(node.child);
	currentChild = std::move(node.currentChild);
}

size_type ResDx::ResDxID::ResDxIDHandle::hash() const
{
	return id->hash();
}

bool ResDx::ResDxID::ResDxIDHandle::valid() const
{
	if(id)
		return id->valid();
	return false;
}

ResDx::ResDxID::ResDxIDHandle::operator size_type()
{
	return id->hash();
}

bool ResDx::ResDxID::ResDxIDHandle::operator==(const ResDxIDHandle& idr) const
{
	return *id == *idr.id;
}

bool ResDx::ResDxID::ResDxIDHandle::operator!=(const ResDxIDHandle& idr) const
{
	return *id != *idr.id;
}

void ResDx::ResDxID::ResDxIDHandle::operator=(const ResDxIDHandle& idr)
{
	id = idr.id;
}

void ResDx::ResDxRenderPacket::RegistVertexBuffer(ResDxVertexBufferView view)
{
	vertex = view;
}

void ResDx::ResDxRenderPacket::RegistIndescBuffer(ResDxIndexBufferView view)
{
	index = view;
}

void ResDx::ResDxRenderPacket::RegistShaderResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[])
{
	SetResource(context, numIDs, viewIDs, ResDxViewType_ShaderResourceView);
}

void ResDx::ResDxRenderPacket::RegistConstantResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[])
{
	SetResource(context, numIDs, viewIDs, ResDxViewType_ConstantBufferView);
}

void ResDx::ResDxRenderPacket::RegistUnorderedResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[])
{
	SetResource(context, numIDs, viewIDs, ResDxViewType_UnorderedResourceView);
}

void ResDx::ResDxRenderPacket::QueryRendererInfo(ResDxContext2& context, ResDxCommandListDirect& command, size_type& numIndex)
{
	SetDescriptorTableView(context);
	command.SetVertexBuffer(&vertex);
	command.SetIndexBuffer(&index);
	for (size_type i = 0; i < ResDxViewType_NumDescriptorHeapViewType; ++i)
		if (descHeapGPUHandle[i].ptr)
			command.SetGraphicsDescriptorTable(i, descHeapGPUHandle[i]);
	numIndex = index.SizeInBytes / GetSizeIndexFromFormat(index.Format);
}

ResDx::vector_t<ResDx::ResDxDescriptorHeapViewID>& ResDx::ResDxRenderPacket::GetBufferViewIDs(ResDxViewType type)
{
	return descHeapIDs[type];
}

ResDx::ResDxVertexBufferView ResDx::ResDxRenderPacket::GetVertexBufferView()
{
	return vertex;
}

ResDx::ResDxIndexBufferView ResDx::ResDxRenderPacket::GetIndexBufferView()
{
	return index;
}



void ResDx::ResDxRenderPacket::operator=(ResDxRenderPacket&& packet)
{
	id = std::move(packet.id);
	vertex = std::move(packet.vertex);
	index = std::move(packet.index);
	for (size_type i = 0; i < ResDxViewType_NumDescriptorHeapViewType; ++i)
	{
		descHeapIDs[i] = std::move(packet.descHeapIDs[i]);
		descHeapGPUHandle[i] = std::move(packet.descHeapGPUHandle[i]);
	}
}

ResDx::ResDxRenderPacket::ResDxRenderPacket() :
	id(ResDxID::create()),
	vertex(),
	index(),
	descHeapIDs(),
	descHeapGPUHandle()
{}

ResDx::ResDxRenderPacket::ResDxRenderPacket(ResDxRenderPacket&& o) :
	id(std::move(o.id)),
	vertex(std::move(o.vertex)),
	index(std::move(o.index)),
	descHeapIDs(),
	descHeapGPUHandle()
{
	for (size_type i = 0; i < ResDxViewType_NumDescriptorHeapViewType; ++i)
	{
		descHeapIDs[i] = std::move(o.descHeapIDs[i]);
		descHeapGPUHandle[i] = std::move(o.descHeapGPUHandle[i]);
	}
}

void ResDx::ResDxRenderPacket::SetResource(ResDxContext2& context, size_type numIDs, ResDxDescriptorHeapViewID viewIDs[], ResDxViewType type)
{
	for (size_type i = 0; i < numIDs; ++i)
		descHeapIDs[type].push_back(viewIDs[i]);
}

void ResDx::ResDxRenderPacket::SetDescriptorTableView(ResDxContext2& context)
{
	auto& table = ResDxDescriptorHeapTable::instance();
	for (size_type i = 0; i < ResDxViewType_NumDescriptorHeapViewType; ++i)
	{
		if (!descHeapIDs[i].get_size())
			continue;
		auto id = table.CreateDescriptorsTable(context, (ResDxViewType)i, &descHeapIDs[i][0], descHeapIDs[i].get_size());
		descHeapGPUHandle[i] = table.GetDescriptorGPUHandle(id);
	}
}

void ResDx::ResDxMeshInstances::SetTransforms(size_type numTransforms, const ResDxTransform transforms[])
{
	for (size_type i = 0; i < numTransforms; ++i)
		transform[i].world = transforms[i].GetTransform();
}

void ResDx::ResDxMeshInstances::SetTransform(size_type index, const ResDxTransform& transform)
{
	this->transform[index].world = transform.GetTransform();
}

void ResDx::ResDxMeshInstances::SetWorldMatrix(size_type index, ResDxMatrix world)
{
	transform[index].world = world;
	bufferID->CopyCpuResource(&transform[index], sizeof(ResDxMeshInstanceTransform) * index, sizeof(ResDxMeshInstanceTransform));
}

ResDx::ResDxMeshInstanceTransform* ResDx::ResDxMeshInstances::GetTransforms()
{
	return &transform[0];
}

size_type ResDx::ResDxMeshInstances::NumTransforms()
{
	return transform.size();
}

ResDx::ResDxDescriptorHeapViewID ResDx::ResDxMeshInstances::GetConstantViewID()
{
	return worldMatrixConstantBufferView;
}

void ResDx::ResDxMeshInstances::operator=(ResDxMeshInstances&& instances)
{
	transform = std::move(instances.transform);
}

ResDx::ResDxMeshInstances::ResDxMeshInstances(ResDxContext2& context, size_type numInstance) :transform(numInstance)
{
	auto& factory = ResDxBufferFactory::instance();
	auto& descHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV);
	bufferID = factory.CreateUnorderedAccessBuffer(context, numInstance, sizeof(ResDxMatrix));
	descHeap.RegistResources(context, &worldMatrixConstantBufferView, ResDxViewType_UnorderedResourceView, 1, &bufferID);
}

ResDx::ResDxMeshInstances::ResDxMeshInstances(ResDxMeshInstances&& instance) :transform(std::move(instance.transform))
{
}

void ResDx::ResDxCameraConfig::SwitchFlags(ResDxCameraFlags flag)
{
	flags = flags ^ flag;
}

ResDx::ResDxCameraFlags ResDx::ResDxCameraConfig::GetConfig()
{
	return flags;
}

void ResDx::ResDxMeshInstanceConfig::SwitchFlags(ResDxMeshInstanceFlags flag)
{
	flags = flags ^ flag;
}

ResDx::ResDxMeshInstanceFlags ResDx::ResDxMeshInstanceConfig::GetConfig()
{
	return flags;
}

ResDx::ResDxSceneObjectRegistory::~ResDxSceneObjectRegistory()
{
	size_type size = list.get_size();
	for (size_type i = 0; i < size; ++i)
		list[i]->Delete(registoryID);
}

void ResDx::ResDxRendererScene::RenderingScene(ResDxContext2& context)
{

}

ResDx::ResDxRendererScene::ResDxRendererScene(ResDxSceneIdent ident) :ident(ident), registory()
{
}

void ResDx::ResDxRenderResourceCorrection::Darty()
{
	darty = true;
}

ResDx::ResDxVertexBufferView** ResDx::ResDxRenderResourceCorrection::GetVertexViews()
{
	if (darty)
		Update();
	return &vertices[0];
}

ResDx::ResDxIndexBufferView** ResDx::ResDxRenderResourceCorrection::GetindexViews()
{
	if (darty)
		Update();
	return &indices[0];
}

ID3D12DescriptorHeap** ResDx::ResDxRenderResourceCorrection::GetDescHeaps()
{
	if (darty)
		Update();
	return &descHeap[0];
}

void ResDx::ResDxRenderResourceCorrection::Update()
{
	ResDxSceneObjectRegistory* r = registory;
	const auto& instances = r->Objects<ResDxMeshInstances>();
	size_type numInstances = r->NumObjects<ResDxMeshInstances>();


	for (size_type i = 0; i < numInstances; ++i)
	{
		const ResDxDescriptorHeap* pDescHeap = nullptr;
		pDescHeap->GetDescriptorHeap();
	}
}

ResDx::ResDxRendererPipeline::ResDxRendererPipeline() :vs(), ps(), cs(), numRenderTarget(), blend(), rasterizer(), depthStencil(), pipeline()
{
}

ResDx::ResDxRendererPipeline::ResDxRendererPipeline(ResDxRendererPipeline&& o) :
	vs(std::move(vs)),
	ps(std::move(ps)),
	cs(std::move(cs))
{
}

D3D12_INPUT_ELEMENT_DESC* ResDx::GetInputElemFromVertexFlags(ResDxResourceFlags flags)
{
	switch (flags)
	{
	case ResDxResourceMeshFlags_VertexType1:
		return ResDxVertex1Elem;

	case ResDxResourceMeshFlags_VertexType2:
		return ResDxVertex2Elem;

	case ResDxResourceMeshFlags_VertexType3:
		return ResDxVertex3Elem;
	default:
		assert(false);
		return nullptr;
		break;
	}
}

ResDxCore::job_t ResDxCore::ResDxCommandJob::ResDxJobExecuteCommandList(job_arg_t args)
{
	ResDxCommandListArgs* arg = (ResDxCommandListArgs*)args;
	auto commandQueue = arg->queue;
	auto counter = arg->counter;
	arg->state = ResDxJobState_Run;
	commandQueue->Swap(ResDxCommandListType_Copy, counter[ResDxCommandListType_Copy]);
	commandQueue->Swap(ResDxCommandListType_Direct, counter[ResDxCommandListType_Direct]);
	commandQueue->Swap(ResDxCommandListType_Compute, counter[ResDxCommandListType_Compute]);

	commandQueue->Execute(ResDxCommandListType_Copy);
	commandQueue->Execute(ResDxCommandListType_Direct);
	commandQueue->Execute(ResDxCommandListType_Compute);
	arg->state = ResDxJobState_End;
	co_yield ResDxJobState_End;
}

ResDxCore::job_t ResDxCore::ResDxCommandJob::ResDxJobExecuteCommandListWait(job_arg_t args)
{
	ResDxCommandListArgs* arg = (ResDxCommandListArgs*)args;
	auto commandQueue = arg->queue;
	auto counter = arg->counter;
	arg->state = ResDxJobState_Run;
	commandQueue->Swap(ResDxCommandListType_Copy, counter[ResDxCommandListType_Copy]);
	commandQueue->Swap(ResDxCommandListType_Direct, counter[ResDxCommandListType_Direct]);
	commandQueue->Swap(ResDxCommandListType_Compute, counter[ResDxCommandListType_Compute]);

	commandQueue->Execute(ResDxCommandListType_Copy);
	commandQueue->Execute(ResDxCommandListType_Direct);
	commandQueue->Execute(ResDxCommandListType_Compute);
	while (!arg->queue->Completed(arg->index))
	{
		arg->state = ResDxJobState_Yield;
		co_yield ResDxJobState_Yield;
	}
	co_yield ResDxJobState_End;
}

ResDxCore::job_t ResDxCore::ResDxCommandJob::ResDxJobResetCommandList(job_arg_t args)
{
	ResDxCommandListArgs* arg = (ResDxCommandListArgs*)args;
	while (!arg->queue->Completed(arg->index))
		co_yield ResDxJobState_Yield;
	arg->commandList->ResetAllocator();
}

D3D12_COMMAND_LIST_TYPE ResDxCore::ResDxCommandJob::GetCommandListTypeFromResDxCommandListType(ResDxCore::ResDxCommandListType type)
{
	switch (type)
	{
	case ResDxCore::ResDxCommandListType_Copy:
		return D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	case ResDxCore::ResDxCommandListType_Direct:
		return D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case ResDxCore::ResDxCommandListType_Compute:
		return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	case ResDxCore::ResDxCommandListType_Bundle:
		return D3D12_COMMAND_LIST_TYPE_BUNDLE;
		break;
	case ResDxCore::ResDxCommandListType_NumType:
		assert(false);
		break;
	default:
		break;
	}
	assert(false);
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE ResDx::GetPrimitiveTopologyFromRendererFlags(ResDxResourceFlags flags)
{
	switch (flags)
	{
	case ResDxResourceRendererFlags_Triangle:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	default:
		assert(false);
		break;
	}

	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

D3D12_DESCRIPTOR_HEAP_TYPE ResDx::GetDescriptorHeapTypeFromResDxHeapType(ResDxDescriptorHeapType type)
{
	switch (type)
	{
	case ResDx::ResDxDescriptorHeapType_CBV_SRV_UAV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	case ResDx::ResDxDescriptorHeapType_RTV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	case ResDx::ResDxDescriptorHeapType_DSV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	case ResDx::ResDxDescriptorHeapType_Sampler:
		return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		break;
	case ResDx::ResDxDescriptorHeapType_NumDescriptorHeapType:
		break;
	default:
		break;
	}
	assert(false);
	return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}

ResDx::ResDxDescriptorHeapType ResDx::GetResDxDescriptorHeapTypeFromResDxViewType(ResDxViewType type)
{
	switch (type)
	{
	case ResDx::ResDxViewType_ConstantBufferView:
	case ResDx::ResDxViewType_ShaderResourceView:
	case ResDx::ResDxViewType_UnorderedResourceView:
		return ResDxDescriptorHeapType_CBV_SRV_UAV;

	case ResDx::ResDxViewType_RenderTargetView:
		return ResDxDescriptorHeapType_RTV;

	case ResDx::ResDxViewType_DepthStencilView:
		return ResDxDescriptorHeapType_DSV;

	default:
		break;
	}
	assert(false);
	return ResDxDescriptorHeapType_CBV_SRV_UAV;
}

size_type ResDx::GetNumInputElemFromVertexFlags(ResDxResourceFlags flags)
{
	switch (flags)
	{
	case ResDxResourceMeshFlags_VertexType1:
		return ResDxVertex1ElemSize;

	case ResDxResourceMeshFlags_VertexType2:
		return ResDxVertex2ElemSize;

	case ResDxResourceMeshFlags_VertexType3:
		return ResDxVertex3ElemSize;
	default:
		assert(false);
		return 0;
		break;
	}
}

size_type ResDx::GetSizeIndexFromFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R16_SINT:
		return sizeof(u16_t);

	case DXGI_FORMAT_R32_UINT:
		return sizeof(u32_t);

	default:
		assert(false);
		break;
	}
	return 0;
}

void ResDx::ResDxInit()
{
	graphics2.Init();
	ResDxContext2 context(&graphics2);
	auto& gDescriptorHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV);
	auto& rtvDescriptorHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_RTV);
	auto& dsvDescriptorHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_DSV);
	auto& tableDescriptorHeap = ResDxDescriptorHeapTable::instance();
	auto& rootSig = ResDxRendererRootSignature::instance();
	gDescriptorHeap.Init(context);
	rtvDescriptorHeap.Init(context);
	dsvDescriptorHeap.Init(context);
	tableDescriptorHeap.Init(context);
	tableDescriptorHeap.Init(context);
	rootSig.Init(context);

	//Job::InitJobSystem();
}

void ResDx::ResDxMessage(string_t message)
{
	string_t resDxMessage = "ResDxMessage :";
	resDxMessage += message;
	printf(resDxMessage.C_Str());
}

void ResDx::ResDxDescriptorHeapTable::Init(ResDxContext2& context)
{
	D3D12_DESCRIPTOR_HEAP_TYPE types[ResDxDescriptorHeapType_NumDescriptorHeapType];
	types[ResDxDescriptorHeapType_CBV_SRV_UAV] = GetDescriptorHeapTypeFromResDxHeapType(ResDxDescriptorHeapType_CBV_SRV_UAV);
	types[ResDxDescriptorHeapType_Sampler] = GetDescriptorHeapTypeFromResDxHeapType(ResDxDescriptorHeapType_Sampler);

	D3D12_DESCRIPTOR_HEAP_FLAGS flags[ResDxDescriptorHeapType_NumDescriptorHeapType];
	flags[ResDxDescriptorHeapType_CBV_SRV_UAV] = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	flags[ResDxDescriptorHeapType_Sampler] = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	tableDescHeap[ResDxDescriptorHeapType_CBV_SRV_UAV].Init(context, RES_DX_MAX_NUM_DESCRIPTOR, types[ResDxDescriptorHeapType_CBV_SRV_UAV], flags[ResDxDescriptorHeapType_CBV_SRV_UAV]);
	tableDescHeap[ResDxDescriptorHeapType_Sampler].Init(context, RES_DX_MAX_NUM_SAMPLER, types[ResDxDescriptorHeapType_Sampler], flags[ResDxDescriptorHeapType_Sampler]);
}

D3D12_GPU_DESCRIPTOR_HANDLE ResDx::ResDxDescriptorHeapTable::GetDescriptorGPUHandle(ResDxDescriptorTableID tableID)
{
	auto handle = GetDescriptorHandle(tableID);
	return tableDescHeap->GetGpuHandle(handle.index);
}

void ResDx::ResDxDescriptorHeapTable::GetDescriptorGPUHandle(ResDxDescriptorTableID tableID, size_type numIDs, D3D12_GPU_DESCRIPTOR_HANDLE dst[])
{
	auto handle = GetDescriptorHandle(tableID);
	auto gpuHandle = tableDescHeap->GetGpuHandle(handle.index);
	assert(numIDs <= handle.numDescriptor);

	for (size_type i = 0; i < numIDs; ++i)
		dst[i].ptr = gpuHandle.ptr + i * handle.descriptorSize;
}

ResDx::ResDxDescriptorHandle ResDx::ResDxDescriptorHeapTable::GetDescriptorHandle(ResDxDescriptorTableID tableID)
{
	assert(tableMap.find(tableID));
	return tableMap[tableID];
}

ResDx::ResDxDescriptorHandle ResDx::ResDxDescriptorHeapTable::GetDescriptorHandle(size_type numIDs, ResDxDescriptorHeapViewID viewIDs[])
{
	auto tableID = GetTableID(numIDs, viewIDs);
	assert(tableMap.find(tableID));
	return tableMap[tableID];
}

void ResDx::ResDxDescriptorHeapTable::GetDescriptorHandle(ResDxDescriptorTableID tableID, size_type numDescriptor, D3D12_CPU_DESCRIPTOR_HANDLE dst[])
{
	auto handle = GetDescriptorHandle(tableID);
	assert(numDescriptor <= handle.numDescriptor);

	for (size_type i = 0; i < numDescriptor; ++i)
		dst[i].ptr = handle.cpuHandle.ptr + (handle.descriptorSize * i);
}

const ResDx::ResDxDescriptorHeap* ResDx::ResDxDescriptorHeapTable::GetDescriptorHeap(ResDxDescriptorHeapType type)const
{
	return &tableDescHeap[type];
}

ResDx::ResDxDescriptorTableID ResDx::ResDxDescriptorHeapTable::CreateDescriptorsTable(ResDxContext2& context, ResDxViewType viewType, ResDxDescriptorHeapViewID ids[], size_type numID)
{
	assert(viewType != ResDxViewType_RenderTargetView);
	assert(viewType != ResDxViewType_DepthStencilView);
	auto tableID = GetTableID(numID, ids);
	if (!tableMap.find(tableID))
		tableMap[tableID] = CopyDescriptors(context, viewType, ids, numID);
	return tableID;
}

ResDx::ResDxDescriptorHeapTable& ResDx::ResDxDescriptorHeapTable::instance()
{
	static ResDxDescriptorHeapTable table;
	return table;
}

ResDx::ResDxDescriptorTableID ResDx::ResDxDescriptorHeapTable::GetTableID(size_type numID, ResDxDescriptorHeapViewID ids[])
{
	return { numID,ids };
}

bool ResDx::ResDxDescriptorHeapTable::FindTableHandle(size_type numID, ResDxDescriptorHeapViewID ids[])
{
	return tableMap.find(GetTableID(numID, ids));
}

ResDx::ResDxDescriptorHandle ResDx::ResDxDescriptorHeapTable::CopyDescriptors(ResDxContext2& context, ResDxViewType viewType, ResDxDescriptorHeapViewID ids[], size_type numID)
{
	ResDxDescriptorHeapType heapType = GetResDxDescriptorHeapTypeFromResDxViewType(viewType);
	D3D12_DESCRIPTOR_HEAP_TYPE d3dHeapType = GetDescriptorHeapTypeFromResDxHeapType(heapType);
	ResDxGlobalDescriptorHeap& globalDescHeap = ResDxGlobalDescriptorHeap::instance(heapType);

	D3D12_CPU_DESCRIPTOR_HANDLE dstHandle[RES_DX_MAX_NUM_COPY_DESCRIPTOR] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE srcHandle[RES_DX_MAX_NUM_COPY_DESCRIPTOR] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE head = tableDescHeap[heapType].GetCpuHandle(nextIndex);
	UINT numDescriptors[RES_DX_MAX_NUM_COPY_DESCRIPTOR] = {};
	ResDxDescriptorHandle ret = { nextIndex,0,context.GetDescriptorSize(d3dHeapType),viewType,head };
	size_type numAllDescriptor = 0;
	for (size_type i = 0; i < numID; ++i)
	{
		auto& id = ids[i];
		auto handle = globalDescHeap.GetDescriptorHandle(id);
		srcHandle[i] = handle.cpuHandle;
		dstHandle[i] = head;
		numDescriptors[i] = handle.numDescriptor;
		head.ptr += numDescriptors[i] * context.GetDescriptorSize(d3dHeapType);
		nextIndex += numDescriptors[i];
		numAllDescriptor += numDescriptors[i];
	}
	context.CopyDescriptors(numID, dstHandle, srcHandle, numDescriptors, d3dHeapType);

	ret.numDescriptor = numAllDescriptor;
	return ret;
}

ResDx::ResDxDescriptorHeapTable::ResDxDescriptorHeapTable()
{
}

void ResDx::ResDxGlobalDescriptorHeap::Init(ResDxContext2& context)
{
	descHeap.Init(context, RES_DX_MAX_NUM_DESCRIPTOR, GetDescriptorHeapTypeFromResDxHeapType(type), D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
}

void ResDx::ResDxGlobalDescriptorHeap::RegistResources(ResDxContext2& context, ResDxDescriptorHeapViewID* dstIDs, ResDxViewType type, size_type numResources, ResDxBufferID resources[])
{

	for (size_type i = 0; i < numResources; ++i)
	{
		if (!resources[i])
			continue;
		ResDxDescriptorHandle handle = {};
		size_type index = PublishNextViewIndex();
		ResDxDescriptorHeapViewID id = ResDxID::create();
		handle = CreateView(context, index, type, resources[i]);
		dstIDs[i] = id;
		descHeapViewMap[id] = handle;
	}
}

void ResDx::ResDxGlobalDescriptorHeap::RemoveResource(ResDxDescriptorHeapViewID id)
{
	freeList.push(id);
	descHeapViewMap.erase(id);
}

ResDx::ResDxDescriptorHandle ResDx::ResDxGlobalDescriptorHeap::GetDescriptorHandle(ResDxDescriptorHeapViewID id)
{
	return descHeapViewMap[id];
}

void ResDx::ResDxGlobalDescriptorHeap::GetDescriptorHandles(size_type numIDs, D3D12_CPU_DESCRIPTOR_HANDLE dstHandles[], ResDxDescriptorHeapViewID ids[])
{
	for (size_type i = 0; i < numIDs; ++i)
		dstHandles[i] = descHeapViewMap[ids[i]].cpuHandle;
}

ResDx::ResDxGlobalDescriptorHeap& ResDx::ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType type)
{
	static ResDxGlobalDescriptorHeap descriptorHeap[ResDxDescriptorHeapType_NumDescriptorHeapType] = { ResDxDescriptorHeapType_CBV_SRV_UAV,ResDxDescriptorHeapType_RTV,ResDxDescriptorHeapType_DSV,ResDxDescriptorHeapType_Sampler };
	return descriptorHeap[type];
}

size_type ResDx::ResDxGlobalDescriptorHeap::PublishNextViewIndex()
{
	ResDxDescriptorIndex index = nextIndex;
	while (!nextIndex.compare_exchange_weak(index, index + 1))
	{
		index = nextIndex;
	}
	return index;
}

ResDx::ResDxDescriptorHandle ResDx::ResDxGlobalDescriptorHeap::CreateView(ResDxContext2& context, size_type index, ResDxViewType type, ResDxBuffer* resource)
{
	auto res = resource->Resource();
	switch (type)
	{
	case ResDx::ResDxViewType_ConstantBufferView:
		return descHeap.CreateConstantBufferView(context, index, res);
		break;
	case ResDx::ResDxViewType_ShaderResourceView:
		return descHeap.CreateShaderResourceView(context, index, res, resource->Data<ResDxBufferType_Texture>()->format);
		break;
	case ResDx::ResDxViewType_UnorderedResourceView:
	{
		auto data = resource->Data<ResDxBufferType_RWStructured>();
		return descHeap.CreateUnorderedBufferView(context, index, res, data->counter, data->numOfElem, data->sizeOfElem);
		break;
	}
	case ResDx::ResDxViewType_NumDescriptorHeapViewType:
		assert(false);
		break;
	case ResDx::ResDxViewType_RenderTargetView:
		return descHeap.CreateRenderTargetView(context, index, res);
		break;
	case ResDx::ResDxViewType_DepthStencilView:
		return descHeap.CreateDepthStencilView(context, index, res);
		break;
	case ResDx::ResDxViewType_None:
		break;
	default:
		break;
	}
	return ResDxDescriptorHandle();
}

ResDx::ResDxGlobalDescriptorHeap::ResDxGlobalDescriptorHeap(ResDxDescriptorHeapType type) :type(type)
{
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateVertexBuffer(ResDxContext2& context, size_type meshIndex, const ResDxMesh& mesh)const
{
	ResDxBufferID buffer = CreateBuffer();
	auto meshData = mesh.GetMeshData(meshIndex);
	buffer->Init(context, InitDxVertex(meshData.NumVertices() * meshData.StrideVertices(), meshData.StrideVertices()));
	buffer->CopyCpuResource(meshData.ResourceVertices());
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateIndexBuffer(ResDxContext2& context, size_type meshIndex, const ResDxMesh& mesh)const
{
	ResDxBufferID buffer = CreateBuffer();
	auto meshData = mesh.GetMeshData(meshIndex);
	buffer->Init(context, InitDxIndex(meshData.NumIndices() * meshData.StrideIndices(), meshData.StrideIndices()));
	buffer->CopyCpuResource(meshData.ResourceIndices());
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateConstantBuffer(ResDxContext2& context, const size_type bufferSize)const
{
	ResDxBufferID buffer = CreateBuffer();
	buffer->Init(context, InitDxConstant(bufferSize));
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateBoneBuffer(ResDxContext2& context, size_type boneIndex, const ResDxBone& boneData)const
{
	assert(boneData.GetNumBone(boneIndex) < MAX_NUM_BONE);
	if (boneData.GetNumBone(boneIndex) == 0)
		return ResDxBufferID();
	ResDxBufferID buffer = CreateBuffer();
	array_t<ResDxMatrix> boneInitMat(boneData.GetNumBone(boneIndex));
	buffer->Init(context, InitDxConstant(sizeof(ResDxMatrix) * MAX_NUM_BONE));
	boneData.GetBoneTransform(boneIndex, &boneInitMat[0]);
	buffer->CopyCpuResource(&boneInitMat[0], 0, sizeof(ResDxMatrix) * boneData.GetNumBone(boneIndex));
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateMaterialBuffer(ResDxContext2& context, size_type materialIndex, const ResDxMaterial& material)
{
	ResDxBufferID buffer = CreateBuffer();
	auto matData = material.GetMaterialData(materialIndex);
	buffer->Init(context, InitDxConstant(sizeof(*matData)));
	buffer->CopyCpuResource(matData);
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateTextureBuffer(ResDxContext2& context, size_type textureIndex, const ResDxTexture& texture)const
{
	ResDxBufferID buffer = CreateBuffer();
	auto textureData = texture.GetTextureData(textureIndex);
	buffer->Init(context, InitDxTexture(
		textureData->width,
		textureData->height,
		DXGI_FORMAT_R8G8B8A8_UNORM
	));
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateUploadBuffer(ResDxContext2& context, const size_type textureIndex, const ResDxTexture& texture)const
{
	;
	ResDxBufferID buffer = CreateBuffer();
	auto textureData = texture.GetTextureData(textureIndex);
	buffer->Init(context, InitDxUpload(
		textureData->width,
		textureData->height,
		DXGI_FORMAT_R8G8B8A8_UNORM
	));
	buffer->CopyCpuResource(&textureData->pixels[0]);
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateParticleBuffer(ResDxContext2& context, const ResDxParticle& particleData)const
{
	assert(false);
	return ResDxBufferID();
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateRenderTaretBuffer(ResDxContext2& context, int width, int height, DXGI_FORMAT format)const
{
	ResDxBufferID buffer = CreateBuffer();
	buffer->Init(context, InitDxRenderTarget((int)width, (int)height, format));
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateDepthStencilBuffer(ResDxContext2& context, int width, int height, DXGI_FORMAT format)const
{
	ResDxBufferID buffer = CreateBuffer();
	buffer->Init(context, InitDxDepthStencil((int)width, (int)height, format));
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateUnorderedAccessBuffer(ResDxContext2& context, int numResource, int sizeInByte) const
{
	ResDxBufferID buffer = CreateBuffer();
	buffer->Init(context, InitDxRWStructured(sizeInByte, numResource));
	return buffer;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateBuffer(ResDxContext2& context, ID3D12Resource* resource, ResDxBufferInitData initData)const
{
	ResDxBufferID buffer = CreateBuffer();
	buffer->Init(context, initData, resource);
	return buffer;
}

void ResDx::ResDxBufferFactory::CreateVertexBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxMesh& mesh) const
{
	for (size_type i = 0; i < mesh.GetNumMeshData(); ++i)
		buffers[i] = CreateVertexBuffer(context, i, mesh);
}

void ResDx::ResDxBufferFactory::CreateIndexBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxMesh& mesh) const
{
	for (size_type i = 0; i < mesh.GetNumMeshData(); ++i)
		buffers[i] = CreateIndexBuffer(context, i, mesh);
}

void ResDx::ResDxBufferFactory::CreateConstantBuffers(ResDxContext2& context, ResDxBufferID* buffers, size_type numBuffer, const size_type bufferSize[]) const
{
	for (size_type i = 0; i < numBuffer; ++i)
		buffers[i] = CreateConstantBuffer(context, bufferSize[i]);
}

void ResDx::ResDxBufferFactory::CreateBoneBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxBone& boneData) const
{
	for (size_type i = 0; i < boneData.GetNumBoneData(); ++i)
		buffers[i] = CreateBoneBuffer(context, i, boneData);

}

void ResDx::ResDxBufferFactory::CreateMaterialBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxMaterial& material)
{
	for (size_type i = 0; i < material.GetNumMaterialData(); ++i)
		buffers[i] = CreateMaterialBuffer(context, i, material);
}

void ResDx::ResDxBufferFactory::CreateTextureBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxTexture& texture) const
{
	for (size_type i = 0; i < texture.GetNumTextureData(); ++i)
		buffers[i] = CreateTextureBuffer(context, i, texture);
}

void ResDx::ResDxBufferFactory::CreateUploadBuffers(ResDxContext2& context, ResDxBufferID* buffers, const ResDxTexture& texture) const
{
	for (size_type i = 0; i < texture.GetNumTextureData(); ++i)
		buffers[i] = CreateUploadBuffer(context, i, texture);

}

void ResDx::ResDxBufferFactory::DeleteBuffer(const ResDxBufferID id)const
{
	bufferPool->Release(id);
}

ResDx::ResDxBufferFactory& ResDx::ResDxBufferFactory::instance()
{
	static ResDxBufferFactory registory;
	return registory;
}

ResDx::ResDxBufferID ResDx::ResDxBufferFactory::CreateBuffer()const
{
	return bufferPool->Get();
}

void ResDx::ResDxRendererRootSignature::Init(ResDxContext2& context)
{
	const size_type numRootParam = 3;
	descRange.Init(numRootParam);
	descRange.SetDescriptorRange(ResDxViewType_ConstantBufferView, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, RES_DX_MAX_NUM_DESCRIPTOR, 0, 0);
	descRange.SetDescriptorRange(ResDxViewType_ShaderResourceView, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, RES_DX_MAX_NUM_DESCRIPTOR, 0, 0);
	descRange.SetDescriptorRange(ResDxViewType_UnorderedResourceView, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, RES_DX_MAX_NUM_DESCRIPTOR, 0, 0);
	rootSig.Init(numRootParam);
	rootSig.SetUsecaseDescriptorTable(ResDxViewType_ConstantBufferView, descRange.GetDescriptorRange(ResDxViewType_ConstantBufferView), 1, 0, 0);
	rootSig.SetUsecaseDescriptorTable(ResDxViewType_ShaderResourceView, descRange.GetDescriptorRange(ResDxViewType_ShaderResourceView), 1, 0, 0);
	rootSig.SetUsecaseDescriptorTable(ResDxViewType_UnorderedResourceView, descRange.GetDescriptorRange(ResDxViewType_UnorderedResourceView), 1, 0, 0);
	rootSig.Create(context);
}

ID3D12RootSignature* ResDx::ResDxRendererRootSignature::RootSignature()
{
	return rootSig.GetRootSignature();
}

ResDx::ResDxRendererRootSignature& ResDx::ResDxRendererRootSignature::instance()
{
	static ResDxRendererRootSignature rootSig;
	return rootSig;
}

ResDx::ResDxRendererRootSignature::ResDxRendererRootSignature()
{
}

DXGI_FORMAT ResDx::GetDXGIFormatFromResDxUsecaseRenderTarget(ResDxRenderTargetFlags usecase)
{
	DXGI_FORMAT f = {};
	ResDxRenderTargetFlags output = usecase &
		(
			ResDxRenderTargetFlags_OutputColor |
			RedDxRenderTargetFlags_OutputWorldPos |
			ResDxRenderTargetFlags_OutputNormal |
			ResDxRenderTargetFlags_OutputBinormal |
			ResDxRenderTargetFlags_OutputTangent |
			ResDxRenderTargetFlags_OutputUV
			);
	switch (output)
	{
	case ResDx::ResDxRenderTargetFlags_None:
		break;
	case ResDx::ResDxRenderTargetFlags_OutputColor:
		f = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case ResDx::RedDxRenderTargetFlags_OutputWorldPos:
		f = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case ResDx::ResDxRenderTargetFlags_OutputNormal:
		f = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case ResDx::ResDxRenderTargetFlags_OutputBinormal:
		f = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case ResDx::ResDxRenderTargetFlags_OutputTangent:
		f = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case ResDx::ResDxRenderTargetFlags_OutputUV:
		f = DXGI_FORMAT_R8G8_UINT;
		break;
	default:
		break;
	}

	return f;
}

void ResDx::GetResolutionFromResDxRenderTargetFlag(int& xResolution, int& yResolution, ResDxRenderTargetFlags flags)
{
	ResDxRenderTargetFlags resolution = (flags) &
		(
			ResDxRenderTargetFlags_Resolution480 |
			ResDxRenderTargetFlags_Resolution720 |
			ResDxRenderTargetFlags_Resolution1280 |
			ResDxRenderTargetFlags_Resolution1920
			);

	ResDxRenderTargetFlags resolutionRate = (flags) &
		(
			ResDxRenderTargetFlags_RateResolution4x3 |
			ResDxRenderTargetFlags_RateResolution16x9 |
			ResDxRenderTargetFlags_RateResolution16x10
			);

	size_t resolutionValue = 0;
	size_t xRateValue = 0;
	size_t yRateValue = 0;
	switch (resolution)
	{
	case ResDxRenderTargetFlags_Resolution480:
		resolutionValue = 480;
		break;
	case ResDxRenderTargetFlags_Resolution720:
		resolutionValue = 720;
		break;
	case ResDxRenderTargetFlags_Resolution1280:
		resolutionValue = 1280;
		break;
	case ResDxRenderTargetFlags_Resolution1920:
		resolutionValue = 1920;
		break;
	case ResDxRenderTargetFlags_ResolutionFrameBuffer:
		resolutionValue = DEFAULT_WINDOW_WIDTH;
		break;
	default:
		assert(false);
		break;
	}

	switch (resolutionRate)
	{
	case ResDxRenderTargetFlags_RateResolution4x3:
		xRateValue = 4;
		yRateValue = 3;
		break;
	case ResDxRenderTargetFlags_RateResolution16x9:
		xRateValue = 16;
		yRateValue = 9;
		break;
	case ResDxRenderTargetFlags_RateResolution16x10:
		xRateValue = 16;
		yRateValue = 10;
		break;
	case ResDxRenderTargetFlags_RateResolutionFrameBuffer:
		xRateValue = DEFAULT_WINDOW_WIDTH;
		yRateValue = DEFAULT_WINDOW_HEIGHT;
		break;
	default:
		assert(false);
		break;
	}

	xResolution = resolutionValue;
	yResolution = (resolutionValue * yRateValue) / xRateValue;
}

void ResDx::ResDxRendererRenderTarget::SetRenderTarget(ResDxContext2& context, int index, ResDxRenderTargetFlags usecase)
{
	assert(index < RES_DX_MAX_NUM_RENDER_TARGET);
	int x = 0;
	int y = 0;

	GetResolutionFromResDxRenderTargetFlag(x, y, usecase);
	format[index] = GetDXGIFormatFromResDxUsecaseRenderTarget(usecase);
	renderTargetBuffer[index] = ResDxBufferFactory::instance().CreateRenderTaretBuffer(context, x, y, format[index]);
}

DXGI_FORMAT ResDx::ResDxRendererRenderTarget::GetRenderTargetFormat(int index)
{
	assert(index < RES_DX_MAX_NUM_RENDER_TARGET);
	return format[index];
}


ResDx::ResDxRendererRenderTarget::ResDxRendererRenderTarget() :renderTargetView(0, nullptr), depthStenilView(0, nullptr), renderTargetBuffer(), format()
{
}

void ResDxCore::ResDxCommandCounter::Init(ID3D12Device5* device)
{
	this->device = device;
}

UINT64 ResDxCore::ResDxCommandCounter::RegistExecuteCommandList(ResDxCommandList* list)
{
	return executeCommandList.push({ 1,{list} });
}

UINT64 ResDxCore::ResDxCommandCounter::RegistExecuteCommandLists(int numCommandList, ResDxCommandList* lists[])
{
	ResDxCommandListPacket data = {};
	data.numCommandList = numCommandList;
	for (int i = 0; i < numCommandList; ++i)
		data.commandlist[i] = lists[i];

	return executeCommandList.push(data);
}

ResDxCore::queue_t<ResDxCore::ResDxCommandListPacket>& ResDxCore::ResDxCommandCounter::GetCommandList()
{
	return executeCommandList;
}

ResDx::ResDxCommandList* ResDxCore::ResDxCommandListDevice::CreateCommandList(ID3D12Device5* device, D3D12_COMMAND_LIST_TYPE type)
{
	ResDxCommandList* list = commandListPool.Get();
	CommandAllocatorData data = {};
	data.allocator[0] = commandAllocatorPool.Get();
	data.allocator[1] = commandAllocatorPool.Get();
	data.allocator[0]->Init(device, type);
	data.allocator[1]->Init(device, type);
	data.backBufferIndex = 1;
	list->Init(device, data.allocator[0]->Get(), type);
	allocatorMap[list] = data;
	return list;
}

ResDx::ResDxCommandListCopy ResDxCore::ResDxCommandListDevice::CreateCommandListCopy(ID3D12Device5* device)
{
	return ResDxCommandListCopy(CreateCommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT));
}

ResDx::ResDxCommandListDirect ResDxCore::ResDxCommandListDevice::CreateCommandListDirect(ID3D12Device5* device)
{
	return ResDxCommandListDirect(CreateCommandList(device, D3D12_COMMAND_LIST_TYPE_DIRECT));
}

ResDx::ResDxCommandListCompute ResDxCore::ResDxCommandListDevice::CreateCommandListCompute(ID3D12Device5* device)
{
	return ResDxCommandListCompute(CreateCommandList(device, D3D12_COMMAND_LIST_TYPE_COMPUTE));
}

ResDx::ResDxCommandListBundle ResDxCore::ResDxCommandListDevice::CreateCommandListBundle(ID3D12Device5* device)
{
	return ResDxCommandListBundle(CreateCommandList(device, D3D12_COMMAND_LIST_TYPE_BUNDLE));
}

void ResDxCore::ResDxCommandListDevice::Reset(ResDxCommandList* list)
{
	auto& data = allocatorMap[list];
	int backBufferIndex = data.backBufferIndex;
	list->Reset(data.allocator[backBufferIndex]->Get());
	resetQueue.push(data.allocator[!backBufferIndex]);
	data.backBufferIndex = !data.backBufferIndex;
}

void ResDxCore::ResDxCommandListDevice::Release(ResDxCommandList* list)
{
	auto& data = allocatorMap[list];
	list->Release();
	commandListPool.Release(list);
	data.allocator[0]->Release();
	data.allocator[1]->Release();

	commandListPool.Release(list);
	commandAllocatorPool.Release(data.allocator[0]);
	commandAllocatorPool.Release(data.allocator[1]);
	allocatorMap.erase(list);
}

void ResDxCore::ResDxCommandListDevice::ResetAllocator()
{
	ResDxCommandAllocator* allocator;
	while (!resetQueue.empty())
	{
		resetQueue.pop_front(allocator);
		allocator->Reset();
	}
}

void ResDxCore::ResDxCommandQueueDevice::Init(ID3D12Device5* device, int numCommandQueue, D3D12_COMMAND_LIST_TYPE type)
{
	executeCommandList.reserve(numCommandQueue);
	commandQueue.reserve(numCommandQueue);
	for (size_type i = 0; i < numCommandQueue; ++i)
		commandQueue[i].Init(device, type);
}

bool ResDxCore::ResDxCommandQueueDevice::Completed(int index)const
{
	return commandQueue[index].Completed();
}

bool ResDxCore::ResDxCommandQueueDevice::Empty(int index)const
{
	return executeCommandList[index].empty();
}

void ResDxCore::ResDxCommandQueueDevice::Execute(int index)
{
	auto& queue = commandQueue[index];
	auto& listQueue = executeCommandList[index];
	ResDxCommandListPacket packet = {};
	while (!listQueue.empty())
	{
		listQueue.pop_front(packet);
		queue.Execute(packet.numCommandList, packet.commandlist);
	}
}

void ResDxCore::ResDxCommandQueueDevice::Swap(int index, ResDxCommandCounter& queue)
{
	queue_t<ResDxCommandListPacket> temp = std::move(executeCommandList[index]);
	queue_t<ResDxCommandListPacket>& other = queue.GetCommandList();
	executeCommandList[index] = std::move(other);
	other = std::move(temp);
}

void ResDxCore::ResDxCommandQueueDevice::WaitCompleted(int index)
{
	commandQueue[index].WaitCompleteExecute();
}

UINT64 ResDxCore::ResDxCommandQueueDevice::CompletedValue(int index)const
{
	return commandQueue[index].FenceCompletedValue();
}

ID3D12CommandQueue* ResDxCore::ResDxCommandQueueDevice::GetQueue(int index)
{
	return commandQueue[index].GetQueue();
}

size_t ResDx::ResDxDescriptorTableID::hash() const
{
	return hashedValue;
}

bool ResDx::ResDxDescriptorTableID::operator==(const ResDxDescriptorTableID& o)
{
	return hashedValue == o.hashedValue;
}

void ResDx::ResDxDescriptorTableID::operator=(const ResDxDescriptorTableID& o)
{
	hashedValue = o.hashedValue;
}

void ResDx::ResDxDescriptorTableID::operator=(ResDxDescriptorTableID&& o)
{
	hashedValue = o.hashedValue;
	o.hashedValue = 0;
}

ResDx::ResDxDescriptorTableID::ResDxDescriptorTableID(size_type numIDs, ResDxDescriptorHeapViewID ids[]) :hashedValue()
{
	std::hash<size_type> hasher;
	for (size_type i = 0; i < numIDs; ++i)
		hashedValue += ids[i].hash();
	hashedValue = hasher(hashedValue);
}

void ResDx::ResDxRendererPipeline::VS(ResDxShader* shader)
{
	vs = shader;
}

void ResDx::ResDxRendererPipeline::PS(ResDxShader* shader)
{
	ps = shader;
}

void ResDx::ResDxRendererPipeline::CS(ResDxShader* shader)
{
	cs = shader;
}

void ResDx::ResDxRendererPipeline::SetInputElem(ResDxResourceFlags flag)
{
	inputElem = GetInputElemFromVertexFlags(flag);
	numInputElem = GetNumInputElemFromVertexFlags(flag);
}

void ResDx::ResDxRendererPipeline::SetBlend(size_type index, ResDxBlendFlag flag)
{
	blend.SetBlendState(index, flag);
}

void ResDx::ResDxRendererPipeline::SetRasterizer(ResDxRasterizerFlags flag)
{
	rasterizer.SetRasterizerState(flag);
}

void ResDx::ResDxRendererPipeline::SetDepthStencil(ResDxDepthStencilFlags flag)
{
	depthStencil.SetDepthStencil(flag);
}

void ResDx::ResDxRendererPipeline::SetRenderTarget(ResDxRenderTargetFlags flag)
{
	SetRenderTarget(1, &flag);
}

void ResDx::ResDxRendererPipeline::SetRenderTarget(size_type numRenderTarget, ResDxRenderTargetFlags flag[])
{
	this->numRenderTarget = numRenderTarget;
	for (size_type i = 0; i < numRenderTarget; ++i)
		renderTarget[i] = flag[i];
}

void ResDx::ResDxRendererPipeline::Create(ResDxContext2& context)
{
	DXGI_FORMAT rtvFormat[RES_DX_MAX_NUM_RENDER_TARGET] = {};
	for (size_type i = 0; i < numRenderTarget; ++i)
		rtvFormat[i] = GetDXGIFormatFromResDxUsecaseRenderTarget(renderTarget[i]);
	pipeline.Init(
		context,
		ResDxRendererRootSignature::instance().RootSignature(),
		numInputElem,
		inputElem,
		*vs,
		*ps,
		blend.GetBlend(),
		depthStencil.GetDepthStencil(),
		rasterizer.GetRasterizer(),
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		numRenderTarget,
		rtvFormat,
		depthStencil.GetFormat()
	);
}

ID3D12PipelineState* ResDx::ResDxRendererPipeline::GetPipelineState() const
{
	return pipeline.GetPipelineState();
}

void ResDxCore::ResDxCommandJob::Init(GraphicsDevice* device, ResDxCommandQueueType usecase)
{
	size_type numThread = std::thread::hardware_concurrency();
	commandQueue.reserve(numThread);
	for (size_type i = 0; i < numThread; ++i)
	{
		commandQueue[i].Init(device->GetDevice(), ResDxCommandListType_NumType, GetCommandListTypeFromResDxCommandListType((ResDxCommandListType)i));
		counter[i].Init(device->GetDevice());
	}
	this->device = device;
}

ResDx::ResDxCommandListCopy ResDxCore::ResDxCommandJob::CreateCommandListCopy()
{
	return ResDxCommandListCopy(commandList.CreateCommandListCopy(device->GetDevice()));
}

ResDx::ResDxCommandListDirect ResDxCore::ResDxCommandJob::CreateCommandListDirect()
{
	return ResDxCommandListDirect(commandList.CreateCommandListCopy(device->GetDevice()));
}

ResDx::ResDxCommandListCompute ResDxCore::ResDxCommandJob::CreateCommandListCompute()
{
	return ResDxCommandListCompute(commandList.CreateCommandListCompute(device->GetDevice()));
}

ResDx::ResDxCommandListBundle ResDxCore::ResDxCommandJob::CreateCommandListBundle()
{
	return ResDxCommandListBundle(commandList.CreateCommandListBundle(device->GetDevice()));
}

void ResDxCore::ResDxCommandJob::RegistCommandListCopy(ResDxCommandListCopy& commandList)
{
	counter[ResDxCommandListType_Copy].RegistExecuteCommandList(commandList);
}

void ResDxCore::ResDxCommandJob::RegistCommandListDirect(ResDxCommandListDirect& commandList)
{
	counter[ResDxCommandListType_Direct].RegistExecuteCommandList(commandList);
}

void ResDxCore::ResDxCommandJob::RegistCommandListCompute(ResDxCommandListCompute& commandList)
{
	counter[ResDxCommandListType_Compute].RegistExecuteCommandList(commandList);
}

void ResDxCore::ResDxCommandJob::ResetCommandList(ResDxCommandList* commandList)
{
	this->commandList.Reset(commandList);
}

void ResDxCore::ResDxCommandJob::ExecuteCommandList(int index)
{
	KickJob(index, ResDxJobExecuteCommandList);
}

void ResDxCore::ResDxCommandJob::ExecuteCommandListWait(int index)
{
	KickJob(index, ResDxJobExecuteCommandListWait);
}

void ResDxCore::ResDxCommandJob::ResetCommandAllocator(int index)
{
	KickJob(index, &ResDxJobResetCommandList);
}

ResDxCore::ResDxJobState ResDxCore::ResDxCommandJob::GetQueueState(size_type index)
{
	return commandListArgs[index].state;
}

void ResDxCore::ResDxCommandJob::KickJob(int index, job_t(*func)(job_arg_t))
{
	Job::Declaration decl = {};
	commandListArgs[index].index = index;
	commandListArgs[index].queue = &commandQueue[index];
	commandListArgs[index].commandList = &commandList;
	decl.entryPoint = func;
	decl.param = (job_arg_t)&commandListArgs[index];
	Job::KickJob(decl);
}

void ResDx::ResDxSwapchainDevice::Init(GraphicsDevice* device, ResDxCommandQueue* commandQueue, int numFrame, float* clearColor, int numRect, ResDxScissorRect* rect)
{
	assert(numFrame < 8);
	auto& descHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_RTV);
	ResDxContext2 context(device);
	ResDxDescriptorHeapViewID ids[8];
	ResDxBuffer buffer[8];
	ResDxBufferID bufferIDs[] = { &buffer[0],&buffer[1],&buffer[2],&buffer[3],&buffer[4],&buffer[5],&buffer[6], &buffer[7] };
	renderTargetHandle.reserve(numFrame);
	swapchain.Init(device, commandQueue->GetQueue(), numFrame, DXGI_FORMAT_R8G8B8A8_UNORM);
	for (int i = 0; i < numFrame; ++i)
	{
		buffer[i].Init(context, InitDxRenderTarget(swapchain.GetFrameBufferWidth(), swapchain.GetFrameBufferHight(), DXGI_FORMAT_R8G8B8A8_UNORM), swapchain.GetFrameRenderTargetBuffer(i));
	}
	descHeap.RegistResources(context, ids, ResDxViewType_RenderTargetView, numFrame, bufferIDs);
	descHeap.GetDescriptorHandles(numFrame, &renderTargetHandle[0], ids);

	if (clearColor)
		for (int i = 0; i < 4; ++i)
			this->clearColor[i] = clearColor[i];
	else
		for (int i = 0; i < 4; ++i)
			this->clearColor[i] = 0.0f;
	if (rect)
	{
		rects.reserve(numRect);
		for (int i = 0; i < numRect; ++i)
			rects[i] = *rect[i];
	}
	this->queue = queue;
	args.queue = queue;
	args.swapchain = &swapchain;
	args.currentIndex = &currentIndex;
}

void ResDx::ResDxSwapchainDevice::Present()
{
	Job::Declaration decl = {};
	decl.entryPoint = &ResDxJobPresent;
	decl.param = (ResDxCore::job_arg_t)&args;
	Job::KickJob(decl);
}

void ResDx::ResDxSwapchainDevice::QuerySetRenderTagret(int index, ResDxCommandListDirect& commandList)
{
	commandList.ResourceBarrier(swapchain.GetFrameRenderTargetBuffer(index), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList.SetRenderTarget(&renderTargetHandle[index], &depthStencilHandle);
}

void ResDx::ResDxSwapchainDevice::QueryClearRenderTarget(int index, ResDxCommandListDirect& commandList)
{
	if (rects.size())
		commandList.ClearRenderTargetView(renderTargetHandle[index], clearColor, rects.size(), &rects[0]);
	else
		commandList.ClearRenderTargetView(renderTargetHandle[index], clearColor, 0, nullptr);
}

void ResDx::ResDxSwapchainDevice::QueryClearDepthStencil(int index, ResDxCommandListDirect& commandList, D3D12_CLEAR_FLAGS flag)
{
	if (rects.size())
		commandList.ClearDepthStencilView(depthStencilHandle, flag, 1.0f, 0, rects.size(), &rects[0]);
	else
		commandList.ClearDepthStencilView(depthStencilHandle, flag, 1.0f, 0, 0, nullptr);
}

UINT ResDx::ResDxSwapchainDevice::GetBackBufferIndex()
{
	return swapchain.GetBackBufferIndex();
}

UINT ResDx::ResDxSwapchainDevice::GetCurrentBufferIndex()
{
	return currentIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE ResDx::ResDxSwapchainDevice::GetRenderTargetHandle(int index)
{
	return renderTargetHandle[index];
}

D3D12_CPU_DESCRIPTOR_HANDLE ResDx::ResDxSwapchainDevice::GetDepthStencilHandle()
{
	return depthStencilHandle;
}

ResDx::ResDxSwapchainDevice& ResDx::ResDxSwapchainDevice::instance()
{
	static ResDxSwapchainDevice swapchain;
	return swapchain;
}

ResDxCore::job_t ResDx::ResDxSwapchainDevice::ResDxJobPresent(ResDxCore::job_arg_t args)
{
	auto arg = (SwapchainJobArgs*)args;
	while (!arg->queue->Completed())
		co_yield ResDxCore::ResDxJobState_Yield;
	*arg->currentIndex = arg->swapchain->GetBackBufferIndex();
	arg->swapchain->Present();
	co_yield ResDxCore::ResDxJobState_End;
}

void ResDx::ResDxRendererObject::ImportModelFile(ResDxContext2& context, string_t filePath)
{
	auto& factory = ResDxBufferFactory::instance();
	auto& globalDescHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV);
	auto& descHeapTable = ResDxDescriptorHeapTable::instance();
	ResDxFileStream::ResDxFileImporter importer;
	ResDxMesh mesh;
	array_t<ResDxVertexBufferView> vertexBufferView;
	array_t<ResDxIndexBufferView> indexBufferView;
	array_t<ResDxDescriptorHeapViewID> textureViewIDs;
	importer.Import(filePath.C_Str());
	importer.GetMeshData(mesh, ResDxResourceMeshFlags_VertexType1 | ResDxResourceMeshFlags_IndexType2);
	importer.GetMaterialData(material);
	importer.GetTextureData(texture);
	importer.GetBoneData(bone);
	importer.GetAnimationData(animation, bone);

	meshName.reserve(mesh.GetNumMeshData());
	vertexBuffer.reserve(mesh.GetNumMeshData());
	indexBuffer.reserve(mesh.GetNumMeshData());
	shaderResourceBuffer.reserve(texture.GetNumTextureData() + ResDxResourceTextureType_NumMapType);
	uploadBuffer.reserve(texture.GetNumTextureData());
	boneBuffer.reserve(bone.GetNumBoneData());
	materialBuffer.reserve(material.GetNumMaterialData());
	renderPacketList.reserve(mesh.GetNumMeshData());
	shaderResourceViewID.reserve(mesh.GetNumMeshData());
	boneViewID.reserve(bone.GetNumBoneData());
	materialViewID.reserve(material.GetNumMaterialData());
	materialIndex.reserve(mesh.GetNumMeshData());
	textureViewIDs.reserve(texture.GetNumTextureData());

	if (vertexBuffer.size())
		factory.CreateVertexBuffers(context, &vertexBuffer[0], mesh);

	if (indexBuffer.size())
		factory.CreateIndexBuffers(context, &indexBuffer[0], mesh);

	if (uploadBuffer.size())
		factory.CreateUploadBuffers(context, &uploadBuffer[0], texture);

	if (shaderResourceBuffer.size())
	{
		factory.CreateTextureBuffers(context, shaderResourceBuffer.data(), texture);
		globalDescHeap.RegistResources(context, textureViewIDs.data(), ResDxViewType_ShaderResourceView, texture.GetNumTextureData(), shaderResourceBuffer.data());
	}

	factory.CreateBoneBuffers(context, boneBuffer.data(), bone);
	globalDescHeap.RegistResources(context, boneViewID.data(), ResDxViewType_ConstantBufferView, boneBuffer.size(), boneBuffer.data());

	factory.CreateMaterialBuffers(context, materialBuffer.data(), material);
	globalDescHeap.RegistResources(context, materialViewID.data(), ResDxViewType_ConstantBufferView, materialBuffer.size(), materialBuffer.data());


	for (size_type i = 0; i < vertexBuffer.size(); ++i)
	{
		int materialIndex = mesh.GetMeshData(i).MaterialIndex();
		meshName[i] = mesh.GetMeshData(i).Name();
		renderPacketList[i].RegistVertexBuffer(vertexBuffer[i]->Data<ResDxBufferType_Vertex>()->view);
		renderPacketList[i].RegistIndescBuffer(indexBuffer[i]->Data<ResDxBufferType_Index>()->view);

		shaderResourceViewID[i].reserve(texture.GetNumTextureData() + ResDxResourceTextureType_NumMapType);
		for (size_type j = 0; j < ResDxResourceTextureType_NumMapType; ++j)
			shaderResourceViewID[i][j] = ResDxRenderDummyObject::instance().GetTextureBufferView()[0];
		for (int j = 0; j < texture.GetNumTextureData(); ++j)
			shaderResourceViewID[i][j + ResDxResourceTextureType_NumMapType] = textureViewIDs[j];
	}

	boneMatrix.reserve(bone.GetNumBoneData());
	for (size_type i = 0; i < bone.GetNumBoneData(); ++i)
		boneMatrix[i].reserve(bone.GetNumBone(i));

	numIndices.reserve(mesh.GetNumMeshData());
	for (size_type i = 0; i < mesh.GetNumMeshData(); ++i)
	{
		numIndices[i] = mesh.GetMeshData(i).NumIndices();
		materialIndex[i] = mesh.GetMeshData(i).MaterialIndex();
	}
	name = FileSystem::Stem(filePath.C_Str()).string().c_str();
	path = filePath.C_Str();
}

void ResDx::ResDxRendererObject::ImportPictureFile(ResDxContext2& context, string_t filePath)
{
	ResDxFileStream::ResDxFileImporter importer;
	auto& globalDescHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV);
	auto& factory = ResDxBufferFactory::instance();
	ResDxTexture texture;

	importer.GetTextureData(texture);

	shaderResourceBuffer.reserve(texture.GetNumTextureData());
	shaderResourceViewID.reserve(1);
	shaderResourceViewID[0].reserve(texture.GetNumTextureData());
	uploadBuffer.reserve(texture.GetNumTextureData());
	factory.CreateTextureBuffers(context, &shaderResourceBuffer[0], texture);
	globalDescHeap.RegistResources(context, &shaderResourceViewID[0][ResDxResourceTextureType_NumMapType], ResDxViewType_ShaderResourceView, texture.GetNumTextureData(), &shaderResourceBuffer[0]);
}

void ResDx::ResDxRendererObject::ImportTextureSet(ResDxTextureSet& textureSet)
{
	for (int i = 0; i < meshName.size(); ++i)
	{
		auto& mesh = meshName[i];
		auto& hint = material.GetHints(materialIndex[i]);
		auto meshViewID = textureSet.GetView(mesh);
		ResDxDescriptorHeapViewID hintViewID;
		if (hint.size())
		{
			auto findStr = FileSystem::Stem(hint[0].C_Str()).string().c_str();
			hintViewID = textureSet.GetView(findStr);
		}
		if (hintViewID.valid())
		{
			shaderResourceViewID[i][ResDxResourceTextureType_DiffuseMap] = hintViewID;
		}
		else if (meshViewID.valid())
		{
			shaderResourceViewID[i][ResDxResourceTextureType_DiffuseMap] = meshViewID;
		}
		else
			continue;

		auto data = materialBuffer[i]->Data<ResDxBufferType_Constant>();
		auto mat = (ResDxMaterialData*)data->map;
		mat->useDiffuseMap = true;
	}
}
void ResDx::ResDxRendererObject::QueryResourceBarrierCommand(ResDxCommandListDirect& direct)
{
	static const int maxNumResources = 16;
	assert(shaderResourceBuffer.size() < maxNumResources);

	ID3D12Resource* resources[maxNumResources] = {};
	for (size_type i = 0; i < maxNumResources && i < shaderResourceBuffer.size(); ++i)
		resources[i] = shaderResourceBuffer[i]->Resource();
	direct.ResourceBarriers(shaderResourceBuffer.size(), resources, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

const ResDx::ResDxVertexBufferView ResDx::ResDxRendererObject::GetVertexBufferView(int meshIndex)
{
	return renderPacketList[meshIndex].GetVertexBufferView();
}

const ResDx::ResDxIndexBufferView ResDx::ResDxRendererObject::GetIndexBufferView(int meshIndex)
{
	return renderPacketList[meshIndex].GetIndexBufferView();
}

const ResDx::array_t<ResDx::ResDxDescriptorHeapViewID>& ResDx::ResDxRendererObject::GetShaderResourceViews(int meshIndex)
{
	if (shaderResourceViewID.size())
		return shaderResourceViewID[meshIndex];
	return ResDxRenderDummyObject::instance().GetTextureBufferView();
}

const ResDx::ResDxDescriptorHeapViewID ResDx::ResDxRendererObject::GetBoneBufferView(int index)
{
	if(boneViewID[index].valid())
		return boneViewID[index];
	return ResDxRenderDummyObject::instance().GetBoneBufferView();
}

const ResDx::ResDxDescriptorHeapViewID ResDx::ResDxRendererObject::GetMaterialBufferView(int index)
{
	if(materialViewID[index].valid())
		return materialViewID[index];
	return ResDxRenderDummyObject::instance().GetMaterialBufferView();
}

void ResDx::ResDxRendererObject::QueryCopyCommand(ResDxCommandListDirect& copy)
{
	for (size_type i = ResDxResourceTextureType_NumMapType; i < shaderResourceBuffer.size(); ++i)
	{
		auto footprint = uploadBuffer[i- ResDxResourceTextureType_NumMapType]->Data<ResDxBufferType_Upload>()->footprint;
		copy.CopyGpuResource(shaderResourceBuffer[i]->Resource(), uploadBuffer[i- ResDxResourceTextureType_NumMapType]->Resource(), footprint);
	}
}

void ResDx::ResDxRendererObject::QueryCopyCommand(ResDxCommandListCopy& copy)
{
	for (size_type i = ResDxResourceTextureType_NumMapType; i < shaderResourceBuffer.size(); ++i)
	{
		auto footprint = uploadBuffer[i- ResDxResourceTextureType_NumMapType]->Data<ResDxBufferType_Upload>()->footprint;
		copy.CopyGpuResource(shaderResourceBuffer[i]->Resource(), uploadBuffer[i- ResDxResourceTextureType_NumMapType]->Resource(), footprint);
	}
}

const ResDx::ResDxBone& ResDx::ResDxRendererObject::GetBoneData()
{
	return bone;
}

const ResDx::ResDxAnimation& ResDx::ResDxRendererObject::GetAnimationData()
{
	return animation;
}

const ResDx::ResDxTexture& ResDx::ResDxRendererObject::GetTextureData()
{
	return texture;
}

const ResDx::array_t<ResDx::ResDxMatrix>& ResDx::ResDxRendererObject::GetBoneMatrix(int index, float animationTick)
{
	assert(index < boneMatrix.size());
	return boneMatrix[index];
}

const ResDx::ResDxMaterial& ResDx::ResDxRendererObject::GetMaterialData()
{
	return material;
}

size_type ResDx::ResDxRendererObject::GetNumBoneData()
{
	return boneMatrix.size();
}

size_type ResDx::ResDxRendererObject::GetNumMesh()
{
	return vertexBuffer.size();
}

size_type ResDx::ResDxRendererObject::GetNumMaterial()
{
	return material.GetNumMaterialData();
}

size_type ResDx::ResDxRendererObject::GetNumTexture()
{
	return shaderResourceViewID.size();
}

size_type ResDx::ResDxRendererObject::GetNumIndices(int index)
{
	return numIndices[index];
}

size_type ResDx::ResDxRendererObject::GetMaterialIndex(int meshIndex)
{
	return materialIndex[meshIndex];
}

ResDx::string_t ResDx::ResDxRendererObject::GetMeshName(int meshIndex)
{
	return meshName[meshIndex];
}

ResDx::string_t ResDx::ResDxRendererObject::GetObjectName()
{
	return name;
}

void ResDx::ResDxResourceView::RegistTableID(int numIDs, ResDxDescriptorHeapViewID ids[])
{
	for (int i = 0; i < numIDs; ++i)
		RegistTableID(ids[i]);
}

void ResDx::ResDxResourceView::RegistTableID(ResDxDescriptorHeapViewID id)
{
	ids.push_back(id);
}

void ResDx::ResDxResourceView::ClearTableID()
{
	ids.clear();
}

void ResDx::ResDxResourceView::Commit(ResDxContext2& context)
{
	auto& table = ResDxDescriptorHeapTable::instance();
	tableID = table.CreateDescriptorsTable(context, ResDxViewType_ConstantBufferView, &ids[0], ids.get_size());
}

ResDx::ResDxDescriptorTableID ResDx::ResDxResourceView::GetTableID()
{
	return tableID;
}

void ResDx::ResDxShaderBinder::Commit(ResDxContext2& context)
{

	for (int i = 0; i < ResDxViewType_NumDescriptorHeapViewType; ++i)
	{
		auto binder = nextBinder;
		while (binder)
		{
			auto& ids = binder->GetViewID((ResDxViewType)i);
			for(int j=0;j<ids.get_size();j++)
				viewID[i].push_back(ids[j]);
			binder = binder->nextBinder;
		}

		if (viewID[i].get_size())
			tableID[i] = ResDxDescriptorHeapTable::instance().CreateDescriptorsTable(context, (ResDxViewType)i, &viewID[i][0], viewID[i].get_size());
	}
}

void ResDx::ResDxShaderBinder::SetShader(ResDxShader* shader)
{
	D3D12_SHADER_DESC desc = {};
	D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
	UINT count[ResDxViewType_NumDescriptorHeapViewType] = {};
	reflection = shader->GetReflection();
	reflection->GetDesc(&desc);
}

ResDx::ResDxDescriptorTableID ResDx::ResDxShaderBinder::GetTableID(ResDxViewType type)
{
	auto& table = ResDxDescriptorHeapTable::instance();
	return tableID[type];
}

ResDx::ResDxDescriptorHeapViewID ResDx::ResDxShaderBinder::GetViewID(UINT index)
{
	D3D12_SHADER_DESC desc = {};
	D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
	reflection->GetDesc(&desc);
	if (desc.BoundResources <= index && nextBinder)
		return GetViewID(desc.BoundResources - index);
	else if (!nextBinder)
		return{};
	reflection->GetResourceBindingDesc(index, &bindDesc);
	return viewID[TransrateViewTypeFromSIT(bindDesc.Type)][bindDesc.BindPoint];
}

ResDx::vector_t<ResDx::ResDxDescriptorHeapViewID>& ResDx::ResDxShaderBinder::GetViewID(ResDxViewType type)
{
	return viewID[type];
}

void ResDx::ResDxShaderBinder::operator+=(ResDxShaderBinder& binder)
{
	this->nextBinder = &binder;

	//for (int i = 0; i < ResDxViewType_NumDescriptorHeapViewType; ++i)
	//	for(int j=0;j<binder.viewID[i].get_size();++j)
	//		viewID[i].push_back(binder.viewID[i][j]);
}

ResDx::ResDxViewType ResDx::ResDxShaderBinder::TransrateViewTypeFromSIT(D3D_SHADER_INPUT_TYPE type)
{
	ResDxViewType ret = ResDxViewType_None;
	switch (type)
	{
	case D3D_SIT_CBUFFER:
		ret = ResDxViewType_ConstantBufferView;
		break;
	case D3D_SIT_TEXTURE:
		ret = ResDxViewType_ShaderResourceView;
		break;
	case D3D_SIT_UAV_RWSTRUCTURED:
		ret = ResDxViewType_UnorderedResourceView;
		break;
	default:
		break;
	}
	return ret;
}

D3D_SHADER_INPUT_TYPE ResDx::ResDxShaderBinder::TransrateSITFromViewType(ResDxViewType type)
{
	D3D_SHADER_INPUT_TYPE ret = {};
	switch (type)
	{
	case ResDx::ResDxViewType_ConstantBufferView:
		ret = D3D_SIT_CBUFFER;
		break;
	case ResDx::ResDxViewType_ShaderResourceView:
		ret = D3D_SIT_TEXTURE;
		break;
	case ResDx::ResDxViewType_UnorderedResourceView:
		ret = D3D_SIT_UAV_RWSTRUCTURED;
		break;
	default:
		ret = (D3D_SHADER_INPUT_TYPE)-1;
		break;
	}
	return ret;
}

bool ResDx::ResDxShaderBinder::IsBound(ResDxViewType type, UINT bindPoint)
{
	if (viewID[type].get_size() <= bindPoint)
		return true;
	return viewID[type][bindPoint].valid();
}

void ResDx::ResDxShaderBinder::PushDescriptorViewID(ResDxViewType type, UINT bindPoint, ResDxDescriptorHeapViewID id)
{
	if (viewID[type].get_size() <= bindPoint)
		viewID[type].resize(bindPoint + 1);
	viewID[type][bindPoint] = id;
}

bool ResDx::ResDxShaderBinder::CheckConstantBufferSize(LPCSTR name,size_type size)
{
	auto constantBuffer = reflection->GetConstantBufferByName(name);
	D3D12_SHADER_BUFFER_DESC desc = {};
	constantBuffer->GetDesc(&desc);
	if (size == desc.Size)
		return true;
	return false;
}

int ResDx::ResDxShaderBinder::GetIndexBindNext(ResDxViewType type, int index)
{

	if (!reflection && nextBinder)
		return GetIndexBindNext(type, index);
	else if (!reflection)
		return -1;

	D3D12_SHADER_DESC desc = {};
	D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
	reflection->GetDesc(&desc);
	for (int i = 0; i < desc.BoundResources; ++i)
	{
		auto result = reflection->GetResourceBindingDesc(i, &bindDesc);
		if (bindDesc.Type == TransrateSITFromViewType(type) && !IsBound(type, bindDesc.BindPoint))
			return index + i;
	}
	if (nextBinder)
		return nextBinder->GetIndexBindNext(type, index + desc.BoundResources);
	return -1;
}

bool ResDx::ResDxShaderBinder::BindFromIndex(int index, ResDxDescriptorHeapViewID id, ResDxViewType type, size_type size)
{
	D3D12_SHADER_DESC shaderDesc = {};
	D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
	D3D12_SHADER_VARIABLE_DESC variableDesc = {};
	reflection->GetDesc(&shaderDesc);
	if (shaderDesc.BoundResources <= index && nextBinder)
		return nextBinder->BindFromIndex(index - shaderDesc.BoundResources, id, type, size);
		
	auto result = reflection->GetResourceBindingDesc(index, &bindDesc);
	if (FAILED(result) || bindDesc.Type != TransrateSITFromViewType(type))
		return false;
	if ((type == ResDxViewType_ConstantBufferView || type==ResDxViewType_UnorderedResourceView) && !CheckConstantBufferSize(bindDesc.Name, size))
		return false;
	

	PushDescriptorViewID(type, bindDesc.BindPoint, id);
	return true;
}

UINT ResDx::ResDxShaderBinder::GetNumRegister()
{
	D3D12_SHADER_DESC desc = {};
	reflection->GetDesc(&desc);
	int num = 0;
	num += desc.BoundResources;
	if (nextBinder)
		num += nextBinder->GetNumRegister();
	return num;
}

UINT ResDx::ResDxShaderBinder::GetNumRegister(ResDxViewType type)
{
	return viewID[type].get_size();
}

ResDx::string_t ResDx::ResDxShaderBinder::GetNameRegister(UINT index)
{
	D3D12_SHADER_DESC shaderDesc = {};
	D3D12_SHADER_INPUT_BIND_DESC desc = {};
	
	reflection->GetDesc(&shaderDesc);
	auto result = reflection->GetResourceBindingDesc(index, &desc);
	
	if (shaderDesc.BoundResources <= index && nextBinder)
		return nextBinder->GetNameRegister(index - shaderDesc.BoundResources);
	if (FAILED(result))
		return "";
	return desc.Name;
}

void ResDx::ResDxRendererCamera::UpdateGpuResource()
{
	SetMatrixView(GetViewMatrix());
	SetMatrixProjection(GetProjectionMatrix());
}

ResDx::ResDxDescriptorHeapViewID ResDx::ResDxRendererCamera::GetViewID()
{
	return viewID;
}

ResDx::ResDxRendererCamera::ResDxRendererCamera(ResDxContext2& context)
{
	buffer = ResDxBufferFactory::instance().CreateConstantBuffer(context, sizeof(ResDxCameraTransform));
	ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV).RegistResources(context, &viewID, ResDxViewType_ConstantBufferView, 1, &buffer);
}

ResDx::ResDxRendererCamera::~ResDxRendererCamera()
{
	ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV).RemoveResource(viewID);
	ResDxBufferFactory::instance().DeleteBuffer(buffer);

}

void ResDx::ResDxRendererCamera::SetMatrixView(ResDxMatrix view)
{
	auto data = buffer->Data<ResDxBufferType_Constant>();
	auto map = (ResDxCameraTransform*)(data->map);
	map->view = view;
}

void ResDx::ResDxRendererCamera::SetMatrixProjection(ResDxMatrix projection)
{
	auto data = buffer->Data<ResDxBufferType_Constant>();
	auto map = (ResDxCameraTransform*)(data->map);
	map->projection = projection;
}

void ResDxCore::ResDxRendererDevice::ExecuteCommandList(ResDxCommandList* list)
{
	counter.RegistExecuteCommandList(list);
	commandQueue.Swap(0, counter);
	commandQueue.Execute(0);
	commandQueue.WaitCompleted(0);
}

void ResDxCore::ResDxRendererDevice::SetFrameRenderTargetView(ResDxCommandList* list)
{
	ResDxScissorRect rect;
	ResDxViewport viewport;
	auto backBuffer = swapchain.GetBackBufferIndex();
	ResDxCommandListDirect directCommandListContext(list);
	rect.SetRectDefault();
	viewport.SetViewportDefault();
	directCommandListContext.SetRenderTarget(&rtvHandle[backBuffer], &dsvHandle);
	directCommandListContext.SetViewport(viewport);
	directCommandListContext.SetScissorRect(rect);
}

void ResDxCore::ResDxRendererDevice::PushCommandList(ResDxCommandList* list)
{
	counter.RegistExecuteCommandList(list);
}

bool ResDxCore::ResDxRendererDevice::StartFrame()
{
	auto backBuffer = swapchain.GetBackBufferIndex();
	counter.RegistExecuteCommandList(commandList[backBuffer]);
	return true;
}

void ResDxCore::ResDxRendererDevice::EndFrame()
{
	static const int numSwapchainBuffer = 3;
	auto backBuffer = swapchain.GetBackBufferIndex();
	counter.RegistExecuteCommandList(commandList[backBuffer + numSwapchainBuffer]);
	commandQueue.Swap(0, counter);
	commandQueue.Execute(0);
	commandQueue.WaitCompleted(0);
	swapchain.Present();
}

ResDxCore::ResDxRendererDevice::ResDxRendererDevice(ResDxCommandListDevice& commandListDevice)
{
	static const int numSwapchainBuffer = 3;
	ResDxBufferID rtvBuffer;
	ResDxBufferID dsvBuffer;
	ResDxContext2 context(&graphics2);
	ResDxBufferFactory& factory = ResDxBufferFactory::instance();
	ResDxDescriptorHeapViewID renderTargetView[numSwapchainBuffer] = {};
	ResDxDescriptorHeapViewID depthStencilView = {};
	
	commandQueue.Init(graphics2.GetDevice(), 2, D3D12_COMMAND_LIST_TYPE_DIRECT);
	swapchain.Init(&graphics2, commandQueue.GetQueue(0), numSwapchainBuffer, DXGI_FORMAT_R8G8B8A8_UNORM);
	ResDxBuffer* renderTargetBuffer[numSwapchainBuffer] =
	{
		factory.CreateBuffer(context,swapchain.GetFrameRenderTargetBuffer(0),InitDxRenderTarget(DEFAULT_WINDOW_WIDTH,DEFAULT_WINDOW_HEIGHT,DXGI_FORMAT_R8G8B8A8_UNORM)),
		factory.CreateBuffer(context,swapchain.GetFrameRenderTargetBuffer(1),InitDxRenderTarget(DEFAULT_WINDOW_WIDTH,DEFAULT_WINDOW_HEIGHT,DXGI_FORMAT_R8G8B8A8_UNORM)),
		factory.CreateBuffer(context,swapchain.GetFrameRenderTargetBuffer(2),InitDxRenderTarget(DEFAULT_WINDOW_WIDTH,DEFAULT_WINDOW_HEIGHT,DXGI_FORMAT_R8G8B8A8_UNORM)),
	};
	dsvBuffer = factory.CreateDepthStencilBuffer(context, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, DXGI_FORMAT_D32_FLOAT);

	auto& gDescriptorHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV);
	auto& rtvDescriptorHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_RTV);
	auto& dsvDescriptorHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_DSV);
	rtvDescriptorHeap.RegistResources(context, renderTargetView, ResDxViewType_RenderTargetView, numSwapchainBuffer, renderTargetBuffer);
	dsvDescriptorHeap.RegistResources(context, &depthStencilView, ResDxViewType_DepthStencilView, 1, &dsvBuffer);

	rtvDescriptorHeap.GetDescriptorHandles(numSwapchainBuffer, rtvHandle, renderTargetView);
	dsvHandle = dsvDescriptorHeap.GetDescriptorHandle(depthStencilView).cpuHandle;

	commandList = new ResDxCommandList * [numSwapchainBuffer * 2];
	for(int i=0;i<numSwapchainBuffer*2;++i)
		commandList[i] = commandListDevice.CreateCommandListDirect(graphics2.GetDevice());
	SetStartFrameCommandList();
	SetEndFrameCommandList();
}

ResDxCore::ResDxRendererDevice::~ResDxRendererDevice()
{
	swapchain.Release();
}

void ResDxCore::ResDxRendererDevice::SetStartFrameCommandList()
{
	static const int numSwapchainBuffer = 3;
	FLOAT clearColor[] = { 0.0f,0.0f,0.0f,0.0f };
	ResDxScissorRect rect;
	ResDxViewport viewport;
	rect.SetRectDefault();
	viewport.SetViewportDefault();
	for (int i = 0; i < numSwapchainBuffer; ++i)
	{
		int backBuffer = i;
		ResDxCommandListDirect directCommandListContext(commandList[i]);
		directCommandListContext.ResourceBarrier(swapchain.GetFrameRenderTargetBuffer(backBuffer), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		directCommandListContext.SetRenderTarget(&rtvHandle[backBuffer], &dsvHandle);
		directCommandListContext.ClearRenderTargetView(rtvHandle[backBuffer], clearColor, 0, nullptr);
		directCommandListContext.ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		directCommandListContext.SetViewport(viewport);
		directCommandListContext.SetScissorRect(rect);
		commandList[i]->Close();
	}
}

void ResDxCore::ResDxRendererDevice::SetEndFrameCommandList()
{
	static const int numSwapchainBuffer = 3;
	for (int i = 0; i < numSwapchainBuffer; ++i)
	{
		int backBuffer = i;
		ResDxCommandListDirect directCommandListContext(commandList[i+numSwapchainBuffer]);

		directCommandListContext.ResourceBarrier(swapchain.GetFrameRenderTargetBuffer(backBuffer), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList[i+numSwapchainBuffer]->Close();
	}
}

void ResDx::ResDxRenderObjectBundle::SetDrawBundle(ResDxContext2& context, ResDxCommandList* list, ResDxRendererObject& obj,ResDxMeshInstances& meshInstance, ResDxRendererCamera& camera)
{
	
	auto& table = ResDxDescriptorHeapTable::instance();
	ResDxCommandListBundle bundle(list);
	
	auto unorderedResourceViewIDs = meshInstance.GetConstantViewID();
	auto unorderedResourceViewTableID = table.CreateDescriptorsTable(context, ResDxViewType_UnorderedResourceView, &unorderedResourceViewIDs, 1);
	auto& rootSig = ResDxRendererRootSignature::instance();
	auto& tableDescriptorHeap = ResDxDescriptorHeapTable::instance();
	bundle.SetGraphicsRootSignature(rootSig.RootSignature());
	bundle.SetDescriptorHeap(tableDescriptorHeap.GetDescriptorHeap(ResDxDescriptorHeapType_CBV_SRV_UAV)->GetDescriptorHeap());
	bundle.SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	bundle.SetGraphicsDescriptorTable(ResDxViewType_UnorderedResourceView, table.GetDescriptorGPUHandle(unorderedResourceViewTableID));

	for (int i = 0; i < obj.GetNumMesh(); ++i)
	{
		ResDxDescriptorHeapViewID constantBufferViewIDs[3] =
		{
			camera.GetViewID(),
			obj.GetMaterialBufferView(obj.GetMaterialIndex(i)),
			obj.GetBoneBufferView(i)
		};
		auto shaderResourceViewIDs = obj.GetShaderResourceViews(i);
		auto constantBufferViewtableID = table.CreateDescriptorsTable(context, ResDxViewType_ConstantBufferView, constantBufferViewIDs, 3);
		auto shaderResourceViewTableID = table.CreateDescriptorsTable(context, ResDxViewType_ShaderResourceView, shaderResourceViewIDs.data(), shaderResourceViewIDs.size());
		bundle.SetGraphicsDescriptorTable(ResDxViewType_ShaderResourceView, table.GetDescriptorGPUHandle(shaderResourceViewTableID));
		bundle.SetGraphicsDescriptorTable(ResDxViewType_ConstantBufferView, table.GetDescriptorGPUHandle(constantBufferViewtableID));

		auto vertexBufferView = obj.GetVertexBufferView(i);
		auto indexBufferView = obj.GetIndexBufferView(i);
		bundle.SetVertexBuffer(&vertexBufferView);
		bundle.SetIndexBuffer(&indexBufferView);
		bundle.DrawIndexedInstanced(obj.GetNumIndices(i), meshInstance.NumTransforms());
	}
}

void ResDx::ResDxRenderObjectBundle::SetResourceBundle(int meshIndex,ResDxContext2& context, ResDxCommandList* list, ResDxRendererObject& obj, ResDxMeshInstances& meshInstance, ResDxRendererCamera& camera)
{

	auto& table = ResDxDescriptorHeapTable::instance();
	ResDxCommandListBundle bundle(list);
	ResDxDescriptorHeapViewID constantBufferViewIDs[3] =
	{
		camera.GetViewID(),
		ResDxDescriptorHeapViewID(),
		ResDxDescriptorHeapViewID()
	};
	auto shaderResourceViewIDs = obj.GetShaderResourceViews(meshIndex);
	auto unorderedResourceViewIDs = meshInstance.GetConstantViewID();
	auto constantBufferViewtableID = table.CreateDescriptorsTable(context, ResDxViewType_ConstantBufferView, constantBufferViewIDs, 3);
	auto shaderResourceViewTableID = table.CreateDescriptorsTable(context, ResDxViewType_ShaderResourceView, shaderResourceViewIDs.data(), shaderResourceViewIDs.size());
	auto unorderedResourceViewTableID = table.CreateDescriptorsTable(context, ResDxViewType_UnorderedResourceView, &unorderedResourceViewIDs, 1);
	bundle.SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	bundle.SetGraphicsDescriptorTable(ResDxViewType_ConstantBufferView, table.GetDescriptorGPUHandle(constantBufferViewtableID));
	bundle.SetGraphicsDescriptorTable(ResDxViewType_ShaderResourceView, table.GetDescriptorGPUHandle(shaderResourceViewTableID));
	bundle.SetGraphicsDescriptorTable(ResDxViewType_UnorderedResourceView, table.GetDescriptorGPUHandle(unorderedResourceViewTableID));

}

void ResDx::ResDxRenderObjectBundle::SetMeshBundle(int meshIndex, ResDxCommandList* list, ResDxRendererObject& obj)
{
	ResDxCommandListBundle bundle(list);
	auto vertexBufferView = obj.GetVertexBufferView(meshIndex);
	auto indexBufferView = obj.GetIndexBufferView(meshIndex);
	bundle.SetVertexBuffer(&vertexBufferView);
	bundle.SetIndexBuffer(&indexBufferView);

}


void ResDx::ResDxRenderShaderBundle::SetDefault()
{
	meshFlags = ResDxResourceMeshFlags_VertexType1;
	rtvFlags = ResDxRenderTargetFlags_OutputColor | ResDxRenderTargetFlags_RateResolutionFrameBuffer | ResDxRenderTargetFlags_ResolutionFrameBuffer;
	dsvFlags = ResDxDepthStencilFlags_FormatD32_FLOAT | ResDxDepthStencilFlags_DepthEnable | ResDxDepthStencilFlags_DepthComparisonLess;
	rasFlags = ResDxRasterizerFlags_CullBack | ResDxRasterizerFlags_DepthBiasNone | ResDxRasterizerFlags_AntialiasedLineEnable | ResDxRasterizerFlags_FillSolid;
}

void ResDx::ResDxRenderShaderBundle::SetVS(string_t path)
{
	vs.LoadVS(path.C_Str(),"main");
}

void ResDx::ResDxRenderShaderBundle::SetPS(string_t path)
{
	ps.LoadPS(path.C_Str(), "main");
}

void ResDx::ResDxRenderShaderBundle::CommitPipelineGraphics(ResDxContext2& context)
{
	pipeline.VS(&vs);
	pipeline.PS(&ps);
	pipeline.SetInputElem(meshFlags);
	pipeline.SetDepthStencil(dsvFlags);
	pipeline.SetRenderTarget(rtvFlags);
	pipeline.SetRasterizer(rasFlags);
	pipeline.Create(context);
}

void ResDx::ResDxRenderShaderBundle::SetShaderBundle(ResDxCommandList* list)
{
	ResDxCommandListBundle bundle(list);
	bundle.SetPipelineState(pipeline.GetPipelineState());

}

const ResDx::ResDxDescriptorHeapViewID ResDx::ResDxRenderDummyObject::GetBoneBufferView()
{
	return boneBufferView;
}

const ResDx::ResDxDescriptorHeapViewID ResDx::ResDxRenderDummyObject::GetMaterialBufferView()
{
	return materialBufferView;
}

const ResDx::array_t<ResDx::ResDxDescriptorHeapViewID>& ResDx::ResDxRenderDummyObject::GetTextureBufferView()
{
	return textureBufferView;
}

ResDx::ResDxRenderDummyObject::ResDxRenderDummyObject()
{
	ResDxContext2 context(&graphics2);
	ResDxCommandQueue queue;
	ResDxCore::ResDxCommandListDevice list;
	ResDxMaterial material;
	ResDxTexture texture;
	ResDxFileStream::ResDxFileImporter importer;
	texture.Init(1);
	material.Init(1);
	material.SetMaterial(0, 1.0, ResDxVector3(1.0, 1.0, 1.0), ResDxVector(1.0, 1.0, 1.0, 1.0), 0, nullptr, nullptr,nullptr);
	importer.ImportTexture(0,"dummy.png",texture);
	auto& factory = ResDxBufferFactory::instance();
	auto& gDescriptorHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV);
	auto materialBufferID = factory.CreateMaterialBuffer(context, 0, material);
	auto textureBufferID = factory.CreateTextureBuffer(context, 0, texture);
	auto boneBufferID = factory.CreateConstantBuffer(context, MAX_NUM_BONE * sizeof(ResDxMatrix));
	auto map = (ResDxMatrix*)boneBufferID->Data<ResDxBufferType_Constant>()->map;
	uploadBuffer = factory.CreateUploadBuffer(context, 0, texture);

	for (int i = 0; i < MAX_NUM_BONE; ++i)
		map[i] = DirectX::XMMatrixIdentity();

	textureBufferView.reserve(1);
	gDescriptorHeap.RegistResources(context, &materialBufferView, ResDxViewType_ConstantBufferView, 1, &materialBufferID);
	gDescriptorHeap.RegistResources(context, &boneBufferView, ResDxViewType_ConstantBufferView, 1, &boneBufferID);
	gDescriptorHeap.RegistResources(context, &textureBufferView[0], ResDxViewType_ShaderResourceView, 1, &textureBufferID);
	
	queue.Init(graphics2.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto directList = list.CreateCommandListDirect(graphics2.GetDevice());
	directList.CopyGpuResource(textureBufferID->Resource(), uploadBuffer->Resource(), uploadBuffer->Data<ResDxBufferType_Upload>()->footprint);
	directList.Get()->Close();
	queue.Execute(directList);
	list.Release(directList.Get());
	queue.Release();
}

ResDx::ResDxRenderDummyObject& ResDx::ResDxRenderDummyObject::instance()
{
	static ResDxRenderDummyObject instance;
	return instance;
}

void ResDx::ResDxTextureSet::ImportTextrueDirectory(ResDxContext2& context, ResDx::string_t dir)
{
	if (!FileSystem::IsDirectory(dir.C_Str()))
		return;
	ResDxCore::ResDxCommandQueue queue;
	ResDxCore::ResDxCommandListDevice list;
	ResDxFileStream::ResDxFileImporter importer;
	auto& gDescHeap = ResDxGlobalDescriptorHeap::instance(ResDxDescriptorHeapType_CBV_SRV_UAV);
	auto& factory = ResDxBufferFactory::instance();
	vector_t<FileSystem::path_t> pathes;
	auto it = FileSystem::fm.file(dir.C_Str());
	
	while (!it.last())
	{
		auto path = it.get();
		it.next();
		
		if (path.extension() != ".png" && path.extension() != ".jpg" && path.extension() != ".jpg")
			continue;
		pathes.push_back(path.string().c_str());
	}
	names.reserve(pathes.get_size());
	buffers.reserve(pathes.get_size());
	upload.reserve(pathes.get_size());
	viewIDs.reserve(pathes.get_size());
	texture.Init(pathes.get_size());
	queue.Init(graphics2.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto direct = list.CreateCommandListDirect(graphics2.GetDevice());
	for (int i = 0; i < pathes.get_size(); ++i) 
	{
		names[i] = pathes[i].stem().string().c_str();
		importer.ImportTexture(i, pathes[i].string().c_str(), texture);
		buffers[i] = factory.CreateTextureBuffer(context, i, texture);
		upload[i] = factory.CreateUploadBuffer(context, i, texture);
		direct.CopyGpuResource(buffers[i]->Resource(), upload[i]->Resource(), upload[i]->Data<ResDxBufferType_Upload>()->footprint);
	}
	gDescHeap.RegistResources(context, viewIDs.data(), ResDxViewType_ShaderResourceView, buffers.size(), buffers.data());
	
	direct.Get()->Close();
	queue.Execute(direct);
	queue.WaitCompleteExecute();
	list.Release(direct);
	queue.Release();
}

ResDx::ResDxDescriptorHeapViewID ResDx::ResDxTextureSet::GetView(ResDx::string_t name) const
{
	int idx = -1;
	for (int i = 0; i < names.size(); ++i)
		if (strstr(names[i].C_Str(), name.C_Str()) && abs(long(names[i].Lengh() - name.Lengh())) < 5)
			idx = i;
	if (idx != -1)
		return viewIDs[idx];
	return ResDxDescriptorHeapViewID();
}

ResDx::ResDxDescriptorHeapViewID ResDx::ResDxTextureSet::GetView(int idx) const
{
	return viewIDs[idx];
}

int ResDx::ResDxTextureSet::GetNumTexture() const
{
	return viewIDs.size();
}
