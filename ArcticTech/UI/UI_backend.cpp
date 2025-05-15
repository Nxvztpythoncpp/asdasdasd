#include "UI.h"

#include "../ImGui/imgui.h"
#include "../ImGui/backends/imgui_impl_win32.h"
#include "../ImGui/backends/imgui_impl_dx9.h"
#include "../ImGui/imgui_internal.h"
#include <d3d9.h>
#include <tchar.h>

#include "../ImGui/imgui_settings.h"

#include "logo.h"
#include "mulish_font.h"
#include "icons.h"

#include "../SDK/Interfaces.h"
#include "../SDK/Misc/xorstr.h"
#include "../SDK/Globals.h"
#include "../Utils/Console.h"

#include "../Features/RageBot/Ragebot.h"

inline std::unique_ptr<CMenu> Menu = std::make_unique<CMenu>();

void OpenURL(const char* url) {
    ShellExecute(0, 0, url, 0, 0, SW_SHOW);
}

namespace font {
    ImFont* general = nullptr;
    ImFont* tab = nullptr;
}

static ImGuiIO* im_io;

void CMenu::Setup() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    im_io = &ImGui::GetIO();

    ImFontConfig cfg;

    font::general = im_io->Fonts->AddFontFromMemoryTTF(mulish, sizeof(mulish), 19.f, &cfg, im_io->Fonts->GetGlyphRangesCyrillic());
    font::tab = im_io->Fonts->AddFontFromMemoryTTF(mulish, sizeof(mulish), 15.f, &cfg, im_io->Fonts->GetGlyphRangesCyrillic());

    ctx.font = im_io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 14.f, &cfg, im_io->Fonts->GetGlyphRangesCyrillic());

    im_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    im_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    D3DDEVICE_CREATION_PARAMETERS creationParameters = { };
    if (FAILED(DirectXDevice->GetCreationParameters(&creationParameters)))
        return;

    // store window pointer
    HWND hWindow = creationParameters.hFocusWindow;
    if (hWindow == nullptr)
        return;

    ImGui_ImplWin32_Init(hWindow);
    ImGui_ImplDX9_Init(DirectXDevice);

    pic::logo = Render->LoadImageFromMemory(ui_logo, sizeof(ui_logo), Vector2(128, 128));
    pic::tab::aimbot = Render->LoadImageFromMemory(aimbot, sizeof(aimbot), Vector2(20.f, 20.f));
    pic::tab::antiaim = Render->LoadImageFromMemory(anti_aimbot, sizeof(anti_aimbot), Vector2(20.f, 20.f));
    pic::tab::visuals = Render->LoadImageFromMemory(visuals, sizeof(visuals), Vector2(20.f, 20.f));
    pic::tab::misc = Render->LoadImageFromMemory(misc, sizeof(misc), Vector2(20.f, 20.f));
    pic::tab::players = Render->LoadImageFromMemory(players, sizeof(players), Vector2(20.f, 20.f));
    pic::tab::skins = Render->LoadImageFromMemory(skins, sizeof(skins), Vector2(20.f, 20.f));
    pic::tab::configs = Render->LoadImageFromMemory(configs, sizeof(configs), Vector2(20.f, 20.f));
    pic::tab::scripts = Render->LoadImageFromMemory(scripts, sizeof(scripts), Vector2(20.f, 20.f));

    m_WindowSize = ImVec2(950, 750);
    m_ItemSpacing = ImVec2(24, 24);

    SetupUI();

    m_bIsInitialized = true;
}

void CMenu::Release() {
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::Shutdown();
}

void DrawAboutWindow(bool open) {
    static float alpha = 0.f;
    alpha = ImLerp(alpha, open ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 8.f);

    if (alpha < 0.1f)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushFont(ctx.font);

    ImGui::SetNextWindowSize(ImVec2(280, 300));
    ImGui::Begin("##about.panel", nullptr, ImGuiWindowFlags_NoDecoration);
    {
        auto draw_list = ImGui::GetWindowDrawList();
        const ImVec2 window_pos = ImGui::GetWindowPos();
        const ImVec2 window_size = ImGui::GetWindowSize();

        draw_list->AddRectFilled(
            window_pos,
            window_pos + window_size,
            ImColor(12, 12, 14, (int)(alpha * 255))
        );

        ImGui::SetCursorPos(ImVec2(15, 12));
        ImGui::TextColored(ImColor(180, 180, 180, (int)(alpha * 255)), "Sunset");

        draw_list->AddLine(
            window_pos + ImVec2(15, 35),
            window_pos + ImVec2(window_size.x - 15, 35),
            ImColor(30, 30, 30, (int)(alpha * 255))
        );

        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);

        ImGui::SetCursorPos(ImVec2(15, 45));
        ImGui::TextColored(ImColor(100, 100, 100, (int)(alpha * 255)),
            (const char*)u8"© 2024-%d. all rights reserved.", timeinfo.tm_year + 1900);

        ImGui::SetCursorPos(ImVec2(15, 80));
        ImGui::TextColored(ImColor(150, 150, 150, (int)(alpha * 255)), "CREDITS");

        std::vector<std::pair<std::string, std::string>> credits = {
            { "Forever", "Owner" },
            { "cacamelio", "Coder" },
            { "dantez_666", "Better Coder" }
        };

        float y_offset = 105;
        for (const auto& [user, desc] : credits) {
            ImGui::SetCursorPos(ImVec2(15, y_offset));
            ImGui::TextColored(ImColor(130, 130, 130, (int)(alpha * 255)), user.c_str());
            ImGui::SameLine(140);
            ImGui::TextColored(ImColor(100, 100, 100, (int)(alpha * 255)), desc.c_str());
            y_offset += 22;
        }

        y_offset += 15;
        ImGui::SetCursorPos(ImVec2(15, y_offset));
        ImGui::TextColored(ImColor(150, 150, 150, (int)(alpha * 255)), "*****");

        std::vector<std::pair<std::string, std::string>> special_thanks = {
            { "cacamelio <3", "dantez <3" }
        };

        y_offset += 25;
        for (const auto& [user, desc] : special_thanks) {
            ImGui::SetCursorPos(ImVec2(15, y_offset));
            ImGui::TextColored(ImColor(130, 130, 130, (int)(alpha * 255)), user.c_str());
            ImGui::SameLine(140);
            ImGui::TextColored(ImColor(100, 100, 100, (int)(alpha * 255)), desc.c_str());
            y_offset += 22;
        }
    }
    ImGui::End();
    ImGui::PopFont();
    ImGui::PopStyleVar(3);
}

void CMenu::Draw() {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool insert_pressed = false;
    static bool open_add_menu = false;

    if (GetAsyncKeyState(VK_INSERT) & 0x8000 || GetAsyncKeyState(VK_DELETE) & 0x8000) {
        if (!insert_pressed) {
            m_bMenuOpened = !m_bMenuOpened;
            insert_pressed = true;
            if (m_bMenuOpened)
                Ragebot->UpdateUI();
        }
    }
    else {
        insert_pressed = false;
    }

    if (m_bMenuOpened) {

        ImGui::GetStyle().ItemSpacing = ImVec2(24, 24);
        ImGui::GetStyle().WindowPadding = ImVec2(0, 0);
        ImGui::GetStyle().ScrollbarSize = 16.f;

        ImGui::SetNextWindowSize(m_WindowSize);

        const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoResize;

        ImGui::Begin("MENU", nullptr, window_flags);
        {
            const ImVec2 window_pos = ImGui::GetWindowPos();
            const ImVec2 window_size = ImGui::GetContentRegionMax();
            const float header_height = 40.f;
            const int window_alpha = ImGui::GetStyle().Alpha * 255.f;

            auto list = ImGui::GetWindowDrawList();

            list->AddRectFilled(window_pos, window_pos + window_size, ImColor(15, 15, 15, window_alpha));

            list->AddRectFilled(window_pos, window_pos + ImVec2(window_size.x, header_height),
                ImColor(24, 24, 24, window_alpha));

            list->AddLine(window_pos + ImVec2(0, header_height),
                window_pos + ImVec2(window_size.x, header_height),
                ImColor(45, 45, 45, window_alpha));

            list->AddLine(window_pos + ImVec2(0, header_height - 1),
                window_pos + ImVec2(window_size.x, header_height - 1),
                ImColor(35, 35, 35, window_alpha));

            ImGui::PushFont(ctx.font);
            const char* logo_text = "Sunset-Software";
            ImVec2 logo_size = ImGui::CalcTextSize(logo_text);
            ImVec2 logo_pos = window_pos + ImVec2(window_size.x / 2 - logo_size.x / 2,
                header_height / 2 - logo_size.y / 2);
            list->AddText(logo_pos, ImColor(255, 255, 255, window_alpha), logo_text);
            ImGui::PopFont();

            ImGui::PushFont(ctx.font);
            const char* c_about_text = "about";
            ImVec2 c_about_size = ImGui::CalcTextSize(c_about_text);
            ImVec2 c_about_pos = window_pos + ImVec2(window_size.x - 35,
                header_height / 2 - c_about_size.y / 2);

            ImRect c_about_rect(c_about_pos, c_about_pos + c_about_size);
            ImColor c_about_color = ImGui::IsMouseHoveringRect(c_about_rect.Min, c_about_rect.Max) ?
                ImColor(150, 150, 150, window_alpha) :
                ImColor(255, 255, 255, window_alpha);

            list->AddText(c_about_pos, c_about_color, c_about_text);

            if (ImGui::IsMouseHoveringRect(c_about_rect.Min, c_about_rect.Max) && ImGui::IsMouseClicked(0))
                open_add_menu = !open_add_menu;

            ImGui::PopFont();

            static int tabs = 0;
            float tab_width = 120;
            const float total_tabs_width = m_Tabs.size() * (tab_width - 40 + ImGui::GetStyle().ItemSpacing.x);
            ImGui::SetCursorPos(ImVec2((window_size.x - total_tabs_width) / 2, 50));

            ImGui::BeginGroup();
            {
                for (int i = 0; i < m_Tabs.size(); i++) {
                    if (i > 0) ImGui::SameLine();

                    CMenuTab* tab = m_Tabs[i];
                    if (ImGui::Tab(i == tabs, tab->icon, tab->name.c_str(),
                        ImVec2(tab_width - 40, 44), tab->icon_size))
                        tabs = i;
                }
            }
            ImGui::EndGroup();

            static float tab_alpha = 0.f;
            static int active_tab = 0;

            tab_alpha = ImClamp(tab_alpha + (4.f * ImGui::GetIO().DeltaTime *
                (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);

            if (tab_alpha == 0.f)
                active_tab = tabs;

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha);
            for (auto group : m_Tabs[active_tab]->groupboxes) {
                group->Render();
            }
            ImGui::PopStyleVar();

            list->AddRect(window_pos, window_pos + window_size,
                ImColor(45, 45, 45, window_alpha));
        }
        ImGui::End();
    }

    Watermark();
    arc_();
    DrawAboutWindow(open_add_menu && m_bMenuOpened);

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool CMenu::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        return true;
    default:
        return ImGui::GetIO().WantTextInput;
    }
}

void CMenu::RecalculateGroupboxes() {
    const float groupbox_width = (m_WindowSize.x / 2) - ((181 / 2) + (m_ItemSpacing.x + 12));
    const ImVec2 base_position((/*181*/90 + m_ItemSpacing.x), (m_ItemSpacing.y * 2) + 58);
    const ImVec2 container_size(groupbox_width * 2 + m_ItemSpacing.x, m_WindowSize.y - base_position.y - m_ItemSpacing.y);

    // size.x - (181 + (spacing.x + 24)))

    for (auto tab : m_Tabs) {
        std::vector<CMenuGroupbox*>& groupboxes = tab->groupboxes;
        const float gb_width = groupboxes.size() > 1 ? groupbox_width : (m_WindowSize.x - (181 + m_ItemSpacing.x + 24));

        float total_relative[2] = { 0.f, 0.f };
        int n_groupboxes[2] = { 0, 0 };
        for (auto gb : groupboxes) {
            total_relative[gb->column] += gb->relative_size;
            n_groupboxes[gb->column]++;
        }

        float available_space[2] = { container_size.y - m_ItemSpacing.y * (n_groupboxes[0] - 1), container_size.y - m_ItemSpacing.y * (n_groupboxes[1] - 1) };
        float current_position[2] = { 0.f, 0.f };

        for (auto gb : groupboxes) {
            gb->position.y = base_position.y + current_position[gb->column];
            gb->position.x = base_position.x + (gb_width + m_ItemSpacing.x) * gb->column;
            gb->size.x = gb_width;
            gb->size.y = available_space[gb->column] * (gb->relative_size / total_relative[gb->column]);

            current_position[gb->column] += gb->size.y + m_ItemSpacing.y;
        }
    }
}

CMenuTab* CMenu::AddTab(const std::string& tab, DXImage icon) {
    CMenuTab* result = new CMenuTab;

    result->icon = icon.texture;
    result->icon_size = ImVec2(icon.width, icon.height);
    result->name = tab;

    m_Tabs.push_back(result);

    return result;
}

CMenuGroupbox* CMenu::AddGroupBox(const std::string& tab, const std::string& groupbox, float realtive_size, int column) {
    CMenuTab* _tab = Menu->FindTab(tab);

    if (!_tab)
        return nullptr;

    CMenuGroupbox* gb = new CMenuGroupbox;

    if (column == -1) {
        int n_columns[2]{ 0, 0 };

        for (auto gb : _tab->groupboxes) {
            n_columns[gb->column]++;
        }

        gb->column = (n_columns[0] <= n_columns[1]) ? 0 : 1;
    }
    else {
        gb->column = column;
    }

    gb->name = groupbox;
    gb->parent = _tab;

    _tab->groupboxes.push_back(gb);

    RecalculateGroupboxes();

    return gb;
}

void CMenuGroupbox::Render() {
    ImGui::SetCursorPos(position);

    ImGui::BeginGroup();
    ImGui::BeginChild(name.c_str(), size);

    for (int i = 0; i < widgets.size(); i++) {
        auto el = widgets[i];
        if (!el || !el->visible || el->GetType() == WidgetType::ColorPicker || el->GetType() == WidgetType::KeyBind)
            continue;

        el->Render();
    }

    ImGui::EndChild();
    ImGui::EndGroup();
}

bool CKeyBind::get() {
    if (key == 0)
        return false;

    if (mode == 2)
        return true;

    if (!ctx.console_visible && ctx.active_app && GetAsyncKeyState(key) & 0x8000) {
        if (!pressed_once) {
            pressed_once = true;
            toggled = !toggled;
        }
    }
    else {
        pressed_once = false;
    }

    if (mode == 1) {
        return toggled;
    }
    else {
        return ctx.active_app && !ctx.console_visible && (GetAsyncKeyState(key) & 0x8000);
    }
};

void CCheckBox::Render() {
    if (ImGui::Checkbox(name.c_str(), &value)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }

    if (additional) {
        ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, (ImGui::GetStyle().ItemSpacing.y * 2) + 4));
        additional->Render();
    }
}

void CSliderInt::Render() {
    if (ImGui::SliderInt(name.c_str(), &value, min, max, format.c_str(), flags)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CSliderFloat::Render() {
    if (ImGui::SliderFloat(name.c_str(), &value, min, max, format.c_str(), flags)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CKeyBind::Render() {
    if (ImGui::Keybind(name.c_str(), &key, &mode, false)) {
        if (key == VK_ESCAPE)
            key = 0;

        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CLabel::Render() {
    ImGui::Text(name.c_str());

    if (additional) {
        ImGui::SetCursorPos(ImGui::GetCursorPos() - ImVec2(0, (ImGui::GetStyle().ItemSpacing.y * 2) + 4));
        additional->Render();
    }
}

void CColorPicker::Render() {
    if (ImGui::ColorEdit4(name.c_str(), value, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | (has_alpha ? 0 : ImGuiColorEditFlags_NoAlpha))) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CComboBox::Render() {
    if (ImGui::Combo(name.c_str(), &value, elements.data(), static_cast<int>(elements.size()), 5, ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CMultiCombo::Render() {
    if (ImGui::MultiCombo(name.c_str(), value, elements.data(), elements.size(), ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CButton::Render() {
    if (ImGui::Button(name.c_str())) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

void CInputBox::Render() {
    if (ImGui::TextField(name.c_str(), nullptr, buf, 64, flags)) {
        for (auto& cb : callbacks)
            cb();
        for (auto lcb : lua_callbacks) {
            auto res = lcb.func();
            if (!res.valid()) {
                sol::error er = res;
                Console->Error(er.what());
            }
        }
    }
}

CMenuTab* CMenu::FindTab(const std::string& name) {
    for (auto tab : m_Tabs)
        if (tab->name == name)
            return tab;

    return nullptr;
}

CMenuGroupbox* CMenu::FindGroupbox(const std::string& tab, const std::string& groupbox) {
    CMenuTab* _tab = FindTab(tab);

    if (!_tab)
        return nullptr;

    for (auto gb : _tab->groupboxes) {
        if (gb->name == groupbox)
            return gb;
    }

    return nullptr;
}

IBaseWidget* CMenu::FindItem(const std::string& tab, const std::string& groupbox, const std::string& name, WidgetType type) {
    CMenuGroupbox* gb = FindGroupbox(tab, groupbox);

    if (!gb)
        return nullptr;

    for (auto item : gb->widgets)
        if (item->name == name && ((type == WidgetType::Any && item->GetType() != WidgetType::Label) || type == item->GetType()))
            return item;

    return nullptr;
}

std::vector<IBaseWidget*> CMenu::GetKeyBinds() {
    return m_KeyBinds;
}

void CMenu::RemoveItem(IBaseWidget* widget) {
    CMenuGroupbox* gb = widget->parent;

    if (widget->GetType() == WidgetType::KeyBind) {
        for (auto it = m_KeyBinds.begin(); it != m_KeyBinds.end();) {
            if (*it == widget) {
                it = m_KeyBinds.erase(it);
                continue;
            }

            it++;
        }
    }

    for (auto it = gb->widgets.begin(); it != gb->widgets.end(); it++) {
        if (*it == widget) {
            if (widget->GetType() == WidgetType::MultiCombo || widget->GetType() == WidgetType::Combo) {
                CComboBox* box = reinterpret_cast<CComboBox*>(widget);

                for (auto st : box->elements)
                    delete[] st;
            }

            gb->widgets.erase(it);

            delete widget;
            return;
        }
    }
}

void CMenu::RemoveGroupBox(CMenuGroupbox* gb) {
    for (auto it = gb->parent->groupboxes.begin(); it != gb->parent->groupboxes.end(); it++) {
        if (*it == gb) {
            gb->parent->groupboxes.erase(it);

            delete gb;
            return;
        }
    }
}

void CMenu::RemoveTab(CMenuTab* tab) {
    for (auto it = m_Tabs.begin(); it != m_Tabs.end(); it++) {
        if (*it == tab) {
            m_Tabs.erase(it);

            delete tab;
            return;
        }
    }
}

CCheckBox* CMenuGroupbox::AddCheckBox(const std::string& name, bool init) {
    CCheckBox* item = new CCheckBox;

    item->name = name;
    item->parent = this;
    item->value = init;

    widgets.push_back(item);

    return item;
}

CSliderInt* CMenuGroupbox::AddSliderInt(const std::string& name, int min, int max, int init, const std::string& format, ImGuiSliderFlags flags) {
    CSliderInt* item = new CSliderInt;

    item->name = name;
    item->parent = this;
    item->min = min;
    item->max = max;
    item->value = init;
    item->format = format;
    item->flags = flags;

    widgets.push_back(item);

    return item;
}

CSliderFloat* CMenuGroupbox::AddSliderFloat(const std::string& name, float min, float max, float init, const std::string& format, ImGuiSliderFlags flags) {
    CSliderFloat* item = new CSliderFloat;

    item->name = name;
    item->parent = this;
    item->min = min;
    item->max = max;
    item->value = init;
    item->format = format;
    item->flags = flags;

    widgets.push_back(item);

    return item;
}

CKeyBind* CMenuGroupbox::AddKeyBind(const std::string& name) {
    IBaseWidget* parent_item = Menu->FindItem(parent->name, this->name, name);

    if (!parent_item)
        parent_item = AddLabel(name);

    CKeyBind* item = new CKeyBind;

    item->name = name;
    item->parent = this;
    item->parent_item = parent_item;
    parent_item->additional = item;

    widgets.push_back(item);
    Menu->m_KeyBinds.push_back(item);

    return item;
}

CLabel* CMenuGroupbox::AddLabel(const std::string& name) {
    CLabel* item = new CLabel;

    item->name = name;
    item->parent = this;

    widgets.push_back(item);

    return item;
}

CColorPicker* CMenuGroupbox::AddColorPicker(const std::string& name, Color init, bool has_alpha) {
    IBaseWidget* parent_item = Menu->FindItem(parent->name, this->name, name);

    if (!parent_item)
        parent_item = AddLabel(name);

    CColorPicker* item = new CColorPicker;

    item->name = name;
    item->parent = this;
    item->parent_item = parent_item;
    parent_item->additional = item;
    item->value[0] = init.r / 255.f;
    item->value[1] = init.g / 255.f;
    item->value[2] = init.b / 255.f;
    item->value[3] = init.a / 255.f;
    item->has_alpha = has_alpha;

    widgets.push_back(item);

    return item;
}

CComboBox* CMenuGroupbox::AddComboBox(const std::string& name, std::vector<std::string> items) {
    CComboBox* item = new CComboBox;

    item->name = name;
    item->parent = this;

    std::vector<const char*> elems;
    for (auto st : items) {
        char* buf = new char[st.size() + 1];
        memcpy(buf, st.c_str(), st.size());
        buf[st.size()] = 0;
        elems.push_back(buf);
    }
    item->elements = elems;

    widgets.push_back(item);

    return item;
}

CMultiCombo* CMenuGroupbox::AddMultiCombo(const std::string& name, std::vector<std::string> items) {
    CMultiCombo* item = new CMultiCombo;

    item->name = name;
    item->parent = this;

    std::vector<const char*> elems;
    for (auto st : items) {
        char* buf = new char[st.size() + 1];
        memcpy(buf, st.c_str(), st.size());
        buf[st.size()] = 0;
        elems.push_back(buf);
    }
    item->elements = elems;

    widgets.push_back(item);

    return item;
}

CButton* CMenuGroupbox::AddButton(const std::string& name) {
    CButton* item = new CButton;

    item->name = name;
    item->parent = this;

    widgets.push_back(item);

    return item;
}

CInputBox* CMenuGroupbox::AddInput(const std::string& name, const std::string& init, ImGuiInputTextFlags flags) {
    CInputBox* item = new CInputBox;

    item->name = name;
    item->parent = this;
    item->flags = flags;
    
    memset(item->buf, 0, 64);
    std::memcpy(item->buf, init.c_str(), min(init.size(), 64));

    widgets.push_back(item);

    return item;
}