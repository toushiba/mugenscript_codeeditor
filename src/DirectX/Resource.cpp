#include <Windows.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include "Buffer/Jpeg.h"
#include "Resource.h"
#include "DirectXTex.h"

#define ERROR_MESSAGE_MODEL_LOADER(message) MessageBox((nullptr),(message),L"Resource Database error",MB_OK)

static std::vector<size_t> meshBaseVertex;
static const char* VSShaderModel = "vs_5_0";
static const char* PSShaderModel = "ps_5_0";
static const char* CSShaderModel = "ps_5_0";

std::wstring ToWCHR(const char* src);
DirectX::XMMATRIX GetMatrixFromAiMatrix(aiMatrix4x4);

struct ModelLoader
{
	void ParseMesh(ResMesh& dst, const aiMesh* srcMesh);
	void ParseMaterial(ResMaterial& dst, const aiMaterial* srcMaterial);
	void LoadMeshBone(int meshIdx, std::vector<BoneData*>&, const aiMesh*);
	void LoadSingleBone(int meshIdx, BoneData& dst, const aiBone*);
	void ReadBoneHierarchy(BoneData& dst, const aiNode* node, const DirectX::XMMATRIX& parentTransform);
	void ParseBoneHierarchy(BoneData& dst, BoneNode*& parent, const aiNode*);
	void AddBoneData(BoneData::Weight&, int boneID, float weight);
	void GetBoneTransforms(BoneData&, const aiScene*);
	int GetBoneID(BoneData& dst, const aiBone* bone);
	void ParseAnimation(AnimationData& dst, const aiAnimation* anim, aiNode* ndoe);
};


LoadedResourceBuffer ResourceLoader::LoadModel(const char* fileName)
{
	if (!fileName)
		return {};

	Assimp::Importer importer;
	LoadedResourceBuffer modelData = {};

	int flag = 0;
	flag |= aiProcess_Triangulate;
	flag |= aiProcess_CalcTangentSpace;
	flag |= aiProcess_GenSmoothNormals;
	flag |= aiProcess_GenUVCoords;
	flag |= aiProcess_OptimizeMeshes;
	flag |= aiProcess_JoinIdenticalVertices;

	auto pScene = importer.ReadFile(fileName, flag);
	if (!pScene)
		return {};

	size_t numMeshes = pScene->mNumMeshes;
	size_t numAnimation = pScene->mNumAnimations;
	modelData.meshes.resize(numMeshes);
	modelData.bones.resize(numMeshes);
	modelData.animations.resize(numAnimation);

	for (int i = 0; i < numMeshes; ++i)
	{
		modelData.meshes[i] = std::unique_ptr<ResMesh>(new ResMesh{});
		modelData.bones[i] = std::unique_ptr<BoneData>(new BoneData{});
	}

	for (auto& a : modelData.animations)
		a = std::unique_ptr<AnimationData>(new AnimationData{});

	ModelLoader loader;
	meshBaseVertex.clear();
	size_t totalVertices = 0;

	//メッシュの読み込み
	for (size_t i = 0; i < numMeshes; ++i)
	{
		const auto pMesh = pScene->mMeshes[i];

		loader.ParseMesh(*modelData.meshes[i], pMesh);//メッシュの初期化データを作成
		meshBaseVertex.push_back(totalVertices);
		totalVertices += pMesh->mNumVertices;

		size_t numBones = pMesh->mNumBones;
		if (pMesh->HasBones())
		{
			DirectX::XMMATRIX rootMat = DirectX::XMMatrixIdentity();
			modelData.bones[i]->boneWeight.resize(pMesh->mNumVertices);

			for (UINT j = 0; j < pMesh->mNumBones; ++j)
			{
				loader.LoadSingleBone(i, *modelData.bones[i].get(), pMesh->mBones[j]);
				modelData.bones[i].get()->meshIdx = i;
			}
			loader.ParseBoneHierarchy(*modelData.bones[i], modelData.bones[i]->rootNode, pScene->mRootNode);
			loader.ReadBoneHierarchy(*modelData.bones[i], pScene->mRootNode, rootMat);
		}
	}

	for (size_t i = 0; i < numAnimation; ++i)
	{
		loader.ParseAnimation(*modelData.animations[i], pScene->mAnimations[i], pScene->mRootNode);
		auto m = pScene->mRootNode->mTransformation;
		modelData.animations[i]->globalInverseTransform = GetMatrixFromAiMatrix(m.Inverse());
	}

	modelData.materials.clear();
	modelData.materials.resize(pScene->mNumMaterials);

	for (size_t i = 0; i < modelData.materials.size(); ++i)
	{
		const auto pMaterial = pScene->mMaterials[i];
		modelData.materials[i].reset(new ResMaterial{});
		modelData.materials[i]->texture.resize(pScene->mNumTextures);
		loader.ParseMaterial(*modelData.materials[i], pMaterial);

		for (size_t j = 0; j < pScene->mNumTextures; ++j)
		{
			ResTexture tex;
			auto srcTex = pScene->mTextures[j];
			tex.width = srcTex->mWidth;
			tex.height = srcTex->mHeight;

			if (tex.height == 0)
			{
				std::string hint = srcTex->achFormatHint;
				if (hint == "jpg" || hint == "jpeg")
				{
					Jpeg::BitmapData bitMap;
					Jpeg::JpegMemReadDecode(&bitMap, (const unsigned char*)srcTex->pcData, tex.width, Jpeg::SAMPLING_OPTION_INV_Y);
					auto cBitMap = Jpeg::ConvertToChannel(&bitMap, 4);
					Jpeg::FreeBitMap(&bitMap);
					tex.name = "texture_";
					tex.name += std::to_string(j);
					tex.height = cBitMap.height;
					tex.width = cBitMap.width;
					tex.pixels = (cBitMap.data);
					tex.sizeOfByte = cBitMap.width * cBitMap.height * cBitMap.ch;
					tex.ch = cBitMap.ch;
					tex.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				}
				else
				{
					ERROR_MESSAGE_MODEL_LOADER(L"サポートされていない圧縮形式です");
				}
			}
			else
			{
				tex.pixels = (new uint8_t[srcTex->mWidth * srcTex->mHeight]);
				tex.height = srcTex->mHeight;
				tex.width = srcTex->mWidth;
				tex.sizeOfByte = srcTex->mWidth * srcTex->mHeight;
				memcpy(tex.pixels, srcTex->pcData, tex.height * tex.width);
			}
			modelData.materials[i]->texture[j] = std::move(tex);
		}
	}
	pScene = nullptr;
	return modelData;
}

void ResourceLoader::DeleteLoadData(LoadedResourceBuffer& buff)
{
	for (auto& a : buff.meshes)
		a.release();
	for (auto& a : buff.materials)
		a.release();
	for (auto& a : buff.bones)
		a.release();
	for (auto& a : buff.animations)
		a.release();
}

void ResourceLoader::DeleteLoadData(ResTexture* tex)
{
	if (tex)
		delete tex;
}

std::wstring ToWCHR(const char* src)
{
	int lengh = MultiByteToWideChar(CP_UTF8, 0, src, -1, nullptr, 0);
	wchar_t* wcp = new wchar_t[lengh];
	MultiByteToWideChar(CP_UTF8, 0, src, -1, wcp, lengh);
	std::wstring ret = wcp;
	delete[] wcp;
	return ret;
}

DirectX::XMMATRIX GetMatrixFromAiMatrix(aiMatrix4x4 vec)
{
	DirectX::XMMATRIX mat =
	{
		vec.a1,vec.a2,vec.a3,vec.a4,
		vec.b1,vec.b2,vec.b3,vec.b4,
		vec.c1,vec.c2,vec.c3,vec.c4,
		vec.d1,vec.d2,vec.d3,vec.d4,
	};
	mat = DirectX::XMMatrixTranspose(mat);
	return mat;
}


void ModelLoader::ParseMesh(ResMesh& dst, const aiMesh* srcMesh)
{
	dst.name = srcMesh->mName.C_Str();
	dst.materialIdx = srcMesh->mMaterialIndex;
	aiVector3D zero3D(0.0f, 0.0f, 0.0f);
	for (auto i = 0u; i < srcMesh->mNumVertices; ++i)
	{
		auto pos = &(srcMesh->mVertices[i]);
		auto normal = &(srcMesh->mNormals[i]);
		auto tex = srcMesh->HasTextureCoords(0) ? &(srcMesh->mTextureCoords[0][i]) : &zero3D;
		auto tangent = srcMesh->HasTangentsAndBitangents() ? &(srcMesh->mTangents[i]) : &zero3D;
		auto binormal = srcMesh->HasTangentsAndBitangents() ? &(srcMesh->mBitangents[i]) : &zero3D;

		dst.vertices.push_back({
			DirectX::XMFLOAT3{pos->x,pos->y,pos->z},
			DirectX::XMFLOAT3{normal->x,normal->y,normal->z},
			DirectX::XMFLOAT3{tangent->x,tangent->y,tangent->z},
			DirectX::XMFLOAT3{binormal->x,binormal->y,binormal->z},
			DirectX::XMFLOAT2{tex->x,tex->y}
			});
	}

	dst.indices.resize(srcMesh->mNumFaces * 3u);
	for (auto i = 0u; i < srcMesh->mNumFaces; ++i)
	{
		const auto& face = srcMesh->mFaces[i];
		assert(face.mNumIndices == 3u);

		dst.indices[i * 3 + 0] = face.mIndices[0];
		dst.indices[i * 3 + 1] = face.mIndices[1];
		dst.indices[i * 3 + 2] = face.mIndices[2];

	}
}

void ResourceLoader::LoadTexture(const char* fileName, std::unique_ptr<ResTexture>& handle)
{

	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage image = {};
	std::wstring fileNameWstr = ToWCHR(fileName);

	auto result = DirectX::LoadFromWICFile(fileNameWstr.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image);
	if (FAILED(result))
	{
		MessageBoxA(nullptr, "画像ファイルの読み込みに失敗しました", "error", MB_OK);
		return;
	}

	auto img = image.GetImage(0, 0, 0);
	auto resTexture = new ResTexture;
	resTexture->name = fileName;
	resTexture->width = img->width;
	resTexture->height = img->height;
	resTexture->rowPitch = img->rowPitch;
	resTexture->depth = metadata.depth;
	resTexture->mip = metadata.mipLevels;
	resTexture->format = img->format;
	resTexture->dimension = (D3D12_RESOURCE_DIMENSION)metadata.dimension;
	resTexture->pixels = (new uint8_t[img->rowPitch * img->height]);
	resTexture->sizeOfByte = img->rowPitch * img->height;
	memcpy(resTexture->pixels, img->pixels, img->rowPitch * img->height);
	handle.reset(resTexture);
}

void ModelLoader::ParseMaterial(ResMaterial& dst, const aiMaterial* srcMaterial)
{

	//拡散反射
	{
		aiColor3D color(0.0f, 0.0f, 0.0f);

		if (srcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
		{
			dst.diffuse.x = color.r;
			dst.diffuse.y = color.g;
			dst.diffuse.z = color.b;
		}
		else
		{
			dst.diffuse.x = 0.5f;
			dst.diffuse.y = 0.5f;
			dst.diffuse.z = 0.5f;
		}
	}

	//鏡面反射
	{
		aiColor3D color(0.0f, 0.0f, 0.0f);

		if (srcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
		{
			dst.specular.x = color.r;
			dst.specular.y = color.g;
			dst.specular.z = color.b;
		}
		else
		{

			dst.specular.x = 0.0f;
			dst.specular.y = 0.0f;
			dst.specular.z = 0.0f;
		}
	}

	//鏡面反射強度
	{
		auto shininess = 0.0f;
		if (srcMaterial->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
		{
			dst.shininess = shininess;
		}
		else
		{
			dst.shininess = 0.0f;
		}

		//ディフューズマップ
		{
			aiString path;
			if (srcMaterial->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), path) == AI_SUCCESS)
			{
				dst.diffuseMap = std::string(path.C_Str());
			}
			else
			{
				dst.diffuseMap.clear();
			}
		}
		//スペキュラマップ
		{
			aiString path;
			if (srcMaterial->Get(AI_MATKEY_TEXTURE_SPECULAR(0), path) == AI_SUCCESS)
			{
				dst.specularMap = std::string(path.C_Str());
			}
			else
				dst.diffuseMap.clear();
		}
		//シャインマップ
		{
			aiString path;
			if (srcMaterial->Get(AI_MATKEY_TEXTURE_SHININESS(0), path) == AI_SUCCESS)
			{
				dst.shininessMap = std::string(path.C_Str());
			}
			else
				dst.shininessMap.clear();
		}
		//ノーマルマップ
		{
			aiString path;
			if (srcMaterial->Get(AI_MATKEY_TEXTURE_NORMALS(0), path) == AI_SUCCESS)
			{
				dst.normalMap = std::string(path.C_Str());
			}
			else
				dst.normalMap.clear();
		}
	}
}


void ModelLoader::LoadMeshBone(int meshIdx, std::vector<BoneData*>& dst, const aiMesh* pMesh)
{

	for (auto i = 0u; i < pMesh->mNumBones; ++i)
	{
		LoadSingleBone(meshIdx, *dst[meshIdx], pMesh->mBones[i]);
		dst[meshIdx]->meshIdx = meshIdx;
	}
}

void ModelLoader::LoadSingleBone(int meshIdx, BoneData& dst, const aiBone* bone)
{
	int boneID = GetBoneID(dst, bone);
	if (boneID == dst.boneIdxMap.size() - 1)
	{
		BoneData::BoneInfo info;
		info.boneOffsetTransform = GetMatrixFromAiMatrix(bone->mOffsetMatrix);
		dst.boneInfo.push_back(info);
	}

	for (auto i = 0u; i < bone->mNumWeights; ++i)
	{
		const aiVertexWeight& vw = bone->mWeights[i];
		size_t vertexID = meshBaseVertex[meshIdx] + bone->mWeights[i].mVertexId;
		AddBoneData(dst.boneWeight[vertexID], boneID, vw.mWeight);
	}
}

void ModelLoader::ReadBoneHierarchy(BoneData& dst, const aiNode* node, const DirectX::XMMATRIX& parentTransform)
{
	std::string nodeName = node->mName.C_Str();
	DirectX::XMMATRIX nodeTransformation = GetMatrixFromAiMatrix(node->mTransformation);
	DirectX::XMMATRIX globalTransformation = parentTransform * nodeTransformation;

	if (dst.boneIdxMap.find(nodeName) != dst.boneIdxMap.end())
	{
		size_t boneIdx = dst.boneIdxMap[nodeName];
		dst.boneInfo[boneIdx].finalTransform = globalTransformation * dst.boneInfo[boneIdx].boneOffsetTransform;
	}

	for (auto i = 0u; i < node->mNumChildren; ++i)
		ReadBoneHierarchy(dst, node->mChildren[i], globalTransformation);
}

void ModelLoader::ParseBoneHierarchy(BoneData& dst, BoneNode*& parent, const aiNode* node)
{
	std::string nodeName = node->mName.C_Str();

	auto newNode = new BoneNode;
	newNode->boneName = nodeName;
	newNode->transform = GetMatrixFromAiMatrix(node->mTransformation);
	newNode->boneID = -1;
	if (dst.boneIdxMap.find(nodeName) != dst.boneIdxMap.end())
	{
		newNode->boneID = (int)dst.boneIdxMap[nodeName];
	}

	if (parent)
		parent->child.push_back(newNode);
	else
		parent = newNode;
	for (auto i = 0u; i < node->mNumChildren; ++i)
		ParseBoneHierarchy(dst, newNode, node->mChildren[i]);
}

void ModelLoader::AddBoneData(BoneData::Weight& dst, int boneID, float weight)
{
	for (auto i = 0u; i < MAX_NUM_BONE_VERTEX; ++i)
	{
		if (dst.weight[i] == 0.0f)
		{
			dst.boneID[i] = boneID;
			dst.weight[i] = weight;
			return;
		}
	}
	assert(0);
}

void ModelLoader::GetBoneTransforms(BoneData& dst, const aiScene* pScene)
{
	auto node = pScene->mRootNode;
	DirectX::XMMATRIX mat = DirectX::XMMatrixIdentity();
	ReadBoneHierarchy(dst, node, mat);
	ParseBoneHierarchy(dst, dst.rootNode, node);
}

int ModelLoader::GetBoneID(BoneData& dst, const aiBone* bone)
{
	int boneID = 0;
	std::string boneName = bone->mName.C_Str();

	if (dst.boneIdxMap.find(boneName) == dst.boneIdxMap.end())
	{
		boneID = (int)dst.boneIdxMap.size();
		dst.boneIdxMap[boneName] = boneID;
	}
	else
	{
		boneID = (int)dst.boneIdxMap[boneName];
	}

	return boneID;
}

void ModelLoader::ParseAnimation(AnimationData& dst, const aiAnimation* anim, aiNode* node)
{
	dst.name = anim->mName.C_Str();
	dst.tickPerSec = anim->mTicksPerSecond;
	dst.duration = anim->mDuration;
	for (auto i = 0u; i < anim->mNumChannels; ++i)
	{
		AnimationData::Channel channel = {};
		auto boneNode = node->FindNode(anim->mChannels[i]->mNodeName);
		if (!boneNode)
			continue;

		channel.boneName = boneNode->mName.C_Str();

		for (auto j = 0u; j < anim->mChannels[i]->mNumPositionKeys; ++j)
		{
			auto key = anim->mChannels[i]->mPositionKeys[j];
			channel.position.push_back({ key.mTime,{key.mValue.x,key.mValue.y,key.mValue.z} });
		}

		for (auto j = 0u; j < anim->mChannels[i]->mNumRotationKeys; ++j)
		{
			auto key = anim->mChannels[i]->mRotationKeys[j];
			channel.rotation.push_back({ key.mTime,{key.mValue.x,key.mValue.y,key.mValue.z,key.mValue.w} });
		}

		for(auto j = 0u; j < anim->mChannels[i]->mNumScalingKeys; ++j)
		{
			auto key = anim->mChannels[i]->mScalingKeys[j];
			channel.scale.push_back({ key.mTime,{key.mValue.x,key.mValue.y,key.mValue.z} });
		}

		dst.channels.push_back(channel);
	}
}


void ResDx::ResDxBuffer::Init(ResDxContext2& context, const ResDxBufferInitData initData)
{
	if (IsInit())
		Clear();

	auto dataType = initData.type;

	switch (dataType)
	{
	case ResDxBufferType_Vertex:
			context.CreateVertexBuffer(initData.data.vertex.bufferSize, initData.data.vertex.stride, &resource);
			CreateDataVertex(initData);
		break;

	case ResDxBufferType_Index:
			context.CreateIndexBuffer(initData.data.index.bufferSize, initData.data.index.stride, &resource);
			CreateDataIndex(initData);
		break;

	case ResDxBufferType_Texture:
			context.CreateTextureBuffer(initData.data.texture.width, initData.data.texture.height, initData.data.texture.format, initData.data.texture.flags,D3D12_RESOURCE_STATE_COPY_DEST,  &resource);
			CreateDataTexture(initData);
			break;

	case ResDxBufferType_Constant:
			context.CreateConstantBuffer(initData.data.constant.bufferSize, &resource);
			CreateDataConstant(initData);
			break;

	case ResDxBufferType_Upload:
	{
		UINT64 uploadSize = {};
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		context.CreateUploadBuffer(initData.data.upload.width, initData.data.upload.height, initData.data.upload.format, &resource, &uploadSize, &footprint);
		CreateDataUpload(initData, uploadSize, footprint);
		break;
	}
	case ResDxBufferType_Structured:
			context.CreateStructuredBuffer(initData.data.structured.sizeOfElem, initData.data.structured.numOfElem, &resource);
			CreateDataStructured(initData);
			break;

	case ResDxBufferType_RWStructured:
	{
		ID3D12Resource* counter = nullptr;
		context.CreateRWStructuredBuffer(initData.data.rwstructured.sizeOfElem, initData.data.rwstructured.numOfElem, &resource, &counter);
		CreateDataRWStructured(initData,counter);
		break;
	}
	case ResDxBufferType_RenderTarget:
			context.CreateRenderTargetBuffer(initData.data.rendertarget.width, initData.data.rendertarget.height, initData.data.rendertarget.format, &resource);
			CreateDataRenderTarget(initData);
		break;
	case ResDxBufferType_DepthStencil:
			context.CreateDepthStencilBuffer(initData.data.depthstencil.width, initData.data.depthstencil.height, initData.data.depthstencil.format, &resource);
			CreateDataDepthStencil(initData);
		break;

	default:
		break;
	}
	type = dataType;
	isInit = true;
	isMine = true;
}

void ResDx::ResDxBuffer::Init(ResDxContext2& context, const ResDxBufferInitData initData, ID3D12Resource* resource)
{
	if (IsInit())
		Clear();

	auto dataType = initData.type;
	this->resource = resource;

	switch (dataType)
	{
	case ResDxBufferType_Vertex:
		CreateDataVertex(initData);
		break;

	case ResDxBufferType_Index:
		CreateDataIndex(initData);
		break;

	case ResDxBufferType_Texture:
		context.CreateTextureBuffer(initData.data.texture.width, initData.data.texture.height, initData.data.texture.format, initData.data.texture.flags, D3D12_RESOURCE_STATE_COPY_DEST, &resource);
		CreateDataTexture(initData);
		break;

	case ResDxBufferType_Constant:
		context.CreateConstantBuffer(initData.data.constant.bufferSize, &resource);
		CreateDataConstant(initData);
		break;

	case ResDxBufferType_Upload:
	{
		UINT64 uploadSize = {};
		UINT64 totalByte = {};
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		footprint = context.GetPlacedFootprint(resource, uploadSize, totalByte);
		CreateDataUpload(initData, uploadSize, footprint);
		break;
	}
	case ResDxBufferType_Structured:
		CreateDataStructured(initData);
		break;

	case ResDxBufferType_RWStructured:
	{
		CreateDataRWStructured(initData, nullptr);
		break;
	}
	case ResDxBufferType_RenderTarget:
		CreateDataRenderTarget(initData);
		break;
	case ResDxBufferType_DepthStencil:
		CreateDataDepthStencil(initData);
		break;

	default:
		break;
	}
	type = dataType;
	isInit = true;
	isMine = false;
}

void ResDx::ResDxBuffer::CopyCpuResource(const void* src, int offsetByteSize, int srcByteSize)
{
	if (!isInit)
	{
		MessageBoxA(nullptr, "リソースが初期化されていません", "error", MB_OK);
		return;
	}
	switch (type)
	{
	case ResDxBufferType_Vertex:
	{
		auto vertexData = static_cast<ResDxBufferVertex*>(data);
		srcByteSize = srcByteSize == 0 ? vertexData->bufferSize : srcByteSize;
		memcpy_s(vertexData->map, vertexData->bufferSize + 1, (char*)src+offsetByteSize, srcByteSize);
		break;
	}
	case ResDxBufferType_Index:
	{
		auto indexData = static_cast<ResDxBufferIndex*>(data);
		srcByteSize = srcByteSize == 0 ? indexData->bufferSize : srcByteSize;
		memcpy_s(indexData->map, indexData->bufferSize + 1, (char*)src + offsetByteSize, srcByteSize);
		break;
	}
	case ResDxBufferType_Texture:
	{
		assert(false);
		break;
	}
	case ResDxBufferType_Constant:
	{
		auto constantData = static_cast<ResDxBufferConstant*>(data);
		srcByteSize = srcByteSize == 0 ? constantData->size: srcByteSize;
		memcpy_s(constantData->map, constantData->size + 1, (char*)src + offsetByteSize, srcByteSize);
		break;
	}
	case ResDxBufferType_Upload:
	{
		auto uploadData = static_cast<ResDxBufferUpload*>(data);
		const uint8_t* srcAddress = static_cast<const uint8_t*>(src);
		uint8_t* map = static_cast<uint8_t*>(uploadData->map);
		UINT rowPitch = uploadData->footprint.Footprint.RowPitch;

		for (int i = 0; i < uploadData->height; i++)
		{
			memcpy(map, srcAddress, rowPitch);

			srcAddress += uploadData->uploadSize;
			map += rowPitch;
		}
		break;
	}
	case ResDxBufferType_Structured:
	{
		MessageBoxA(nullptr, "cpuからのコピーが出来ないリソースです(構造体リソース)", "error", MB_OK);
		break;
	}
	case ResDxBufferType_RWStructured:
	{
		auto rwstructuredData = static_cast<ResDxBufferRWStructured*>(data);
		void* ptr = (void*)((uintptr_t)rwstructuredData->map + (uintptr_t)offsetByteSize);
		memcpy(ptr, src, srcByteSize);
		break;
	}
	default:
		MessageBoxA(nullptr, "無効なリソース", "error", MB_OK);
		break;
	}
}

void ResDx::ResDxBuffer::Clear()
{
	if (!isInit && !isMine)
		return;
	if (type == ResDxBufferType_RWStructured)
	{
		auto counter= Data<ResDxBufferType_RWStructured>()->counter;
		if (counter)
			counter->Release();
	}
	free(data);
	resource->Release();
	data = nullptr;
	resource = nullptr;

	dataSize = 0;
	type = ResDxBufferType_None;
	isInit = false;
}

bool ResDx::ResDxBuffer::IsInit() const
{
	return isInit == true;
}

ID3D12Resource* ResDx::ResDxBuffer::Resource() const
{
	return resource;
}

ResDx::ResDxBufferType ResDx::ResDxBuffer::Type() const
{
	return type;
}

void ResDx::ResDxBuffer::operator=(ResDxBuffer&& src)
{
	data = src.data;
	dataSize = src.dataSize;
	resource = src.resource;
	type = src.type;
	isInit = src.isInit;
	
	src.data = nullptr;
	src.dataSize = 0;
	src.resource = nullptr;
	src.type = ResDxBufferType_None;
	src.isInit = false;
}

ResDx::ResDxBuffer::ResDxBuffer(ResDxBuffer&&src)
{
	*this = std::move(src);
}

ResDx::ResDxBuffer::~ResDxBuffer()
{
	Clear();
}

void ResDx::ResDxBuffer::CreateDataVertex(const ResDxBufferInitData initData)
{
	dataSize = sizeof(ResDxBufferVertex);
	data = calloc(1, dataSize);
	const auto vertex = initData.data.vertex;
	auto vdata = static_cast<ResDxBufferVertex*>(data);
	if (!vdata)
		return;
	vdata->bufferSize = vertex.bufferSize;
	vdata->stride = vertex.stride;
	vdata->view.BufferLocation = resource->GetGPUVirtualAddress();
	vdata->view.SizeInBytes = vertex.bufferSize;
	vdata->view.StrideInBytes = vertex.stride;
	vdata->numVertex = vertex.bufferSize / vertex.stride;
	resource->Map(0, nullptr, &vdata->map);
}

void ResDx::ResDxBuffer::CreateDataIndex(const ResDxBufferInitData initData)
{
	dataSize = sizeof(ResDxBufferIndex);
	data = calloc(1, dataSize);
	const auto index = initData.data.index;
	auto idata = static_cast<ResDxBufferIndex*>(data);
	if (!idata)
		return;
	idata->bufferSize = index.bufferSize;
	idata->stride = index.stride;
	idata->view.BufferLocation = resource->GetGPUVirtualAddress();
	idata->view.Format = DXGI_FORMAT_R32_UINT;
	idata->view.SizeInBytes = index.bufferSize;
	idata->numIndex = index.bufferSize / index.stride;
	resource->Map(0, nullptr, &idata->map);
}

void ResDx::ResDxBuffer::CreateDataTexture(const ResDxBufferInitData initData)
{
	dataSize = sizeof(ResDxBufferTexture);
	data = calloc(1, dataSize);
	const auto texture = initData.data.texture;
	auto tdata = static_cast<ResDxBufferTexture*>(data);
	if (!tdata)
		return;
	tdata->width = texture.width;
	tdata->height = texture.height;
	tdata->format = texture.format;
}

void ResDx::ResDxBuffer::CreateDataConstant( const ResDxBufferInitData initData)
{
	dataSize = sizeof(ResDxBufferConstant);
	data = calloc(1, dataSize);
	auto cdata = static_cast<ResDxBufferConstant*>(data);
	auto constant = initData.data.constant;
	if (!cdata)
		return;
	auto result = resource->Map(0, nullptr, &cdata->map);
	cdata->size = Alignmentof(constant.bufferSize);
	if (FAILED(result))
		MessageBoxA(nullptr, "定数バッファのマップに失敗しました", "error", MB_OK);
}

void ResDx::ResDxBuffer::CreateDataUpload(const ResDxBufferInitData initData,UINT uploadSize,D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint)
{
	dataSize = sizeof(ResDxBufferUpload);
	data = calloc(1, dataSize);

	const auto upload= initData.data.upload;
	auto udata = static_cast<ResDxBufferUpload*>(data);
	if (!udata)
		return;
	udata->width = upload.width;
	udata->height = upload.height;
	udata->format = upload.format;
	udata->uploadSize = uploadSize;
	udata->footprint = footprint;
	auto result = resource->Map(0, nullptr, &udata->map);

	if (FAILED(result))
		MessageBoxA(nullptr, "アップロードバッファのマップに失敗しました", "error", MB_OK);
}

void ResDx::ResDxBuffer::CreateDataStructured(const ResDxBufferInitData initData)
{
	dataSize = sizeof(ResDxBufferStructured);
	data = calloc(1, dataSize);

	D3D12_RANGE range = {};
	range.Begin = 0;
	range.End = 0;
	const auto structured = initData.data.structured;
	auto sdata = static_cast<ResDxBufferStructured*>(data);
	if (!sdata)
		return;
	sdata->sizeOfElem = structured.sizeOfElem;
	sdata->numOfElem = structured.numOfElem;
	resource->Map(0, &range, &sdata->map);
}

void ResDx::ResDxBuffer::CreateDataRWStructured(const ResDxBufferInitData initData, ID3D12Resource* counter)
{
	dataSize = sizeof(ResDxBufferRWStructured);
	data = calloc(1, dataSize);

	const auto rwstructured = initData.data.rwstructured;
	auto rwdata = static_cast<ResDxBufferRWStructured*>(data);
	CD3DX12_RANGE range(0, 0);
	if (!rwdata)
		return;
	resource->Map(0, nullptr, &rwdata->map);

	rwdata->sizeOfElem = rwstructured.sizeOfElem;
	rwdata->numOfElem = rwstructured.numOfElem;
	rwdata->counter = counter;
}

void ResDx::ResDxBuffer::CreateDataRenderTarget(const ResDxBufferInitData initData)
{
	dataSize = sizeof(ResDxBufferRenderTarget);
	data = calloc(1, dataSize);

	const auto rendertarget = initData.data.rendertarget;
	auto rdata = static_cast<ResDxBufferRenderTarget*>(data);
	if (!rdata)
		return;
	rdata->width = rendertarget.width;
	rdata->height = rendertarget.height;
	rdata->format = rendertarget.format;
}

void ResDx::ResDxBuffer::CreateDataDepthStencil(const ResDxBufferInitData initData)
{
	dataSize = sizeof(ResDxBufferDepthStencil);
	data = calloc(1, dataSize);

	const auto depthstencil = initData.data.depthstencil;
	auto ddata = static_cast<ResDxBufferDepthStencil*>(data);
	if (!ddata)
		return;
	ddata->width = depthstencil.width;
	ddata->height = depthstencil.height;
	ddata->format = depthstencil.format;
}

void ResDx::ResDxContext::CreateVertexBuffer(int bufferSize, int stride, ResDxBufferVertex& data, ID3D12Resource** buffer)
{
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	auto prop = CD3DX12_HEAP_DESC(bufferSize, D3D12_HEAP_TYPE_UPLOAD);
	auto result = device->CreateCommittedResource(
		&prop.Properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	if (FAILED(result))
		MessageBoxA(nullptr, "頂点バッファの生成に失敗しました", "error", MB_OK);

	data.bufferSize = bufferSize;
	data.stride = stride;
	data.view.BufferLocation = (*buffer)->GetGPUVirtualAddress();
	data.view.SizeInBytes = bufferSize;
	data.view.StrideInBytes = stride;
	data.numVertex = bufferSize / stride;

	(*buffer)->Map(0, nullptr, &data.map);
}

void ResDx::ResDxContext::CreateIndexBuffer(int bufferSize, int stride, ResDxBufferIndex& data, ID3D12Resource** buffer)
{
	if (stride == 2)
		bufferSize *= 2;
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	auto result = device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer));

	if (FAILED(result))
		MessageBoxA(nullptr, "インデックスバッファの生成に失敗しました", "error", MB_OK);

	data.bufferSize = bufferSize;
	data.stride = stride;
	data.view.BufferLocation = (*buffer)->GetGPUVirtualAddress();
	data.view.Format = DXGI_FORMAT_R32_UINT;
	data.view.SizeInBytes = bufferSize;
	data.numIndex = bufferSize / stride;

	(*buffer)->Map(0, nullptr, (void**)&data.map);
}

void ResDx::ResDxContext::CreateTextureBuffer(int width, int height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, ResDxBufferTexture& data, ID3D12Resource** buffer)
{
	if (*buffer)
		(*buffer)->Release();
	*buffer = nullptr;
	auto resDesc = CD3DX12_RESOURCE_DESC(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		flags);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	auto result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	if (FAILED(result))
	{
		MessageBoxA(nullptr, "テクスチャバッファの生成に失敗しました", "error", MB_OK);
		return;
	}

	data.width = width;
	data.height = height;
	data.format = format;
}

void ResDx::ResDxContext::CreateConstantBuffer(int bufferSize, ResDxBufferConstant& data, ID3D12Resource** buffer)
{
	auto alignedBbufferSize = Alignmentof(bufferSize);
	auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC cbDesc;
	cbDesc.Format = DXGI_FORMAT_UNKNOWN;
	cbDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedBbufferSize);
	data.size = alignedBbufferSize;
	device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	auto result = (*buffer)->Map(0, nullptr, &data.map);
	if (FAILED(result))
		MessageBoxA(nullptr, "定数バッファのマップに失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext::CreateUploadBuffer(int width, int height, DXGI_FORMAT format, ResDxBufferUpload& data, ID3D12Resource** buffer)
{
	CD3DX12_RESOURCE_DESC texBufDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_NONE
	);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	unsigned int rowSizeBytes = 0;
	UINT64 uploadSize = 0;
	UINT64 totalBytes = 0;
	device->GetCopyableFootprints(&texBufDesc, 0, 1, 0, &footprint, &rowSizeBytes, &uploadSize, &totalBytes);

	auto texBufRowPitch = footprint.Footprint.RowPitch;
	auto upResDesc = CD3DX12_RESOURCE_DESC::Buffer(texBufRowPitch * texBufDesc.Height);
	auto upHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto result = device->CreateCommittedResource(
		&upHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&upResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);
	data.width = width;
	data.height = height;
	data.format = format;
	data.uploadSize = uploadSize;
	data.footprint = footprint;
	result = (*buffer)->Map(0, nullptr, &data.map);

	if (FAILED(result))
		MessageBoxA(nullptr, "アップロードバッファのマップに失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext::CreateStructuredBuffer(int sizeOfElem, int numOfElem, ResDxBufferStructured& data, ID3D12Resource** buffer)
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer((UINT64)(sizeOfElem * numOfElem));

	auto dev = graphics.GetDevice();
	auto hr = dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);
	D3D12_RANGE range = {};
	range.Begin = 0;
	range.End = 0;
	hr = (*buffer)->Map(0, &range, (void**)&data.map);

	data.sizeOfElem = sizeOfElem;
	data.numOfElem = numOfElem;
}

void ResDx::ResDxContext::CreateRWStructuredBuffer(int sizeOfElem, int numOfElem, ResDxBufferRWStructured& data, ID3D12Resource** buffer)
{
	auto device = graphics.GetDevice();
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeOfElem * numOfElem);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	int bufferNo = 0;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProp.CreationNodeMask = 1;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProp.VisibleNodeMask = 1;

	//リソース作成
	auto result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	//カウンターリソース作成
	result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&data.counter)
	);

	if (FAILED(result))
		MessageBoxA(nullptr, "アンオーダーアクセスバッファの生成に失敗しました", "error", MB_OK);

	CD3DX12_RANGE range(0, 0);
	(*buffer)->Map(0, nullptr, &data.map);

	data.sizeOfElem = sizeOfElem;
	data.numOfElem = numOfElem;
}

void ResDx::ResDxContext::CopyGpuResource(ID3D12Resource* dstResource, ID3D12Resource* srcResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint)
{
	auto footprint = srcFootprint;
	auto cmdList = commandList;
	D3D12_TEXTURE_COPY_LOCATION copyLoc = {};//コピー元
	copyLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	copyLoc.PlacedFootprint = footprint;
	copyLoc.PlacedFootprint.Offset = 0;
	copyLoc.pResource = srcResource;

	D3D12_TEXTURE_COPY_LOCATION targetLoc = {};//コピー先
	targetLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	targetLoc.SubresourceIndex = 0;
	targetLoc.pResource = dstResource;

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		dstResource,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->CopyTextureRegion(&targetLoc, 0, 0, 0, &copyLoc, nullptr);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		dstResource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	cmdList->ResourceBarrier(1, &barrier);

}

void ResDx::ResDxContext::CopyGpuResourceFromTempUploadBuffer(ID3D12Resource* dstResource, ResDxBuffer&& tempUploadBuffer, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint)
{
	assert(tempBufferCount < RES_DX_MAX_NUM_TEMP_BUFFER);
	size_t backBuffer = GetBackBufferIndex();
	auto& buffer = temp[backBuffer][tempBufferCount];
	buffer = std::move(tempUploadBuffer);
	CopyGpuResource(dstResource, buffer.Resource(), srcFootprint);
	++tempBufferCount;
}

void ResDx::ResDxContext::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC& desc, ID3D12DescriptorHeap** descHeap)
{
	auto result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(descHeap));
	if (FAILED(result))
		MessageBoxA(nullptr, "ディスクリプタヒープの生成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateConstantBufferView(&desc, handle);
}

void ResDx::ResDxContext::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateShaderResourceView(resource, &desc, handle);
}

void ResDx::ResDxContext::CreateUnorderedAccessView(ID3D12Resource* resource, ID3D12Resource* counter, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateUnorderedAccessView(resource, counter, &desc, handle);
}

void ResDx::ResDxContext::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateRenderTargetView(resource, desc, handle);
}

void ResDx::ResDxContext::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateDepthStencilView(resource, desc, handle);
}

void ResDx::ResDxContext::CreateRootSignature(UINT nodeMask, const void* blobWithRootSignature, UINT blobLenghInBytes, ID3D12RootSignature** rootSignature)
{
	auto result = device->CreateRootSignature(nodeMask, blobWithRootSignature, blobLenghInBytes, IID_PPV_ARGS(rootSignature));
	if (FAILED(result))
		MessageBoxA(nullptr, "ルートシグネチャの作成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext::CreateGraphicsPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState)
{
	auto result = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineState));
	if (FAILED(result))
		MessageBoxA(nullptr, "パイプラインステートの作成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext::CreateComputePipelineState(D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState)
{
	auto result = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(pipelineState));
	if (FAILED(result))
		MessageBoxA(nullptr, "パイプラインステートの作成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext::ReleaseResource(ID3D12Resource* resource)
{
	resource->Release();
}

void ResDx::ResDxContext::ReleaseDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
{
	descriptorHeap->Release();
}

void ResDx::ResDxContext::ReleaseRootSignature(ID3D12RootSignature* rootSignature)
{
	rootSignature->Release();
}

void ResDx::ResDxContext::ReleasePipelineState(ID3D12PipelineState* pipelineState)
{
	pipelineState->Release();
}

void ResDx::ResDxContext::ReleaseBlob(ID3DBlob* blob)
{
	blob->Release();
}

D3D12_PLACED_SUBRESOURCE_FOOTPRINT ResDx::ResDxContext::GetPlacedFootprint(ID3D12Resource* resource)
{
	auto desc = resource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr);
	return footprint;
}

int ResDx::ResDxContext::GetFrameRenderTargetWidth()
{
	return graphics.GetFrameBufferWidth();
}

int ResDx::ResDxContext::GetFrameRenderTargetHeight()
{
	return graphics.GetFrameBufferHight();
}

DXGI_FORMAT ResDx::ResDxContext::GetFrameRTVFormat()
{
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

DXGI_FORMAT ResDx::ResDxContext::GetFrameDSVFormat()
{
	return graphics.GetFrameDsvFormat();
}

UINT ResDx::ResDxContext::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	return device->GetDescriptorHandleIncrementSize(type);
}

float ResDx::ResDxContext::GetFrameAspect()
{
	return graphics.GetFrameBufferAspect();
}

UINT ResDx::ResDxContext::GetBackBufferIndex()
{
	return graphics.GetBackBufferIndex();
}

void ResDx::ResDxContext::SetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW* vertexBufferView)
{
	commandList->IASetVertexBuffers(0, 1, vertexBufferView);
}

void ResDx::ResDxContext::SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView)
{
	commandList->IASetIndexBuffer(indexBufferView);
}

void ResDx::ResDxContext::SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
{
	commandList->SetDescriptorHeaps(1, &descriptorHeap);
}

void ResDx::ResDxContext::SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps)
{
	commandList->SetDescriptorHeaps(numDescriptorHeap,descriptorHeaps);
}

void ResDx::ResDxContext::SetGraphicsDescriptorTable(UINT indexRootParameter, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle)
{
	commandList->SetGraphicsRootDescriptorTable(indexRootParameter, descriptorHandle);
}

void ResDx::ResDxContext::SetGraphicsRootSignature(ID3D12RootSignature* rootSignature)
{
	commandList->SetGraphicsRootSignature(rootSignature);
}

void ResDx::ResDxContext::SetComputeRootSignature(ID3D12RootSignature* rootSignature)
{
	commandList->SetComputeRootSignature(rootSignature);
}

void ResDx::ResDxContext::SetGraphicsRoot32BitConstant(UINT rootParameter, UINT src, UINT dstOffset)
{
	commandList->SetGraphicsRoot32BitConstant(rootParameter, src, dstOffset);
}

void ResDx::ResDxContext::SetGraphicsRoot32BitConstants(UINT rootParameter, UINT num32BitValue, const void* src, UINT dstOffset)
{
	commandList->SetGraphicsRoot32BitConstants(rootParameter, num32BitValue, src, dstOffset);
}

void ResDx::ResDxContext::SetPipelineState(ID3D12PipelineState* pipelineState)
{
	commandList->SetPipelineState(pipelineState);
}

void ResDx::ResDxContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV)
{
	commandList->OMSetRenderTargets(1, descriptorHandleRTV, FALSE, descriptorHandleDSV);
}

void ResDx::ResDxContext::SetRenderTargets(UINT numDescriptorRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV)
{
	commandList->OMSetRenderTargets(numDescriptorRTV, descriptorHandleRTV, FALSE, descriptorHandleDSV);
}

void ResDx::ResDxContext::SetScissorRect(D3D12_RECT* rect)
{
	commandList->RSSetScissorRects(1, rect);
}

void ResDx::ResDxContext::SetViewport(D3D12_VIEWPORT* viewport)
{
	commandList->RSSetViewports(1, viewport);
}

void ResDx::ResDxContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	commandList->IASetPrimitiveTopology(topology);
}

void ResDx::ResDxContext::SetFrameScissorRect()
{
	SetScissorRect(&graphics.GetScissorRect());
}

void ResDx::ResDxContext::SetFrameViewport()
{
	SetViewport(&graphics.GetViewport());
}

void ResDx::ResDxContext::SetFrameRenderTarget()
{
	auto rtvHandle = graphics.GetRtvFrameBuffer();
	auto dsvHandle = graphics.GetDsvFrameBuffer();
	SetRenderTarget(&rtvHandle, &dsvHandle);
}

void ResDx::ResDxContext::SetDevice(ID3D12Device* device, ID3D12GraphicsCommandList* command)
{
	this->device = device;
	commandList = command;
}

void ResDx::ResDxContext::ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
	commandList->ResourceBarrier(1, &barrier);
}

void ResDx::ResDxContext::ResourceBarriers(UINT numResource, ID3D12Resource** resources, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	for (auto i = 0u; i < numResource; i++)
		ResourceBarrier(resources[i], before, after);
}

void ResDx::ResDxContext::DrawInstanced(UINT numVertex)
{
	commandList->DrawInstanced(numVertex, 1, 0, 0);
}

void ResDx::ResDxContext::DrawIndexedInstanced(UINT numIndex)
{
	commandList->DrawIndexedInstanced(numIndex, 1, 0, 0, 0);
}

void ResDx::ResDxContext::DrawIndexedInstanced(UINT numIndex, UINT numInstance)
{
	commandList->DrawIndexedInstanced(numIndex, numInstance, 0, 0, 0);
}

void ResDx::ResDxContext::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
{
	commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void ResDx::ResDxContext::CopyDescriptorSimple(UINT numDescriptor, ResDxDescriptorHeap* dst, UINT dstStartIndex, ResDxDescriptorHeap* src, UINT srcStartIndex, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	assert(numDescriptor < RES_DX_MAX_NUM_COPY_DESCRIPTOR);
	const auto dstCpuHandle = dst->GetCpuHandle(dstStartIndex);
	const auto srcCpuHandle = src->GetCpuHandle(srcStartIndex);
	device->CopyDescriptorsSimple(numDescriptor, dstCpuHandle, srcCpuHandle, type);
}

void ResDx::ResDxContext::CopyDescriptors(const UINT numDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE dstHandles[], const D3D12_CPU_DESCRIPTOR_HANDLE srcHandles[], const UINT numDescriptors[],D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	device->CopyDescriptors(numDescriptorRanges, dstHandles, numDescriptors, numDescriptorRanges, srcHandles, numDescriptors, type);
}

void ResDx::ResDxContext::BeginRender()
{
	ClearTempBuffer(GetBackBufferIndex());
	graphics.BeginRender();
}

void ResDx::ResDxContext::EndRender()
{
	graphics.EndRender();
}

ResDx::ResDxContext& ResDx::ResDxContext::instance()
{
	static ResDxContext context;
	return context;
}

void ResDx::ResDxContext::ClearTempBuffer(size_t index)
{
	for (size_t i = 0; i < RES_DX_MAX_NUM_TEMP_BUFFER; ++i)
		temp[index][i].Clear();
}

ResDx::ResDxContext::ResDxContext() :device(graphics.GetDevice()), commandList(graphics.GetCmdList())
{
	assert(RES_DX_BUFFER_COUNT == BUFFER_COUNT);
}

void ResDx::ResDxDescriptorHeap::Init(ResDxContext2& context, int numDescriptor)
{
	Init(context, numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
}

void ResDx::ResDxDescriptorHeap::Init(ResDxContext2& context, int numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE type,D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	if (isInit)
		Clear();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.Flags = flags;
	desc.NumDescriptors = numDescriptor;
	context.CreateDescriptorHeap(desc, &descriptorHeap);

	isInit = true;
	descriptorSize = context.GetDescriptorSize(type);
	this->numDescriptor = numDescriptor;
}



void ResDx::ResDxDescriptorHeap::Clear()
{
	if (!isInit)
		return;
	descriptorHeap->Release();

	descriptorHeap = nullptr;
	isInit = false;
	numDescriptor = 0;
	descriptorSize = 0;
}

ResDx::ResDxDescriptorHeap::ResDxDescriptorHandle ResDx::ResDxDescriptorHeap::CreateShaderResourceView(ResDxContext2& context, int index, ID3D12Resource* resource, DXGI_FORMAT format)
{
	assert(index < numDescriptor);
	ResDxDescriptorHandle descHandle = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = format;
	cpuHandle.ptr += descriptorSize * index;

	context.CreateShaderResourceView(resource, desc,cpuHandle);
	descHandle.cpuHandle = cpuHandle;
	descHandle.index = index;
	descHandle.numDescriptor = 1;
	descHandle.descriptorSize = context.GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.type = ResDxViewType_ShaderResourceView;
	descHandle.resource = resource;
	return descHandle;
}

ResDx::ResDxDescriptorHeap::ResDxDescriptorHandle ResDx::ResDxDescriptorHeap::CreateConstantBufferView(ResDxContext2& context, int index, ID3D12Resource* resource)
{
	ResDxDescriptorHandle descHandle = {};
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	desc.BufferLocation=resource->GetGPUVirtualAddress();
	desc.SizeInBytes = resource->GetDesc().Width;

	cpuHandle.ptr += descriptorSize * index;
	context.CreateConstantBufferView(desc, cpuHandle);

	descHandle.cpuHandle = cpuHandle;
	descHandle.index = index;
	descHandle.descriptorSize = context.GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.numDescriptor = 1;
	descHandle.type = ResDxViewType_ConstantBufferView;
	descHandle.resource = resource;

	return descHandle;
}

ResDx::ResDxDescriptorHeap::ResDxDescriptorHandle ResDx::ResDxDescriptorHeap::CreateUnorderedBufferView(ResDxContext2& context, int index, ID3D12Resource*resource, ID3D12Resource* count, int numElements,int sizeOfElem)
{
	ResDxDescriptorHandle descHandle = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Buffer.NumElements = numElements;
	desc.Buffer.StructureByteStride = sizeOfElem;

	cpuHandle.ptr += descriptorSize * index;

	context.CreateUnorderedAccessView(resource, count, desc, cpuHandle);
	
	descHandle.cpuHandle = cpuHandle;
	descHandle.index = index;
	descHandle.numDescriptor = 1;
	descHandle.descriptorSize = context.GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descHandle.type = ResDxViewType_UnorderedResourceView;
	descHandle.resource = resource;
	return descHandle;
}

ResDx::ResDxDescriptorHeap::ResDxDescriptorHandle ResDx::ResDxDescriptorHeap::CreateRenderTargetView(ResDxContext2& context, int index, ID3D12Resource* resource)
{
	ResDxDescriptorHandle descHandle = {};	
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	cpuHandle.ptr += descriptorSize * index;

	context.CreateRenderTargetView(resource, nullptr, cpuHandle);
	
	descHandle.cpuHandle = cpuHandle;
	descHandle.index = index;
	descHandle.numDescriptor = 1;
	descHandle.descriptorSize = context.GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descHandle.type = ResDxViewType_RenderTargetView;
	descHandle.resource = resource;
	return descHandle;
}

ResDx::ResDxDescriptorHeap::ResDxDescriptorHandle ResDx::ResDxDescriptorHeap::CreateDepthStencilView(ResDxContext2& context, int index, ID3D12Resource* resource)
{
	ResDxDescriptorHandle descHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	cpuHandle.ptr += descriptorSize * index;

	context.CreateDepthStencilView(resource, nullptr, cpuHandle);

	descHandle.cpuHandle = cpuHandle;
	descHandle.index = index;
	descHandle.numDescriptor = 1;
	descHandle.descriptorSize = context.GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	descHandle.type = ResDxViewType_DepthStencilView;
	descHandle.resource = resource;
	return descHandle;
}



D3D12_CPU_DESCRIPTOR_HANDLE ResDx::ResDxDescriptorHeap::GetCpuHandle(int index)
{
	auto handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += index * descriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE ResDx::ResDxDescriptorHeap::GetGpuHandle(int index)
{
	auto handle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += index * descriptorSize;
	return handle;
}

ID3D12DescriptorHeap* ResDx::ResDxDescriptorHeap::GetDescriptorHeap()const
{
	return descriptorHeap;
}


void ResDx::ResDxDescriptorHeap::operator=(ResDxDescriptorHeap&& src)
{
	descriptorHeap = src.descriptorHeap;
	numDescriptor = src.numDescriptor;
	descriptorSize = src.descriptorSize;
	isInit = src.isInit;

	src.descriptorHeap = nullptr;
	src.isInit = false;
	src.numDescriptor = 0;
	src.descriptorSize = 0;
}

ResDx::ResDxDescriptorHeap::ResDxDescriptorHeap():
descriptorHeap(),
numDescriptor(),
isInit()
{}

ResDx::ResDxDescriptorHeap::ResDxDescriptorHeap(ResDxDescriptorHeap&& src)
{
	*this = std::move(src);
}

ResDx::ResDxDescriptorHeap::~ResDxDescriptorHeap()
{
	Clear();
}

void ResDx::ResDxSampler::Init(int numSampler, D3D12_STATIC_SAMPLER_DESC* desc)
{
	samplerDesc = new D3D12_STATIC_SAMPLER_DESC[numSampler]();
	for (int i = 0; i < numSampler; ++i)
		samplerDesc[i] = desc[i];

	this->numSampler = numSampler;
	isInit = true;
}

void ResDx::ResDxSampler::Init(int numSampler)
{
	samplerDesc = new D3D12_STATIC_SAMPLER_DESC[numSampler]();

	this->numSampler = numSampler;
	isInit = true;

	for (int i = 0; i < numSampler; ++i)
		SetSamplerDesc(
			i,
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP
		);
}

void ResDx::ResDxSampler::Init(D3D12_FILTER samplerFilter, D3D12_TEXTURE_ADDRESS_MODE addressU, D3D12_TEXTURE_ADDRESS_MODE addressV, D3D12_TEXTURE_ADDRESS_MODE addressW)
{
	samplerDesc = new D3D12_STATIC_SAMPLER_DESC[1];

	numSampler = 1;
	isInit = true;

	SetSamplerDesc(
		0,
		samplerFilter,
		addressU,
		addressV,
		addressW
	);
}

void ResDx::ResDxSampler::Init()
{
	samplerDesc = nullptr;
	numSampler = 0;
	isInit = true;
}

void ResDx::ResDxSampler::SetSamplerDesc(int index, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addressU, D3D12_TEXTURE_ADDRESS_MODE addressV, D3D12_TEXTURE_ADDRESS_MODE addressW, UINT shaderRegister, UINT registerSpace, D3D12_COMPARISON_FUNC comparisonFunc, D3D12_STATIC_BORDER_COLOR borderColor, FLOAT minLOD, FLOAT maxLOD, D3D12_SHADER_VISIBILITY shaderVisiblity)
{
	if (!isInit)
		MessageBoxA(nullptr, "サンプラーが初期化されていません", "error", MB_OK);

	assert(index < numSampler);

	UINT registerNum = shaderRegister;
	if (registerNum == (UINT)-1)
		registerNum = index;

	auto* sampler = &samplerDesc[index];
	sampler->Filter = filter;
	sampler->AddressU = addressU;
	sampler->AddressV = addressV;
	sampler->AddressW = addressW;
	sampler->ShaderRegister = registerNum;
	sampler->RegisterSpace = registerSpace;
	sampler->ComparisonFunc = comparisonFunc;
	sampler->BorderColor = borderColor;
	sampler->MinLOD = minLOD;
	sampler->MaxLOD = maxLOD;
	sampler->ShaderVisibility = shaderVisiblity;
}

int ResDx::ResDxSampler::GetNumSampler()
{
	return numSampler;
}

D3D12_STATIC_SAMPLER_DESC* ResDx::ResDxSampler::GetSampler()
{
	return samplerDesc;
}

bool ResDx::ResDxSampler::IsInit()
{
	return isInit;
}

void ResDx::ResDxSampler::Clear()
{
	if (!isInit)
		return;
	if (samplerDesc)
		delete[] samplerDesc;

	samplerDesc = nullptr;
	numSampler = 0;
	isInit = false;
}

void ResDx::ResDxSampler::operator=(ResDxSampler&& src)
{
	samplerDesc = src.samplerDesc;
	numSampler = src.numSampler;
	isInit = src.isInit;
}

ResDx::ResDxSampler::ResDxSampler(ResDxSampler&& src)
{
	*this = std::move(src);
}

ResDx::ResDxSampler::~ResDxSampler()
{
	Clear();
}

void ResDx::ResDxRootSignature::Init()
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumStaticSamplers = 0;
	desc.NumParameters = 0;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	desc.pParameters = nullptr;
	desc.pStaticSamplers = nullptr;
	this->desc = desc;
	sampler.Init();
	rootSignature = nullptr;
}

void ResDx::ResDxRootSignature::Init(int numRootParameter)
{
	rootParameter = new D3D12_ROOT_PARAMETER[numRootParameter]();
	sampler.Init(1);

	this->numRootParameter = numRootParameter;
	desc.NumStaticSamplers = 1;
	desc.NumParameters = numRootParameter;
	desc.pParameters = rootParameter;
	desc.pStaticSamplers = sampler.GetSampler();
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignature = nullptr;
}

void ResDx::ResDxRootSignature::Init(ResDxSampler& sampler, int numRootParameter)
{
	auto smp = &this->sampler;
	smp->Init(sampler.GetNumSampler(), sampler.GetSampler());

	rootParameter = new D3D12_ROOT_PARAMETER[numRootParameter]();
	this->numRootParameter = numRootParameter;
	desc.NumStaticSamplers = smp->GetNumSampler();
	desc.NumParameters = numRootParameter;
	desc.pParameters = rootParameter;
	desc.pStaticSamplers = smp->GetSampler();
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignature = nullptr;
}

void ResDx::ResDxRootSignature::SetUsecaseConstant(int index, uint32_t num, int shaderRegister, int registerSpace)
{
	assert(index < numRootParameter);

	auto* param = &rootParameter[index];

	param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param->Constants.Num32BitValues = num;
	param->Constants.ShaderRegister = shaderRegister;
	param->Constants.RegisterSpace = registerSpace;
	param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

void ResDx::ResDxRootSignature::SetUsecaseDescriptor(int index, D3D12_ROOT_PARAMETER_TYPE descriptorType, int shaderRegister, int registerSpace)
{
	assert(index < numRootParameter);

	if (!D3D12_ROOT_PARAMETER_TYPE_CBV && !D3D12_ROOT_PARAMETER_TYPE_SRV && !D3D12_ROOT_PARAMETER_TYPE_UAV)
	{
		MessageBoxA(nullptr, "無効なパラメータータイプです", "error", MB_OK);
		return;
	}
	auto param = &rootParameter[index];
	param->ParameterType = descriptorType;
	param->Descriptor.ShaderRegister = shaderRegister;
	param->Descriptor.RegisterSpace = registerSpace;
	param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

void ResDx::ResDxRootSignature::SetUsecaseDescriptorTable(int index, D3D12_DESCRIPTOR_RANGE* descriptorRange, int numDescriptorRange, int shaderRegister, int registerSpace)
{
	assert(index < numRootParameter);

	auto param = &rootParameter[index];
	param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param->DescriptorTable.pDescriptorRanges = descriptorRange;
	param->DescriptorTable.NumDescriptorRanges = numDescriptorRange;
	param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

ResDx::ResDxRootSignature::RootDescriptorTableHandle ResDx::ResDxRootSignature::SetUsecaseDescriptorTable(int index, ResDxDescriptorRange::ResDxDescriptorRangeHandle descriptorRangeHandle, int shaderRegister, int registerSpace)
{
	assert(index < numRootParameter);

	auto param = &rootParameter[index];
	param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param->DescriptorTable.pDescriptorRanges = descriptorRangeHandle.descriptorRange;
	param->DescriptorTable.NumDescriptorRanges = 1;
	param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	RootDescriptorTableHandle handle;
	handle.gpuHandle = descriptorRangeHandle.descriptorHandle;
	handle.rootParameterIndex = index;

	return handle;
}

ID3D12RootSignature* ResDx::ResDxRootSignature::GetRootSignature()
{
	return rootSignature;
}

bool ResDx::ResDxRootSignature::IsInit()
{
	return isInit == true;
}

void ResDx::ResDxRootSignature::operator=(ResDxRootSignature&& src)
{
	sampler = std::move(src.sampler);
	desc = std::move(src.desc);
	rootSignature = src.rootSignature;
	rootParameter = src.rootParameter;
	numRootParameter = src.numRootParameter;
	isInit = src.isInit;

	src.rootSignature = nullptr;
	src.rootParameter = nullptr;
	src.numRootParameter = 0;
	src.isInit = false;
}

ResDx::ResDxRootSignature::ResDxRootSignature():
sampler(),
rootSignature(),
desc(),
rootParameter(),
numRootParameter(),
isInit()
{
}

ResDx::ResDxRootSignature::ResDxRootSignature(ResDxRootSignature&& src)
{
	sampler = std::move(src.sampler);
	desc = std::move(src.desc);
	rootSignature = src.rootSignature;
	rootParameter = src.rootParameter;
	numRootParameter = src.numRootParameter;
	isInit = src.isInit;

	src.rootSignature = nullptr;
	src.rootParameter = nullptr;
	src.numRootParameter = 0;
	src.isInit = false;
}

ResDx::ResDxRootSignature::~ResDxRootSignature()
{
	Clear();
}

void ResDx::ResDxRootSignature::Create(ResDxContext2& context)
{
	if (rootSignature)
		rootSignature->Release();

	ID3DBlob* blob = nullptr;
	ID3DBlob* error = nullptr;
	auto result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &error);

	if (error)
	{
		MessageBoxA(nullptr, "ルートシグネチャのシリアライズに失敗しました", "error", MB_OK);
		return;
	}

	context.CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), &rootSignature);
}

void ResDx::ResDxRootSignature::Clear()
{
	if (!IsInit())
		return;

	sampler.Clear();
	rootSignature->Release();
	
	if (rootParameter)
		delete[] rootParameter;

	rootSignature = nullptr;
	rootParameter = nullptr;
	numRootParameter = 0;
	desc = D3D12_ROOT_SIGNATURE_DESC();
	isInit = false;
}

void ResDx::ResDxDescriptorRange::Init(int numDescriptorRange)
{
	descriptorRange = new D3D12_DESCRIPTOR_RANGE[numDescriptorRange]();
	this->numDescriptorRange = numDescriptorRange;
	isInit = true;
}

void ResDx::ResDxDescriptorRange::Clear()
{
	if (!IsInit())
		return;
	delete[] descriptorRange;

	descriptorRange = nullptr;
	numDescriptorRange = 0;
	isInit = false;
}

void ResDx::ResDxDescriptorRange::SetDescriptorRange(int index, D3D12_DESCRIPTOR_RANGE_TYPE type, int numDescriptor, int baseRegister, int registerSpace)
{
	assert(index < numDescriptorRange);
	
	auto range = &descriptorRange[index];

	range->RangeType = type;
	range->NumDescriptors = numDescriptor;
	range->BaseShaderRegister = baseRegister;
	range->RegisterSpace = registerSpace;
	range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

void ResDx::ResDxDescriptorRange::SetDescriptorRange(int index, ResDxViewType type, int numDescriptor, int baseRegister, int registerSpace)
{
	assert(index < numDescriptorRange);

	auto range = &descriptorRange[index];
	auto resourceType = type;

	switch (resourceType)
	{
	case ResDxViewType_ShaderResourceView:
		range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		break;

	case ResDxViewType_ConstantBufferView:
		range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		break;

	case ResDxViewType_UnorderedResourceView:
		range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		break;

	default:
		assert(0);
		break;
	}

	range->BaseShaderRegister = baseRegister;
	range->NumDescriptors = numDescriptor;
	range->RegisterSpace = registerSpace;
	range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

ResDx::ResDxDescriptorRange::ResDxDescriptorRangeHandle ResDx::ResDxDescriptorRange::SetDescriptorRange(int index, ResDxDescriptorHeap::ResDxDescriptorHandle descriptorHandle, int baseRegister, int registerSpace)
{
	assert(index < numDescriptorRange);
	SetDescriptorRange(index, descriptorHandle.type,descriptorHandle.numDescriptor, baseRegister, registerSpace);
	ResDxDescriptorRangeHandle handle;
	handle.descriptorRange = &descriptorRange[index];

	return handle;
}

D3D12_DESCRIPTOR_RANGE* ResDx::ResDxDescriptorRange::GetDescriptorRange(int index)
{
	return &descriptorRange[index];
}

bool ResDx::ResDxDescriptorRange::IsInit()
{
	return isInit==true;
}

void ResDx::ResDxDescriptorRange::operator=(ResDxDescriptorRange&& src)
{
	descriptorRange = src.descriptorRange;
	numDescriptorRange = src.numDescriptorRange;
	isInit = src.isInit;

	src.descriptorRange = nullptr;
	src.numDescriptorRange = 0;
	src.isInit = false;
}

ResDx::ResDxDescriptorRange::ResDxDescriptorRange(ResDxDescriptorRange&& src)
{
	*this = std::move(src);
}

ResDx::ResDxDescriptorRange::~ResDxDescriptorRange()
{
	Clear();
}

void ResDx::ResDxPipelineState::Init(ResDxContext2& context, D3D12_GRAPHICS_PIPELINE_STATE_DESC desc)
{
	context.CreateGraphicsPipelineState(desc, &pipeline);
}

void ResDx::ResDxPipelineState::Init(ResDxContext2& context, D3D12_COMPUTE_PIPELINE_STATE_DESC desc)
{
	context.CreateComputePipelineState(desc, &pipeline);
}

void ResDx::ResDxPipelineState::Init(
	ResDxContext2& context,
	ID3D12RootSignature* rootSignature,
	UINT numLayout,
	D3D12_INPUT_ELEMENT_DESC* elementDesc,
	ResDxShader& vs,
	ResDxShader& ps, 
	D3D12_BLEND_DESC blend,
	D3D12_DEPTH_STENCIL_DESC depthStencil, 
	D3D12_RASTERIZER_DESC rasterizer, 
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
	UINT numRenderTarget, 
	DXGI_FORMAT RTVFormats[],
	DXGI_FORMAT DSVFormat)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	auto VS = vs.GetCompileBlob();
	auto PS = ps.GetCompileBlob();

	desc.pRootSignature = rootSignature;
	desc.InputLayout.pInputElementDescs = elementDesc;
	desc.InputLayout.NumElements = numLayout;
	
	desc.VS.pShaderBytecode = VS->GetBufferPointer();
	desc.VS.BytecodeLength = VS->GetBufferSize();
	desc.PS.pShaderBytecode = PS->GetBufferPointer();
	desc.PS.BytecodeLength = PS->GetBufferSize();

	desc.DepthStencilState = depthStencil;
	desc.RasterizerState = rasterizer;
	desc.PrimitiveTopologyType = topology;
	desc.NumRenderTargets = numRenderTarget;
	
	for (auto i = 0u; i < numRenderTarget; ++i)
		desc.RTVFormats[i] = RTVFormats[i];
	desc.DSVFormat = DSVFormat;

	desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	desc.BlendState.RenderTarget[0].BlendEnable = false;
	desc.BlendState.RenderTarget[0].LogicOpEnable = false;
	desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	context.CreateGraphicsPipelineState(desc, &pipeline);
	isInit = true;
}

void ResDx::ResDxPipelineState::Init(
	ResDxContext2& context,
	ID3D12RootSignature* rootSignature,
	UINT numLayout, 
	D3D12_INPUT_ELEMENT_DESC* elementDesc, 
	ResDxShader& vs, 
	ResDxShader& ps,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
	UINT							numRenderTarget,
	DXGI_FORMAT						RTVFormats[],
	DXGI_FORMAT						DSVFormat
)
{
	D3D12_DEPTH_STENCIL_DESC depthStencil = {};
	auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	auto blend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	
	depthStencil.DepthEnable = TRUE;
	depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	rasterizer.CullMode = D3D12_CULL_MODE_NONE;
	
	Init(
		context,
		rootSignature, 
		numLayout, 
		elementDesc, 
		vs, 
		ps, 
		blend, 
		depthStencil, 
		rasterizer, 
		topology, 
		numRenderTarget,
		RTVFormats,
		DSVFormat
	);
}

void ResDx::ResDxPipelineState::Init(
	ResDxContext2& context,
	ID3D12RootSignature* rootSignature,
	ResDxInputElementFlags inputElem, 
	ResDxShader& vs, 
	ResDxShader& ps, 
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology,
	UINT							numRenderTarget,
	DXGI_FORMAT						RTVFormats[],
	DXGI_FORMAT						DSVFormat
)
{
	D3D12_INPUT_ELEMENT_DESC elementDescs[32] = {};
	int count = 0;

	if ((inputElem & ResDxInputElementFlags_SV_POSITION) != 0)
		elementDescs[count++] = { "SV_POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	if ((inputElem & ResDxInputElementFlags_POSITION)!=0)
		elementDescs[count++] = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	if((inputElem&ResDxInputElementFlags_NORMAL)!=0)
		elementDescs[count++] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	if ((inputElem & ResDxInputElementFlags_BINORMAL) != 0)
		elementDescs[count++] = { "BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	if ((inputElem & ResDxInputElementFlags_TANGENT) != 0)
		elementDescs[count++] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	if ((inputElem & ResDxInputElementFlags_TEXCOORD) != 0)
		elementDescs[count++] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	if ((inputElem & ResDxInputElementFlags_BONENO) != 0)
	{
		elementDescs[count++] = { "BONENO", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		elementDescs[count++] = { "BONENO", 1, DXGI_FORMAT_R16G16B16A16_UINT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	}
	if ((inputElem & ResDxInputElementFlags_BONE_WEIGHT) != 0)
	{
		elementDescs[count++] = { "WEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		elementDescs[count++] = { "WEIGHT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	}

	Init(context, rootSignature, count, elementDescs, vs, ps, topology, numRenderTarget, RTVFormats, DSVFormat);

}

void ResDx::ResDxPipelineState::Init(ResDxContext2& context, ID3D12RootSignature* rootSignature, ResDxShader& cs, D3D12_PIPELINE_STATE_FLAGS flags, UINT nodeMask)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	auto CS = cs.GetCompileBlob();

	desc.pRootSignature = rootSignature;
	desc.CS.pShaderBytecode = CS->GetBufferPointer();
	desc.CS.BytecodeLength = CS->GetBufferSize();
	desc.Flags = flags;
	desc.NodeMask = nodeMask;
	
	context.CreateComputePipelineState(desc, &pipeline);
	isInit = true;
}

bool ResDx::ResDxPipelineState::IsInit()
{
	return isInit == true;
}

void ResDx::ResDxPipelineState::Clear()
{
	pipeline->Release();
	isInit = false;
}

ID3D12PipelineState* ResDx::ResDxPipelineState::GetPipelineState()const
{
	return pipeline;
}

void ResDx::ResDxPipelineState::operator=(ResDxPipelineState&&src)
{
	pipeline = src.pipeline;
	isInit = src.isInit;

	src.pipeline = nullptr;
	src.isInit = false;
}

ResDx::ResDxPipelineState::ResDxPipelineState():pipeline(),isInit()
{
}

ResDx::ResDxPipelineState::ResDxPipelineState(ResDxPipelineState&& src)
{
	*this = std::move(src);
}

ResDx::ResDxPipelineState::~ResDxPipelineState()
{
	Clear();
}

void ResDx::ResDxShader::Load(const wchar_t* filePath, const char* entryPoint, const char* compileShaderModel)
{
#ifdef _DEBUG
	UINT compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlag = 0;
#endif // _DEBUG

	ID3D10Blob* errorBlob;
	auto result = D3DCompileFromFile(
		filePath,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint,
		compileShaderModel,
		compileFlag,
		0,
		&blob,
		&errorBlob
	);

	D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&reflection));
	
	if (FAILED(result))
	{


		if (errorBlob)
		{
			std::wstring message = L"filePath:";
			message += filePath;
			MessageBoxW(nullptr, message.c_str(), L"シェーダーコンパイルエラー", MB_OK);

		}

		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			std::wstring message = L"filePath:";
			message += filePath;
			MessageBoxW(nullptr, message.c_str(), L"シェーダーファイルが見つかりませんでした", MB_OK);

		}
	}
	isInit = true;
}

void ResDx::ResDxShader::LoadPS(const wchar_t* filePath, const char* entryPointName)
{
	Load(filePath, entryPointName, PSShaderModel);
}

void ResDx::ResDxShader::LoadVS(const wchar_t* filePath, const char* entryPointName)
{
	Load(filePath, entryPointName, VSShaderModel);
}

void ResDx::ResDxShader::LoadCS(const wchar_t* filePath, const char* entryPointName)
{
	Load(filePath, entryPointName, CSShaderModel);
}

void ResDx::ResDxShader::LoadPS(const char* filePath, const char* entryPointName)
{
	wchar_t wcFilePath[1024];
	size_t ref;
	mbstowcs_s(&ref, wcFilePath, filePath, _TRUNCATE);

	LoadPS(wcFilePath, entryPointName);
}

void ResDx::ResDxShader::LoadVS(const char* filePath, const char* entryPointName)
{
	wchar_t wcFilePath[1024];
	size_t ref;
	mbstowcs_s(&ref, wcFilePath, filePath, _TRUNCATE);

	LoadVS(wcFilePath, entryPointName);
}

void ResDx::ResDxShader::LoadCS(const char* filePath, const char* entryPointName)
{
	wchar_t wcFilePath[1024];
	size_t ref;
	mbstowcs_s(&ref, wcFilePath, filePath, _TRUNCATE);

	LoadCS(wcFilePath, entryPointName);
}

ID3DBlob* ResDx::ResDxShader::GetCompileBlob()
{
	return blob;
}

ID3D12ShaderReflection* ResDx::ResDxShader::GetReflection()
{
	return reflection;
}

bool ResDx::ResDxShader::IsInited()
{
	return isInit == true;
}

void ResDx::ResDxShader::Clear()
{
	blob->Release();
	reflection->Release();
	blob = nullptr;
	isInit = false;
}

void ResDx::ResDxShader::operator=(ResDxShader&& src)
{
	blob = src.blob;
	isInit = src.isInit;

	src.blob = nullptr;
	src.isInit = false;
}

ResDx::ResDxShader::ResDxShader(ResDxShader&& src)
{
	*this = std::move(src);
}

ResDx::ResDxShader::~ResDxShader()
{
	Clear();
}

void ResDx::ResDxCamera::Init(ResDxVector pos, ResDxVector target, ResDxVector up, float fovY, float aspect, float nearZ, float farZ,	ResDxCameraFlags flags,ResDxIO* io)
{
	eyePos = pos;
	targetPos = target;
	upVector = DirectX::XMVector3Normalize(up);
	forwardVector = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, pos));
	rightVector = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(upVector, forwardVector));

	this->fovY = fovY;
	this->aspect = aspect;
	this->nearZ = nearZ;
	this->farZ = farZ;


	viewMatrix = ResDxMatrix();
	projectionMatrix = ResDxMatrix();

	this->flags = flags;

	this->io = io;
}

ResDx::ResDxCamera::ResDxCamera():
eyePos(),
targetPos(),
upVector(),
forwardVector(),
rightVector(),
fovY(),
aspect(),
nearZ(),
farZ(),
viewMatrix(),
projectionMatrix(),
flags(),
io(nullptr)
{}

void ResDx::ResDxCamera::operator=(const ResDxCamera& camera)
{
	eyePos = camera.eyePos;
	targetPos = camera.targetPos;
	upVector = camera.upVector;
	forwardVector = camera.forwardVector;
	rightVector = camera.rightVector;
	fovY = camera.fovY;
	aspect = camera.aspect;
	nearZ = camera.nearZ;
	farZ = camera.farZ;
	viewMatrix = camera.viewMatrix;
	projectionMatrix = camera.projectionMatrix;
	flags = camera.flags;
	io = camera.io;
}

void ResDx::ResDxCamera::Update()
{
	projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
	viewMatrix = DirectX::XMMatrixLookAtLH(eyePos, targetPos, upVector);
	forwardVector = DirectX::XMVectorSubtractAngles(targetPos, eyePos);
	
	if (io)
		io->ResDxIO_Camera(this);
}

void ResDx::ResDxCamera::MoveEyePos(ResDxVector vector)
{
	QueryCameraTransform(
		&ResDxCamera::LocalMoveEyePos,
		&ResDxCamera::WorldMoveEyePos,
		vector,
		ResDxCameraFlags_AllowMoveEyePosX,
		ResDxCameraFlags_AllowMoveEyePosY,
		ResDxCameraFlags_AllowMoveEyePosZ
	);	
}

void ResDx::ResDxCamera::MoveTargetPos(ResDxVector vector)
{
	QueryCameraTransform(
		&ResDxCamera::LocalMoveTargetPos,
		&ResDxCamera::WorldMoveTargetPos,
		vector,
		ResDxCameraFlags_AllowMoveTargetPosX,
		ResDxCameraFlags_AllowMoveTargetPosY,
		ResDxCameraFlags_AllowMoveTargetPosZ
	);
}

void ResDx::ResDxCamera::MoveForward(float quantity)
{
	QueryCameraTransform(
		&ResDxCamera::LocalMoveEyePos,
		&ResDxCamera::LocalMoveEyePos,
		{ 0,0,quantity},
		ResDxCameraFlags_AllowMoveTargetPosX,
		ResDxCameraFlags_AllowMoveTargetPosY,
		ResDxCameraFlags_AllowMoveTargetPosZ
	);
	QueryCameraTransform(
		&ResDxCamera::LocalMoveTargetPos,
		&ResDxCamera::LocalMoveTargetPos,
		{ 0,0,quantity },
		ResDxCameraFlags_AllowMoveTargetPosX,
		ResDxCameraFlags_AllowMoveTargetPosY,
		ResDxCameraFlags_AllowMoveTargetPosZ
	);
}

void ResDx::ResDxCamera::MoveRight(float quantity)
{
	QueryCameraTransform(
		&ResDxCamera::LocalMoveEyePos,
		&ResDxCamera::LocalMoveEyePos,
		{ quantity,0,0 },
		ResDxCameraFlags_AllowMoveTargetPosX,
		ResDxCameraFlags_AllowMoveTargetPosY,
		ResDxCameraFlags_AllowMoveTargetPosZ
	);
	QueryCameraTransform(
		&ResDxCamera::LocalMoveTargetPos,
		&ResDxCamera::LocalMoveTargetPos,
		{ quantity,0,0 },
		ResDxCameraFlags_AllowMoveTargetPosX,
		ResDxCameraFlags_AllowMoveTargetPosY,
		ResDxCameraFlags_AllowMoveTargetPosZ
	);
}

void ResDx::ResDxCamera::MoveUp(float quantity)
{
	QueryCameraTransform(
		&ResDxCamera::LocalMoveEyePos,
		&ResDxCamera::LocalMoveEyePos,
		{ 0,quantity,0 },
		ResDxCameraFlags_AllowMoveTargetPosX,
		ResDxCameraFlags_AllowMoveTargetPosY,
		ResDxCameraFlags_AllowMoveTargetPosZ
	);
	QueryCameraTransform(
		&ResDxCamera::LocalMoveTargetPos,
		&ResDxCamera::LocalMoveTargetPos,
		{ 0,quantity,0 },
		ResDxCameraFlags_AllowMoveTargetPosX,
		ResDxCameraFlags_AllowMoveTargetPosY,
		ResDxCameraFlags_AllowMoveTargetPosZ
	);

}

void ResDx::ResDxCamera::Rotation(ResDxVector rotation)
{
	QueryCameraTransform(
		&ResDxCamera::LocalRotation,
		&ResDxCamera::WorldRotation,
		rotation,
		ResDxCameraFlags_AllowRotationX,
		ResDxCameraFlags_AllowRotationY,
		ResDxCameraFlags_AllowRotationZ
	);
}

void ResDx::ResDxCamera::LocalMoveEyePos(ResDxVector vector)
{
	if (flags & ResDxCameraFlags_NotAllowMoveEyePos)
		return;

	auto moveX = DirectX::XMVectorScale(rightVector, vector.x());
	auto moveY = DirectX::XMVectorScale(upVector, vector.y());
	auto moveZ = DirectX::XMVectorScale(forwardVector, vector.z());

	eyePos = DirectX::XMVectorAdd(eyePos, moveX);
	eyePos = DirectX::XMVectorAdd(eyePos, moveY);
	eyePos = DirectX::XMVectorAdd(eyePos, moveZ);
}

void ResDx::ResDxCamera::LocalMoveTargetPos(ResDxVector vector)
{
	if (flags & ResDxCameraFlags_NotAllowMoveTargetPos)
		return;

	auto moveX = DirectX::XMVectorScale(rightVector, vector.x());
	auto moveY = DirectX::XMVectorScale(upVector, vector.y());
	auto moveZ = DirectX::XMVectorScale(forwardVector, vector.z());

	targetPos = DirectX::XMVectorAdd(targetPos, moveX);
	targetPos = DirectX::XMVectorAdd(targetPos, moveY);
	targetPos = DirectX::XMVectorAdd(targetPos, moveZ);
}

void ResDx::ResDxCamera::LocalRotation(ResDxVector rotation)
{
	if (flags & ResDxCameraFlags_NotAllowRotation)
		return;

	auto matRotation= GetMatrixLocalRotation(rotation);
	ResDxVector vec = {};

	if (flags == 0 || flags & ResDxCameraFlags_RotateLocalFromEyePos)//カメラの座標を中心とした回転
	{
		vec = DirectX::XMVectorSubtract(targetPos, eyePos);
		vec = DirectX::XMVector3Transform(vec, matRotation);
		targetPos = DirectX::XMVectorAdd(eyePos, vec);
		upVector = DirectX::XMVector3Transform(upVector, matRotation);
		rightVector = DirectX::XMVector3Cross(upVector, DirectX::XMVectorSubtract(targetPos, eyePos));
	}
	else if (flags & ResDxCameraFlags_RotateLocalFromTargetPos)//注視点を中心とした回転
	{

		vec = DirectX::XMVectorSubtract(eyePos, targetPos);
		vec = DirectX::XMVector3Transform(vec, matRotation);
		eyePos = DirectX::XMVectorAdd(targetPos, vec);
		upVector = DirectX::XMVector3Transform(upVector, matRotation);
		rightVector = DirectX::XMVector3Cross(upVector, targetPos);
	}

}

void ResDx::ResDxCamera::WorldMoveEyePos(ResDxVector vector)
{
	if (flags & ResDxCameraFlags_NotAllowMoveEyePos)
		return;

	DirectX::XMVectorAdd(eyePos, vector);
}

void ResDx::ResDxCamera::WorldMoveTargetPos(ResDxVector vector)
{
	if (flags & ResDxCameraFlags_NotAllowMoveTargetPos)
		return;

	DirectX::XMVectorAdd(targetPos, vector);
}

void ResDx::ResDxCamera::WorldRotation(ResDxVector rotation)
{
	if (flags & ResDxCameraFlags_NotAllowRotation)
		return;

	auto matRotation = GetMatrixWorldRotation(rotation);
	ResDxVector vec = {};

	vec = DirectX::XMVectorSubtract(targetPos, eyePos);
	vec = DirectX::XMVector3Transform(vec, matRotation);
	targetPos = DirectX::XMVectorAdd(targetPos, vec);
}

ResDx::ResDxCameraFlags ResDx::ResDxCamera::GetCameraFlags()
{
	return flags;
}

void ResDx::ResDxCamera::SetCameraFlags(ResDxCameraFlags flags)
{
	this->flags = flags;
}

ResDx::ResDxMatrix ResDx::ResDxCamera::GetViewMatrix()
{
	return viewMatrix;
}

void ResDx::ResDxCamera::SetIO(ResDxIO* io)
{
	this->io = io;
}

ResDx::ResDxMatrix ResDx::ResDxCamera::GetProjectionMatrix()
{
	return projectionMatrix;
}

void ResDx::ResDxCamera::QueryCameraTransform(
	ResDxCameraQueryFunctionType	localFunc, 
	ResDxCameraQueryFunctionType	worldFunc, 
	ResDxVector						vector,
	ResDxCameraFlags				coordinateFlagsX,
	ResDxCameraFlags				coordinateFlagsY,
	ResDxCameraFlags				coordinateFlagsZ
)
{
	if (flags == 0)
	{
		(this->*localFunc)(vector);
	}
	else if (flags & ResDxCameraFlags_LocalCoordinate)
	{
		(this->*localFunc)
		(
			{
				vector.x() * ((flags & coordinateFlagsX) != 0),
				vector.y() * ((flags & coordinateFlagsY) != 0),
				vector.z() * ((flags & coordinateFlagsZ) != 0),
				0.0f
			}
		);
	}
	else if (flags & ResDxCameraFlags_WorldCoordinate)
	{
		(this->*worldFunc)
		(
			{
				vector.x() * ((flags & coordinateFlagsX) != 0),
				vector.y() * ((flags & coordinateFlagsY) != 0),
				vector.z() * ((flags & coordinateFlagsZ) != 0),
				0.0f
			}
		);
	}
}

ResDx::ResDxMatrix ResDx::ResDxCamera::GetMatrixLocalRotation(ResDxVector rotation)
{
	ResDxAngle angle;
	angle.SetAngleEuler(rotation);
	auto rotationX = DirectX::XMQuaternionRotationAxis(upVector, angle.RadianX());
	auto rotationY = DirectX::XMQuaternionRotationAxis(rightVector, angle.RadianY());
	auto rotationZ = DirectX::XMQuaternionRotationAxis(forwardVector, angle.RadianZ());
	auto result = DirectX::XMVECTOR();

	result = DirectX::XMQuaternionMultiply(rotationY, rotationZ);
	result = DirectX::XMQuaternionMultiply(rotationX, result);
	result = DirectX::XMQuaternionNormalize(result);

	return DirectX::XMMatrixRotationQuaternion(result);
}

ResDx::ResDxMatrix ResDx::ResDxCamera::GetMatrixWorldRotation(ResDxVector rotation)
{
	return DirectX::XMMatrixRotationRollPitchYaw(
		rotation.x(),
		rotation.y(), 
		rotation.z()
	);
}

void ResDx::ResDxIODefault::ResDxIO_Camera(ResDxCamera* camera)
{
	if (!camera)
		return;
	
	if (key->IsPushKey('W'))
		camera->MoveForward(1.0);
	if (key->IsPushKey('A'))
		camera->MoveRight(-1.0);
	if (key->IsPushKey('S'))
		camera->MoveForward(-1.0);
	if (key->IsPushKey('D'))
		camera->MoveRight(1.0);
	if (key->IsPushKey(VK_SPACE))
	{
		if (key->IsPushKey(VK_CONTROL))
			camera->MoveUp(-1.0);
		else
			camera->MoveUp(1.0);
	}

	POINT pos = mouse->GetMouseMoveVector();
	ResDxVector vec = { pos.x * 0.1f,pos.y * 0.1f,0.0f };
	camera->Rotation(vec);
}

ResDx::ResDxIODefault& ResDx::ResDxIODefault::instance()
{
	static ResDxIODefault io;
	return io;
}


ResDx::ResDxVector4::operator DirectX::XMVECTOR()const
{
	return value;
}

ResDx::ResDxVector4::operator DirectX::XMVECTOR* ()
{
	return &value;
}

ResDx::ResDxVector4::operator DirectX::XMFLOAT4()const
{
	DirectX::XMFLOAT4 ret = {};
	DirectX::XMStoreFloat4(&ret, value);
	return ret;
}

ResDx::ResDxVector4::operator DirectX::XMFLOAT3() const
{
	DirectX::XMFLOAT3 ret = {};
	DirectX::XMStoreFloat3(&ret, value);
	return ret;
}

ResDx::ResDxVector4::operator DirectX::XMFLOAT2()const
{
	DirectX::XMFLOAT2 ret = {};
	DirectX::XMStoreFloat2(&ret, value);
	return ret;
}

void ResDx::ResDxVector4::operator=(const DirectX::XMVECTOR& src)
{
	value = src;
}

void ResDx::ResDxVector4::operator=(const DirectX::XMFLOAT4& src)
{
	value = DirectX::XMLoadFloat4(&src);
}

void ResDx::ResDxVector4::operator=(const DirectX::XMFLOAT2& src)
{
	value = DirectX::XMLoadFloat2(&src);
}

void ResDx::ResDxVector4::operator=(const DirectX::XMFLOAT3& src)
{
	value = DirectX::XMLoadFloat3(&src);
}

void ResDx::ResDxVector4::operator=(const ResDxVector4& src)
{
	value = src.value;
}

void ResDx::ResDxVector4::operator=(ResDxVector4&& src)
{
	value = std::move(src.value);
}

ResDx::ResDxVector4 ResDx::ResDxVector4::operator*(const ResDxVector4& src)
{
	return DirectX::XMVectorMultiply(value, src);
}

ResDx::ResDxVector4 ResDx::ResDxVector4::operator+(const ResDxVector4& src)
{
	return DirectX::XMVectorAdd(value, src);
}

ResDx::ResDxVector4 ResDx::ResDxVector4::operator-(const ResDxVector4& src)
{
	return DirectX::XMVectorSubtract(value, src);
}

ResDx::ResDxVector4 ResDx::ResDxVector4::operator*(float factor)
{
	return DirectX::XMVectorScale(value, factor);
}

float ResDx::ResDxVector4::x()
{
	return ((DirectX::XMFLOAT4)*this).x;
}

float ResDx::ResDxVector4::y()
{
	return ((DirectX::XMFLOAT4)*this).y;
}

float ResDx::ResDxVector4::z()
{
	return ((DirectX::XMFLOAT4)*this).z;
}

float ResDx::ResDxVector4::w()
{
	return ((DirectX::XMFLOAT4)*this).w;
}

ResDx::ResDxVector4::ResDxVector4(float x, float y, float z, float w) :value()
{
	DirectX::XMFLOAT4 src = { x,y,z,w };
	value = DirectX::XMLoadFloat4(&src);
}

void ResDx::ResDxQuaternion::SetQuaternionRotationEuler(float pitch, float yaw, float roll)
{
	value = DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
}

void ResDx::ResDxQuaternion::SetQuaternionRotationEuler(const ResDxVector rotate)
{
	value = DirectX::XMQuaternionRotationRollPitchYawFromVector(rotate);
}

void ResDx::ResDxQuaternion::SetQuaternionRotationAxis(const ResDxVector axis, const float angle)
{
	value = DirectX::XMQuaternionRotationAxis(axis, angle);
}

void ResDx::ResDxQuaternion::SetQuaternionSlep(const ResDxQuaternion q1, const ResDxQuaternion q2, float t)
{
	value = DirectX::XMQuaternionSlerp(q1, q2, t);
}

void ResDx::ResDxQuaternion::SetQuaternionRotationMatrix(const ResDxMatrix rotation)
{
	DirectX::XMQuaternionRotationMatrix(rotation);
}

ResDx::ResDxMatrix ResDx::ResDxQuaternion::GetRotationMatrix()
{
	return DirectX::XMMatrixRotationQuaternion(value);
}

ResDx::ResDxQuaternion ResDx::ResDxQuaternion::GetQuaternionInverse()
{
	return DirectX::XMQuaternionInverse(value);
}

ResDx::ResDxQuaternion ResDx::ResDxQuaternion::operator*(const ResDxQuaternion& quaternion)
{
	return DirectX::XMQuaternionMultiply(value,quaternion.value);
}

ResDx::ResDxQuaternion::operator DirectX::XMVECTOR()const
{
	return value;
}

ResDx::ResDxQuaternion::operator DirectX::XMFLOAT4()const
{
	DirectX::XMFLOAT4 t;
	DirectX::XMStoreFloat4(&t, value);
	return t;
}

void ResDx::ResDxQuaternion::operator=(const DirectX::XMVECTOR& src)
{
	value = src;
}

void ResDx::ResDxQuaternion::operator=(const DirectX::XMFLOAT4& src)
{
	value = DirectX::XMLoadFloat4(&src);
}

void ResDx::ResDxQuaternion::operator=(const ResDxQuaternion& src)
{
	value = src;
}

void ResDx::ResDxQuaternion::operator=(ResDxQuaternion&& src)
{
	value = std::move(src);
}

void ResDx::ResDxTransform::operator=(const ResDxTransform& transform)
{
	loc = transform.loc;
	rot = transform.rot;
	scl = transform.scl;
}

void ResDx::ResDxTransform::operator=(ResDxTransform&& src)
{
	loc = std::move(src.loc);
	rot = std::move(src.rot);
	scl = std::move(src.scl);
}

ResDx::ResDxTransform::ResDxTransform(const ResDxTransform& src)
{
	*this = src;
}

ResDx::ResDxTransform::ResDxTransform(ResDxTransform&& src)
{
	*this = std::move(src);
}
/*
void ResDx::ResDxTransform::QueryCalcrateTransform(const ResDxVector src, const ResDxTransformFlags flags)
{
	const ResDxTransformFlags targetMask = 
		ResDxTransformFlags_Location | 
		ResDxTransformFlags_Rotation | 
		ResDxTransformFlags_Scale;
	
	const ResDxTransformFlags operateMask =
		ResDxTransformFlags_OperateStore|
		ResDxTransformFlags_OperateClear|
		ResDxTransformFlags_OperateCalc;

	const ResDxTransformFlags calcrateMask=
		ResDxTransformFlags_CalcrateAdd|
		ResDxTransformFlags_CalcrateSub|
		ResDxTransformFlags_CalcrateMul|
		ResDxTransformFlags_CalcrateDiv;

	ResDxVector* dst = nullptr;
	ResDxTransformFlags target = flags & targetMask;
	ResDxTransformFlags operate = flags & operateMask;
	ResDxTransformFlags calcrate = flags & calcrateMask;

	switch (target)
	{
	case ResDxTransformFlags_None:
		return;
		break;
	case ResDxTransformFlags_Location:
		dst = &loc;
		break;
	case ResDxTransformFlags_Rotation:
		dst = &rot;
		break;
	case ResDxTransformFlags_Scale:
		dst = &scl;
		break;
	default:
		assert(0);
		break;
	}

	switch (operate)
	{

	case ResDxTransformFlags_OperateStore:
		*dst = src;
		return;
		break;
	case ResDxTransformFlags_OperateClear:
		*dst = ResDxVector();
		return;
		break;
	case ResDxTransformFlags_OperateCalc:
		//なにもしない
		break;
	default:
		return;
		break;
	}

	switch (calcrate)
	{

	case ResDxTransformFlags_CalcrateAdd:
		*dst = DirectX::XMVectorAdd(*dst, src);
		break;
	case ResDxTransformFlags_CalcrateSub:
		*dst = DirectX::XMVectorSubtract(*dst, src);
		break;
	case ResDxTransformFlags_CalcrateMul:
		*dst = DirectX::XMVectorMultiply(*dst, src);
		break;
	case ResDxTransformFlags_CalcrateDiv:
		*dst = DirectX::XMVectorDivide(*dst, src);
		break;
	default:
		break;
	}
}
*/
void ResDx::ResDxTransform::SetLocation(location_t vector)
{
	loc = vector;
}

void ResDx::ResDxTransform::SetRotation(rotation_t quaternion)
{
	rot = quaternion;
}

void ResDx::ResDxTransform::SetRotationSlep(rotation_t quaternion, float t)
{
	rot.SetQuaternionSlep(rot, quaternion, t);
}

void ResDx::ResDxTransform::SetRotationEuler(float pitch, float yaw, float roll)
{
	rot.SetQuaternionRotationEuler(pitch, yaw, roll);
}

void ResDx::ResDxTransform::SetScale(scale_t vector)
{
	scl = vector;
}

void ResDx::ResDxTransform::AddLocation(location_t vector)
{
	loc = loc + vector;
}

void ResDx::ResDxTransform::AddRotation(rotation_t rotate)
{
	rot = rot * rotate;
}

void ResDx::ResDxTransform::AddRotationEuler(float pitch, float yaw, float roll)
{
	ResDxQuaternion rotate(pitch, yaw, roll);
	rot = rot * rotate;
}

void ResDx::ResDxTransform::AddScale(scale_t vector)
{
	scl = scl + vector;
}

ResDx::ResDxTransform::matrix_t ResDx::ResDxTransform::GetTransform()const
{
	return	DirectX::XMMatrixTranslationFromVector(loc) *
			DirectX::XMMatrixRotationQuaternion(rot) *
			DirectX::XMMatrixScalingFromVector(scl);
}

ResDx::ResDxTransform::location_t ResDx::ResDxTransform::GetLocation()const
{
	return loc;
}

ResDx::ResDxTransform::rotation_t ResDx::ResDxTransform::GetRotation()const
{
	return rot;
}

ResDx::ResDxTransform::scale_t ResDx::ResDxTransform::GetScale()const
{
	return scl;
}

ResDx::ResDxVector4x4::operator DirectX::XMMATRIX() const
{
	return matrix;
}

ResDx::ResDxVector4x4::operator DirectX::XMMATRIX* ()
{
	return &matrix;
}

void ResDx::ResDxVector4x4::operator=(const DirectX::XMMATRIX& src)
{
	matrix = src;
}

void ResDx::ResDxVector4x4::operator=(DirectX::XMMATRIX&& src)
{
	matrix = std::move(src);
}

void ResDx::ResDxVector4x4::operator=(const ResDxVector4x4& src)
{
	matrix = src.matrix;
}

void ResDx::ResDxVector4x4::operator=(ResDxVector4x4&& src)
{
	matrix = std::move(src.matrix);
}

ResDx::ResDxVector4x4 ResDx::ResDxVector4x4::operator*(const ResDxVector4x4 src)const
{
	return DirectX::XMMatrixMultiply(matrix, src.matrix);
}

ResDx::ResDxMatrix ResDx::ResDxVector4x4::Inverse() const
{
	return DirectX::XMMatrixInverse(nullptr, matrix);
}

void ResDx::ResDxAngle::SetAngleEuler(float x, float y, float z)
{
	value = TranslateEulerToRadian({ x,y,z });
}

void ResDx::ResDxAngle::SetAngleEuler(ResDxVector angle)
{
	value = TranslateEulerToRadian(angle);
}

void ResDx::ResDxAngle::SetAngleRadian(float x, float y, float z)
{
	value = { x,y,z };
}

void ResDx::ResDxAngle::SetAngleRadian(ResDxVector angle)
{
	value = angle;
}

ResDx::ResDxAngle::angle_t ResDx::ResDxAngle::GetAngleEuler()
{
	return TranslateRadianToEuler(value);
}

ResDx::ResDxAngle::angle_t ResDx::ResDxAngle::GetAngleRadian()
{
	return value;
}

ResDx::ResDxQuaternion ResDx::ResDxAngle::GetAngleQuaternion()
{
	ResDxQuaternion quaternion;
	quaternion.SetQuaternionRotationEuler(TranslateRadianToEuler(value));
	return ResDxQuaternion();
}

float ResDx::ResDxAngle::RadianX()
{
	return value.x;
}

float ResDx::ResDxAngle::RadianY()
{
	return value.y;
}

float ResDx::ResDxAngle::RadianZ()
{
	return value.z;
}

ResDx::ResDxAngle::operator angle_t()
{
	return value;
}

void ResDx::ResDxAngle::operator=(const ResDxAngle& angle)
{
	value = angle.value;
}

void ResDx::ResDxAngle::operator=(ResDxAngle&& angle)
{
	value = std::move(angle.value);
}

ResDx::ResDxAngle::angle_t ResDx::ResDxAngle::TranslateEulerToRadian(angle_t angle)
{
	value.x = angle.x * PI / 180;
	value.y = angle.y * PI / 180;
	value.z = angle.z * PI / 180;
	return value;
}

ResDx::ResDxAngle::angle_t ResDx::ResDxAngle::TranslateRadianToEuler(angle_t angle)
{
	value.x = angle.x * 180 / PI;
	value.y = angle.y * 180 / PI;
	value.z = angle.z * 180 / PI;
	return value;
}

void ResDx::ResDxBlendState::Init()
{
	blend = D3D12_BLEND_DESC();
}

void ResDx::ResDxBlendState::SetBlendState(int index, ResDxBlendFlag flag)
{
	assert(index < RES_DX_MAX_NUM_RENDER_TARGET);
	auto desc = blend.RenderTarget[index];
	desc.BlendEnable = true;
	switch (flag)
	{
	case ResDxRendererBlendFlag_None:
		break;
	case ResDxRendererBlendFlga_Alpha:

		desc.BlendEnable = true;
		desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		desc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		desc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

		break;

	case ResDxRendererBlendFlga_Alpha_Add:

		desc.BlendEnable = true;
		desc.BlendOp = D3D12_BLEND_OP_ADD;
		desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		desc.DestBlend = D3D12_BLEND_ONE;

		break;
	case ResDxRendererBlendFlga_Add:

		desc.BlendEnable = true;
		desc.BlendOp = D3D12_BLEND_OP_ADD;
		desc.SrcBlend = D3D12_BLEND_ONE;
		desc.DestBlend = D3D12_BLEND_ONE;

		break;
	case ResDxRendererBlendFlga_Sub:
		desc.BlendEnable = true;
		desc.BlendOp = D3D12_BLEND_OP_SUBTRACT;
		desc.SrcBlend = D3D12_BLEND_ONE;
		desc.DestBlend = D3D12_BLEND_ONE;

		break;
	default:
		break;
	}
}

D3D12_BLEND_DESC ResDx::ResDxBlendState::GetBlend()
{
	return blend;
}

void ResDx::ResDxBlendState::operator=(const ResDxBlendState& o)
{
	blend = o.blend;
}

void ResDx::ResDxBlendState::operator=(ResDxBlendState&& o)
{
	blend = std::move(o.blend);
}

void ResDx::ResDxRasterizer::Init()
{
	rasterizer = D3D12_RASTERIZER_DESC();
}

void ResDx::ResDxRasterizer::SetRasterizerState(ResDxRasterizerFlags flags)
{
	ResDxRasterizerFlags fillMode = flags & (ResDxRasterizerFlags_FillSolid | ResDxRasterizerFlags_FillFrame);
	ResDxRasterizerFlags cullMode = flags & (ResDxRasterizerFlags_CullNone | ResDxRasterizerFlags_CullFront | ResDxRasterizerFlags_CullBack);
	ResDxRasterizerFlags depthBias = flags & (ResDxRasterizerFlags_DepthBiasNone | ResDxRasterizerFlags_DepthBiasMax | ResDxRasterizerFlags_DepthBiasMin);
	ResDxRasterizerFlags multisample = flags & (ResDxRasterizerFlags_MultisampleEnable);
	ResDxRasterizerFlags antialiased = flags & (ResDxRasterizerFlags_AntialiasedLineEnable);

	switch (fillMode)
	{
	case ResDxRasterizerFlags_FillSolid:
		rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
		break;

	case ResDxRasterizerFlags_FillFrame:
		rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
		break;

	default:
		rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
		break;
	}

	switch (cullMode)
	{
	case ResDxRasterizerFlags_CullNone:
		rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		break;

	case ResDxRasterizerFlags_CullFront:
		rasterizer.CullMode = D3D12_CULL_MODE_FRONT;
		break;

	case ResDxRasterizerFlags_CullBack:
		rasterizer.CullMode = D3D12_CULL_MODE_BACK;
		break;

	default:
		rasterizer.CullMode = D3D12_CULL_MODE_NONE;
		break;
	}

	switch (depthBias)
	{
	case ResDxRasterizerFlags_DepthBiasNone:
		rasterizer.DepthBias = 0;
		rasterizer.DepthBiasClamp = 0.0f;
		rasterizer.SlopeScaledDepthBias = 0.0f;
		break;

	case ResDxRasterizerFlags_DepthBiasMax:
		rasterizer.DepthBias = 1;
		rasterizer.DepthBiasClamp = 1.0f;
		rasterizer.SlopeScaledDepthBias = 1.0f;
		break;

	case ResDxRasterizerFlags_DepthBiasMin:
		rasterizer.DepthBias = 1;
		rasterizer.DepthBiasClamp = -1.0f;
		rasterizer.SlopeScaledDepthBias = 1.0f;
		break;

	default:
		rasterizer.DepthBias = 0;
		rasterizer.DepthBiasClamp = 0.0f;
		rasterizer.SlopeScaledDepthBias = 0.0f;
		break;
	}

	rasterizer.MultisampleEnable = (BOOL)multisample;
	rasterizer.AntialiasedLineEnable = (BOOL)antialiased;
}

D3D12_RASTERIZER_DESC ResDx::ResDxRasterizer::GetRasterizer()
{
	return rasterizer;
}

void ResDx::ResDxRasterizer::operator=(const ResDxRasterizer& o)
{
	rasterizer = o.rasterizer;
}

void ResDx::ResDxRasterizer::operator=(ResDxRasterizer&& o)
{
	rasterizer = std::move(o.rasterizer);
}

void ResDx::ResDxDepthStencil::Init()
{
}

void ResDx::ResDxDepthStencil::SetDepthStencil(ResDxDepthStencilFlags flags)
{
	auto depthEnable = flags & ResDxDepthStencilFlags_DepthEnable;
	auto stencilEnable = flags & ResDxDepthStencilFlags_StencilEnable;
	auto format = flags & (ResDxDepthStencilFlags_FormatD32_FLOAT | ResDxDepthStencilFlags_FormatD32_FLOAT_S824_UINT | ResDxDepthStencilFlags_D16_UNORM | ResDxDepthStencilFlags_D24_UNORM_S8_UINT);
	auto depthFunc = flags & (ResDxDepthStencilFlags_DepthComparisonLess | ResDxDepthStencilFlags_DepthComparisonGreater);

	switch (depthEnable | stencilEnable | format)
	{
	case ResDxDepthStencilFlags_DepthEnable | ResDxDepthStencilFlags_FormatD32_FLOAT:
		desc.DepthEnable = TRUE;
		this->format = DXGI_FORMAT_D32_FLOAT;
		break;

	case ResDxDepthStencilFlags_DepthEnable | ResDxDepthStencilFlags_StencilEnable | ResDxDepthStencilFlags_FormatD32_FLOAT_S824_UINT:
		desc.DepthEnable = TRUE;
		desc.StencilEnable = TRUE;
		this->format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		break;

	case ResDxDepthStencilFlags_DepthEnable| ResDxDepthStencilFlags_D16_UNORM:
		desc.DepthEnable = TRUE;
		this->format = DXGI_FORMAT_D16_UNORM;
		break;

	case ResDxDepthStencilFlags_DepthEnable | ResDxDepthStencilFlags_StencilEnable | ResDxDepthStencilFlags_D24_UNORM_S8_UINT:
		desc.DepthEnable = TRUE;
		this->format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	default:
		assert(false);
		break;
	}

	switch (depthFunc)
	{
	case ResDxDepthStencilFlags_DepthComparisonLess:
		desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		break;

	case ResDxDepthStencilFlags_DepthComparisonGreater:
		desc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
		break;

	default:
		assert(false);
		break;
	}

	desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
}

DXGI_FORMAT ResDx::ResDxDepthStencil::GetFormat()
{
	return format;
}

D3D12_DEPTH_STENCIL_DESC ResDx::ResDxDepthStencil::GetDepthStencil()
{
	return desc;
}


void ResDx::ResDxDepthStencil::operator=(const ResDxDepthStencil& o)
{
	format = o.format;
	desc = o.desc;
}

void ResDx::ResDxDepthStencil::operator=(ResDxDepthStencil&& o)
{
	format = std::move(o.format);
	desc = std::move(desc);

	o.format = DXGI_FORMAT();
	o.desc = D3D12_DEPTH_STENCIL_DESC();
}

void ResDx::ResDxCommandList::Init(ID3D12Device5* device, ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type)
{
	this->type = type;
	device->CreateCommandList(0, type, allocator, nullptr, IID_PPV_ARGS(&commandList));
}

void ResDx::ResDxCommandList::Reset(ID3D12CommandAllocator* allocator)
{
	commandList->Reset(allocator, nullptr);
}

void ResDx::ResDxCommandList::Close()
{
	commandList->Close();
}

void ResDx::ResDxCommandList::Release()
{
	if (!commandList)
		return;
	type = D3D12_COMMAND_LIST_TYPE();
	commandList->Release();
	commandList = nullptr;
}

ID3D12GraphicsCommandList4* ResDx::ResDxCommandList::Get()
{
	return commandList;
}

D3D12_COMMAND_LIST_TYPE ResDx::ResDxCommandList::Type()
{
	return type;
}

void ResDx::ResDxCommandList::operator=(ResDxCommandList&& o)
{
	type = std::move(o.type);
	commandList = std::move(o.commandList);
	o.type = D3D12_COMMAND_LIST_TYPE();
	o.commandList = nullptr;
}

ResDx::ResDxCommandList::ResDxCommandList(ResDxCommandList&& o)
{
	*this = std::move(o);
}

ResDx::ResDxCommandList::~ResDxCommandList()
{
	assert(!commandList);
}

void ResDx::ResDxCommandAllocator::Init(ID3D12Device5* device, D3D12_COMMAND_LIST_TYPE type)
{
	this->type = type;
	device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator));
}

void ResDx::ResDxCommandAllocator::Reset()
{
	allocator->Reset();
}

void ResDx::ResDxCommandAllocator::Release()
{
	if (!allocator)
		return;

	type = D3D12_COMMAND_LIST_TYPE();
	allocator->Release();
	allocator = nullptr;
}

ID3D12CommandAllocator* ResDx::ResDxCommandAllocator::Get()
{
	return allocator;
}

void ResDx::ResDxCommandAllocator::operator=(ResDxCommandAllocator&& o)
{
	type = std::move(o.type);
	allocator = std::move(o.allocator);
	o.type = D3D12_COMMAND_LIST_TYPE();
	o.allocator = nullptr;
}

ResDx::ResDxCommandAllocator::ResDxCommandAllocator(ResDxCommandAllocator&& o)
{
	*this = std::move(o);
}

ResDx::ResDxCommandAllocator::~ResDxCommandAllocator()
{
	assert(!allocator);
}

void ResDx::ResDxCommandQueue::Init(ID3D12Device5* device, D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Type = type;
	device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue));
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	ClearFenceValue();
}

void ResDx::ResDxCommandQueue::WaitCompleteExecute()
{
	auto complete = fence->GetCompletedValue();
	if (complete >= fenceValue)
	{
		return;
	}
	auto eve = CreateEvent(nullptr, false, false, nullptr);
	fence->SetEventOnCompletion(fenceValue, eve);
	WaitForSingleObject(eve, INFINITE);
	CloseHandle(eve);
}

void ResDx::ResDxCommandQueue::Release()
{
	if (!commandQueue)
		return;

	commandQueue->Release();
	fence->Release();
	commandQueue = nullptr;
	fence = nullptr;
	fenceValue = 0;
}

UINT64 ResDx::ResDxCommandQueue::Execute(ResDxCommandList* commandList)
{
	auto cmd = commandList->Get();
	UINT64 currentFenceValue = fenceValue;
	while (!fenceValue.compare_exchange_weak(currentFenceValue, currentFenceValue + 1))
		currentFenceValue = fenceValue;

	ID3D12CommandList* lists[] = { commandList->Get() };
	commandQueue->ExecuteCommandLists(1, lists);
	commandQueue->Signal(fence,fenceValue);
	return currentFenceValue;
}

UINT64 ResDx::ResDxCommandQueue::Execute(int numCommandList, ResDxCommandList* commandList[])
{
	assert(numCommandList < RES_DX_MAX_NUM_EXECUTE_COMMAND_LISTS);
	ID3D12CommandList* cmd[RES_DX_MAX_NUM_EXECUTE_COMMAND_LISTS];
	for (int i = 0; i < numCommandList; ++i)
		cmd[i] = commandList[i]->Get();
	UINT64 currentFenceValue = fenceValue;
	while (!fenceValue.compare_exchange_weak(currentFenceValue, currentFenceValue + 1))
		currentFenceValue = fenceValue;

	commandQueue->ExecuteCommandLists(numCommandList, cmd);
	commandQueue->Signal(fence, currentFenceValue+1);
	return currentFenceValue + 1;
}

UINT64 ResDx::ResDxCommandQueue::FenceCompletedValue()const
{
	return fence->GetCompletedValue();
}

bool ResDx::ResDxCommandQueue::Completed()const
{
	return fence->GetCompletedValue() == fenceValue;
}

UINT64 ResDx::ResDxCommandQueue::FenceValue()const
{
	return fenceValue;
}

void ResDx::ResDxCommandQueue::ClearFenceValue()
{
	fenceValue = 0;
}

ID3D12CommandQueue* ResDx::ResDxCommandQueue::GetQueue()
{
	return commandQueue;
}

void ResDx::ResDxCommandQueue::operator=(ResDxCommandQueue&& o)
{
	commandQueue = std::move(o.commandQueue);
	fence = std::move(fence);
	fenceValue = o.fenceValue.load();
	o.commandQueue = nullptr;
	o.fence = nullptr;
	o.fenceValue = 0;
}

ResDx::ResDxCommandQueue::ResDxCommandQueue(ResDxCommandQueue&&o)
{
	*this = std::move(o);
}

ResDx::ResDxCommandQueue::~ResDxCommandQueue()
{
	if (commandQueue)
		commandQueue->Release();
}

void ResDx::ResDxSwapChain::Init(GraphicsDevice* device, ID3D12CommandQueue* commandQueue, int numFrame, DXGI_FORMAT rtvFormat)
{
	HRESULT result = {};
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	IDXGIFactory4* factory = device->GetFactory();
	HWND window = device->GetWindowHWND();
	swapChainDesc.Width = DEFAULT_WINDOW_WIDTH;
	swapChainDesc.Height = DEFAULT_WINDOW_HEIGHT;
	swapChainDesc.Format = rtvFormat;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferCount = numFrame;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = factory->CreateSwapChainForHwnd(
		commandQueue,
		window,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapchain
	);

	resources = new ID3D12Resource * [numFrame];
	for (UINT i = 0; i < numFrame; ++i)
		swapchain->GetBuffer(i, IID_PPV_ARGS(&resources[i]));
	this->width = DEFAULT_WINDOW_WIDTH;
	this->height = DEFAULT_WINDOW_HEIGHT;
}

void ResDx::ResDxSwapChain::Present()
{
	swapchain->Present(1,0);
}

void ResDx::ResDxSwapChain::Release()
{
	swapchain->Release();
	for (int i = 0; i < numFrame; ++i)
		resources[i]->Release();
	delete[] resources;
	swapchain = nullptr;
	resources = nullptr;
}

ID3D12Resource* ResDx::ResDxSwapChain::GetFrameRenderTargetBuffer(UINT index)
{
	return resources[index];
}

const int ResDx::ResDxSwapChain::GetFrameBufferWidth()
{
	return width;
}

const int ResDx::ResDxSwapChain::GetFrameBufferHight()
{
	return height;
}

const UINT ResDx::ResDxSwapChain::GetBackBufferIndex()
{
	return swapchain->GetCurrentBackBufferIndex();
}

void ResDx::ResDxSwapChain::operator=(ResDxSwapChain&& o)
{
	swapchain = std::move(o.swapchain);
	o.swapchain = nullptr;
}

ResDx::ResDxSwapChain::ResDxSwapChain(ResDxSwapChain&& o)
{
	*this = std::move(o);
}

ResDx::ResDxSwapChain::~ResDxSwapChain()
{
	assert(!swapchain);
}

void ResDx::ResDxCommandListCopy::CopyGpuResource(ID3D12Resource* dstResource, ID3D12Resource* srcResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint)
{
	auto footprint = srcFootprint;
	auto cmdList = commandList;
	D3D12_TEXTURE_COPY_LOCATION copyLoc = {};//コピー元
	copyLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	copyLoc.PlacedFootprint = footprint;
	copyLoc.PlacedFootprint.Offset = 0;
	copyLoc.pResource = srcResource;

	D3D12_TEXTURE_COPY_LOCATION targetLoc = {};//コピー先
	targetLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	targetLoc.SubresourceIndex = 0;
	targetLoc.pResource = dstResource;

	cmdList->CopyTextureRegion(&targetLoc, 0, 0, 0, &copyLoc, nullptr);
}

void ResDx::ResDxCommandListCopy::ExecuteBundle(ID3D12GraphicsCommandList* bundle)
{
	commandList->ExecuteBundle(bundle);
}

ResDx::ResDxCommandListCopy::operator ResDx::ResDxCommandList*()const
{
	return resDxCommandList;
}

void ResDx::ResDxCommandListCopy::operator=(const ResDxCommandListCopy& commandList)
{
	this->commandList = commandList.commandList;
	resDxCommandList = commandList.resDxCommandList;
}

void ResDx::ResDxCommandListCopy::operator=(ResDxCommandListCopy&& commandList)
{
	this->commandList = std::move(commandList.commandList);
	this->resDxCommandList = std::move(commandList.resDxCommandList);
	commandList.commandList = nullptr;
	commandList.resDxCommandList = nullptr;
}

void ResDx::ResDxCommandListDirect::SetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW* vertexBufferView)
{
	commandList->IASetVertexBuffers(0, 1, vertexBufferView);
}

void ResDx::ResDxCommandListDirect::SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView)
{
	commandList->IASetIndexBuffer(indexBufferView);
}

void ResDx::ResDxCommandListDirect::SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
{
	commandList->SetDescriptorHeaps(1, &descriptorHeap);
}

void ResDx::ResDxCommandListDirect::SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps)
{
	commandList->SetDescriptorHeaps(numDescriptorHeap, descriptorHeaps);
}

void ResDx::ResDxCommandListDirect::SetGraphicsDescriptorTable(UINT indexRootParameter, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle)
{
	commandList->SetGraphicsRootDescriptorTable(indexRootParameter, descriptorHandle);
}

void ResDx::ResDxCommandListDirect::SetGraphicsRootSignature(ID3D12RootSignature* rootSignature)
{
	commandList->SetGraphicsRootSignature(rootSignature);
}

void ResDx::ResDxCommandListDirect::SetComputeRootSignature(ID3D12RootSignature* rootSignature)
{
	commandList->SetComputeRootSignature(rootSignature);
}

void ResDx::ResDxCommandListDirect::SetGraphicsRoot32BitConstant(UINT rootParameter, UINT src, UINT dstOffset)
{
	commandList->SetGraphicsRoot32BitConstant(rootParameter, src, dstOffset);
}

void ResDx::ResDxCommandListDirect::SetGraphicsRoot32BitConstants(UINT rootParameter, UINT num32BitValue, const void* src, UINT dstOffset)
{
	commandList->SetGraphicsRoot32BitConstants(rootParameter, num32BitValue, src, dstOffset);
}

void ResDx::ResDxCommandListDirect::SetPipelineState(ID3D12PipelineState* pipelineState)
{
	commandList->SetPipelineState(pipelineState);
}

void ResDx::ResDxCommandListDirect::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV)
{
	commandList->OMSetRenderTargets(1, descriptorHandleRTV, FALSE, descriptorHandleDSV);
}

void ResDx::ResDxCommandListDirect::SetRenderTargets(UINT numDescriptorRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV)
{
	commandList->OMSetRenderTargets(numDescriptorRTV, descriptorHandleRTV, FALSE, descriptorHandleDSV);
}

void ResDx::ResDxCommandListDirect::SetScissorRect(D3D12_RECT* rect)
{
	commandList->RSSetScissorRects(1, rect);
}

void ResDx::ResDxCommandListDirect::SetViewport(D3D12_VIEWPORT* viewport)
{
	commandList->RSSetViewports(1, viewport);
}

void ResDx::ResDxCommandListDirect::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	commandList->IASetPrimitiveTopology(topology);
}

void ResDx::ResDxCommandListDirect::ExecuteBundle(ID3D12GraphicsCommandList* bundle)
{
	commandList->ExecuteBundle(bundle);
}

void ResDx::ResDxCommandListDirect::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE view, const FLOAT* color, const UINT numRect, const D3D12_RECT* rects)
{
	commandList->ClearRenderTargetView(view, color, numRect, rects);
}

void ResDx::ResDxCommandListDirect::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE view, D3D12_CLEAR_FLAGS flags, FLOAT depth, UINT stencil, const UINT numRect, const D3D12_RECT* rects)
{
	commandList->ClearDepthStencilView(view, flags, depth, stencil, numRect, rects);
}

void ResDx::ResDxCommandListDirect::ResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
	commandList->ResourceBarrier(1, &barrier);
}

void ResDx::ResDxCommandListDirect::ResourceBarriers(UINT numResource, ID3D12Resource** resources, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	assert(numResource < RES_DX_MAX_NUM_RENDER_TARGET);
	D3D12_RESOURCE_BARRIER barrier[RES_DX_MAX_NUM_RENDER_TARGET] = {};
	for (size_t i = 0; i < RES_DX_MAX_NUM_RENDER_TARGET; ++i)
		barrier[i] = CD3DX12_RESOURCE_BARRIER::Transition(resources[i], before, after);

	commandList->ResourceBarrier(numResource, barrier);
}

void ResDx::ResDxCommandListDirect::DrawInstanced(UINT numVertex)
{
	commandList->DrawInstanced(numVertex, 1, 0, 0);
}

void ResDx::ResDxCommandListDirect::DrawIndexedInstanced(UINT numIndex)
{
	commandList->DrawIndexedInstanced(numIndex, 1, 0, 0, 0);
}

void ResDx::ResDxCommandListDirect::DrawIndexedInstanced(UINT numIndex, UINT numInstance)
{
	commandList->DrawIndexedInstanced(numIndex, numInstance, 0, 0, 0);
}

void ResDx::ResDxCommandListDirect::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
{
	commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void ResDx::ResDxCommandListDirect::CopyGpuResource(ID3D12Resource* dstResource, ID3D12Resource* srcResource, D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcFootprint)
{
	auto footprint = srcFootprint;
	auto cmdList = commandList;
	D3D12_TEXTURE_COPY_LOCATION copyLoc = {};//コピー元
	copyLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	copyLoc.PlacedFootprint = footprint;
	copyLoc.PlacedFootprint.Offset = 0;
	copyLoc.pResource = srcResource;

	D3D12_TEXTURE_COPY_LOCATION targetLoc = {};//コピー先
	targetLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	targetLoc.SubresourceIndex = 0;
	targetLoc.pResource = dstResource;

	cmdList->CopyTextureRegion(&targetLoc, 0, 0, 0, &copyLoc, nullptr);
}

ResDx::ResDxCommandList* ResDx::ResDxCommandListDirect::Get()
{
	return resDxCommandList;
}

ResDx::ResDxCommandListDirect::operator ResDx::ResDxCommandList*()const
{
	return resDxCommandList;
}

void ResDx::ResDxCommandListDirect::operator=(const ResDxCommandListDirect& commandList)
{
	this->commandList = commandList.commandList;
	this->resDxCommandList = commandList.resDxCommandList;
}

void ResDx::ResDxCommandListDirect::operator=(ResDxCommandListDirect&& commandList)
{
	this->commandList = std::move(commandList.commandList);
	this->resDxCommandList = std::move(commandList.resDxCommandList);
	commandList.commandList = nullptr;
	commandList.resDxCommandList = nullptr;

}

void ResDx::ResDxCommandListCompute::SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
{
	commandList->SetDescriptorHeaps(1, &descriptorHeap);
}

void ResDx::ResDxCommandListCompute::SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps)
{
	commandList->SetDescriptorHeaps(numDescriptorHeap, descriptorHeaps);
}

void ResDx::ResDxCommandListCompute::SetComputeRootSignature(ID3D12RootSignature* rootSignature)
{
	commandList->SetComputeRootSignature(rootSignature);
}

void ResDx::ResDxCommandListCompute::SetPipelineState(ID3D12PipelineState* pipelineState)
{
	commandList->SetPipelineState(pipelineState);
}

void ResDx::ResDxCommandListCompute::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
{
	commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void ResDx::ResDxCommandListCompute::ExecuteBundle(ID3D12GraphicsCommandList* bundle)
{
	commandList->ExecuteBundle(bundle);
}

ResDx::ResDxCommandListCompute::operator ResDx::ResDxCommandList*()const
{
	return resDxCommandList;
}

void ResDx::ResDxCommandListCompute::operator=(const ResDxCommandListCompute& o)
{
	resDxCommandList = o.resDxCommandList;
	commandList = o.commandList;
}

void ResDx::ResDxCommandListCompute::operator=(ResDxCommandListCompute&& o)
{
	resDxCommandList = std::move(o.resDxCommandList);
	commandList = std::move(o.commandList);
	o.resDxCommandList = nullptr;
	o.commandList = nullptr;
}

void ResDx::ResDxCommandListBundle::SetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW* vertexBufferView)
{
	commandList->IASetVertexBuffers(0, 1, vertexBufferView);
}

void ResDx::ResDxCommandListBundle::SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView)
{
	commandList->IASetIndexBuffer(indexBufferView);
}

void ResDx::ResDxCommandListBundle::SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
{
	commandList->SetDescriptorHeaps(1, &descriptorHeap);
}

void ResDx::ResDxCommandListBundle::SetDescriptorHeaps(UINT numDescriptorHeap, ID3D12DescriptorHeap** descriptorHeaps)
{
	commandList->SetDescriptorHeaps(numDescriptorHeap, descriptorHeaps);
}

void ResDx::ResDxCommandListBundle::SetGraphicsDescriptorTable(UINT indexRootParameter, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle)
{
	commandList->SetGraphicsRootDescriptorTable(indexRootParameter, descriptorHandle);
}

void ResDx::ResDxCommandListBundle::SetGraphicsRootSignature(ID3D12RootSignature* rootSignature)
{
	commandList->SetGraphicsRootSignature(rootSignature);
}

void ResDx::ResDxCommandListBundle::SetComputeRootSignature(ID3D12RootSignature* rootSignature)
{
	commandList->SetComputeRootSignature(rootSignature);
}

void ResDx::ResDxCommandListBundle::SetGraphicsRoot32BitConstant(UINT rootParameter, UINT src, UINT dstOffset)
{
	commandList->SetGraphicsRoot32BitConstant(rootParameter, src, dstOffset);
}

void ResDx::ResDxCommandListBundle::SetGraphicsRoot32BitConstants(UINT rootParameter, UINT num32BitValue, const void* src, UINT dstOffset)
{
	commandList->SetGraphicsRoot32BitConstants(rootParameter, num32BitValue, src, dstOffset);
}

void ResDx::ResDxCommandListBundle::SetPipelineState(ID3D12PipelineState* pipelineState)
{
	commandList->SetPipelineState(pipelineState);
}

void ResDx::ResDxCommandListBundle::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV)
{
	commandList->OMSetRenderTargets(1, descriptorHandleRTV, FALSE, descriptorHandleDSV);
}

void ResDx::ResDxCommandListBundle::SetRenderTargets(UINT numDescriptorRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleRTV, D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandleDSV)
{
	commandList->OMSetRenderTargets(numDescriptorRTV, descriptorHandleRTV, FALSE, descriptorHandleDSV);
}

void ResDx::ResDxCommandListBundle::SetScissorRect(D3D12_RECT* rect)
{
	commandList->RSSetScissorRects(1, rect);
}

void ResDx::ResDxCommandListBundle::SetViewport(D3D12_VIEWPORT* viewport)
{
	commandList->RSSetViewports(1, viewport);
}

void ResDx::ResDxCommandListBundle::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	commandList->IASetPrimitiveTopology(topology);
}

void ResDx::ResDxCommandListBundle::DrawInstanced(UINT numVertex)
{
	commandList->DrawInstanced(numVertex, 1, 0, 0);
}

void ResDx::ResDxCommandListBundle::DrawIndexedInstanced(UINT numIndex)
{
	commandList->DrawIndexedInstanced(numIndex, 1, 0, 0, 0);
}

void ResDx::ResDxCommandListBundle::DrawIndexedInstanced(UINT numIndex, UINT numInstance)
{
	commandList->DrawIndexedInstanced(numIndex, numInstance, 0, 0, 0);
}

void ResDx::ResDxCommandListBundle::Dispatch(UINT threadGroupCountX, UINT threadGroupCountY, UINT threadGroupCountZ)
{
	commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

ResDx::ResDxCommandListBundle::operator ResDx::ResDxCommandList*()const
{
	return resDxCommandList;
}

void ResDx::ResDxCommandListBundle::operator=(const ResDxCommandListBundle& o)
{
	resDxCommandList = o.resDxCommandList;
	commandList = o.commandList;
}

void ResDx::ResDxCommandListBundle::operator=(ResDxCommandListBundle&& o)
{
	resDxCommandList = std::move(o.resDxCommandList);
	commandList = o.commandList;
}


void ResDx::ResDxContext2::CreateVertexBuffer(int bufferSize, int stride,ID3D12Resource** buffer)
{
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	auto prop = CD3DX12_HEAP_DESC(bufferSize, D3D12_HEAP_TYPE_UPLOAD);
	auto result = device->CreateCommittedResource(
		&prop.Properties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	if (FAILED(result))
		MessageBoxA(nullptr, "頂点バッファの生成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext2::CreateIndexBuffer(int bufferSize, int stride,ID3D12Resource** buffer)
{
	if (stride == 2)
		bufferSize *= 2;
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	auto result = device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer));

	if (FAILED(result))
		MessageBoxA(nullptr, "インデックスバッファの生成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext2::CreateTextureBuffer(int width, int height, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state, ID3D12Resource** buffer)
{
	if (*buffer)
		(*buffer)->Release();
	*buffer = nullptr;
	auto resDesc = CD3DX12_RESOURCE_DESC(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		flags);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	auto result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		state,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	if (FAILED(result))
	{
		MessageBoxA(nullptr, "テクスチャバッファの生成に失敗しました", "error", MB_OK);
		return;
	}
}

void ResDx::ResDxContext2::CreateRenderTargetBuffer(int width, int height, DXGI_FORMAT format, ID3D12Resource** buffer)
{
	if (*buffer)
		(*buffer)->Release();
	*buffer = nullptr;
	auto resDesc = CD3DX12_RESOURCE_DESC(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	auto result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	if (FAILED(result))
	{
		MessageBoxA(nullptr, "テクスチャバッファの生成に失敗しました", "error", MB_OK);
		return;
	}
}

void ResDx::ResDxContext2::CreateDepthStencilBuffer(int width, int height, DXGI_FORMAT format,ID3D12Resource** buffer)
{

	if (*buffer)
		(*buffer)->Release();
	*buffer = nullptr;
	auto resDesc = CD3DX12_RESOURCE_DESC(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	D3D12_CLEAR_VALUE dsvClearValue = {};
	dsvClearValue.Format = format;
	dsvClearValue.DepthStencil.Depth = 1.0f;
	dsvClearValue.DepthStencil.Stencil = 0;

	auto result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&dsvClearValue,
		IID_PPV_ARGS(buffer)
	);

	if (FAILED(result))
	{
		MessageBoxA(nullptr, "テクスチャバッファの生成に失敗しました", "error", MB_OK);
		return;
	}
}

void ResDx::ResDxContext2::CreateConstantBuffer(int bufferSize, ID3D12Resource** buffer)
{
	auto alignedBbufferSize = Alignmentof(bufferSize);
	auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC cbDesc;
	cbDesc.Format = DXGI_FORMAT_UNKNOWN;
	cbDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedBbufferSize);
	device->CreateCommittedResource(
		&heap,
		D3D12_HEAP_FLAG_NONE,
		&cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

}

void ResDx::ResDxContext2::CreateUploadBuffer(int width, int height, DXGI_FORMAT format, ID3D12Resource** buffer, UINT64* puploadSize , D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pfootprint)
{
	CD3DX12_RESOURCE_DESC texBufDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		format,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_NONE
	);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
	unsigned int rowSizeBytes = 0;
	UINT64 uploadSize = 0;
	UINT64 totalBytes = 0;
	device->GetCopyableFootprints(&texBufDesc, 0, 1, 0, &footprint, &rowSizeBytes, &uploadSize, &totalBytes);

	auto texBufRowPitch = footprint.Footprint.RowPitch;
	auto upResDesc = CD3DX12_RESOURCE_DESC::Buffer(texBufRowPitch * texBufDesc.Height);
	auto upHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto result = device->CreateCommittedResource(
		&upHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&upResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);
	if (pfootprint)
		*pfootprint = footprint;
	if (puploadSize)
		*puploadSize = uploadSize;
}

void ResDx::ResDxContext2::CreateStructuredBuffer(int sizeOfElem, int numOfElem, ID3D12Resource** buffer)
{
	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer((UINT64)(sizeOfElem * numOfElem));

	auto hr = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer)
	);
}

void ResDx::ResDxContext2::CreateRWStructuredBuffer(int sizeOfElem, int numOfElem,ID3D12Resource** buffer,ID3D12Resource** counter)
{
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(sizeOfElem * numOfElem);
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	int bufferNo = 0;

	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProp.CreationNodeMask = 1;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProp.VisibleNodeMask = 1;

	//リソース作成
	auto result = device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(buffer)
	);

	desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT));
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	//カウンターリソース作成
	if (counter)
		result = device->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(counter)
		);

	if (FAILED(result))
		MessageBoxA(nullptr, "アンオーダーアクセスバッファの生成に失敗しました", "error", MB_OK);

}

void ResDx::ResDxContext2::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_DESC& desc, ID3D12DescriptorHeap** descHeap)
{
	auto result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(descHeap));
	if (FAILED(result))
		MessageBoxA(nullptr, "ディスクリプタヒープの生成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext2::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateConstantBufferView(&desc, handle);
}

void ResDx::ResDxContext2::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateShaderResourceView(resource, &desc, handle);
}

void ResDx::ResDxContext2::CreateUnorderedAccessView(ID3D12Resource* resource, ID3D12Resource* counter, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateUnorderedAccessView(resource, counter, &desc, handle);
}

void ResDx::ResDxContext2::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateRenderTargetView(resource, desc, handle);
}

void ResDx::ResDxContext2::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	device->CreateDepthStencilView(resource, desc, handle);
}

void ResDx::ResDxContext2::CreateRootSignature(UINT nodeMask, const void* blobWithRootSignature, UINT blobLenghInBytes, ID3D12RootSignature** rootSignature)
{
	auto result = device->CreateRootSignature(nodeMask, blobWithRootSignature, blobLenghInBytes, IID_PPV_ARGS(rootSignature));
	if (FAILED(result))
		MessageBoxA(nullptr, "ルートシグネチャの作成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext2::CreateGraphicsPipelineState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState)
{
	auto result = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineState));
	if (FAILED(result))
		MessageBoxA(nullptr, "パイプラインステートの作成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext2::CreateComputePipelineState(D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, ID3D12PipelineState** pipelineState)
{
	auto result = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(pipelineState));
	if (FAILED(result))
		MessageBoxA(nullptr, "パイプラインステートの作成に失敗しました", "error", MB_OK);
}

void ResDx::ResDxContext2::operator=(const ResDxContext2& o)
{
	device = o.device;
}

void ResDx::ResDxContext2::operator=(ResDxContext2&& o)
{
	device = std::move(o.device);
	o = nullptr;
}

void ResDx::ResDxContext2::CopyDescriptorSimple(UINT numDescriptor, ResDxDescriptorHeap* dst, UINT dstStartIndex, ResDxDescriptorHeap* src, UINT srcStartIndex, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	assert(numDescriptor < RES_DX_MAX_NUM_COPY_DESCRIPTOR);
	const auto dstCpuHandle = dst->GetCpuHandle(dstStartIndex);
	const auto srcCpuHandle = src->GetCpuHandle(srcStartIndex);
	device->CopyDescriptorsSimple(numDescriptor, dstCpuHandle, srcCpuHandle, type);
}

void ResDx::ResDxContext2::CopyDescriptors(const UINT numDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE dstHandles[], const D3D12_CPU_DESCRIPTOR_HANDLE srcHandles[], const UINT numDescriptors[], D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	device->CopyDescriptors(numDescriptorRanges, dstHandles, numDescriptors, numDescriptorRanges, srcHandles, numDescriptors, type);
}

UINT ResDx::ResDxContext2::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	return device->GetDescriptorHandleIncrementSize(type);
}

D3D12_PLACED_SUBRESOURCE_FOOTPRINT ResDx::ResDxContext2::GetPlacedFootprint(ID3D12Resource* resource, UINT64& uploadSize, UINT64& totalByte)
{
	auto desc = resource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, &uploadSize, &totalByte);
	return footprint;
}

void ResDx::ResDxScissorRect::Init()
{
	rect = D3D12_RECT();
}

void ResDx::ResDxScissorRect::SetRect(LONG left, LONG top, LONG right, LONG bottom)
{
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;
}

void ResDx::ResDxScissorRect::SetRectDefault()
{
	SetRect(0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
}

ResDx::ResDxScissorRect::operator D3D12_RECT* ()
{
	return &rect;
}

void ResDx::ResDxScissorRect::operator=(const ResDxScissorRect&o)
{
	rect = o.rect;
}

void ResDx::ResDxScissorRect::operator=(ResDxScissorRect&& o)
{
	rect = o.rect;
	o.rect = D3D12_RECT();
}

void ResDx::ResDxViewport::Init()
{
	viewport = D3D12_VIEWPORT();
}

void ResDx::ResDxViewport::SetViewort(FLOAT left, FLOAT top, FLOAT width, FLOAT height, FLOAT maxDepth, FLOAT minDepth)
{
	viewport.TopLeftX = left;
	viewport.TopLeftY = top;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MaxDepth = maxDepth;
	viewport.MinDepth = minDepth;
}

void ResDx::ResDxViewport::SetViewportDefault()
{
	SetViewort(0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 1.0f, 0.0f);
}

ResDx::ResDxViewport::operator D3D12_VIEWPORT* ()
{
	return &viewport;
}

void ResDx::ResDxViewport::operator=(const ResDxViewport& o)
{
	viewport = o.viewport;
}

void ResDx::ResDxViewport::operator=(ResDxViewport&& o)
{
	viewport = std::move(o.viewport);
	o.viewport = D3D12_VIEWPORT();
}
