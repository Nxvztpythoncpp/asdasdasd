#include "panorama_engine.h"
#include <stdexcept>
#include "../Utils/Utils.h"

// uwukson: function typdefs
typedef void* (__thiscall* AccessUIEngine_t)(void*, void*);
typedef bool(__thiscall* IsValidPanelPtr_t)(void*, void*);
typedef void* (__thiscall* GetLastTargetPanel_t)(void*);
typedef int(__thiscall* RunScript_t)(void*, void*, const char*, const char*, int, int, bool, bool);
typedef const char* (__thiscall* GetPanelId_t)(void*, void*);
typedef void* (__thiscall* GetParent_t)(void*);
typedef void* (__thiscall* SetVisible_t)(void*, bool);

CanoramaEngine::CanoramaEngine() {
    m_panoramaEngine = Utils::CreateInterface("panorama.dll", "PanoramaUIEngine001");
    if (!m_panoramaEngine)
        return;

    auto vtable = *reinterpret_cast<void***>(m_panoramaEngine);
    auto accessUIEngine = reinterpret_cast<AccessUIEngine_t>(vtable[11]);
    m_uiEngine = accessUIEngine(m_panoramaEngine, nullptr);

    m_rootPanel = GetRoot(m_uiEngine);
    if (!m_rootPanel)
        return;
}

CanoramaEngine::~CanoramaEngine() {
    m_panoramaEngine = nullptr;
    m_uiEngine = nullptr;
    m_rootPanel = nullptr;
}

void* CanoramaEngine::GetLastTargetPanel(void* uiEnginePtr) {
    auto vtable = *reinterpret_cast<void***>(uiEnginePtr);
    auto fn = reinterpret_cast<GetLastTargetPanel_t>(vtable[56]);
    return fn(uiEnginePtr);
}

bool CanoramaEngine::IsValidPanelPtr(void* uiEnginePtr, void* itr) {
    if (!itr) return false;
    auto vtable = *reinterpret_cast<void***>(uiEnginePtr);
    auto fn = reinterpret_cast<IsValidPanelPtr_t>(vtable[36]);
    return fn(uiEnginePtr, itr);
}

std::string CanoramaEngine::GetPanelId(void* panelPtr) {
    auto vtable = *reinterpret_cast<void***>(panelPtr);
    auto fn = reinterpret_cast<GetPanelId_t>(vtable[9]);
    return fn(panelPtr, nullptr);
}

void CanoramaEngine::SetPanelVisible(void* panelPtr, bool state) {
    auto vtable = *reinterpret_cast<void***>(panelPtr);
    auto fn = reinterpret_cast<SetVisible_t>(vtable[27]);
    fn(panelPtr, state);
}

void* CanoramaEngine::GetParent(void* panelPtr) {
    auto vtable = *reinterpret_cast<void***>(panelPtr);
    auto fn = reinterpret_cast<GetParent_t>(vtable[25]);
    return fn(panelPtr);
}

void* CanoramaEngine::GetRoot(void* uiEnginePtr, const char* customPanel) {
    void* itr = GetLastTargetPanel(uiEnginePtr);
    if (!itr)
        return nullptr;

    void* ret = nullptr;
    void* panelPtr = nullptr;

    while (itr && IsValidPanelPtr(uiEnginePtr, itr)) {
        panelPtr = reinterpret_cast<void**>(itr);
        std::string panelId = GetPanelId(panelPtr);

        if (customPanel && panelId == customPanel) {
            ret = itr;
            break;
        }
        else if (panelId == "CSGOHud" || panelId == "CSGOMainMenu") {
            ret = itr;
            break;
        }

        itr = GetParent(panelPtr);
        if (!itr)
            return nullptr;
    }

    return ret;
}

void CanoramaEngine::Eval(const std::string& code, void* customPanel, const std::string& customFile) {
    void* targetPanel = customPanel ? customPanel : m_rootPanel;
    if (!targetPanel) {
        targetPanel = GetRoot(m_uiEngine);
        if (!targetPanel)
            return;
    }

    std::string file = customFile.empty() ? "panorama/layout/base_mainmenu.xml" : customFile;
    auto vtable = *reinterpret_cast<void***>(m_uiEngine);
    auto runScript = reinterpret_cast<RunScript_t>(vtable[113]);

    runScript(m_uiEngine, targetPanel, code.c_str(), file.c_str(), 8, 10, false, false);
}

void* CanoramaEngine::GetChild(const std::string& childName) {
    void* child = GetRoot(m_uiEngine, childName.c_str());
    if (!child)
        return nullptr;

    return child;
}

std::string CanoramaEngine::GetChildName(void* childPtr) {
    if (!IsValidPanelPtr(m_uiEngine, childPtr))
        throw std::runtime_error("Invalid panel");

    return GetPanelId(reinterpret_cast<void**>(childPtr));
}

void CanoramaEngine::SetVisible(void* childPtr, bool state) {
    if (!IsValidPanelPtr(m_uiEngine, childPtr))
        return;

    SetPanelVisible(reinterpret_cast<void**>(childPtr), state);
}