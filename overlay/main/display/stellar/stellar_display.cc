#include "stellar_display.h"

#include "stellar_avatar.h"
#include "application.h"
#include "device_state.h"

#include <esp_log.h>
#include <esp_timer.h>

#include <cstdio>
#include <cstring>
#include <ctime>

namespace {
constexpr const char* TAG = "StellarV51";

// Touch UI V5.1: one coherent pastel palette across cards and character scene.
// No dark center panel, no network/status clutter on the home page.
constexpr uint32_t kBg = 0xFCF7FB;
constexpr uint32_t kPaper = 0xFFFFFF;
constexpr uint32_t kCardPink = 0xFFFDFE;
constexpr uint32_t kCardLavender = 0xFBF8FF;
constexpr uint32_t kCardBlue = 0xF8FBFF;
constexpr uint32_t kCardCream = 0xFFFDF8;
constexpr uint32_t kInk = 0x3A435C;
constexpr uint32_t kMuted = 0x7A7A9A;
constexpr uint32_t kLine = 0xECE6F2;
constexpr uint32_t kPink = 0xEFB7CB;
constexpr uint32_t kPurple = 0x8E7CEB;
constexpr uint32_t kBlue = 0x8CB6F1;
constexpr uint32_t kAmber = 0xE6B870;
constexpr uint32_t kGreen = 0x59B98A;
constexpr uint32_t kRed = 0xD86C7C;

constexpr int kScrollDelayMs = 2000;
constexpr int kScrollMsPerPixel = 120;
constexpr int64_t kReturnHomeDelayUs = 4500000LL;  // 4.5 seconds

lv_obj_t* MakeCard(lv_obj_t* parent,
                   int x,
                   int y,
                   int w,
                   int h,
                   uint32_t bg,
                   uint32_t border,
                   int radius = 22,
                   lv_opa_t opacity = LV_OPA_90) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_radius(card, radius, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(card, opacity, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(border), 0);
    lv_obj_set_style_border_opa(card, LV_OPA_50, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0xDCCFEA), 0);
    lv_obj_set_style_shadow_width(card, 14, 0);
    lv_obj_set_style_shadow_spread(card, 0, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}


lv_obj_t* MakeLabel(lv_obj_t* parent, const char* text, int x, int y, uint32_t color) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    // Do not pin a raw runtime font pointer. Labels inherit the current screen
    // font so xiaozhi can safely swap/load fonts after networking starts.
    return label;
}

void MakeDivider(lv_obj_t* parent, int x, int y, int w, uint32_t color = kLine) {
    lv_obj_t* line = lv_obj_create(parent);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, w, 1);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_bg_color(line, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
}

void MakeDot(lv_obj_t* parent, int x, int y, uint32_t color, int size = 7) {
    lv_obj_t* dot = lv_obj_create(parent);
    lv_obj_set_pos(dot, x, y);
    lv_obj_set_size(dot, size, size);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
}

const char* StateText(DeviceState state) {
    switch (state) {
        case kDeviceStateStarting:
            return "启动中";
        case kDeviceStateWifiConfiguring:
            return "等待配网";
        case kDeviceStateIdle:
            return "待命中";
        case kDeviceStateConnecting:
            return "连接中";
        case kDeviceStateListening:
            return "正在聆听";
        case kDeviceStateSpeaking:
            return "正在回答";
        case kDeviceStateNotifying:
            return "消息提醒";
        case kDeviceStateUpgrading:
            return "正在升级";
        case kDeviceStateActivating:
            return "正在激活";
        case kDeviceStateAudioTesting:
            return "音频测试";
        case kDeviceStateFatalError:
            return "系统异常";
        case kDeviceStateUnknown:
        default:
            return "状态未知";
    }
}

uint32_t StateColor(DeviceState state) {
    if (state == kDeviceStateIdle) {
        return kGreen;
    }
    if (state == kDeviceStateFatalError) {
        return kRed;
    }
    if (state == kDeviceStateListening) {
        return kBlue;
    }
    if (state == kDeviceStateSpeaking) {
        return kPurple;
    }
    return kMuted;
}

const char* WeekdayText(int wday) {
    static const char* kWeekdays[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    if (wday < 0 || wday > 6) {
        return "";
    }
    return kWeekdays[wday];
}
}  // namespace

void StellarDisplay::SetupUI() {
    // Keep the xiaozhi native objects alive behind STELLAR so framework code
    // such as theme/font replacement, notifications and power logic still has
    // the objects it expects. STELLAR layers are opaque and stay on top.
    LcdDisplay::SetupUI();

    {
        DisplayLockGuard lock(this);
        BuildHomeUI();
        BuildChatUI();
        lv_obj_add_flag(chat_layer_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(home_layer_);
    }

    last_device_state_ = Application::GetInstance().GetDeviceState();
    UpdateHomeInfo();
    ESP_LOGI(TAG, "星语伴侣 Touch UI V5.1 创建完成");
}

void StellarDisplay::BuildHomeUI() {
    lv_obj_t* screen = lv_screen_active();

    home_layer_ = lv_obj_create(screen);
    lv_obj_set_pos(home_layer_, 0, 0);
    lv_obj_set_size(home_layer_, width_, height_);
    lv_obj_set_style_radius(home_layer_, 0, 0);
    lv_obj_set_style_bg_color(home_layer_, lv_color_hex(kBg), 0);
    lv_obj_set_style_bg_opa(home_layer_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(home_layer_, 0, 0);
    lv_obj_set_style_pad_all(home_layer_, 0, 0);
    lv_obj_set_scrollbar_mode(home_layer_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(home_layer_, LV_OBJ_FLAG_SCROLLABLE);

    // Center scene first so the light room/background is part of the whole
    // composition instead of looking like a dark rectangular sticker.
    home_avatar_ = lv_image_create(home_layer_);
    lv_image_set_src(home_avatar_, &stellar_avatar);
    lv_obj_set_pos(home_avatar_, 140, 0);  // asset is 200 x 320
    lv_obj_add_flag(home_avatar_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(home_avatar_, TalkEventCallback, LV_EVENT_CLICKED, this);

    // Left upper: keep the same product direction the user approved, but make
    // the card larger and visually lighter so it stays readable on a 3.5-inch panel.
    lv_obj_t* time_card = MakeCard(home_layer_, 14, 16, 132, 112, kCardPink, 0xF0DCE5);
    MakeDot(time_card, 13, 13, kPink, 6);
    date_label_ = MakeLabel(time_card, "日期同步中", 24, 8, kMuted);
    lv_obj_set_width(date_label_, 96);
    lv_label_set_long_mode(date_label_, LV_LABEL_LONG_CLIP);
    time_label_ = MakeLabel(time_card, "--:--", 10, 33, kInk);
    lv_obj_set_width(time_label_, 112);
    lv_obj_set_style_text_align(time_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(time_label_, 2, 0);
    MakeDivider(time_card, 12, 74, 108, 0xF3E6EC);
    lv_obj_t* day_tip = MakeLabel(time_card, "新的一天，和我聊聊天吧～", 8, 82, kMuted);
    lv_obj_set_width(day_tip, 116);
    lv_label_set_long_mode(day_tip, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(day_tip, LV_TEXT_ALIGN_CENTER, 0);

    // Left lower: keep only the most important lines so the real hardware stays readable.
    lv_obj_t* ai_card = MakeCard(home_layer_, 14, 146, 132, 116, kCardLavender, 0xDCD0FB);
    MakeDot(ai_card, 13, 13, kBlue, 6);
    MakeLabel(ai_card, "AI 助手", 24, 8, kInk);
    ai_state_label_ = MakeLabel(ai_card, "待命中", 18, 33, kPurple);
    lv_obj_set_width(ai_state_label_, 96);
    lv_obj_set_style_text_align(ai_state_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t* ai_hint = MakeLabel(ai_card, "我在这里，随时陪伴你～", 10, 64, kMuted);
    lv_obj_set_width(ai_hint, 112);
    lv_obj_set_style_text_align(ai_hint, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t* talk_pill = MakeCard(ai_card, 10, 84, 112, 24, 0xEEE7FF, 0xD8CCFA, 12, LV_OPA_COVER);
    lv_obj_t* talk_text = MakeLabel(talk_pill, "点击开始对话", 4, 2, kPurple);
    lv_obj_set_width(talk_text, 104);
    lv_obj_set_style_text_align(talk_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(ai_card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ai_card, TalkEventCallback, LV_EVENT_CLICKED, this);

    // Right upper: weather, softened into the same overall pastel scene.
    lv_obj_t* weather_card = MakeCard(home_layer_, 334, 16, 132, 112, kCardBlue, 0xD7E4FB);
    MakeDot(weather_card, 13, 13, kBlue, 6);
    MakeLabel(weather_card, "天气", 24, 8, kInk);
    weather_value_label_ = MakeLabel(weather_card, "--°C", 12, 37, kBlue);
    lv_obj_set_width(weather_value_label_, 108);
    lv_obj_set_style_text_align(weather_value_label_, LV_TEXT_ALIGN_CENTER, 0);
    weather_hint_label_ = MakeLabel(weather_card, "等待天气数据", 12, 74, kMuted);
    lv_obj_set_width(weather_hint_label_, 108);
    lv_obj_set_style_text_align(weather_hint_label_, LV_TEXT_ALIGN_CENTER, 0);

    // Right lower: today's to-do only. Keep placeholder truthful until a real
    // reminder provider is connected.
    lv_obj_t* todo_card = MakeCard(home_layer_, 346, 136, 126, 174, kCardCream, 0xF2D7B4);
    MakeDot(todo_card, 12, 12, kAmber, 6);
    MakeLabel(todo_card, "今日待办", 25, 7, kInk);
    MakeDivider(todo_card, 12, 38, 102, 0xF3E6D5);
    todo_value_label_ = MakeLabel(todo_card, "暂无待办\n\n今天也要\n轻松一点", 12, 55, kMuted);
    lv_obj_set_width(todo_value_label_, 102);
    lv_obj_set_style_text_align(todo_value_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(todo_value_label_, LV_LABEL_LONG_WRAP);

    // A small glass-like prompt bridges the cards and central illustration.
    // It uses the same pale palette so the character visually belongs to the UI.
    lv_obj_t* caption = MakeCard(home_layer_, 154, 278, 172, 32, 0xFFF8FD, 0xE7CBE3, 14,
                                 LV_OPA_90);
    lv_obj_t* caption_text = MakeLabel(caption, "点击人物开始对话", 5, 5, kPurple);
    lv_obj_set_width(caption_text, 162);
    lv_obj_set_style_text_align(caption_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(caption, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(caption, TalkEventCallback, LV_EVENT_CLICKED, this);
}

void StellarDisplay::BuildChatUI() {
    lv_obj_t* screen = lv_screen_active();

    chat_layer_ = lv_obj_create(screen);
    lv_obj_set_pos(chat_layer_, 0, 0);
    lv_obj_set_size(chat_layer_, width_, height_);
    lv_obj_set_style_radius(chat_layer_, 0, 0);
    lv_obj_set_style_bg_color(chat_layer_, lv_color_hex(kBg), 0);
    lv_obj_set_style_bg_opa(chat_layer_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chat_layer_, 0, 0);
    lv_obj_set_style_pad_all(chat_layer_, 0, 0);
    lv_obj_set_scrollbar_mode(chat_layer_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(chat_layer_, LV_OBJ_FLAG_SCROLLABLE);

    // Slim top status bar in the same pastel language as the home page.
    lv_obj_t* header = MakeCard(chat_layer_, 8, 8, 464, 36, kCardBlue, 0xD1E1FA, 14);
    MakeDot(header, 11, 14, kBlue, 7);
    chat_header_label_ = MakeLabel(header, "正在与星语对话", 25, 6, kInk);
    chat_hint_label_ = MakeLabel(header, "BOOT 可打断", 338, 6, kMuted);
    lv_obj_set_width(chat_hint_label_, 112);
    lv_obj_set_style_text_align(chat_hint_label_, LV_TEXT_ALIGN_RIGHT, 0);

    // Main answer card. In short mode it leaves a generous companion panel on
    // the right; long replies expand the card and shrink the companion image.
    chat_card_ = MakeCard(chat_layer_, 8, 52, 338, 260, kPaper, 0xD8CCFA, 18);
    chat_title_label_ = MakeLabel(chat_card_, "星语", 12, 8, kPurple);
    user_preview_label_ = MakeLabel(chat_card_, "你：等待说话", 64, 8, kMuted);
    lv_obj_set_width(user_preview_label_, 260);
    lv_label_set_long_mode(user_preview_label_, LV_LABEL_LONG_CLIP);
    MakeDivider(chat_card_, 12, 37, 314, 0xECE7F3);

    chat_text_box_ = lv_obj_create(chat_card_);
    lv_obj_set_pos(chat_text_box_, 12, 47);
    lv_obj_set_size(chat_text_box_, 314, 201);
    lv_obj_set_style_bg_opa(chat_text_box_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chat_text_box_, 0, 0);
    lv_obj_set_style_pad_all(chat_text_box_, 0, 0);
    lv_obj_set_style_radius(chat_text_box_, 0, 0);
    lv_obj_set_style_clip_corner(chat_text_box_, true, 0);
    lv_obj_set_scrollbar_mode(chat_text_box_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(chat_text_box_, LV_OBJ_FLAG_SCROLLABLE);

    chat_text_label_ = lv_label_create(chat_text_box_);
    lv_obj_set_pos(chat_text_label_, 0, 0);
    lv_obj_set_width(chat_text_label_, 314);
    lv_obj_set_style_text_color(chat_text_label_, lv_color_hex(kInk), 0);
    lv_obj_set_style_text_align(chat_text_label_, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_line_space(chat_text_label_, 4, 0);
    lv_label_set_long_mode(chat_text_label_, LV_LABEL_LONG_WRAP);
    lv_label_set_text(chat_text_label_, "我在听，请慢慢说。\n回答会从顶部开始显示。");

    chat_avatar_frame_ = MakeCard(chat_layer_, 354, 62, 118, 236, kCardPink, 0xF0D6E3, 18);
    chat_avatar_ = lv_image_create(chat_avatar_frame_);
    lv_image_set_src(chat_avatar_, &stellar_avatar);
    lv_image_set_pivot(chat_avatar_, 0, 0);
    lv_image_set_scale(chat_avatar_, 145);  // 200x320 -> about 113x181
    lv_obj_set_pos(chat_avatar_, 2, 18);
}

void StellarDisplay::TalkEventCallback(lv_event_t* event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }
    auto* self = static_cast<StellarDisplay*>(lv_event_get_user_data(event));
    if (self == nullptr) {
        return;
    }

    auto& app = Application::GetInstance();
    DeviceState state = app.GetDeviceState();
    if (state == kDeviceStateStarting || state == kDeviceStateWifiConfiguring ||
        state == kDeviceStateActivating || state == kDeviceStateUpgrading ||
        state == kDeviceStateFatalError) {
        return;
    }

    self->ShowChat();
    app.ToggleChatState();
}

void StellarDisplay::ShowHome() {
    DisplayLockGuard lock(this);
    if (home_layer_ == nullptr || chat_layer_ == nullptr) {
        return;
    }
    StopChatScrollInternal();
    SetChatExpandedInternal(false);
    lv_obj_add_flag(chat_layer_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(home_layer_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(home_layer_);
    chat_visible_ = false;
    return_home_deadline_us_ = 0;
}

void StellarDisplay::ShowChat() {
    DisplayLockGuard lock(this);
    if (home_layer_ == nullptr || chat_layer_ == nullptr) {
        return;
    }
    lv_obj_add_flag(home_layer_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(chat_layer_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(chat_layer_);
    chat_visible_ = true;
    return_home_deadline_us_ = 0;

    if (chat_header_label_ != nullptr) {
        lv_label_set_text(chat_header_label_, "正在与星语对话...");
    }
}

void StellarDisplay::StopChatScrollInternal() {
    if (chat_text_label_ != nullptr) {
        lv_anim_delete(chat_text_label_, nullptr);
        lv_obj_set_y(chat_text_label_, 0);
    }
}

void StellarDisplay::SetChatExpandedInternal(bool expanded) {
    if (chat_card_ == nullptr || chat_text_box_ == nullptr || chat_text_label_ == nullptr ||
        chat_avatar_frame_ == nullptr || chat_avatar_ == nullptr) {
        return;
    }
    if (chat_expanded_ == expanded) {
        return;
    }

    StopChatScrollInternal();
    chat_expanded_ = expanded;

    if (expanded) {
        // RLCD-inspired long-answer mode: the answer gets priority. The card
        // expands horizontally while the companion becomes a small side strip.
        lv_obj_set_pos(chat_card_, 8, 52);
        lv_obj_set_size(chat_card_, 382, 260);
        lv_obj_set_width(user_preview_label_, 304);
        lv_obj_set_pos(chat_text_box_, 12, 47);
        lv_obj_set_size(chat_text_box_, 358, 201);
        lv_obj_set_width(chat_text_label_, 358);
        lv_obj_set_style_text_line_space(chat_text_label_, 3, 0);

        lv_obj_set_pos(chat_avatar_frame_, 396, 120);
        lv_obj_set_size(chat_avatar_frame_, 76, 166);
        lv_image_set_pivot(chat_avatar_, 0, 0);
        lv_image_set_scale(chat_avatar_, 92);  // about 72x115
        lv_obj_set_pos(chat_avatar_, 1, 20);
    } else {
        lv_obj_set_pos(chat_card_, 8, 52);
        lv_obj_set_size(chat_card_, 338, 260);
        lv_obj_set_width(user_preview_label_, 260);
        lv_obj_set_pos(chat_text_box_, 12, 47);
        lv_obj_set_size(chat_text_box_, 314, 201);
        lv_obj_set_width(chat_text_label_, 314);
        lv_obj_set_style_text_line_space(chat_text_label_, 4, 0);

        lv_obj_set_pos(chat_avatar_frame_, 354, 62);
        lv_obj_set_size(chat_avatar_frame_, 118, 236);
        lv_image_set_pivot(chat_avatar_, 0, 0);
        lv_image_set_scale(chat_avatar_, 145);
        lv_obj_set_pos(chat_avatar_, 2, 18);
    }

    lv_obj_set_y(chat_text_label_, 0);
    lv_obj_update_layout(chat_text_label_);
}

void StellarDisplay::SetAssistantTextInternal(const char* content) {
    StopChatScrollInternal();
    lv_label_set_long_mode(chat_text_label_, LV_LABEL_LONG_WRAP);
    lv_label_set_text(chat_text_label_, content);
    lv_obj_set_y(chat_text_label_, 0);
    lv_obj_update_layout(chat_text_label_);

    int visible_h = 201;
    int label_h = lv_obj_get_height(chat_text_label_);

    // Expand only when the reply actually overflows. Once expanded, it stays
    // expanded until the next user turn so sentence updates do not pump layout.
    if (!chat_expanded_ && label_h > visible_h) {
        SetChatExpandedInternal(true);
        lv_obj_update_layout(chat_text_label_);
        label_h = lv_obj_get_height(chat_text_label_);
    }

    if (label_h > visible_h) {
        const int distance = label_h - visible_h;
        int duration = distance * kScrollMsPerPixel;
        if (duration < 2000) {
            duration = 2000;
        }

        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, chat_text_label_);
        lv_anim_set_values(&anim, 0, -distance);
        lv_anim_set_delay(&anim, kScrollDelayMs);
        lv_anim_set_duration(&anim, duration);
        lv_anim_set_exec_cb(&anim, [](void* obj, int32_t value) {
            lv_obj_set_y(static_cast<lv_obj_t*>(obj), value);
        });
        // One-way only. No repeat and no circular jump back to the top.
        lv_anim_start(&anim);

        ESP_LOGI(TAG,
                 "V4 long reply: %dpx > %dpx, delay=%dms, speed=%dms/px, duration=%dms",
                 label_h, visible_h, kScrollDelayMs, kScrollMsPerPixel, duration);
    }
}

void StellarDisplay::SetChatMessage(const char* role, const char* content) {
    if (!setup_ui_called_ || role == nullptr || content == nullptr) {
        return;
    }

    // Empty protocol messages must not erase the last readable answer.
    if (content[0] == '\0') {
        return;
    }

    DisplayLockGuard lock(this);
    if (chat_layer_ == nullptr || chat_text_label_ == nullptr) {
        return;
    }

    if (home_layer_ != nullptr) {
        lv_obj_add_flag(home_layer_, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(chat_layer_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(chat_layer_);
    chat_visible_ = true;
    return_home_deadline_us_ = 0;

    if (std::strcmp(role, "user") == 0) {
        StopChatScrollInternal();
        SetChatExpandedInternal(false);
        lv_label_set_text_fmt(user_preview_label_, "你：%s", content);
        lv_label_set_text(chat_header_label_, "已经听到，正在思考...");
        lv_label_set_text(chat_title_label_, "星语");
        lv_label_set_text(chat_text_label_, "让我想一想... ");
        lv_obj_set_y(chat_text_label_, 0);
        return;
    }

    if (std::strcmp(role, "assistant") == 0) {
        lv_label_set_text(chat_header_label_, "星语正在回答...");
        lv_label_set_text(chat_title_label_, "星语");
        SetAssistantTextInternal(content);
        return;
    }

    // System text only updates the slim status line. It never becomes a giant
    // bubble or replaces the current assistant answer.
    if (std::strcmp(role, "system") == 0) {
        lv_label_set_text(chat_header_label_, content);
    }
}

void StellarDisplay::ClearChatMessages() {
    // Do not jump home immediately: protocol transitions can clear text before
    // TTS is fully finished. UpdateStatusBar() owns the delayed return home.
    DisplayLockGuard lock(this);
    StopChatScrollInternal();
    if (user_preview_label_ != nullptr) {
        lv_label_set_text(user_preview_label_, "你：等待说话");
    }
    if (chat_header_label_ != nullptr) {
        lv_label_set_text(chat_header_label_, "正在与星语对话");
    }
    if (chat_text_label_ != nullptr) {
        lv_label_set_text(chat_text_label_, "我在这里。\n点击人物或按 BOOT 键，可以再次开始对话。");
        lv_obj_set_y(chat_text_label_, 0);
    }
    SetChatExpandedInternal(false);
}

void StellarDisplay::UpdateChatState(DeviceState state) {
    DisplayLockGuard lock(this);
    if (!chat_visible_ || chat_header_label_ == nullptr) {
        return;
    }

    switch (state) {
        case kDeviceStateListening:
            lv_label_set_text(chat_header_label_, "正在与星语对话...");
            break;
        case kDeviceStateSpeaking:
            lv_label_set_text(chat_header_label_, "星语正在回答...");
            break;
        case kDeviceStateConnecting:
            lv_label_set_text(chat_header_label_, "正在连接 AI...");
            break;
        case kDeviceStateFatalError:
            lv_label_set_text(chat_header_label_, "系统异常");
            break;
        default:
            break;
    }
}

void StellarDisplay::UpdateStatusBar(bool update_all) {
    // Keep native xiaozhi status/power behavior behind our opaque UI.
    LvglDisplay::UpdateStatusBar(update_all);
    UpdateHomeInfo();

    DeviceState state = Application::GetInstance().GetDeviceState();
    UpdateChatState(state);

    int64_t now_us = esp_timer_get_time();

    if (state == kDeviceStateSpeaking) {
        return_home_deadline_us_ = 0;
        last_device_state_ = state;
        return;
    }

    if (last_device_state_ == kDeviceStateSpeaking && chat_visible_) {
        // Leave the final answer on-screen long enough to finish reading.
        return_home_deadline_us_ = now_us + kReturnHomeDelayUs;
    }

    if (return_home_deadline_us_ > 0 && now_us >= return_home_deadline_us_ &&
        state != kDeviceStateSpeaking) {
        ShowHome();
    }

    last_device_state_ = state;
}

void StellarDisplay::UpdateHomeInfo() {
    DeviceState state = Application::GetInstance().GetDeviceState();

    std::time_t now = std::time(nullptr);
    std::tm local_tm = {};
    localtime_r(&now, &local_tm);

    char time_buffer[16] = "--:--";
    char date_buffer[32] = "日期同步中";
    if (local_tm.tm_year + 1900 >= 2024) {
        std::strftime(time_buffer, sizeof(time_buffer), "%H:%M", &local_tm);
        std::snprintf(date_buffer, sizeof(date_buffer), "%02d/%02d %s",
                      local_tm.tm_mon + 1, local_tm.tm_mday, WeekdayText(local_tm.tm_wday));
    }

    DisplayLockGuard lock(this);
    if (home_layer_ == nullptr) {
        return;
    }

    if (time_label_ != nullptr) {
        lv_label_set_text(time_label_, time_buffer);
    }
    if (date_label_ != nullptr) {
        lv_label_set_text(date_label_, date_buffer);
    }
    if (ai_state_label_ != nullptr) {
        lv_label_set_text(ai_state_label_, StateText(state));
        lv_obj_set_style_text_color(ai_state_label_, lv_color_hex(StateColor(state)), 0);
    }

    // V4 keeps these placeholders honest. Do not invent weather/tasks before a
    // real provider is connected.
    if (weather_value_label_ != nullptr) {
        lv_label_set_text(weather_value_label_, "--°C");
    }
    if (weather_hint_label_ != nullptr) {
        lv_label_set_text(weather_hint_label_, "等待天气数据");
    }
    if (todo_value_label_ != nullptr) {
        lv_label_set_text(todo_value_label_, "10:00 查看邮件\n14:30 项目会议\n20:00 休息放松");
    }
}
