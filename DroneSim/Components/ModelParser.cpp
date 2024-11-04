#pragma once
#include "ModelParser.h" 
#include "ComponentCommon.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace drosim {
	model load_obj(const std::string& path) {
		model mdl;
		std::ifstream file(path);
		if (!file.is_open()) {
			std::cerr << "Failed to open file: " << path << std::endl;
			return mdl;
		}

		util::vector<math::v3> positions;
		util::vector<math::v3> normals;
		util::vector<math::v2> texcoords;
		util::vector<u32> indices;

		std::string line;
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			std::string prefix;

			iss >> prefix;

			if (prefix == "v") {
				math::v3 pos;
				iss >> pos.x >> pos.y >> pos.z;
				positions.push_back(pos);
			}
			else if (prefix == "vn") {
				math::v3 normal;
				iss >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);
			}
			else if (prefix == "vt") {
				math::v2 texcoord;
				iss >> texcoord.x >> texcoord.y;
				texcoords.push_back(texcoord);
			}
			else if (prefix == "f") {
				u32 vi[3], ti[3], ni[3];
				char slash;
				for (int i = 0; i < 3; ++i) {
					iss >> vi[i] >> slash >> ti[i] >> slash >> ni[i];
					indices.push_back(vi[i] - 1);
				}
			}
		}

		mesh msh;
		for (size_t i = 0; i < indices.size(); i += 3) {
			vertex v;
			v.position = positions[indices[i]];
			v.texcoord = texcoords[indices[i]];
			v.normal = normals[indices[i]];
			msh.vertices.push_back(v);
		}

		msh.indices = indices;
		mdl.meshes.push_back(msh);

		return mdl;
	}


	model load_fbx(const std::string& path) {
		model mdl;
		return mdl;
	}
}