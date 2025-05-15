#pragma once
#include <string>
#include <memory>

class CanoramaEngine {
public:
    static CanoramaEngine* Get() {
        static CanoramaEngine instance;
        return &instance;
    }

    void Eval(const std::string& code, void* customPanel = nullptr, const std::string& customFile = "");
    void* GetChild(const std::string& childName);
    std::string GetChildName(void* childPtr);
    void SetVisible(void* childPtr, bool state);

private:
    CanoramaEngine();
    ~CanoramaEngine();

    void* GetRoot(void* uiEnginePtr, const char* customPanel = nullptr);
    void* GetLastTargetPanel(void* uiEnginePtr);
    bool IsValidPanelPtr(void* uiEnginePtr, void* itr);
    std::string GetPanelId(void* panelPtr);
    void SetPanelVisible(void* panelPtr, bool state);
    void* GetParent(void* panelPtr);

    void* m_panoramaEngine;
    void* m_uiEngine;
    void* m_rootPanel;
};

#define PEngine CanoramaEngine::Get()