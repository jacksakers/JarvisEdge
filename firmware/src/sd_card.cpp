// Project  : House Phone
// File     : sd_card.cpp
// Purpose  : SD card SPI init and mount (SPI3/HSPI — separate from display SPI2)
// Depends  : sd_card.h, <SPI.h>, <SD.h>

#include "sd_card.h"
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

/* ── SD card SPI pins (Elecrow CrowPanel Advance 3.5") ───────────────────────
 * The display uses SPI2/FSPI (SCLK=42, MOSI=39).
 * The SD card uses SPI3/HSPI to avoid bus contention.
 * Carried over from the HelpDesk firmware for the same board family —
 * re-verify against the real House Phone schematic once in hand.
 * ─────────────────────────────────────────────────────────────────────────── */
#define SD_MOSI  6
#define SD_MISO  4
#define SD_CLK   5
#define SD_CS    7
#define SD_FREQ  25000000   /* 25 MHz — safe for most SD cards */

static SPIClass s_sd_spi(HSPI);
static bool s_mounted = false;

void sdCardInit()
{
    s_sd_spi.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

    if (!SD.begin(SD_CS, s_sd_spi, SD_FREQ)) {
        Serial.println("[SD] Mount failed — check card insertion and wiring.");
        return;
    }

    s_mounted = true;

    uint64_t total_mb = SD.totalBytes() / (1024ULL * 1024ULL);
    uint64_t used_mb  = SD.usedBytes()  / (1024ULL * 1024ULL);
    Serial.printf("[SD] Mounted OK. Type=%d  %llu MB total  %llu MB used\n",
                  (int)SD.cardType(), total_mb, used_mb);

    /* /queue holds recordings awaiting upload (see docs/sdd.txt section 4.1) */
    if (!SD.exists("/queue")) SD.mkdir("/queue");
}

bool sdCardMounted()
{
    return s_mounted;
}

int sdCardCountQueueFiles()
{
    if (!s_mounted) return 0;

    File dir = SD.open("/queue");
    if (!dir) return 0;

    int count = 0;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        if (!f.isDirectory()) count++;
        f.close();
    }
    dir.close();
    return count;
}
