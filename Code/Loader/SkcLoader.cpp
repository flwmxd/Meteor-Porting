//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////
#include "SkcLoader.h"

#include <Animation/Skeleton.h>
#include <Assets/MeshResource.h>
#include <Engine/Core.h>
#include <Engine/Material.h>
#include <Engine/Profiler.h>
#include <Others/Console.h>
#include <Others/StringUtils.h>
#include <Scene/Component/Transform.h>

#include <fstream>
#include <sstream>

namespace meteor
{
	//inline block
	namespace
	{
		inline auto readMaterial(std::ifstream &skcIn, CharacterMaterial &material)
		{
			std::string line;
			std::string temp;
			while (std::getline(skcIn, line))
			{
				std::stringstream sstream(line);
				sstream >> temp;
				if (temp == "Material")
					continue;

				if (temp == "Texture")
				{
					sstream >> material.texture;
					continue;
				}

				if (temp == "ColorKey")
				{
					sstream >> material.colorKey.x >> material.colorKey.y >> material.colorKey.z >> material.colorKey.w;
					continue;
				}

				if (temp == "Ambient")
				{
					sstream >> material.ambient.x >> material.ambient.y >> material.ambient.z;
					continue;
				}

				if (temp == "Diffuse")
				{
					sstream >> material.diffuse.x >> material.diffuse.y >> material.diffuse.z;
					continue;
				}
				if (temp == "Specular")
				{
					sstream >> material.specular.x >> material.specular.y >> material.specular.z;
					continue;
				}
				if (temp == "Emissive")
				{
					sstream >> material.emissive.x >> material.emissive.y >> material.emissive.z;
					continue;
				}

				if (temp == "Opacity")
				{
					sstream >> material.opacity;
					continue;
				}

				if (temp == "Option")
				{
					sstream >> material.option;
					continue;
				}

				if (temp == "TwoSide")
				{
					std::string str;
					sstream >> str;
					material.twoSide = str == "TRUE";
					break;
				}
			}
		}

		inline auto readVerticesAndBone(std::ifstream &skcIn, maple::SkinnedVertex &vertex)
		{
			std::string line;
			std::getline(skcIn, line);
			std::stringstream sstream(line);
			sstream >> line;        //v
			sstream >> vertex.pos.x >> vertex.pos.z >> vertex.pos.y;
			sstream >> line;        //vt
			sstream >> vertex.texCoord.x >> vertex.texCoord.y;
			sstream >> line;        //bones
			vertex.normal  = {};
			vertex.tangent = {};

			uint32_t boneCount = 0;
			sstream >> boneCount;
			vertex.boneIndices = {};
			vertex.boneWeights = {};
			vertex.color       = glm::vec4{1.f};
			for (auto i = 0; i < boneCount; i++)
			{
				uint32_t index  = 0;
				float    weight = 0.f;
				sstream >> index >> weight;
				vertex.boneIndices[i] = index;
				vertex.boneWeights[i] = weight;
			}
		}

		inline auto readTriangles(std::ifstream &skcIn, std::vector<uint32_t> &indices)
		{
			std::string line;
			std::getline(skcIn, line);
			std::stringstream sstream(line);
			sstream >> line;        //f

			uint32_t temp     = 0;
			uint32_t v1       = 0;
			uint32_t v2       = 0;
			uint32_t v3       = 0;
			int32_t  material = 0;

			sstream >> temp >> material >> v1 >> v3 >> v2;

			indices.emplace_back(v1);
			indices.emplace_back(v2);
			indices.emplace_back(v3);

			return material;
		}

		inline auto readStaticSkin(std::ifstream &skcIn, SkcFile &file)
		{
			char                              skinName[100];
			std::string                       line;
			std::vector<CharacterMaterial>    cMaterials;
			std::vector<uint32_t>             indices;
			std::vector<maple::SkinnedVertex> vertices;

			auto outMesh = std::make_shared<maple::MeshResource>(file.name);

			while (std::getline(skcIn, line))
			{
				maple::StringUtils::trim(line);
				maple::StringUtils::trim(line, "\t");
				if (maple::StringUtils::startWith(line, "{"))
				{        //start
					continue;
				}
				if (maple::StringUtils::startWith(line, "}"))
				{        //end
					break;
				}

				if (maple::StringUtils::startWith(line, "Static Skin"))
				{
					sscanf(line.c_str(), "Static Skin %s", skinName);
				}

				if (maple::StringUtils::startWith(line, "Materials"))
				{
					int32_t count;
					sscanf(line.c_str(), "Materials: %d", &count);
					cMaterials.resize(count);

					for (auto i = 0; i < count; i++)
					{
						readMaterial(skcIn, cMaterials[i]);
					}
				}

				if (maple::StringUtils::startWith(line, "Vertices"))
				{
					int32_t count;
					sscanf(line.c_str(), "Vertices: %d", &count);
					vertices.resize(count);
					for (auto i = 0; i < count; i++)
					{
						readVerticesAndBone(skcIn, vertices[i]);
					}
				}

				if (maple::StringUtils::startWith(line, "Triangles"))
				{
					int32_t count;
					sscanf(line.c_str(), "Triangles: %d", &count);

					std::vector<uint32_t> idxOffset;
					int32_t               materialId   = -1;
					uint32_t              offset       = 0;
					int32_t               subMeshCount = 0;

					std::vector<std::shared_ptr<maple::Material>> materials;

					for (auto i = 0; i < count; i++)
					{
						auto newMaterial = readTriangles(skcIn, indices);
						if (newMaterial != materialId)
						{
							if (subMeshCount > 0)
							{
								idxOffset.emplace_back(offset);
							}

							auto material = std::make_shared<maple::Material>();

							maple::PBRMataterialTextures textures;
							const auto &                 textureName = cMaterials[newMaterial].texture;

							if (!maple::StringUtils::endWith(textureName, ".ifl"))
								textures.albedo = maple::Texture2D::create(textureName, "meteor/textures/" + textureName);
							material->setTextures(textures);
							materials.emplace_back(material);

							subMeshCount++;
							materialId = newMaterial;
						}
						offset += 3;
					}

					maple::Mesh::generateNormals(vertices, indices);
					maple::Mesh::generateTangents(vertices, indices);
					//maple::Mesh::generateBitangents(vertices, indices);

					auto mesh = std::make_shared<maple::Mesh>(indices, vertices);
					mesh->setName(skinName);
					mesh->setMaterial(materials);
					mesh->setSubMeshCount(subMeshCount);
					mesh->setSubMeshIndex(idxOffset);
					outMesh->addMesh(skinName, mesh);
				}
			}

			return outMesh;
		}

		inline auto readBone(std::ifstream &fileIn, MeteorBone &bone)
		{
			std::string line;
			while (std::getline(fileIn, line))
			{
				maple::StringUtils::trim(line);
				maple::StringUtils::trim(line, "\t");

				if (maple::StringUtils::startWith(line, "{"))
				{        //start
					continue;
				}
				if (maple::StringUtils::startWith(line, "}"))
				{        //end
					break;
				}

				if (maple::StringUtils::startWith(line, "parent"))
				{
					std::stringstream sstream(line);
					sstream >> line >> bone.parent;
					continue;
				}

				if (maple::StringUtils::startWith(line, "pivot"))
				{
					std::stringstream sstream(line);
					sstream >> line;        //pivot
					sstream >> bone.offset.x >> bone.offset.z >> bone.offset.y;
					continue;
				}

				if (maple::StringUtils::startWith(line, "quaternion"))
				{
					std::stringstream sstream(line);
					sstream >> line;        //pivot
					sstream >> bone.rotation.w >> bone.rotation.x >> bone.rotation.z >> bone.rotation.y;
					bone.rotation.x *= -1;
					bone.rotation.y *= -1;
					bone.rotation.z *= -1;
					continue;
				}

				if (maple::StringUtils::startWith(line, "children"))
				{
					std::stringstream sstream(line);
					sstream >> line;        //pivot
					sstream >> bone.children;
					continue;
				}
			}
		}

		inline auto findParent(ozz::vector<ozz::animation::offline::RawSkeleton::Joint> &joins, const std::string &name) -> ozz::animation::offline::RawSkeleton::Joint *
		{
			for (auto &j : joins)
			{
				if (j.name == ozz::string(name))
				{
					return &j;
				}

				auto deepJoint = findParent(j.children, name);

				if (deepJoint != nullptr)
				{
					return deepJoint;
				}
			}

			return nullptr;
		}

		inline auto depthSet(const std::set<int32_t> &                          parents,
		                     ozz::animation::offline::RawSkeleton::Joint &      rawJoints,
		                     const std::vector<MeteorBone> &                    bones,
		                     const std::function<int32_t(const std::string &)> &getParentId) -> void
		{
			std::set<int32_t> nextParents;

			for (auto i : parents)
			{
				for (auto j = 0; j < bones.size(); j++)        // find the children belong to this node.
				{
					auto &b        = bones[j];
					auto  parentId = getParentId(b.parent);
					if (parentId == i)
					{
						if (rawJoints.name == ozz::string(b.parent))
						{
							auto &newJoint                 = rawJoints.children.emplace_back();
							newJoint.name                  = b.name;
							newJoint.transform             = ozz::math::Transform::identity();
							newJoint.transform.rotation    = {b.rotation.x, b.rotation.y, b.rotation.z, b.rotation.w};
							newJoint.transform.translation = {b.offset.x, b.offset.y, b.offset.z};
						}
						else
						{
							auto parentNode = findParent(rawJoints.children, b.parent);
							if (parentNode)
							{
								auto &newJoint                 = parentNode->children.emplace_back();
								newJoint.name                  = b.name;
								newJoint.transform             = ozz::math::Transform::identity();
								newJoint.transform.rotation    = {b.rotation.x, b.rotation.y, b.rotation.z, b.rotation.w};
								newJoint.transform.translation = {b.offset.x, b.offset.y, b.offset.z};
							}
						}
						nextParents.emplace(j);
					}
				}
			}
			if (!nextParents.empty())
			{
				depthSet(nextParents, rawJoints, bones, getParentId);
			}
		}

		inline auto loadBone(const std::string &fileName, std::vector<std::shared_ptr<maple::IResource>> &out)
		{
			auto          name    = maple::StringUtils::removeExtension(fileName) + ".bnc";
			auto &        bncFile = BncFileCache::get(name);
			std::ifstream bncIn;
			bncIn.open(name);
			std::string line;

			while (std::getline(bncIn, line))
			{
				maple::StringUtils::trim(line, "\t");
				if (maple::StringUtils::startWith(line, "#"))
					continue;

				if (bncFile.boneSize == 0 && bncFile.dummeySize == 0)
				{
					sscanf(line.c_str(), "Bones: %d Dummy: %d", &bncFile.boneSize, &bncFile.dummeySize);
				}
				bool isBone   = maple::StringUtils::startWith(line, "bone");
				bool isDummey = maple::StringUtils::startWith(line, "Dummy");
				if (isBone || isDummey)
				{
					auto  vec  = maple::StringUtils::split(line, " ");
					auto &bone = bncFile.bones.emplace_back();
					bone.dummy = isDummey;
					bone.name  = vec[1];
					readBone(bncIn, bone);
				}
			}

			if (!bncFile.bones.empty())
			{
				//convert to skeleton
				ozz::animation::offline::RawSkeleton                     rawSkeleton;
				std::vector<ozz::animation::offline::RawSkeleton::Joint> rawJoints;

				for (auto i = 0; i < bncFile.bones.size(); i++)
				{
					auto &b                   = bncFile.bones[i];
					auto &raw                 = rawJoints.emplace_back();
					raw.transform             = ozz::math::Transform::identity();
					raw.transform.rotation    = {b.rotation.x, b.rotation.y, b.rotation.z, b.rotation.w};
					raw.transform.translation = {b.offset.x, b.offset.y, b.offset.z};
					raw.name                  = b.name;
				}

				auto getParentId = [&](const std::string &name) -> int32_t {
					int32_t i = 0;
					for (auto &joint : bncFile.bones)
					{
						if (joint.name == name)
						{
							return i;
						}
						i++;
					}
					return -1;
				};

				depthSet({0}, rawJoints[0], bncFile.bones, getParentId);
				rawSkeleton.roots.emplace_back(rawJoints[0]);
				auto skeleton = std::make_shared<maple::animation::Skeleton>(rawSkeleton);
				out.emplace_back(skeleton);
			}
			bncIn.close();
		}
	}        // namespace

	auto SkcLoader::load(const std::string &fileName, const std::string &extension, std::vector<std::shared_ptr<maple::IResource>> &out) const -> void
	{
		std::ifstream skcIn;
		skcIn.open(fileName);
		std::string line;

		auto &skcFile = SkcFileCache::get(fileName);

		while (std::getline(skcIn, line))
		{
			if (maple::StringUtils::startWith(line, "#"))
				continue;

			if (skcFile.staticSkins == 0 && skcFile.dynmaicSkins == 0)
			{
				sscanf(line.c_str(), "Static Skins: %d Dynamic Skins: %d", &skcFile.staticSkins, &skcFile.dynmaicSkins);
			}

			for (int32_t i = 0; i < skcFile.staticSkins; i++)
			{
				auto staticMesh = readStaticSkin(skcIn, skcFile);
				out.emplace_back(staticMesh);
			}
		}

		loadBone(fileName, out);

		skcIn.close();
	}
}        // namespace meteor
