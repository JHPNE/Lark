#include "ComponentView.h"
#include "../ViewModels/ComponentViewModel.h"
#include "Style/CustomWidgets.h"
#include "Style/CustomWindow.h"

#include <imgui.h>

using namespace LarkStyle;

ComponentView::ComponentView() {
    m_viewModel = std::make_unique<ComponentViewModel>();
}

ComponentView::~ComponentView() = default;

void ComponentView::SetActiveProject(std::shared_ptr<Project> activeProject) {
    if (m_viewModel) {
        m_viewModel->SetProject(activeProject);
    }
}

void ComponentView::Draw() {
    if (!m_show || !m_viewModel) return;

    CustomWindow::WindowConfig config;
    config.title = "Component View";
    config.icon = "◈";  // Optional icon, can be empty
    config.p_open = &m_show;
    config.allowDocking = true;
    config.defaultSize = ImVec2(350, 600);
    config.minSize = ImVec2(250, 400);

    if (CustomWindow::Begin("ComponentView", config)) {
        if (m_viewModel->HasSingleSelection.Get()) {
            DrawSingleSelection();
        } else if (m_viewModel->HasMultipleSelection.Get()) {
            DrawMultiSelection();
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No entity selected");
        }
    }
    CustomWindow::End();
}

void ComponentView::DrawSingleSelection() {
    auto entity = m_viewModel->SelectedEntity.Get();
    if (!entity) return;
    
    // Entity header
    CustomWidgets::BeginPanel("EntityInfo", ImVec2(0, 60));
    ImGui::Text("Entity");
    ImGui::SameLine();
    ImGui::TextColored(Colors::AccentInfo, "%s", entity->GetName().c_str());
    ImGui::Text("ID: %u", entity->GetID());
    CustomWidgets::EndPanel();

    // Transform Component
    if (m_viewModel->HasTransform.Get()) {
        DrawTransformComponent();
    }
    
    // Script Component
    if (m_viewModel->HasScript.Get()) {
        DrawScriptComponent();
    }
    
    // Geometry Component
    if (m_viewModel->HasGeometry.Get()) {
        DrawGeometryComponent();
    }

    // Physics Component
    if (m_viewModel->HasPhysics.Get()){
        DrawPhysicsComponent();
    }

    // Material Component
    if (m_viewModel->HasMaterial.Get()){
        DrawMaterialComponent();
    }

    // Drone Component
    if (entity->GetComponent<Drone>()) {
        DrawDroneComponent();
    }
    
    // Add Component Button
    DrawAddComponentButton();
}

void ComponentView::DrawMultiSelection() {
    ImGui::Text("Selected Entities: %zu", m_viewModel->SelectionCount.Get());
    
    for (const auto& entity : m_viewModel->SelectedEntities.Get()) {
        ImGui::BulletText("%s", entity->GetName().c_str());
    }
    
    ImGui::Separator();
    
    // Multi-Transform
    if (m_viewModel->HasTransform.Get()) {
        if (ImGui::CollapsingHeader("Multi-Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            TransformData avgTransform = m_viewModel->AverageTransform.Get();
            
            // Position
            float pos[3] = {avgTransform.position.x, avgTransform.position.y, avgTransform.position.z};
            if (ImGui::DragFloat3("Position##Multi", pos, 0.1f)) {
                glm::vec3 newPos(pos[0], pos[1], pos[2]);
                m_viewModel->UpdatePositionCommand->Execute(newPos);
            }
            
            // Rotation
            float rot[3] = {avgTransform.rotation.x, avgTransform.rotation.y, avgTransform.rotation.z};
            if (ImGui::DragFloat3("Rotation##Multi", rot, 0.1f)) {
                glm::vec3 newRot(rot[0], rot[1], rot[2]);
                m_viewModel->UpdateRotationCommand->Execute(newRot);
            }
            
            // Scale
            float scale[3] = {avgTransform.scale.x, avgTransform.scale.y, avgTransform.scale.z};
            if (ImGui::DragFloat3("Scale##Multi", scale, 0.1f)) {
                glm::vec3 newScale(scale[0], scale[1], scale[2]);
                m_viewModel->UpdateScaleCommand->Execute(newScale);
            }
        }
    }
    
    // Common Scripts
    if (!m_viewModel->CommonScripts.Get().empty()) {
        if (ImGui::CollapsingHeader("Common Scripts", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& scriptName : m_viewModel->CommonScripts.Get()) {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
                
                ImGui::BeginChild("CommonScriptBox", ImVec2(ImGui::GetContentRegionAvail().x, 30), true);
                ImGui::Text("Script: %s", scriptName.c_str());
                ImGui::EndChild();
                
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }
        }
    }
}

void ComponentView::DrawTransformComponent() {
    if (!CustomWidgets::BeginSection("Transform", true))
        return;
    
    TransformData transform = m_viewModel->CurrentTransform.Get();
    bool changed = false;

    CustomWidgets::BeginPropertyTable();
    
    // Position with X/Y/Z color coding
    float pos[3] = {transform.position.x, transform.position.y, transform.position.z};
    if (CustomWidgets::PropertyFloat3("Position", pos, "%.2f")) {
        transform.position = glm::vec3(pos[0], pos[1], pos[2]);
        changed = true;
    }
    
    // Rotation
    float rot[3] = {transform.rotation.x, transform.rotation.y, transform.rotation.z};
    if (CustomWidgets::PropertyFloat3("Rotation", rot, "%.1f")) {
        transform.rotation = glm::vec3(rot[0], rot[1], rot[2]);
        changed = true;
    }
    
    // Scale with warning color if not uniform
    float scale[3] = {transform.scale.x, transform.scale.y, transform.scale.z};
    bool uniformScale = (scale[0] == scale[1] && scale[1] == scale[2]);

    if (!uniformScale) {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentWarning);
    }

    if (CustomWidgets::PropertyFloat3("Scale", scale, "%.2f")) {
        transform.scale = glm::vec3(scale[0], scale[1], scale[2]);
        changed = true;
    }

    if (!uniformScale) {
        ImGui::PopStyleColor();
    }

    CustomWidgets::EndPropertyTable();

    // Reset buttons
    ImGui::Spacing();
    if (CustomWidgets::Button("Reset Position", ImVec2(100, 0))) {
        transform.position = glm::vec3(0, 0, 0);
        changed = true;
    }
    ImGui::SameLine();
    if (CustomWidgets::Button("Reset Rotation", ImVec2(100, 0))) {
        transform.rotation = glm::vec3(0, 0, 0);
        changed = true;
    }
    ImGui::SameLine();
    if (CustomWidgets::Button("Reset Scale", ImVec2(100, 0))) {
        transform.scale = glm::vec3(1, 1, 1);
        changed = true;
    }

    if (changed) {
        if (!m_viewModel->IsEditingTransform.Get()) {
            m_viewModel->StartTransformEdit();
        }
        m_viewModel->UpdateTransformCommand->Execute(transform);
    }
    
    // End edit on mouse release
    if (m_viewModel->IsEditingTransform.Get() && ImGui::IsMouseReleased(0)) {
        m_viewModel->EndTransformEdit();
    }

    CustomWidgets::EndSection();
}

void ComponentView::DrawScriptComponent() {
    if (!CustomWidgets::BeginSection("Script", true))
        return;
    
    CustomWidgets::BeginPanel("ScriptInfo", ImVec2(0, 40));
    ImGui::Text("Script: ");
    ImGui::SameLine();
    ImGui::TextColored(Colors::AccentSuccess, "%s", m_viewModel->ScriptName.Get().c_str());
    CustomWidgets::EndPanel();

    ImGui::Spacing();

    if (CustomWidgets::ColoredButton("Remove Script", WidgetColorType::Danger, ImVec2(120, 0))) {
        m_viewModel->RemoveScriptCommand->Execute();
    }

    CustomWidgets::EndSection();
}

void ComponentView::DrawGeometryComponent() {
    if (!CustomWidgets::BeginSection("Geometry", true))
        return;

    CustomWidgets::BeginPropertyTable();

    ImGui::Text("Type");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::TextColored(Colors::AccentInfo,
                      m_viewModel->GeometryType.Get() == GeometryType::PrimitiveType ?
                      "Primitive" : "Imported");

    ImGui::Text("Name");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::TextColored(Colors::Text, "%s", m_viewModel->GeometryName.Get().c_str());

    bool visible = m_viewModel->GeometryVisible.Get();
    CustomWidgets::PropertyBool("Visible", &visible);
    if (visible != m_viewModel->GeometryVisible.Get()) {
        m_viewModel->SetGeometryVisibilityCommand->Execute(visible);
    }

    CustomWidgets::EndPropertyTable();

    ImGui::Spacing();
    CustomWidgets::SeparatorText("Actions");

    if (CustomWidgets::ColoredButton("Randomize Vertices",
                                     WidgetColorType::Warning,
                                     ImVec2(150, 0))) {
        m_viewModel->RandomizeGeometryCommand->Execute();
                                     }

    CustomWidgets::EndSection();
}

void ComponentView::DrawPhysicsComponent()
{
    if (!CustomWidgets::BeginSection("Physics", true)) return;


    auto* physics = m_viewModel->SelectedEntity.Get()->GetComponent<Physics>();
    if (!physics)
    {
        CustomWidgets::EndSection();
        return;
    }

    CustomWidgets::BeginPropertyTable();

    // TODO: updates over ComponetnViewModel and Service instead of directly
    float mass = physics->GetMass();
    if (CustomWidgets::PropertyFloat("Mass", &mass, 0.01f, 1000.0f, "%.2f"))
    {
        physics->SetMass(mass);

        if (auto scene = m_viewModel->SelectedEntity.Get()->GetScene().lock()) {
            scene->UpdateEntity(m_viewModel->SelectedEntity.Get()->GetID());
        }
    }

    bool kinematic = physics->IsKinematic();
    if (CustomWidgets::PropertyBool("Kinematic", &kinematic)) {
        physics->SetKinematic(kinematic);

        if (auto scene = m_viewModel->SelectedEntity.Get()->GetScene().lock()) {
            scene->UpdateEntity(m_viewModel->SelectedEntity.Get()->GetID());
        }
    }

    glm::vec3 inertia = physics->GetInertia();
    if (CustomWidgets::PropertyFloat3("Inertia", &inertia.x, "%.3f")) {
        physics->SetInertia(inertia);

        if (auto scene = m_viewModel->SelectedEntity.Get()->GetScene().lock()) {
            scene->UpdateEntity(m_viewModel->SelectedEntity.Get()->GetID());
        }
    }

    CustomWidgets::EndPropertyTable();

    ImGui::Spacing();

    if (CustomWidgets::ColoredButton("Remove Physics", WidgetColorType::Danger, ImVec2(120, 0))) {
        // Execute remove physics command
    }

    CustomWidgets::EndSection();
}

void ComponentView::DrawMaterialComponent() {
    if (!CustomWidgets::BeginSection("Material", true))
        return;

    auto* material = m_viewModel->SelectedEntity.Get()->GetComponent<Material>();
    if (!material) {
        CustomWidgets::EndSection();
        return;
    }

    bool materialChanged = false;

    CustomWidgets::BeginPropertyTable();

    const char* materialTypes[] = {
        "Lambertian", "Metal", "Dielectric"
    };
    int currentType = static_cast<int>(m_viewModel->MaterialTypeUsed.Get());

    ImGui::Text("Material Type");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::PushItemWidth(Sizing::PropertyControlWidth);

    if (ImGui::Combo("##MaterialType", &currentType, materialTypes, IM_ARRAYSIZE(materialTypes))) {
        m_viewModel->MaterialTypeUsed.Set(static_cast<MaterialType>(currentType));
        materialChanged = true;
    }

    ImGui::PopItemWidth();

    glm::vec3 albedo = m_viewModel->MaterialAlbedo.Get();
    if (CustomWidgets::PropertyFloat3("Albedo", &albedo.x, "%.3f")) {
        m_viewModel->MaterialAlbedo.Set(albedo);
        materialChanged = true;
    }

    float roughness = m_viewModel->MaterialRoughness.Get();
    if (CustomWidgets::PropertyFloat("Roughness", &roughness, 0.0f, 1.0f, "%.3f")) {
        m_viewModel->MaterialRoughness.Set(roughness);
        materialChanged = true;
    }

    float ao = m_viewModel->MaterialAO.Get();
    if (CustomWidgets::PropertyFloat("AO", &ao, 0.0f, 1.0f, "%.3f")) {
        m_viewModel->MaterialAO.Set(ao);
        materialChanged = true;
    }

    float metallic = m_viewModel->MaterialMetallic.Get();
    if (CustomWidgets::PropertyFloat("Mettalic", &metallic, 0.0f, 1.0f, "%.3f")) {
        m_viewModel->MaterialMetallic.Set(metallic);
        materialChanged = true;
    }

    glm::vec3 emissive = m_viewModel->MaterialEmissive.Get();
    if (CustomWidgets::PropertyFloat3("Emissive", &emissive.x, "%.3f")) {
        m_viewModel->MaterialEmissive.Set(emissive);
        materialChanged = true;
    }

    float ior = m_viewModel->MaterialIOR.Get();
    if (CustomWidgets::PropertyFloat("IOR", &ior, 1.0f, 3.0f, "%.3f")) {
        m_viewModel->MaterialIOR.Set(ior);
        materialChanged = true;
    }

    float transparency = m_viewModel->MaterialTransparency.Get();
    if (CustomWidgets::PropertyFloat("Transparency", &transparency, 0.0f, 1.0f, "%.3f")) {
        m_viewModel->MaterialTransparency.Set(transparency);
        materialChanged = true;
    }

    CustomWidgets::EndPropertyTable();

    if (materialChanged)
    {
        m_viewModel->UpdateMaterialCommand->Execute();
    }

    ImGui::Spacing();

    if (CustomWidgets::ColoredButton("Remove Material", WidgetColorType::Danger, ImVec2(130, 0))) {
        m_viewModel->RemoveMaterialCommand->Execute();
    }

    CustomWidgets::EndSection();
}

void ComponentView::DrawDroneComponent() {
    if (!CustomWidgets::BeginSection("Drone", true))
        return;

    if (!m_viewModel->HasDrone.Get()) {
        ImGui::TextColored(Colors::TextDim, "No drone component");
        CustomWidgets::EndSection();
        return;
    }

    CustomWidgets::BeginPropertyTable();

    // Control abstraction dropdown
    const char* controlModes[] = {
        "Motor Speeds", "Motor Thrusts", "Body Rates",
        "Body Moments", "Attitude", "Velocity", "Acceleration"
    };
    int currentMode = static_cast<int>(m_viewModel->DroneControlAbstraction.Get());

    ImGui::Text("Control Mode");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::PushItemWidth(Sizing::PropertyControlWidth);
    if (ImGui::Combo("##ControlMode", &currentMode, controlModes, IM_ARRAYSIZE(controlModes))) {
        m_viewModel->UpdateDroneControlCommand->Execute(static_cast<control_abstraction>(currentMode));
    }
    ImGui::PopItemWidth();

    // Trajectory type
    const char* trajectoryTypes[] = {"Circular", "Chaos"};
    int trajType = static_cast<int>(m_viewModel->DroneTrajectoryType.Get());

    ImGui::Text("Trajectory");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::PushItemWidth(Sizing::PropertyControlWidth);
    if (ImGui::Combo("##Trajectory", &trajType, trajectoryTypes, IM_ARRAYSIZE(trajectoryTypes))) {
        m_viewModel->UpdateDroneTrajectoryCommand->Execute(static_cast<trajectory_type>(trajType));
    }
    ImGui::PopItemWidth();

    CustomWidgets::EndPropertyTable();

    // Drone state display (read-only)
    if (ImGui::CollapsingHeader("Drone State")) {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
            m_viewModel->DronePosition.Get().x,
            m_viewModel->DronePosition.Get().y,
            m_viewModel->DronePosition.Get().z);
        ImGui::Text("Velocity: (%.2f, %.2f, %.2f)",
            m_viewModel->DroneVelocity.Get().x,
            m_viewModel->DroneVelocity.Get().y,
            m_viewModel->DroneVelocity.Get().z);
        ImGui::Text("Rotor Speeds: (%.1f, %.1f, %.1f, %.1f)",
            m_viewModel->DroneRotorSpeeds.Get().x,
            m_viewModel->DroneRotorSpeeds.Get().y,
            m_viewModel->DroneRotorSpeeds.Get().z,
            m_viewModel->DroneRotorSpeeds.Get().w);
    }

    ImGui::Spacing();

    if (CustomWidgets::ColoredButton("Remove Drone", WidgetColorType::Danger, ImVec2(120, 0))) {
        m_viewModel->RemoveDroneCommand->Execute();
    }

    CustomWidgets::EndSection();
}


void ComponentView::DrawAddComponentButton()
{
    ImGui::Spacing();
    ImGui::Spacing();

    float buttonWidth = 140.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    float off = (avail - buttonWidth) * 0.5f;
    if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    if (CustomWidgets::Button("+ Add Component", ImVec2(buttonWidth, 36))) {
        ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
        if (ImGui::MenuItem("Physics Component") && m_viewModel->HasGeometry.Get() && !m_viewModel->SelectedEntity.Get()->GetComponent<Physics>())
        {
            m_viewModel->AddPhysicsCommand->Execute();
        }

        if (ImGui::MenuItem("Script Component"))
        {
            m_viewModel->AddScriptCommand->Execute();
        }

        if (ImGui::MenuItem("Material Component", nullptr, false,
                       m_viewModel->HasGeometry.Get() && !m_viewModel->HasMaterial.Get()))
        {
            m_viewModel->AddMaterialCommand->Execute();
        }

        if (ImGui::MenuItem("Drone Component", nullptr, false,
                       m_viewModel->HasPhysics.Get() && !m_viewModel->HasDrone.Get()))
        {
            m_viewModel->AddDroneCommand->Execute();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}