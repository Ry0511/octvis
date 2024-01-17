//
// Date       : 11/01/2024
// Project    : octvis
// Author     : -Ry
//

#include "Camera.h"

octvis::Camera::Camera() noexcept:
        m_Position(0, 0, 0), m_Rotate(0, 0, 0),
        // These have fixed values in compute_view_matrix
        m_Up(), m_Forward(), m_Right(),
        m_Projection(), m_ViewMatrix(),
        m_IsMovementXZ(false) {
    set_projection(90.0F, 0.1F, 100.0F, 16.0F / 10.0F);
    compute_view_matrix();
}

void octvis::Camera::translate_forward(float dt) noexcept {
    if (m_IsMovementXZ) {
        glm::vec3 forward_xz = glm::normalize(glm::vec3(m_Forward.x, 0.0f, m_Forward.z));
        m_Position += forward_xz * dt;
    } else {
        m_Position += m_Forward * dt;
    }
    compute_view_matrix();
}

void octvis::Camera::translate_horizontal(float dt) noexcept {
    if (m_IsMovementXZ) {
        glm::vec3 right_xz = glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z));
        m_Position += right_xz * dt;
    } else {
        m_Position += m_Right * dt;
    }
    compute_view_matrix();
}

void octvis::Camera::translate(const glm::vec2& dt) noexcept {
    m_Position += m_Forward * dt.x;
    m_Position += m_Right * dt.x;
    compute_view_matrix();
}

void octvis::Camera::look_vertical(float dt) noexcept {
    m_Rotate.y += dt;
    normalise_rotation();
    compute_view_matrix();
}

void octvis::Camera::look_horizontal(float dt) noexcept {
    m_Rotate.x += dt;
    normalise_rotation();
    compute_view_matrix();
}

void octvis::Camera::look_pitch(float dt) noexcept {
    m_Rotate.z += dt;
    normalise_rotation();
    compute_view_matrix();
}

void octvis::Camera::look(const glm::vec3& dt) noexcept {
    m_Rotate += dt;
    normalise_rotation();
    compute_view_matrix();
}

void octvis::Camera::look(const glm::vec2& dt) noexcept {
    m_Rotate += glm::vec3{ dt, 0.0F };
    normalise_rotation();
    compute_view_matrix();
}
