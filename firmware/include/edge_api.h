// Project  : Jarvis Edge Node
// File     : edge_api.h
// Purpose  : Fire-and-forget HTTP helpers for talking to the Phase 3 backend
//            from UI event callbacks without blocking LVGL (docs/coding.txt 2.2)
// Depends  : settings.h

#pragma once

// POSTs {backend}/focus/{id}/toggle. Called when the user taps a Daily
// Focus row so the server-side FocusItem.done flag stays in sync with the
// on-device strike-through toggle.
void edgeApiToggleFocus(int id);

// POSTs {backend}/actions/{action_type} with body {"text": text}.
// action_type must be one of "time_track" | "note" | "alert" | "dismiss"
// (see VALID_ACTION_TYPES in ../../backend/app/main.py). Pass "" for text
// when the action doesn't need one (time_track/dismiss).
void edgeApiTriggerAction(const char * action_type, const char * text);
