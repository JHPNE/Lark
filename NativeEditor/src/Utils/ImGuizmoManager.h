#pragma once
#include "imgui.h"
#include "ImGuizmo.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ImGuizmoManager {
public:
    enum class Operation {
        None = -1,
        Translate = ImGuizmo::TRANSLATE,
        Rotate = ImGuizmo::ROTATE,
        Scale = ImGuizmo::SCALE
    };

    static ImGuizmoManager& Get() {
        static ImGuizmoManager instance;
        return instance;
    }

    void Initialize() {
        ImGuizmo::Enable(true);
        m_currentOperation = Operation::Translate;
        m_currentMode = ImGuizmo::LOCAL;
        m_initialized = true;
    }

    bool IsInitialized() const { return m_initialized; }

    void BeginFrame() {
        if (!m_initialized) return;
        ImGuizmo::BeginFrame();
    }

    void SetOrthographic(bool isOrtho) {
        m_isOrthographic = isOrtho;
    }

    void ConfigureStyle() {
        ImGuizmo::SetOrthographic(m_isOrthographic);
        ImGuizmo::AllowAxisFlip(false);  // Prevent axis flipping
    }

    void SetRect(float x, float y, float width, float height) {
        ImGuizmo::SetRect(x, y, width, height);
    }

    void HandleInput() {
        // Handle keyboard shortcuts for operation changes
        if (ImGui::IsKeyPressed(ImGuiKey_T))
            m_currentOperation = Operation::Translate;
        if (ImGui::IsKeyPressed(ImGuiKey_R))
            m_currentOperation = Operation::Rotate;
        if (ImGui::IsKeyPressed(ImGuiKey_S))
            m_currentOperation = Operation::Scale;

        // Handle space to switch between local/world space
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
            m_currentMode = (m_currentMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    }

    bool Manipulate(const glm::mat4& view, const glm::mat4& projection,
                   glm::mat4& matrix, bool snap = false, const glm::vec3& snapValues = glm::vec3(1.0f)) {
        ConfigureStyle();

        float snapValuesArray[3] = { snapValues.x, snapValues.y, snapValues.z };

        return ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            static_cast<ImGuizmo::OPERATION>(m_currentOperation),
            m_currentMode,
            glm::value_ptr(matrix),
            nullptr,
            snap ? snapValuesArray : nullptr
        );
    }

    void DecomposeTransform(const glm::mat4& matrix, glm::vec3& position, glm::vec3& rotation, glm::vec3& scale) {
        float matrixValues[16];
        float position_[3], rotation_[3], scale_[3];

        // Copy matrix values to array
        memcpy(matrixValues, glm::value_ptr(matrix), 16 * sizeof(float));

        // Decompose the matrix
        ImGuizmo::DecomposeMatrixToComponents(matrixValues, position_, rotation_, scale_);

        // Convert to glm vectors
        position = glm::vec3(position_[0], position_[1], position_[2]);
        rotation = glm::vec3(rotation_[0], rotation_[1], rotation_[2]);
        scale = glm::vec3(scale_[0], scale_[1], scale_[2]);
    }

    void RecomposeTransform(glm::mat4& matrix, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
        float matrixValues[16];
        float position_[3] = { position.x, position.y, position.z };
        float rotation_[3] = { rotation.x, rotation.y, rotation.z };
        float scale_[3] = { scale.x, scale.y, scale.z };

        ImGuizmo::RecomposeMatrixFromComponents(position_, rotation_, scale_, matrixValues);
        matrix = glm::make_mat4(matrixValues);
    }

    bool IsUsing() const { return ImGuizmo::IsUsing(); }
    bool IsOver() const { return ImGuizmo::IsOver(); }
    Operation GetCurrentOperation() const { return m_currentOperation; }
    ImGuizmo::MODE GetCurrentMode() const { return m_currentMode; }

private:
    ImGuizmoManager() = default;
    ~ImGuizmoManager() = default;

    Operation m_currentOperation{Operation::None};
    ImGuizmo::MODE m_currentMode{ImGuizmo::LOCAL};
    bool m_isOrthographic{false};
    bool m_initialized{false};
};