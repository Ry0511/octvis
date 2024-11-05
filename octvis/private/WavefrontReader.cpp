//
// Date       : 18/01/2024
// Project    : octvis
// Author     : -Ry
//

#include "WavefrontReader.h"

#include <fstream>
#include <sstream>

namespace octvis {

    bool WavefrontReader::load_from_path(const char* obj_file) noexcept {
        if (m_ObjPath != nullptr) clear();

        m_ObjPath = obj_file;

        // Open File
        std::ifstream stream{ m_ObjPath };
        if (!stream || !stream.is_open()) {
            OCTVIS_WARN("Failed to open filestream for file '{}'", m_ObjPath);
            return false;
        }

        // Read File
        using It = std::istreambuf_iterator<char>;
        std::string file_content{ It{ stream }, It{} };

        // Process File
        std::stringstream line_stream{ std::move(file_content) };
        line_stream << std::noskipws;
        std::string line{};
        line.reserve(256);

        constexpr auto is_id = [](const char xs[2], const char vs[2]) -> bool {
            return xs[0] == vs[0] && xs[1] == vs[1];
        };

        while (std::getline(line_stream, line)) {
            char id[2]{ line[0], line[1] };

            if (is_id(id, "v ") && !read_vec3(line, m_Positions)) {
                OCTVIS_WARN("Failed to read vec3 from line '{}' in file '{}'", line, m_ObjPath);
                clear();
                return false;

            } else if (is_id(id, "vn") && !read_vec3(line, m_Normals)) {
                OCTVIS_WARN("Failed to read vec3 from line '{}' in file '{}'", line, m_ObjPath);
                clear();
                return false;

            } else if (is_id(id, "vt") && !read_vec2(line, m_TexturePositions)) {
                OCTVIS_WARN("Failed to read vec2 from line '{}' in file '{}'", line, m_ObjPath);
                clear();
                return false;

            } else if (is_id(id, "f ") && !read_triangle(line, m_Indices)) {
                OCTVIS_WARN("Failed to read indexed triangle from line '{}' in file '{}'", line, m_ObjPath);
                clear();
                return false;
            }
        }

        return true;
    }

    //############################################################################//
    // | READING VALUES |
    //############################################################################//

    bool WavefrontReader::read_vec3(
            std::string_view line,
            std::vector<glm::vec3>& xs
    ) noexcept {
        std::stringstream stream(line.data());
        stream << std::noskipws;
        char ignored{};
        glm::vec3 vec{};

        if (!(stream >> ignored >> ignored)) return false;
        stream << std::skipws;

        // Expecting: '__ F F F' where _ is an arbitrary character and F is a number
        if (!(stream >> vec.x >> vec.y >> vec.z)) return false;
        xs.push_back(vec);

        return true;
    }

    bool WavefrontReader::read_vec2(
            std::string_view line,
            std::vector<glm::vec2>& xs
    ) noexcept {
        std::stringstream stream(line.data());
        stream << std::noskipws;
        char ignored{};
        glm::vec2 vec{};

        if (!(stream >> ignored >> ignored)) return false;
        stream << std::skipws;

        // Expecting: '__ F F F' where _ is an arbitrary character and F is a number
        if (!(stream >> vec.x >> vec.y)) return false;
        xs.push_back(vec);

        return true;
    }

    bool WavefrontReader::read_triangle(
            std::string_view line,
            std::vector<IndexedTriangle>& xs
    ) noexcept {

        IndexedTriangle triangle{};
        std::stringstream ss(line.data());
        ss << std::noskipws;

        // Input: 'f 5/5/1 3/3/1 1/1/1'
        char ignored{};
        if (!(ss >> ignored >> ignored)) return false;
        ss << std::skipws;

        // Input: '5/5/1 3/3/1 1/1/1'
        for (glm::ivec3& vec : triangle) {
            if (!(ss >> vec.x >> ignored >> vec.z >> ignored >> vec.y)) {
                return false;
            }
        }

        xs.push_back(triangle);

        return true;
    }

} // octvis