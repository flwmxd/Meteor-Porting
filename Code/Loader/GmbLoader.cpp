//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "GmbLoader.h"
#include <Engine/Core.h>
#include <Engine/Buffer.h>
#include <Engine/Vertex.h>
#include <Engine/Mesh.h>
#include <Engine/Material.h>
#include <Others/Console.h>
#include <Others/StringUtils.h>
#include <FileSystem/File.h>
#include <FileSystem/MeshResource.h>
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

				maple::StringUtils::trim(line);
				maple::StringUtils::replace(line, "\t", " ");
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

						if (maple::StringUtils::startWith(obj, "#"))
							continue;
						if (maple::StringUtils::startWith(obj, "{"))
						{
							readLeftToken = true;
							leftTokenStack++;
							continue;
						}
						std::string temp;
						std::stringstream sstream(obj);
						if (maple::StringUtils::startWith(obj, "Position:") && readLeftToken && leftTokenStack == 1)
						{
							sstream >> temp >> attr.pos.x >> attr.pos.z >> attr.pos.y;
						}
						if (maple::StringUtils::startWith(obj, "Quaternion:") && readLeftToken && leftTokenStack == 1)
						{
							sstream >> temp >> attr.quat.w >> attr.quat.x >> attr.quat.z >> attr.quat.y;
							attr.quat.x *= -1.f;
							attr.quat.y *= -1.f;
							attr.quat.z *= -1.f;
						}

						if (maple::StringUtils::startWith(obj, "TextureAnimation:") && readLeftToken && leftTokenStack == 1)
						{
							sstream >> temp >> attr.useTextAnimation >> attr.textAnimation.x >> attr.textAnimation.y;
						}
						if (maple::StringUtils::startWith(obj, "Custom:") && readLeftToken && leftTokenStack == 1)
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

		inline auto loadGmbFile(GModelFile& gmbFile, DesFile& desFile, const std::string& fileName, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>& outMeshes)
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


			std::vector<std::shared_ptr<maple::Material>> catchMaterials;
			catchMaterials.emplace_back(std::make_shared<maple::Material>());
			for (auto i = 0; i < gmbFile.shaderCount; i++)
			{
				auto& shader = gmbFile.shaders[i];
				auto material = std::make_shared<maple::Material>();
				if (shader.textureArg0 != -1)
				{
					maple::PBRMataterialTextures textures;
					const auto & textureName = gmbFile.texturesNames[shader.textureArg0];
					if(!maple::StringUtils::endWith(textureName,".ifl"))
						textures.albedo = maple::Texture2D::create(textureName, "meteor/textures/" + textureName);
					material->setTextures(textures);
				}
				catchMaterials.emplace_back(material);
			}

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

				std::vector<std::shared_ptr<maple::Material>> materials;
				

				std::vector<uint32_t> idxOffset;
				int32_t textureId = -1;
				uint32_t offset = 0;
				int32_t subMeshCount = 0;

				std::vector<uint32_t> shaderIds;

				for (int32_t n = 0; n < numOfFaces; n++)
				{
					int32_t currentTextureId = binaryReader.read<int32_t>();
					uint32_t a = binaryReader.read<int32_t>();
					uint32_t b = binaryReader.read<int32_t>();
					uint32_t c = binaryReader.read<int32_t>();

					indicies.emplace_back(a);
					indicies.emplace_back(c);
					indicies.emplace_back(b);
					
					if (textureId != currentTextureId)
					{
						if (subMeshCount > 0)
						{
							idxOffset.emplace_back(offset);
						}

						materials.emplace_back(catchMaterials[currentTextureId + 1]);

						textureId = currentTextureId;
						subMeshCount++;
					}
					offset += 3;
					binaryReader.skip(12);//3 * sizeof(int32_t)
				}

				maple::Mesh::generateNormals(vertices, indicies);
				maple::Mesh::generateTangents(vertices, indicies);
				auto mesh = std::make_shared<maple::Mesh>(indicies, vertices);

				mesh->setName(meshName);
				mesh->setMaterial(materials);
				mesh->setSubMeshCount(subMeshCount);
				mesh->setSubMeshIndex(idxOffset);
				MAPLE_ASSERT(subMeshCount == materials.size(), "the count subMesh is different from materials");
				outMeshes.emplace(meshName, mesh);
			}
		}

		inline auto loadGmcFile(GModelFile& gmcFile, DesFile& desFile, const std::string& fileName, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>& outMeshes)
		{
			std::ifstream gmcIn;
			gmcIn.open(fileName);
			std::string line;

			std::vector<std::shared_ptr<maple::Material>> catchMaterials;
			
			while (std::getline(gmcIn, line))
			{
				if (maple::StringUtils::startWith(line, "#"))
					continue;

				auto keyValue = maple::StringUtils::split(line, " ");
				
				if (keyValue[0] == "Textures") 
				{
					gmcFile.texturesCount = std::stoi(keyValue[1]);
					std::string line;
					while (std::getline(gmcIn, line)) 
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

					for (auto i = 0; i < gmcFile.shaderCount; i++)
					{
						auto& shader = gmcFile.shaders.emplace_back();
						std::string line;
						while (std::getline(gmcIn, line)) 
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
				

					catchMaterials.emplace_back(std::make_shared<maple::Material>());
					for (auto i = 0; i < gmcFile.shaderCount; i++)
					{
						auto& shader = gmcFile.shaders[i];
						auto material = std::make_shared<maple::Material>();
						if (shader.textureArg0 != -1)
						{
							maple::PBRMataterialTextures textures;
							textures.albedo = maple::Texture2D::create(gmcFile.texturesNames[shader.textureArg0], "meteor/textures/" + gmcFile.texturesNames[shader.textureArg0]);
							material->setTextures(textures);
						}
						catchMaterials.emplace_back(material);
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
						std::getline(gmcIn, line);
					}

					auto keyValue = maple::StringUtils::split(line, " ");
					std::string meshName = keyValue[1];

					std::getline(gmcIn, line);//{

					std::getline(gmcIn, line);// Vertices 156 Faces 138

					std::vector<maple::Vertex> vertices;
					std::vector<uint32_t> indices;


					std::vector<std::shared_ptr<maple::Material>> materials;
					std::vector<std::shared_ptr<maple::Material>> catchMaterials;

					catchMaterials.resize(gmcFile.shaderCount);
					std::vector<uint32_t> idxOffset;
					int32_t textureId = -1;
					uint32_t offset = 0;
					int32_t subMeshCount = 0;

					while (std::getline(gmcIn, line))
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
							int32_t currentTextureId = 0;
							strstream >> str >> currentTextureId >> b >> c >> d;

							indices.emplace_back(b);
							indices.emplace_back(d);
							indices.emplace_back(c);
							
							if (textureId != currentTextureId)
							{
								if (subMeshCount > 0)
								{
									idxOffset.emplace_back(offset);
								}
								materials.emplace_back(catchMaterials[currentTextureId + 1]);
								textureId = currentTextureId;
								subMeshCount++;
							}
							offset += 3;
						}
					}
					maple::Mesh::generateNormals(vertices, indices);
					maple::Mesh::generateTangents(vertices, indices);
					auto mesh = std::make_shared<maple::Mesh>(indices,vertices);
					mesh->setName(meshName);
					mesh->setMaterial(materials);
					mesh->setSubMeshCount(subMeshCount);
					mesh->setSubMeshIndex(idxOffset);
					outMeshes[meshName] = mesh;

					MAPLE_ASSERT(subMeshCount == materials.size(), "the count subMesh is different from materials");
				}
			}
		}
		
		inline auto loadFmc(const std::string& fileName) 
		{
			auto fmc = std::make_shared<FmcFile>();
			std::ifstream fmcIn;
			fmcIn.open(fileName);

			std::string line;
			while (std::getline(fmcIn, line))
			{
				if (maple::StringUtils::startWith(line, "#"))
					continue;

				if (maple::StringUtils::startWith(line, "SceneObjects"))
				{
					std::stringstream sstream(line);
					std::string str;
					sstream >> str >> fmc->scemeObjCount >> str >> fmc->dummyObjCount;
					continue;
				}

				if (maple::StringUtils::startWith(line, "FPS"))
				{
					std::stringstream sstream(line);
					std::string str;
					sstream >> str >> fmc->fps >> str >> fmc->frames;
					fmc->fmcFrames.resize(fmc->frames);
					continue;
				}

				if (maple::StringUtils::startWith(line, "frame")) 
				{
					std::stringstream sstream(line);
					std::string str;
					int32_t index;
					sstream >> str >> index;
					auto& frame = fmc->fmcFrames[index];
					frame.frameIdx = index;

					while (std::getline(fmcIn, str))
					{
						if (str == "{") 
							continue;
						if(str == "}")
							break;

						std::stringstream sstream2(str);
						std::string str2;
						auto & pos = frame.pos.emplace_back();
						auto & quat = frame.quat.emplace_back();
						sstream2 >> 
							str2 >> pos.x >> pos.z >> pos.y >> 
							str2 >> quat.w >> quat.x >> quat.z >> quat.y;
						
						quat.x *= -1.f;
						quat.y *= -1.f;
						quat.z *= -1.f;
					}	
				}
			}
			return fmc;
		}
	}

	auto GmbLoader::load(const std::string& name, const std::string& extension, std::vector<std::shared_ptr<maple::IResource>>& out) -> void
	{
		auto objName = maple::StringUtils::getFileNameWithoutExtension(name);
		auto& sceneObj = MeteorSceneObjectCache::get(objName);

		loadDesFile(sceneObj.desFile, maple::StringUtils::removeExtension(name) + ".des");
	
		auto meshse = std::make_shared<maple::MeshResource>(name);

		if (extension == "gmb") 
		{
			loadGmbFile(sceneObj.gmbFile, sceneObj.desFile, name, meshse->getMeshes());
		}
		else if (extension == "gmc")
		{
			loadGmcFile(sceneObj.gmbFile, sceneObj.desFile, name, meshse->getMeshes());
		}

		auto fmcFile = maple::StringUtils::removeExtension(name) + ".fmc";
		if (maple::File::fileExists(fmcFile)) 
		{
			sceneObj.fmcFile = loadFmc(fmcFile);
		}

		out.emplace_back(meshse);
	}
}