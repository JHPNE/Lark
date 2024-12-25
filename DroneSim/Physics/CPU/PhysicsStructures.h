#pragma once
#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <vector>

namespace drosim::physics {
  // Forward declaration
  class Collider;
  class RigidBody;
  struct Ray3;
  struct ContactInfo;

  struct Ray3 {
    glm::vec3 pos;
    glm::vec3 dir;
  };

  struct RayCastResult {
    bool hit;
    Collider *collider;
    glm::vec3 position;
    glm::vec3 normal;
    float t;
  };
 /**
     * @brief Simple Axis-Aligned Bounding Box (AABB) structure.
     *
     * - Stores minimum and maximum corners in world or local space
     *   (depending on how you use it).
     * - Provides utility methods for union, containment tests,
     *   and point / box intersection checks.
     */
    struct AABB
    {
        glm::vec3 minPoint;  ///< minimum corner (x_min, y_min, z_min)
        glm::vec3 maxPoint;  ///< maximum corner (x_max, y_max, z_max)
        void* userData = nullptr;

        //---------------------------------------------------------
        // 1) Constructors
        //---------------------------------------------------------

        /**
         * @brief Default constructor makes an 'invalid' or zero-size box.
         */
        AABB()
        : minPoint( +std::numeric_limits<float>::max() ),
          maxPoint( -std::numeric_limits<float>::max() )
        {
            // This way, if you call AABB::Union() or expand with points,
            // the first union or expand sets a real bounding region.
        }

        /**
         * @brief Constructs an AABB from given min & max corners.
         */
        AABB(const glm::vec3 &minPt, const glm::vec3 &maxPt)
        : minPoint(minPt),
          maxPoint(maxPt)
        {
        }

        //---------------------------------------------------------
        // 2) Utility / Accessors
        //---------------------------------------------------------

        /**
         * @brief Compute the center of this AABB.
         */
        glm::vec3 Center() const
        {
            return 0.5f * (minPoint + maxPoint);
        }

        /**
         * @brief Compute the extents (half-widths) along each axis.
         */
        glm::vec3 Extents() const
        {
            return 0.5f * (maxPoint - minPoint);
        }

        /**
         * @brief Compute the volume (in cubic units).
         */
        float Volume() const
        {
            glm::vec3 size = maxPoint - minPoint;
            return size.x * size.y * size.z;
        }

        /**
         * @brief Check if this AABB is valid (min <= max on all axes).
         *        If the box was never expanded or unioned, it might be invalid.
         */
        bool IsValid() const
        {
            return (minPoint.x <= maxPoint.x &&
                    minPoint.y <= maxPoint.y &&
                    minPoint.z <= maxPoint.z);
        }

        //---------------------------------------------------------
        // 3) Expansion & Union
        //---------------------------------------------------------

        /**
         * @brief Expands this AABB so that it also includes 'p'.
         */
        void Expand(const glm::vec3 &p)
        {
            minPoint = glm::min(minPoint, p);
            maxPoint = glm::max(maxPoint, p);
        }

        /**
         * @brief Returns a new AABB that is the union of *this and 'other'.
         */
        AABB Union(const AABB &other) const
        {
            AABB result;
            result.minPoint = glm::min(minPoint, other.minPoint);
            result.maxPoint = glm::max(maxPoint, other.maxPoint);
            return result;
        }

        /**
         * @brief Expands this AABB in-place to also include 'other'.
         */
        void UnionInPlace(const AABB &other)
        {
            minPoint = glm::min(minPoint, other.minPoint);
            maxPoint = glm::max(maxPoint, other.maxPoint);
        }

        //---------------------------------------------------------
        // 4) Contains & Intersects
        //---------------------------------------------------------

        /**
         * @brief Returns true if this AABB fully contains the given point.
         */
        bool Contains(const glm::vec3 &point) const
        {
            return (point.x >= minPoint.x && point.x <= maxPoint.x &&
                    point.y >= minPoint.y && point.y <= maxPoint.y &&
                    point.z >= minPoint.z && point.z <= maxPoint.z);
        }

        /**
         * @brief Returns true if this AABB fully contains the other AABB.
         *        i.e., the 'other' is entirely inside *this.
         */
        bool Contains(const AABB &other) const
        {
            return (other.minPoint.x >= minPoint.x &&
                    other.minPoint.y >= minPoint.y &&
                    other.minPoint.z >= minPoint.z &&
                    other.maxPoint.x <= maxPoint.x &&
                    other.maxPoint.y <= maxPoint.y &&
                    other.maxPoint.z <= maxPoint.z);
        }

        /**
         * @brief Returns true if *this intersects/overlaps with 'other'.
         *        For broadphase usage, this is often called 'Collides'.
         */
        bool Intersects(const AABB &other) const
        {
            // Debug AABB overlap test
            std::cout << "AABB Intersection Test:\n"
                      << "This AABB: Y=[" << minPoint.y << ", " << maxPoint.y << "]\n"
                      << "Other AABB: Y=[" << other.minPoint.y << ", " << other.maxPoint.y << "]\n";

            bool overlapY = !(maxPoint.y < other.minPoint.y || minPoint.y > other.maxPoint.y);
            std::cout << "Y-axis overlap: " << (overlapY ? "YES" : "NO") << "\n";

            // Overlap along X?
            if (maxPoint.x < other.minPoint.x || minPoint.x > other.maxPoint.x)
                return false;
            // Overlap along Y?
            if (maxPoint.y < other.minPoint.y || minPoint.y > other.maxPoint.y)
                return false;
            // Overlap along Z?
            if (maxPoint.z < other.minPoint.z || minPoint.z > other.maxPoint.z)
                return false;

            return true;
        }

        /**
         * @brief Often in broadphase code, we use the name 'Collides()'
         *        for "Intersects()" or "Overlap()".
         *        Provided here as an alias.
         */
        bool Collides(const AABB &other) const
        {
            return Intersects(other);
        }

        //---------------------------------------------------------
        // 5) Static Helpers
        //---------------------------------------------------------

        /**
         * @brief Create an AABB that encloses a list of points.
         */
        static AABB FromPoints(const std::vector<glm::vec3> &points)
        {
            AABB box;
            for (const auto &p : points)
                box.Expand(p);
            return box;
        }
    };

    //---------------------------------------------------------
    // Optional free function for Ray vs AABB intersection
    // (param tMinOut / tMaxOut define near/far intersection points)
    //---------------------------------------------------------
    inline bool RayAABB(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDir,
        const AABB       &box,
        float &tMinOut,
        float &tMaxOut)
    {
        // tMin/tMax are the bounds along the ray param for intersection
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; ++i)
        {
            float invD = 1.0f / rayDir[i];
            float t0   = (box.minPoint[i] - rayOrigin[i]) * invD;
            float t1   = (box.maxPoint[i] - rayOrigin[i]) * invD;

            if (invD < 0.0f)
                std::swap(t0, t1);

            tMin = t0 > tMin ? t0 : tMin;
            tMax = t1 < tMax ? t1 : tMax;

            if (tMax < tMin)
                return false;
        }

        tMinOut = tMin;
        tMaxOut = tMax;
        return true;
    }
};