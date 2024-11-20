// Utils.h
#pragma once
#include <string>
#include <filesystem>
#include <optional>

#include "EngineAPI.h"
#include "MathUtils.h"

namespace fs = std::filesystem;

class Utils {
public:
    static void SetEnvironmentVariable(const std::string& name, const std::string& value);
    static std::string GetEnvironmentVariable(const std::string& name);
    static bool ShowSetEnginePathPopup();
    static bool s_showEnginePathPopup;

    // New methods for platform-specific path handling
    static fs::path GetApplicationDataPath();
    static fs::path GetEngineResourcePath();
    static std::optional<fs::path> GetBundlePath();
    static fs::path GetDefaultEnginePath();

	// New method for checking if an ID is invalid
    static bool IsInvalidID(int id) { return id != INVALIDID; };

	// Descriptor Functions
	static void SetTransform(game_entity_descriptor desc, MathUtils::Vec3 position, MathUtils::Vec3 rotation, MathUtils::Vec3 scale) {
		desc.transform.position[0] = position.x;
		desc.transform.position[1] = position.y;
		desc.transform.position[2] = position.z;
		desc.transform.rotation[0] = rotation.x;
		desc.transform.rotation[1] = rotation.y;
		desc.transform.rotation[2] = rotation.z;
		desc.transform.scale[0] = scale.x;
		desc.transform.scale[1] = scale.y;
		desc.transform.scale[2] = scale.z;
	};

private:
	static const int INVALIDID = -1;
};