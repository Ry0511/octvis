//
// Date       : 18/01/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_WAVEFRONTREADER_H
#define OCTVIS_WAVEFRONTREADER_H

#include "RenderApplication.h"

#include <vector>

namespace octvis {

    class WavefrontReader {

        //
        // Realistically speaking this does not need to be for triangle models only.
        //

      private: // Triangle => '1/1/1 2/2/2 3/3/3' Vertex, Normal, Texture
        struct IndexedTriangle {
            glm::ivec3 v0, v1, v2;

            auto begin() noexcept { return &v0; }
            auto end() noexcept { return (&v2) + 1; }
            auto begin() const noexcept { return &v0; }
            auto end() const noexcept { return (&v2) + 1; }
        };

      private: // @off
        const char*                  m_ObjPath;
        std::vector<glm::vec3>       m_Positions;
        std::vector<glm::vec3>       m_Normals;
        std::vector<glm::vec2>       m_TexturePositions;
        std::vector<IndexedTriangle> m_Indices;
      // @on

      public:
        WavefrontReader() noexcept = default;
        ~WavefrontReader() noexcept = default;

      public:
        bool load_from_path(const char* obj_file) noexcept;

      private:
        bool read_vec3(std::string_view line, std::vector<glm::vec3>& xs) noexcept;
        bool read_vec2(std::string_view line, std::vector<glm::vec2>& xs) noexcept;
        bool read_triangle(std::string_view line, std::vector<IndexedTriangle>& xs) noexcept;

      public:
        inline void clear() noexcept {
            m_ObjPath = nullptr;

            // Clear
            m_Positions.clear();
            m_Normals.clear();
            m_TexturePositions.clear();
            m_Indices.clear();

            // Shrink
            m_Positions.shrink_to_fit();
            m_Normals.shrink_to_fit();
            m_TexturePositions.shrink_to_fit();
            m_Indices.shrink_to_fit();
        };

      public:
        inline std::vector<Vertex> get_vertices() noexcept {
            std::vector<Vertex> vertices{};
            vertices.reserve(m_Indices.size() * 3);

            for (const IndexedTriangle& triangle : m_Indices) {
                for (const glm::ivec3& vec : triangle) {
                    vertices.emplace_back(
                            Vertex{
                                    m_Positions[vec.x - 1],
                                    m_Normals[vec.y - 1],
                                    m_TexturePositions[vec.z - 1],
                                    glm::vec4{ 1.0F }
                            }
                    );
                }
            }

            return vertices;
        }
    };

} // octvis

#endif
