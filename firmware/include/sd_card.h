// Project  : Jarvis Edge Node
// File     : sd_card.h
// Purpose  : SD card initialisation — public interface
// Depends  : <SPI.h>, <SD.h>

#pragma once

// Initialise the dedicated SPI bus (SPI3/HSPI) and mount the SD card.
// Must be called in setup() before mic_capture can queue recordings.
void sdCardInit();

// Returns true if the SD card was successfully mounted.
bool sdCardMounted();

// Counts the .wav files currently waiting in /queue (used to seed the
// status bar's queue badge; see docs/sdd.txt section 4.2 Auto-Sync).
int sdCardCountQueueFiles();
