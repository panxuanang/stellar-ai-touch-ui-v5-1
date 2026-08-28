#pragma once

#include "display/lcd_display.h"
#include "device_state.h"

#include <cstdint>

class StellarDisplay : public SpiLcdDisplay {
public:
    StellarDisplay(esp_lcd_panel_io_handle_t panel_io,
                   esp_lcd_panel_handle_t panel,
                   int width,
                   int height,
                   int offset_x,
                   int offset_y,
                   bool mirror_x,
                   bool mirror_y,
                   bool swap_xy)
        : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y,
                        mirror_x, mirror_y, swap_xy) {}

    void SetupUI() override;
    void SetChatMessage(const char* role, const char* content) override;
    void ClearChatMessages() override;
    void UpdateStatusBar(bool update_all = false) override;

private:
    // Home page
    lv_obj_t* home_layer_ = nullptr;
    lv_obj_t* time_label_ = nullptr;
    lv_obj_t* date_label_ = nullptr;
    lv_obj_t* ai_state_label_ = nullptr;
    lv_obj_t* weather_value_label_ = nullptr;
    lv_obj_t* weather_hint_label_ = nullptr;
    lv_obj_t* todo_value_label_ = nullptr;
    lv_obj_t* home_avatar_ = nullptr;

    // Custom STELLAR conversation page. The upstream WeChat bubble UI is
    // disabled for this board by scripts/apply_mods.py.
    lv_obj_t* chat_layer_ = nullptr;
    lv_obj_t* chat_header_label_ = nullptr;
    lv_obj_t* chat_hint_label_ = nullptr;
    lv_obj_t* user_preview_label_ = nullptr;
    lv_obj_t* chat_card_ = nullptr;
    lv_obj_t* chat_title_label_ = nullptr;
    lv_obj_t* chat_text_box_ = nullptr;
    lv_obj_t* chat_text_label_ = nullptr;
    lv_obj_t* chat_avatar_frame_ = nullptr;
    lv_obj_t* chat_avatar_ = nullptr;

    bool chat_expanded_ = false;
    bool chat_visible_ = false;
    DeviceState last_device_state_ = kDeviceStateUnknown;
    int64_t return_home_deadline_us_ = 0;

    void BuildHomeUI();
    void BuildChatUI();
    void ShowHome();
    void ShowChat();
    void UpdateHomeInfo();
    void UpdateChatState(DeviceState state);
    void SetChatExpandedInternal(bool expanded);
    void SetAssistantTextInternal(const char* content);
    void StopChatScrollInternal();

    static void TalkEventCallback(lv_event_t* event);
};
