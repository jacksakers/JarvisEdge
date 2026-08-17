// Project  : Jarvis Edge Node
// File     : sync_manager.h
// Purpose  : Background auto-sync of /queue/*.wav to the backend — public interface
// Depends  : (none)

#pragma once

// Arms the auto-sync FreeRTOS task (pinned to core 0, same pattern as the
// mic capture writer task). Call once from setup(), after sdCardInit() and
// wifiManagerInit(). The task self-schedules — nothing to call per loop().
void syncManagerInit();
