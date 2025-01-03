// Asset.h
#pragma once
#include <cassert>

namespace lark {
    namespace editor {

        enum class AssetType {
            Unknown,
            Animation,
            Audio,
            Material,
            Mesh,
            Skeleton,
            Texture
        };

        class Asset {
        public:
            explicit Asset(AssetType type) {
                assert(type != AssetType::Unknown);
                _type = type;
            }

            virtual ~Asset() = default;

            AssetType GetType() const { return _type; }

        private:
            AssetType _type;
        };

    } // namespace editor
} // namespace lark
