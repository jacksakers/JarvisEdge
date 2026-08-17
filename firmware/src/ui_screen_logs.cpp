// Project  : Jarvis Edge Node
// File     : ui_screen_logs.cpp
// Purpose  : "Voice Logs" tile — displays voice logs and their transcriptions saved to SD card
// Depends  : ui_screen_logs.h, ui.h, sd_card.h, <SD.h>

#include "ui_screen_logs.h"
#include "ui.h"
#include "sd_card.h"
#include <SD.h>
#include <Arduino.h>

static lv_obj_t * s_logs_ta = nullptr;

void uiLogsScreenInit(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, lv_color_hex(UI_CLR_BG), 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * hint = lv_label_create(tile);
    lv_label_set_text(hint, "RECORDED VOICE LOGS (SD)");
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_CLR_MUTED), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 10);

    // Create a scrollable text area to read and show logs.
    s_logs_ta = lv_textarea_create(tile);
    lv_obj_set_size(s_logs_ta, UI_SCREEN_W - 30, UI_CAROUSEL_H - 50);
    lv_obj_align(s_logs_ta, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_textarea_set_cursor_click_pos(s_logs_ta, false);
    lv_textarea_set_text_selection(s_logs_ta, false);
    lv_obj_set_style_bg_color(s_logs_ta, lv_color_hex(UI_CLR_SURFACE), 0);
    lv_obj_set_style_text_color(s_logs_ta, lv_color_hex(UI_CLR_TEXT), 0);
    lv_obj_set_style_border_width(s_logs_ta, 0, 0);
    lv_obj_set_style_text_font(s_logs_ta, &lv_font_montserrat_12, 0);

    uiLogsScreenReload();
}

void uiLogsScreenReload()
{
    if (!s_logs_ta) return;

    if (!sdCardMounted()) {
        lv_textarea_set_text(s_logs_ta, "SD Card not mounted.");
        return;
    }

    if (!SD.exists("/logs/history.txt")) {
        lv_textarea_set_text(s_logs_ta, "No recorded voice logs found yet.\nStart recording using the hardware BOOT button!");
        return;
    }

    File f = SD.open("/logs/history.txt", FILE_READ);
    if (!f) {
        lv_textarea_set_text(s_logs_ta, "Failed to read logs history.");
        return;
    }

    // Read lines from the file and load them into a String buffer
    String content = "";
    while (f.available()) {
        content += (char)f.read();
    }
    f.close();

    if (content.length() == 0) {
        lv_textarea_set_text(s_logs_ta, "Logs history is empty.");
    } else {
        lv_textarea_set_text(s_logs_ta, content.c_str());
        // Scroll to the bottom to see latest logs first
        lv_obj_scroll_to_y(s_logs_ta, 99999, LV_ANIM_OFF);
    }
}
