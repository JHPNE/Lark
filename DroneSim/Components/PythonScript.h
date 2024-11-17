#pragma once
#include <string>
#include <memory>
#include "../Scripting/ScriptContext.h"

class PythonScript {
public:
    PythonScript() = default;
    explicit PythonScript(const std::string& scriptPath);
    
    void SetScriptPath(const std::string& path);
    const std::string& GetScriptPath() const { return scriptPath; }
    
    bool IsLoaded() const { return scriptContext != nullptr; }
    void Reload();
    
    // Called by PythonScriptSystem
    void OnInit();
    void OnUpdate(float deltaTime);
    
private:
    std::string scriptPath;
    std::shared_ptr<ScriptContext> scriptContext;
};