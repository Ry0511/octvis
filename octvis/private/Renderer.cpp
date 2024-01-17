//
// Created by -Ry on 14/12/2023.
//

#include "Renderer.h"

#include "glm/gtc/type_ptr.hpp"

//@off
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//@on

#include <fstream>

namespace octvis::renderer {

    //############################################################################//
    // | BUFFER |
    //############################################################################//

    Buffer::Buffer(BufferType type)
            : m_BufferID(INVALID_ID),
              m_Type(type),
              m_Usage(BufferUsage::STATIC),
              m_SizeInBytes(0),
              m_MappedData(nullptr) {
        GL_CALL(glGenBuffers(1, &m_BufferID));
        OCTVIS_TRACE("Created Buffer ( {}, {:#06x} )", m_BufferID, get_type());
    }

    Buffer::~Buffer() {
        if (m_BufferID == INVALID_ID) return;
        if (m_MappedData != nullptr) release_mapping();

        OCTVIS_TRACE(
                "Deleting Buffer '{}, {:#06x}, {:#06x}, {:#06x}'",
                m_BufferID,
                (GLenum) m_Type,
                (GLenum) m_Usage,
                m_SizeInBytes
        );
        GL_CALL(glDeleteBuffers(1, &m_BufferID));
    }

    Buffer::Buffer(Buffer&& other) {
        std::swap(m_BufferID, other.m_BufferID);
        std::swap(m_Type, other.m_Type);
        std::swap(m_Usage, other.m_Usage);
        std::swap(m_SizeInBytes, other.m_SizeInBytes);
        std::swap(m_MappedData, other.m_MappedData);
    }

    Buffer& Buffer::operator =(Buffer&& other) {
        std::swap(m_BufferID, other.m_BufferID);
        std::swap(m_Type, other.m_Type);
        std::swap(m_Usage, other.m_Usage);
        std::swap(m_SizeInBytes, other.m_SizeInBytes);
        std::swap(m_MappedData, other.m_MappedData);
        return *this;
    }

    void Buffer::bind() const {
        OCTVIS_ASSERT(is_valid(), "Buffer is invalid...");
        GL_CALL(glBindBuffer(get_type(), m_BufferID));
    }

    void Buffer::unbind() const {
        GL_CALL(glBindBuffer(get_type(), 0));
    }

    bool Buffer::is_bound() const {
        id_t bound_buffer = INVALID_ID;
        switch (m_Type) {

            case BufferType::ARRAY:
                glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*) &bound_buffer);
                break;

            case BufferType::ELEMENT:
                glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint*) &bound_buffer);
                break;

            case BufferType::UNIFORM:
                glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, (GLint*) &bound_buffer);
                break;

            case BufferType::SSBO:
                glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, (GLint*) &bound_buffer);
                break;

        }
        return is_valid() && bound_buffer == m_BufferID;
    }

    void Buffer::init_raw(size_t bytes, const void* const data, BufferUsage usage) {
        OCTVIS_ASSERT(is_valid(), "Buffer is invalid");
        OCTVIS_ASSERT(bytes > 0, "Buffer size can't be zero");
        bind();
        OCTVIS_TRACE("Buffer Data ( {}, {:p}, {:#06x} )", bytes, data, (GLenum) usage);
        GL_CALL(glBufferData(get_type(), bytes, data, (GLenum) usage));
        m_Usage = usage;
        m_SizeInBytes = bytes;
        unbind();
    }

    void Buffer::set_range(size_t start_bytes, const void* const data, size_t size_bytes) const {
        bind();
        GL_CALL(glBufferSubData(get_type(), start_bytes, size_bytes, data));
        unbind();
    }

    void* Buffer::create_mapping(BufferMapping mode) {
        OCTVIS_ASSERT(m_MappedData == nullptr, "Buffer has already been mapped.");
        bind();
        m_MappedData = GL_CALL(glMapBuffer(get_type(), static_cast<GLenum>(mode)));
        unbind();
        return m_MappedData;
    }

    void Buffer::release_mapping() {
        OCTVIS_ASSERT(m_MappedData != nullptr, "Buffer has not been mapped.");
        bind();
        GL_CALL(glUnmapBuffer(get_type()));
        m_MappedData = nullptr;
        unbind();
    }

    //############################################################################//
    // | TEXTURE |
    //############################################################################//

    RawImage::~RawImage() noexcept {
        if (!dealloc_pixel_data || pixel_data == nullptr) return;
        stbi_image_free(pixel_data);
    }

    void RawImage::load_from_path(const char* path, int desired_channels) {
        void* data = stbi_load(path, &width, &height, &channels, desired_channels);

        // Delete if existing then assign
        if (data != nullptr && pixel_data != nullptr) stbi_image_free(pixel_data);
        pixel_data = static_cast<pixel_t*>(data);

    }

    Texture2D::Texture2D() : m_Identity(INVALID_ID) {}

    Texture2D::~Texture2D() {
        if (!is_valid()) return;
        deinit();
    }

    void Texture2D::init(const RawImage& img) {
        OCTVIS_ASSERT(!is_valid(), "Texture {:#06x} has already been initialised", m_Identity);
        OCTVIS_ASSERT(img.valid(), "The provided Image is invalid; RawImage: '{}'", img.to_string());

        GL_CALL(glGenTextures(1, &m_Identity));
        OCTVIS_ASSERT(m_Identity != INVALID_ID, "Failed to generate texture id");
        bind();

        GL_CALL(glTexImage2D(
                GL_TEXTURE_2D,
                img.level,
                static_cast<GLint>(img.format),
                img.width,
                img.height,
                0,
                static_cast<GLenum>(img.format),
                static_cast<GLenum>(img.pixel_type),
                img.data()
        ));

        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
        unbind();

    }

    void Texture2D::deinit() {
        if (!is_valid()) return;
        OCTVIS_TRACE("Deleting Texture2D -> {:#06x}", m_Identity);
        GL_CALL(glDeleteTextures(1, &m_Identity));
    }

    void Texture2D::bind(index_t index) const {
        OCTVIS_ASSERT(is_valid(), "Texture is invalid");
        OCTVIS_ASSERT(is_index_valid(index), "Index is invalid");

        GL_CALL(glActiveTexture(GL_TEXTURE0 + index));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, m_Identity));
    }

    void Texture2D::unbind(index_t index) const {
        OCTVIS_ASSERT(is_index_valid(index), "Index is invalid");
        GL_CALL(glActiveTexture(GL_TEXTURE0 + index));
        GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    }

    void Texture2D::unbind_all() {
        unbind_range(0, MAX_INDEX + 1);
    }

    void Texture2D::unbind_range(Texture2D::index_t begin, Texture2D::index_t end) {
        OCTVIS_ASSERT(is_index_valid(begin), "Begin index is invalid");
        OCTVIS_ASSERT(is_index_valid(end), "End index is invalid");

        for (index_t i = begin; i < end; ++i) {
            GL_CALL(glActiveTexture(GL_TEXTURE0 + i));
            GL_CALL(glBindTexture(GL_TEXTURE_2D, INVALID_ID));
        }
    }

    //############################################################################//
    // | SINGLE SHADER |
    //############################################################################//

    Shader::Shader(ShaderType type) noexcept: m_ShaderID(INVALID_ID), m_Type(type) {

    }

    Shader::Shader(ShaderType type, const char* path) noexcept: Shader(type) {
        load_from_path(path);
    }

    Shader::~Shader() noexcept {
        if (!is_valid()) return;
        OCTVIS_TRACE(
                "Destroying Shader: '{}', {}",
                m_ShaderID,
                (m_Type == ShaderType::VERTEX)
                ? "VertexShader"
                : "FragmentShader"
        );
        GL_CALL(glDeleteShader(m_ShaderID));
    }

    Shader::Shader(Shader&& other) {
        std::swap(m_ShaderID, other.m_ShaderID);
        std::swap(m_Type, other.m_Type);
    }

    Shader& Shader::operator =(Shader&& other) {
        std::swap(m_ShaderID, other.m_ShaderID);
        std::swap(m_Type, other.m_Type);
        return *this;
    }

    void Shader::load_from_path(const char* path) {

        using It = std::istreambuf_iterator<char>;
        std::fstream fstream{ path };

        if (!fstream.is_open()) {
            OCTVIS_ERROR("Failed to open file '{}'", path);
            return;
        }

        std::string file_content{ It{ fstream }, It{} };
        OCTVIS_TRACE("Loading Shader from '{}'", path);
        load_from_bytes(file_content.c_str());
    }

    void Shader::load_from_bytes(const char* bytes) {

        OCTVIS_ASSERT(
                !is_valid(),
                "Shader needs to be deinitialised before being initialised again."
        );

        // Create and Assign Resources
        m_ShaderID = GL_CALL(glCreateShader(get_type_i()));
        GL_CALL(glShaderSource(m_ShaderID, 1, &bytes, nullptr));
        GL_CALL(glCompileShader(m_ShaderID));

        // Ensure Compilation was successful
        GLint compile_okay;
        GL_CALL(glGetShaderiv(m_ShaderID, GL_COMPILE_STATUS, &compile_okay));
        if (compile_okay == GL_TRUE) return;

        // Get Error Information
        GLint info_log_length = 0;
        GL_CALL(glGetShaderiv(m_ShaderID, GL_INFO_LOG_LENGTH, &info_log_length));
        OCTVIS_ASSERT(info_log_length > 0, "Log length for shader is invalid...");
        char* info_message = new char[info_log_length]{};
        GL_CALL(glGetShaderInfoLog(m_ShaderID, info_log_length, nullptr, info_message));

        // Log the error and cleanup
        OCTVIS_ERROR("Failed to compile shader\n{}", info_message);
        delete[] info_message;
        deinit();
        OCTVIS_ASSERT(false, "Failed to compile shader...");
    }

    void Shader::deinit() {
        OCTVIS_TRACE("Deinitialising Shader '{}', '{:#06x}'", m_ShaderID, get_type_i());
        GL_CALL(glDeleteShader(m_ShaderID));
        m_ShaderID = INVALID_ID;
    }

    bool Shader::is_valid() const noexcept {
        return m_ShaderID != INVALID_ID;
    }

    void Shader::attach(ShaderProgram& program) const noexcept {
        OCTVIS_ASSERT(is_valid(), "Can't attach invalid shader to {}", program.get_id());
        OCTVIS_ASSERT(program.is_valid(), "Can't attach shader to invalid program {}", program.get_id());
        GL_CALL(glAttachShader(program.get_id(), m_ShaderID));
    }

    void Shader::detach(ShaderProgram& program) const noexcept {
        OCTVIS_ASSERT(is_valid(), "Can't deattach invalid shader to {}", program.get_id());
        OCTVIS_ASSERT(program.is_valid(), "Can't deattach shader to invalid program {}", program.get_id());
        GL_CALL(glDetachShader(program.get_id(), m_ShaderID));
    }

    //############################################################################//
    // | SHADER PROGRAM |
    //############################################################################//

    ShaderProgram::ShaderProgram() : m_ShaderProgramID(INVALID_ID) {

    }

    ShaderProgram::ShaderProgram(
            const Shader& vertex,
            const Shader& fragment
    ) noexcept: ShaderProgram() {
        OCTVIS_ASSERT(
                vertex.is_vertex_shader() && fragment.is_fragment_shader(),
                "Shader type/s are invalid"
        );
        attach_shader(vertex);
        attach_shader(fragment);
    }

    ShaderProgram::~ShaderProgram() {
        if (is_valid()) deinit();
    }

    ShaderProgram::ShaderProgram(ShaderProgram&& other) {
        std::swap(this->m_ShaderProgramID, other.m_ShaderProgramID);
    }

    ShaderProgram& ShaderProgram::operator =(ShaderProgram&& other) {
        std::swap(this->m_ShaderProgramID, other.m_ShaderProgramID);
        return *this;
    }

    void ShaderProgram::init() {
        OCTVIS_ASSERT(!is_valid(), "Already initialised ShaderProgram!");
        m_ShaderProgramID = GL_CALL(glCreateProgram());
        OCTVIS_TRACE("Created Shader Program: {}", m_ShaderProgramID);
    }

    void ShaderProgram::deinit() {
        OCTVIS_ASSERT(is_valid(), "Can't delete invalid ShaderProgram!");
        OCTVIS_TRACE("Deleting Shader Program: {}", m_ShaderProgramID);
        GL_CALL(glDeleteProgram(m_ShaderProgramID));
        m_ShaderProgramID = INVALID_ID;
    }

    void ShaderProgram::attach_shader(const Shader& shader) {
        OCTVIS_ASSERT(is_valid(), "Can't attach shader '{}' to invalid program.", shader.get_id());
        shader.attach(*this);
    }

    void ShaderProgram::detach_shader(const Shader& shader) {
        OCTVIS_ASSERT(is_valid(), "Can't deatach shader '{}' to invalid program.", shader.get_id());
        shader.detach(*this);
    }

    void ShaderProgram::link() {
        OCTVIS_ASSERT(is_valid(), "Program is invalid.");
        GL_CALL(glLinkProgram(m_ShaderProgramID));

        // Check for linking errors
        GLint link_status;
        GL_CALL(glGetProgramiv(m_ShaderProgramID, GL_LINK_STATUS, &link_status));

        if (link_status == GL_FALSE) {
            GLint info_log_length;
            GL_CALL(glGetProgramiv(m_ShaderProgramID, GL_INFO_LOG_LENGTH, &info_log_length));
            std::vector<char> info_log(info_log_length + 1);

            if (info_log_length > 0) {
                glGetProgramInfoLog(m_ShaderProgramID, info_log_length, nullptr, info_log.data());
            }

            OCTVIS_ASSERT(false, "Shader Program linking failed; '{}'", info_log.data());
        }
    }

    bool ShaderProgram::is_valid() const noexcept {
        return m_ShaderProgramID != INVALID_ID;
    }

    void ShaderProgram::activate() const {
        OCTVIS_ASSERT(is_valid(), "Shader Program is invalid.");
        GL_CALL(glUseProgram(m_ShaderProgramID));
    }

    void ShaderProgram::deactivate() const {
        OCTVIS_ASSERT(is_valid(), "Shader Program is invalid.");
        GL_CALL(glUseProgram(INVALID_ID));
    }

    int ShaderProgram::get_uniform(const char* name) const noexcept {
        OCTVIS_ASSERT(is_valid(), "ShaderProgram is invalid!");
        GLint id = GL_CALL(glGetUniformLocation(m_ShaderProgramID, name));
        OCTVIS_ASSERT(id != -1, "Uniform '{}' does not exist.", name);
        return id;
    }

    void ShaderProgram::set_float(const char* name, float value) {
        GL_CALL(glUniform1f(get_uniform(name), value));
    }

    void ShaderProgram::set_vec2(const char* name, glm::vec2 value) {
        GL_CALL(glUniform2f(get_uniform(name), value.x, value.y));
    }

    void ShaderProgram::set_vec3(const char* name, glm::vec3 value) {
        GL_CALL(glUniform3f(get_uniform(name), value.x, value.y, value.z));
    }

    void ShaderProgram::set_mat3(const char* name, const glm::mat3& value) {
        GL_CALL(glUniformMatrix3fv(get_uniform(name), 1, GL_FALSE, glm::value_ptr(value)));
    }

    void ShaderProgram::set_mat4(const char* name, const glm::mat4& value) {
        GL_CALL(glUniformMatrix4fv(get_uniform(name), 1, GL_FALSE, glm::value_ptr(value)));
    }

    void ShaderProgram::set_ubo(const Buffer& ubo, unsigned int index, const char* name) {
        OCTVIS_ASSERT(ubo.is_valid() && ubo.is_uniform_buffer(), "Provided buffer is invlid; {:#06x}", ubo.id());

        GL_CALL(glBindBufferBase(GL_UNIFORM_BUFFER, index, ubo.id()));
        GLuint ubo_index = GL_CALL(glGetUniformBlockIndex(m_ShaderProgramID, name));
        GL_CALL(glUniformBlockBinding(m_ShaderProgramID, ubo_index, index));
    }

    void ShaderProgram::set_texture(const Texture2D& texture, Texture2D::index_t index, const char* name) {
        texture.bind(index);
        GLint location = GL_CALL(glGetUniformLocation(m_ShaderProgramID, name));
        OCTVIS_ASSERT(location != -1, "Uniform '{}' is invalid", name);
        GL_CALL(glUniform1i(location, index));
    }

    //############################################################################//
    // | VERTEX ARRAY OBJECT |
    //############################################################################//

    VertexArrayObject::VertexArrayObject() : m_Identity(INVALID_ID) {

    }

    VertexArrayObject::~VertexArrayObject() {
        if (is_valid()) GL_CALL(glDeleteVertexArrays(1, &m_Identity));
    }

    VertexArrayObject::VertexArrayObject(VertexArrayObject&& other) {
        std::swap(m_Identity, other.m_Identity);
    }

    VertexArrayObject& VertexArrayObject::operator =(VertexArrayObject&& other) {
        std::swap(m_Identity, other.m_Identity);
        return *this;
    }

    void VertexArrayObject::init() {
        OCTVIS_ASSERT(!is_valid(), "VAO is already initialised.");
        GL_CALL(glGenVertexArrays(1, &m_Identity));
    }

    bool VertexArrayObject::is_valid() const {
        return m_Identity != INVALID_ID;
    }

    void VertexArrayObject::bind() const {
        OCTVIS_ASSERT(is_valid(), "VAO is invalid.");
        GL_CALL(glBindVertexArray(m_Identity));
    }

    void VertexArrayObject::unbind() const {
        OCTVIS_ASSERT(is_valid(), "VAO is invalid.");
        GL_CALL(glBindVertexArray(INVALID_ID));
    }

    bool VertexArrayObject::is_bound() const {
        id_t active = INVALID_ID;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (int*) &active);
        return active != INVALID_ID && active == m_Identity;
    }

    VertexArrayObject& VertexArrayObject::attach_buffer(const Buffer& buffer) {
        if (!is_bound()) bind();
        buffer.bind();
        return *this;
    }

    VertexArrayObject& VertexArrayObject::enable_attribute(index_t index, bool is_enabled) {
        if (!is_bound()) bind();
        if (is_enabled) {
            GL_CALL(glEnableVertexAttribArray(index));
        } else {
            GL_CALL(glDisableVertexAttribArray(index));
        }
        return *this;
    }

    VertexArrayObject& VertexArrayObject::enable_attribute_range(
            index_t begin, index_t end, bool is_enabled
    ) {
        if (!is_bound()) bind();
        for (index_t i = begin; i < end; ++i) {
            if (is_enabled) {
                GL_CALL(glEnableVertexAttribArray(i));
            } else {
                GL_CALL(glDisableVertexAttribArray(i));
            }
        }
        return *this;
    }

    VertexArrayObject& VertexArrayObject::set_divisor(index_t index, index_t divisor) {
        if (!is_bound()) bind();
        GL_CALL(glVertexAttribDivisor(index, divisor));
        return *this;
    }

    VertexArrayObject& VertexArrayObject::set_divisor_range(
            index_t begin, index_t end, index_t divisor
    ) {
        for (int i = begin; i < end; ++i) {
            GL_CALL(glVertexAttribDivisor(i, divisor));
        }
        return *this;
    }

}