//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Loader.h"
#include <Engine/Core.h>
#include <Engine/Buffer.h>
#include <Engine/Vertex.h>
#include <Engine/Mesh.h>
#include <Others/Console.h>
#include <Others/StringUtils.h>

#include <mio.hpp>

namespace meteor 
{
	static std::string FileHeader = "GMDL V1.00";
	static constexpr char* ENCODING = "BIG5HKSCS";

	auto GmbLoader::load(const std::string& name, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>& outMeshes) -> void
	{
		GmbFile gmbFile;

		mio::mmap_source mmap(name);
		MAPLE_ASSERT(mmap.is_open(), "open gmb file failed");
		maple::BinaryReader binaryReader((const uint8_t*)mmap.data(), mmap.length());
		binaryReader.skip(FileHeader.length());
		gmbFile.texturesCount = binaryReader.read<int32_t>();

		for (int32_t i = 0; i < gmbFile.texturesCount; i++)
		{
			int32_t count = binaryReader.read<int32_t>();
			auto fileName = binaryReader.readBytes(count);
			auto outChar = std::unique_ptr<char[]>(new char[count * 2]);
			maple::StringUtils::codeConvert(ENCODING, "UTF-8", (const char*)fileName.get(), count, outChar.get(), count * 2);
			std::string textureName(outChar.get());
			LOGI("TextureName : {0}", textureName);
			gmbFile.texturesNames.emplace_back(std::move(textureName));
		}

		gmbFile.shaderCount = binaryReader.read<int32_t>();
		for (int32_t j = 0; j < gmbFile.shaderCount; j++)
		{
			auto& shader = gmbFile.shaders.emplace_back();
			shader.textureArg0 = binaryReader.read<int32_t>();
			int32_t count = binaryReader.read<int32_t>();
			auto fileName = binaryReader.readBytes(count);
			shader.textureArg1 = std::string((const char*)fileName.get(), count);
			shader.twoSideArg0 = binaryReader.read<int8_t>();

			count = binaryReader.read<int32_t>();
			fileName = binaryReader.readBytes(count);
			std::string str((const char*)fileName.get(), count);

			auto strs = maple::StringUtils::split(str, " ");
			shader.blendArg0 = strs[0];
			shader.blendArg1 = strs[1];
			shader.blendArg2 = strs[2];
			shader.opaqueArg0 = binaryReader.read<float>();
		}

		gmbFile.sceneObjectsCount = binaryReader.read<int32_t>();
		gmbFile.dummeyObjectsCount = binaryReader.read<int32_t>();
		gmbFile.verticesCount = binaryReader.read<int32_t>();
		gmbFile.facesCount = binaryReader.read<int32_t>();

		for (int32_t k = 0; k < gmbFile.sceneObjectsCount; k++)
		{
			int32_t count = binaryReader.read<int32_t>();
			//原版是 繁体中文制作.
			auto bufferName = binaryReader.readBytes(count);
			auto outChar = std::unique_ptr<char[]>(new char[count * 2]);
			maple::StringUtils::codeConvert(ENCODING, "UTF-8", (const char*)bufferName.get(), count, outChar.get(), count * 2);
			std::string meshName(outChar.get());

			int32_t numOfVertex = binaryReader.read<int32_t>();
			int32_t numOfFaces = binaryReader.read<int32_t>();

			/**
			*   Vector3 pos ;
			*   Vector3 normal;
			*   Color color;
			*   Vector2 uv;
			*/
			std::vector<maple::Vertex> vertices;
			std::vector<uint32_t> indicies;

			for (auto i = 0; i < numOfVertex; i++)
			{
				auto& vertex = vertices.emplace_back();
				vertex.pos = binaryReader.readVec3();//4
				std::swap(vertex.pos.y, vertex.pos.z);
				vertex.normal = binaryReader.readVec3();//4
				vertex.color.r = binaryReader.read<uint8_t>() / 255.f;
				vertex.color.g = binaryReader.read<uint8_t>() / 255.f;
				vertex.color.b = binaryReader.read<uint8_t>() / 255.f;
				vertex.color.a = binaryReader.read<uint8_t>() / 255.f;
				vertex.texCoord = binaryReader.readVec2();
			}

			for (int32_t n = 0; n < numOfFaces; n++)
			{
				binaryReader.skip(4);//material ID ?

				uint32_t a = binaryReader.read<int32_t>();
				uint32_t b = binaryReader.read<int32_t>();
				uint32_t c = binaryReader.read<int32_t>();

				indicies.emplace_back(a);
				indicies.emplace_back(c);
				indicies.emplace_back(b);

				binaryReader.skip(12);//3 * sizeof(int32_t)
			}

			auto mesh = std::make_shared<maple::Mesh>(indicies, vertices);
			mesh->setName(meshName);
			gmbFile.meshes.emplace_back(mesh);

			outMeshes.emplace(meshName, mesh);
		}
	}
}