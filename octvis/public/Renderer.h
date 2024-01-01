//
// Created by -Ry on 14/12/2023.
//

#ifndef OCTVIS_RENDERER_H
#define OCTVIS_RENDERER_H

#include <GL/glew.h>
//#include <GL/GL.h>

#include "glm/glm.hpp"
#include "Logging.h"

#include <type_traits>

namespace octvis::renderer {

    using id_t = unsigned int;

    constexpr id_t INVALID_ID = 0U;

    inline void assert_gl_okay(
            const char* call,
            const char* function,
            unsigned int line
    ) {
        int error_count = 0;

        // Poll Errors
        constexpr int MAX_ERRORS = 8;
        GLenum error_list[MAX_ERRORS]{ 0 };
        GLenum current_error = GL_NO_ERROR;
        while ((current_error = glGetError()) != GL_NO_ERROR) {
            error_list[error_count % MAX_ERRORS] = current_error;
            ++error_count;
        }

        if (error_count == 0) return;

        // Stringify the errors
        std::stringstream ss{};
        ss.str().reserve(64);
        for (int i = 0; i < error_count; ++i) {
            ss << "0x" << std::hex << error_list[i];
            if ((i + 1) < error_count) ss << ", ";
        }

        OCTVIS_ASSERT(
                error_count == 0,
                "OpenGL has errors after '{}'  => '{}'; Function: '{}', Line: '{}'",
                call,
                function,
                line,
                ss.str()
        );
    }

    #define GL_CALL(x) x; ::octvis::renderer::assert_gl_okay(#x, __FUNCTION__, __LINE__);

    //############################################################################//
    // | BUFFER DEFINITION |
    //############################################################################//

// region Buffer Enumerations
// @off

    enum class BufferType : GLenum {
        ARRAY   = GL_ARRAY_BUFFER,
        ELEMENT = GL_ELEMENT_ARRAY_BUFFER,
        UNIFORM = GL_UNIFORM_BUFFER,
        SSBO    = GL_SHADER_STORAGE_BUFFER
    };

    enum class BufferUsage : GLenum {
        STATIC  = GL_STATIC_DRAW,
        DYNAMIC = GL_DYNAMIC_DRAW,
        STREAM  = GL_STREAM_DRAW
    };

    enum class BufferMapping : GLenum {
        READ       = GL_READ_ONLY,
        WRITE      = GL_WRITE_ONLY,
        READ_WRITE = GL_READ_WRITE,
    };

// @on
// endregion

    class Buffer {

      private:
        id_t m_BufferID;
        BufferType m_Type;
        BufferUsage m_Usage;
        size_t m_SizeInBytes;
        void* m_MappedData;

      public:
        Buffer(BufferType type);
        ~Buffer();

      public:
        Buffer(const Buffer&) = delete;
        Buffer& operator =(const Buffer&) = delete;
        Buffer(Buffer&&);
        Buffer& operator =(Buffer&&);

      public:
        inline GLenum get_type() const noexcept {
            return static_cast<GLenum>(m_Type);
        }
        inline GLenum get_usage() const noexcept {
            return static_cast<GLenum>(m_Usage);
        }
        inline GLenum id() const noexcept {
            return m_BufferID;
        }

      public:
        inline bool is_array_buffer() const noexcept {
            return m_Type == BufferType::ARRAY;
        }
        inline bool is_element_buffer() const noexcept {
            return m_Type == BufferType::ELEMENT;
        }
        inline bool is_uniform_buffer() const noexcept {
            return m_Type == BufferType::UNIFORM;
        }
        inline bool is_ssbo_buffer() const noexcept {
            return m_Type == BufferType::SSBO;
        }
        inline bool is_valid() const noexcept {
            return m_BufferID != INVALID_ID;
        }

      public:
        void bind() const;
        void unbind() const;
        bool is_bound() const;
        void init_raw(size_t bytes, const void* const data, BufferUsage usage);
        void set_range(size_t start_bytes, const void* const data, size_t size_bytes) const;
        void* create_mapping(BufferMapping mode = BufferMapping::READ_WRITE);
        void release_mapping();

      public:
        template<class T>
        inline void init(size_t count, const T* const data, BufferUsage usage) {
            static_assert(sizeof(T) > 0);
            init_raw(sizeof(T) * count, data, usage);
        }

        template<class T, bool IsBeginInBytes = false>
        inline void set_range(size_t begin, const T* const data, size_t count) const {
            if constexpr (IsBeginInBytes) {
                set_range(begin, static_cast<const void*>(data), sizeof(T) * count);

            } else {
                set_range(
                        sizeof(T) * begin,
                        static_cast<const void*>(data),
                        sizeof(T) * count
                );
            }
        }
    };

    //############################################################################//
    // | TEXTURE |
    //############################################################################//

// region Texture 2D Dependencies
// @off

    enum class ColourFormat : GLenum {
        RGB  = GL_RGB,
        RGBA = GL_RGBA,
        BGR  = GL_BGR,
        BGRA = GL_BGRA
    };

    enum class PixelType : GLenum {
        BYTE   = GL_BYTE,
        UBYTE  = GL_UNSIGNED_BYTE,
        SHORT  = GL_SHORT,
        USHORT = GL_UNSIGNED_SHORT,
        INT    = GL_INT,
        UINT   = GL_UNSIGNED_INT,
        FLOAT  = GL_FLOAT
    };

    struct RawImage {

        using pixel_t = unsigned char;

      public:
        int          level      = 0;
        ColourFormat format     = ColourFormat::RGBA;
        PixelType    pixel_type = PixelType::UBYTE;
        int          width      = 0;
        int          height     = 0;
        int          channels   = 0;
        pixel_t*     pixel_data = nullptr;

      public:
        bool dealloc_pixel_data = false;

      public:
        ~RawImage() noexcept;

      public:
        inline bool valid() const noexcept {
            return pixel_data != nullptr && width > 0 && height > 0;
        }

        inline std::string to_string() const noexcept {
            return std::vformat(
                    "{}, {:#06x}, {:#06x}, {}, {}, {:p}",
                    std::make_format_args(
                            level,
                            static_cast<GLenum>(format),
                            static_cast<GLenum>(pixel_type),
                            width,
                            height,
                            (void*)pixel_data
                    )
            );
        }

        inline const void* const data() const noexcept {
            return static_cast<const void* const>(pixel_data);
        }

      public:
        void load_from_path(const char* path, int desired_channels = 0);
    };

// @on
// endregion

    class Texture2D {

      public:
        using index_t = unsigned int;
        static constexpr index_t MAX_INDEX = 31;

      private:
        id_t m_Identity;

      public:
        Texture2D();
        ~Texture2D();

      public:
        Texture2D(const Texture2D&) = delete;
        Texture2D& operator =(const Texture2D&) = delete;
        Texture2D(Texture2D&&) = delete;
        Texture2D& operator =(Texture2D&&) = delete;

      public:
        inline bool is_valid() const noexcept {
            return m_Identity != INVALID_ID;
        }

      public:
        void init(const RawImage& img);
        void deinit();

      public:
        void bind(index_t index = 0) const;
        void unbind(index_t index = 0) const;

      public:
        static void unbind_all();
        static void unbind_range(index_t begin, index_t end);

        inline static bool is_index_valid(index_t index) {
            static_assert(std::is_unsigned_v<index_t>);
            static_assert((GL_TEXTURE0 + MAX_INDEX) == GL_TEXTURE31);
            return index <= MAX_INDEX;
        }
    };

    //############################################################################//
    // | SHADER |
    //############################################################################//

    class ShaderProgram;

//     @off

    enum class ShaderType : GLenum {
        VERTEX   = GL_VERTEX_SHADER,
        FRAGMENT = GL_FRAGMENT_SHADER
    };

//     @on

    class Shader {
      private:
        id_t m_ShaderID;
        ShaderType m_Type;

      public:
        Shader(ShaderType type) noexcept;
        Shader(ShaderType type, const char* path) noexcept;
        ~Shader() noexcept;

      public:
        Shader(const Shader&) = delete;
        Shader& operator =(const Shader&) = delete;
        Shader(Shader&&);
        Shader& operator =(Shader&&);

      public:
        inline id_t get_id() const noexcept {
            return m_ShaderID;
        };
        inline ShaderType get_type() const noexcept {
            return m_Type;
        };
        inline GLenum get_type_i() const noexcept {
            return static_cast<GLenum>(m_Type);
        };

      public:
        inline bool is_vertex_shader() const noexcept {
            return get_type() == ShaderType::VERTEX;
        }
        inline bool is_fragment_shader() const noexcept {
            return get_type() == ShaderType::FRAGMENT;
        }

      public:
        void load_from_path(const char* path);
        void load_from_bytes(const char* bytes);
        void deinit();

      public:
        bool is_valid() const noexcept;
        void attach(ShaderProgram& program) const noexcept;
        void detach(ShaderProgram& program) const noexcept;

    };

    //############################################################################//
    // | SHADER PROGRAM |
    //############################################################################//

    class ShaderProgram {

      private:
        id_t m_ShaderProgramID;

      public:
        ShaderProgram();
        ShaderProgram(const Shader& vertex, const Shader& fragment) noexcept;
        ~ShaderProgram();

      public:
        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator =(const ShaderProgram&) = delete;

        ShaderProgram(ShaderProgram&&);
        ShaderProgram& operator =(ShaderProgram&&);

      public:
        inline id_t get_id() const noexcept {
            return m_ShaderProgramID;
        }

      public:
        void init();
        void deinit();
        void attach_shader(const Shader&);
        void detach_shader(const Shader&);
        void link();

      public:
        bool is_valid() const noexcept;
        void activate() const;
        void deactivate() const;

      public:
        int get_uniform(const char* name) const noexcept;

      public:
        void set_float(const char* name, float value);
        void set_vec2(const char* name, glm::vec2 value);
        void set_vec3(const char* name, glm::vec3 value);
        void set_mat3(const char* name, const glm::mat3& value);
        void set_mat4(const char* name, const glm::mat4& value);

      public:
        void set_ubo(const Buffer& ubo, unsigned int index, const char* name);
        void set_texture(const Texture2D& texture, Texture2D::index_t index, const char* name);

    };

    //############################################################################//
    // | TYPE MAPPINGS |
    //############################################################################//

    template<class T>
    constexpr GLenum gl_typeof() {

        // Signed Integral
        if constexpr (std::is_same_v<T, char>) return GL_BYTE;
        else if constexpr (std::is_same_v<T, short>) return GL_SHORT;
        else if constexpr (std::is_same_v<T, int>) return GL_INT;

            // Unsigned Integral
        else if constexpr (std::is_same_v<T, unsigned char>) return GL_UNSIGNED_BYTE;
        else if constexpr (std::is_same_v<T, unsigned short>) return GL_UNSIGNED_SHORT;
        else if constexpr (std::is_same_v<T, unsigned int>) return GL_UNSIGNED_INT;

            // Real
        else if constexpr (std::is_same_v<T, float>) return GL_FLOAT;
        else if constexpr (std::is_same_v<T, double>) return GL_DOUBLE;

            // Real Matrix
        else if constexpr (std::is_same_v<T, glm::vec2>) return GL_FLOAT;
        else if constexpr (std::is_same_v<T, glm::vec3>) return GL_FLOAT;
        else if constexpr (std::is_same_v<T, glm::vec4>) return GL_FLOAT;
        else if constexpr (std::is_same_v<T, glm::mat2>) return GL_FLOAT;
        else if constexpr (std::is_same_v<T, glm::mat3>) return GL_FLOAT;
        else if constexpr (std::is_same_v<T, glm::mat4>) return GL_FLOAT;
        else {
            static_assert(std::is_same_v<T, std::false_type>, "Unknown GL Type 'T'");
        }
    }

    template<class T>
    constexpr int gl_countof() {
        if constexpr (std::is_unsigned_v<T>) return gl_countof<std::make_signed<T>>();

        // Integral Types
        if constexpr (std::is_same_v<T, char>) return 1;
        else if constexpr (std::is_same_v<T, short>) return 1;
        else if constexpr (std::is_same_v<T, int>) return 1;

            // Real Types
        else if constexpr (std::is_same_v<T, float>) return 1;
        else if constexpr (std::is_same_v<T, double>) return 1;

            // Vectors
        else if constexpr (std::is_same_v<T, glm::vec2>) return 2;
        else if constexpr (std::is_same_v<T, glm::vec3>) return 3;
        else if constexpr (std::is_same_v<T, glm::vec4>) return 4;

            // Matrices
        // else if constexpr (std::is_same_v<T, glm::mat2>) return 2; // 2x2
        // else if constexpr (std::is_same_v<T, glm::mat3>) return 3; // 3x3
        else if constexpr (std::is_same_v<T, glm::mat4>) return 4; // 4x4

            // Unknown Count
        else {
            static_assert(std::is_same_v<T, std::false_type>, "Unknown Component Count for type 'T'");
        }
    }

    template<class T>
    struct VertexAttribute {
        using index_t = unsigned int;

        constexpr void create(
                index_t index,
                bool normalise = false,
                size_t stride = 0,
                const void* ptr = (const void*) 0
        ) const {
            if (stride == 0) stride = sizeof(T);

            OCTVIS_TRACE(
                    "Create Attribute ( {}, {}, {:#06x}, {}, {:#06x}, {:p} )",
                    index, gl_countof<T>(), gl_typeof<T>(),
                    normalise ? "True" : "False",
                    stride,
                    ptr
            );
            glVertexAttribPointer(
                    index,
                    gl_countof<T>(),
                    gl_typeof<T>(),
                    normalise ? GL_TRUE : GL_FALSE,
                    stride,
                    ptr
            );
        };

        constexpr void enable(index_t index, bool enabled = true) const {
            if (enabled) {
                glEnableVertexAttribArray(index);
            } else {
                glDisableVertexAttribArray(index);
            }
        }
    };

    template<>
    struct VertexAttribute<glm::mat4> {
        using index_t = unsigned int;

        constexpr void create(
                index_t index,
                bool normalise = false,
                size_t stride = 0,
                const void* ptr = (const void*) 0
        ) const {
            if (stride == 0) stride = sizeof(glm::mat4);

            size_t offset = (size_t) ptr;

            constexpr VertexAttribute<glm::vec4> attributes[4]{};
            attributes[0].create(index + 0, normalise, stride, (void*) (offset + sizeof(glm::vec4) * 0));
            attributes[1].create(index + 1, normalise, stride, (void*) (offset + sizeof(glm::vec4) * 1));
            attributes[2].create(index + 2, normalise, stride, (void*) (offset + sizeof(glm::vec4) * 2));
            attributes[3].create(index + 3, normalise, stride, (void*) (offset + sizeof(glm::vec4) * 3));
        };

        constexpr void enable(index_t index, bool enabled = true) const {
            if (enabled) {
                glEnableVertexAttribArray(index + 0);
                glEnableVertexAttribArray(index + 1);
                glEnableVertexAttribArray(index + 2);
                glEnableVertexAttribArray(index + 3);
            } else {
                glDisableVertexAttribArray(index + 0);
                glDisableVertexAttribArray(index + 1);
                glDisableVertexAttribArray(index + 2);
                glDisableVertexAttribArray(index + 3);
            }
        }
    };

    template<class T>
    struct VertexAttributeSlotCount {
        constexpr static int value = 1;
    };

    template<>
    struct VertexAttributeSlotCount<glm::mat4> {
        constexpr static int value = 4;
    };

    //############################################################################//
    // | VERTEX ARRAY OBJECT |
    //############################################################################//

    class VertexArrayObject {

      public:
        using index_t = unsigned int;

      private:
        id_t m_Identity;

      public:
        VertexArrayObject();
        ~VertexArrayObject();

      public:
        VertexArrayObject(const VertexArrayObject&) = delete;
        VertexArrayObject& operator =(const VertexArrayObject&) = delete;

        VertexArrayObject(VertexArrayObject&&);
        VertexArrayObject& operator =(VertexArrayObject&&);

      public:
        void init();
        bool is_valid() const;
        bool is_bound() const;
        void bind() const;
        void unbind() const;

      public: // Returns Self for chaining
        VertexArrayObject& attach_buffer(const Buffer& buffer);
        VertexArrayObject& enable_attribute(index_t index, bool is_enabled = true);
        VertexArrayObject& enable_attribute_range(index_t begin, index_t end, bool is_enabled = true);
        VertexArrayObject& set_divisor(index_t index, index_t divisor);
        VertexArrayObject& set_divisor_range(index_t begin, index_t end, index_t divisor);

      public:
        template<class T>
        inline VertexArrayObject& set_divisor(index_t index, index_t divisor) noexcept {
            constexpr int slot_count = VertexAttributeSlotCount<T>::value;
            if constexpr (slot_count == 1) {
                set_divisor(index, divisor);
            } else if constexpr (slot_count > 1) {
                set_divisor_range(index, index + slot_count, divisor);
            } else {
                static_assert(std::is_same_v<T, std::false_type>, "Invalid Vertex Attribute Slot Count...");
            }
            return *this;
        }

      public: // Non Packed Attribute
        template<class T>
        inline VertexArrayObject& add_attribute(
                index_t index,
                bool normalise = false,
                size_t stride = sizeof(T),
                void* ptr = (void*) 0,
                bool is_enabled = true
        ) {
            VertexAttribute <T> attribute{};
            attribute.create(index, normalise, stride, ptr);
            if (is_enabled) attribute.enable(index);
            return *this;
        }

      public: // Add Packed/Interleaved Attributes
        template<class... Types>
        inline VertexArrayObject& add_interleaved_attributes(index_t index) {
            static_assert(sizeof...(Types) > 0);
            constexpr size_t stride = (sizeof(Types) + ...);
            add_interleaved_attribute_recursively<Types...>(index, stride, (void*) 0);
            return *this;
        }

      private:
        template<class Head, class... Tail>
        constexpr void add_interleaved_attribute_recursively(index_t index, size_t stride, void* offset) {
            add_attribute<Head>(index, false, stride, offset);
            if constexpr (sizeof...(Tail) == 0) {
                return;
            } else {
                add_interleaved_attribute_recursively<Tail...>(
                        index + VertexAttributeSlotCount<Head>::value,
                        stride,
                        (void*) ((size_t) offset + sizeof(Head))
                );
            }
        }
    };

}

#endif
