//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Loader.h"
#include <Engine/Core.h>
#include <Engine/Buffer.h>
#include <Engine/Vertex.h>
#include <Engine/Mesh.h>
#include <Engine/Material.h>
#include <Others/Console.h>
#include <Others/StringUtils.h>
#include <Scene/Component/Transform.h>

#include <mio.hpp>
#include <fstream>
#include <sstream>

namespace meteor 
{
	static std::string FileHeader = "GMDL V1.00";
	static constexpr char* ENCODING = "BIG5HKSCS";
	namespace 
	{
		inline auto loadDesFile(DesFile& des, const std::string& fileName)
		{
			des.sceneItems.clear();
			MAPLE_ASSERT(fileName != "", "file name should not be null");

			std::ifstream desFile;
			desFile.open(fileName);

			std::string line;
			while (std::getline(desFile, line))
			{
				if (maple::StringUtils::startWith(line, "#"))
					continue;

				auto keyValue = maple::StringUtils::split(line, " ");
				if (keyValue.empty())
				{
					LOGE("incomplete line : {0}", line);
					continue;
				}

				if (keyValue[0] == "SceneObjects" && keyValue[2] == "DummeyObjects")
				{
					des.objectCount = std::stoi(keyValue[1]);
					des.dummyCount = std::stoi(keyValue[3]);
					continue;
				}

				if (keyValue[0] == "Object")
				{
					DesItem& attr = des.sceneItems[keyValue[1]];
					attr.name = keyValue[1];

					bool readLeftToken = false;
					int32_t leftTokenStack = 0;
					std::string obj;
					while (std::getline(desFile, obj))
					{
						maple::StringUtils::trim(obj);

						auto keyValue = maple::StringUtils::split(obj, " ");

						if (keyValue.empty())
							continue;
						if (keyValue[0] == "#")
							continue;
						if (keyValue[0] == "{")
						{
							readLeftToken = true;
							leftTokenStack++;
							continue;
						}
						//Z UP TO Y UP x轴z轴取反
						if (keyValue[0] == "Position:" && readLeftToken && leftTokenStack == 1)
						{
							attr.pos.x = std::floorf(10000 * std::stof(keyValue[1])) / 10000.0f;
							attr.pos.z = std::floorf(10000 * std::stof(keyValue[2])) / 10000.0f;
							attr.pos.y = std::floorf(10000 * std::stof(keyValue[3])) / 10000.0f;
						}
						if (keyValue[0] == "Quaternion:" && readLeftToken && leftTokenStack == 1)
						{
							attr.quat.w = std::floorf(10000 * std::stof(keyValue[1])) / 10000.0f;
							attr.quat.x = -std::floorf(10000 * std::stof(keyValue[2])) / 10000.0f;
							attr.quat.y = -std::floorf(10000 * std::stof(keyValue[4])) / 10000.0f;
							attr.quat.z = -std::floorf(10000 * std::stof(keyValue[3])) / 10000.0f;
						}
						if (keyValue[0] == "TextureAnimation:" && readLeftToken && leftTokenStack == 1)
						{
							attr.useTextAnimation = (std::stoi(keyValue[1]) == 1);
							attr.textAnimation.x = std::stof(keyValue[2]);
							attr.textAnimation.y = std::stof(keyValue[3]);
						}
						if (keyValue[0] == "Custom:" && readLeftToken && leftTokenStack == 1)
						{
							std::string obj;
							while (std::getline(desFile, obj))
							{
								if (obj == "{")
								{
									leftTokenStack++;
									continue;
								}
								if (obj == "}")
								{
									leftTokenStack--;
									if (leftTokenStack == 1)
										break;
									continue;
								}
								if (obj == "#" || obj == "")
									continue;
								attr.custom.emplace_back(obj);
							}
						}
						if (keyValue[0] == "}")
						{
							leftTokenStack--;
							if (leftTokenStack == 0)
								break;
						}
					}
				}
			}
			LOGI("Parse {0} finished", fileName);
		}

		inline auto loadGmbFile(GmbFile& gmbFile, DesFile& desFile, const std::string& fileName, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>& outMeshes)
		{
			mio::mmap_source mmap(fileName);
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

				maple::component::Transform transform;

				if (auto iter = desFile.sceneItems.find(meshName); iter != desFile.sceneItems.end()) 
				{
					transform.setLocalPosition(iter->second.pos);
					transform.setLocalOrientation(iter->second.quat);
					transform.setWorldMatrix(glm::mat4(1.0));
				}

				for (auto i = 0; i < numOfVertex; i++)
				{
					auto& vertex = vertices.emplace_back();
					vertex.pos = binaryReader.readVec3();//4
					std::swap(vertex.pos.y, vertex.pos.z);

					vertex.pos = transform.getWorldMatrix() * glm::vec4(vertex.pos, 1);

					vertex.normal = transform.getWorldMatrix() * glm::vec4(binaryReader.readVec3(), 1.0);//4

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

				auto material = std::make_shared<maple::Material>();

				maple::PBRMataterialTextures textures;
				textures.albedo = maple::Texture2D::create(gmbFile.texturesNames[0], "meteor/textures/" + gmbFile.texturesNames[0]);
				material->setTextures(textures);

				auto mesh = std::make_shared<maple::Mesh>(indicies, vertices);
				mesh->setName(meshName);
				mesh->setMaterial(material);
				outMeshes.emplace(meshName, mesh);
			}
		}

		inline auto loadGmcFile(GmcFile& gmcFile, const std::string& fileName, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>& outMeshes)
		{
			std::ifstream desFile;
			desFile.open(fileName);

			std::string line;
			while (std::getline(desFile, line))
			{
				if (maple::StringUtils::startWith(line, "#"))
					continue;

				auto keyValue = maple::StringUtils::split(line, " ");
				
				if (keyValue[0] == "Textures") 
				{
					gmcFile.texturesCount = std::stoi(keyValue[1]);
					std::string line;
					while (std::getline(desFile, line)) 
					{
						maple::StringUtils::trim(line);
						maple::StringUtils::trim(line,"\t");
						if(line == "{")
							continue;
						if(line == "}")
							break;

						gmcFile.texturesNames.emplace_back(line);
					}
					continue;
				}

				if (keyValue[0] == "Shaders")
				{
					gmcFile.shaderCount = std::stoi(keyValue[1]);

					for (auto i = 0;i<gmcFile.shaderCount;i++)
					{
						auto& shader = gmcFile.shaders.emplace_back();
						std::string line;
						while (std::getline(desFile, line)) 
						{
							if(line == "{")
								continue;
							if(line == "}")
								break;

							std::stringstream strstream(line);
							std::string str = "";

							strstream >> str;

							if (str == "Texture") 
							{
								strstream  >> shader.textureArg0 >> shader.textureArg1;
							}

							if (str == "TwoSide")
							{
								strstream >> shader.twoSideArg0;
							}

							if (str == "Blend")
							{
								strstream >> shader.blendArg0 >> shader.blendArg1 >> shader.blendArg2;
							}

							if (str == "Opaque")
							{
								strstream >> shader.opaqueArg0;
							}
						}
					}
					continue;
				}

				if (keyValue[0] == "SceneObjects") 
				{
					gmcFile.sceneObjectsCount = std::stoi(keyValue[1]);
					gmcFile.dummeyObjectsCount = std::stoi(keyValue[3]);
					continue;
				}

				if (keyValue[0] == "Vertices")
				{
					gmcFile.verticesCount = std::stoi(keyValue[1]);
					gmcFile.facesCount = std::stoi(keyValue[3]);
					continue;
				}

				for (auto i = 0;i< gmcFile.sceneObjectsCount;i++)
				{
					if (i > 0) 
					{
						std::getline(desFile, line);
					}

					auto keyValue = maple::StringUtils::split(line, " ");
					std::string meshName = keyValue[1];

					std::getline(desFile, line);//{

					std::getline(desFile, line);// Vertices 156 Faces 138

					std::vector<maple::Vertex> vertices;
					std::vector<uint32_t> indices;

					int32_t textureId = -1;

					while (std::getline(desFile, line))
					{
						if (line == "}")
							break;
						maple::StringUtils::trim(line);
						maple::StringUtils::trim(line, "\t");

						auto & vertex = vertices.emplace_back();

						if (maple::StringUtils::startWith(line,"v")) 
						{
							std::stringstream strstream(line);
							std::string str = "";
							uint8_t r, g, b;

							strstream >> str >> vertex.pos.x >> vertex.pos.z >> vertex.pos.y
								>> str >> vertex.normal.x >> vertex.normal.z >> vertex.normal.y
								>> str >> r >> g >> b
								>> str >> vertex.texCoord.x >> vertex.texCoord.y;

							vertex.color = { r / 255.f, g / 255.f ,b/ 255.f, 1.f};
						}


						if (maple::StringUtils::startWith(line, "f"))
						{
							std::string str = "";
							uint32_t b, c, d;
							std::stringstream strstream(line);

							strstream >> str >> textureId >> b >> c >> d;

							indices.emplace_back(b);
							indices.emplace_back(d);
							indices.emplace_back(c);
						}
					}

					auto mesh = std::make_shared<maple::Mesh>(indices,vertices);
					mesh->setName(meshName);
					if (textureId > -1) 
					{
						auto material = std::make_shared<maple::Material>();
						maple::PBRMataterialTextures textures;
						textures.albedo = maple::Texture2D::create(gmcFile.texturesNames[textureId], "meteor/textures/" + gmcFile.texturesNames[textureId]);
						material->setTextures(textures);
						mesh->setMaterial(material);
					}
					outMeshes[meshName] = mesh;
				}
			}
		}
	}

	auto GmbLoader::load(const std::string& name, const std::string& extension, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>& outMeshes) -> void
	{
		if (extension == "gmb") 
		{
			auto objName = maple::StringUtils::getFileNameWithoutExtension(name);
			auto& sceneObj = MeteorSceneObjectCache::get(objName);
			loadDesFile(sceneObj.desFile, maple::StringUtils::removeExtension(name) + ".des");
			loadGmbFile(sceneObj.gmbFile, sceneObj.desFile, name, outMeshes);
		}
		else if (extension == "gmc")
		{
			auto& gmcFile = GmcFileCache::get(name);
			loadGmcFile(gmcFile,name, outMeshes);
		}
	}
}