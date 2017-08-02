/*
 * Arduino program for sending preset data to a Yamaha THR10 guitar amplifier.
 * Copyright (C) 2016 Mathis Rosenhauer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
*/

/* Set USE_SDCARD to 1 to use SD card reader to read from patch
   file. If USE_SDCARD is set to 0 then the file patches.h is included
   which needs to define a PROGMEM variable named patches. Use
   patchdump.py to generate patches.h from a .YDL file. E.g.

   ./patchdump.py THR10C.YDL -n 12 > patches.h

   to dump the first 12 patches from THR10C.YDL. The Arduino IDE will
   complain if the patches won't fit into PROGMEM. On a Uno -n 100
   (all patches) are too many but 79 worked at one point in time.
*/
#define USE_SDCARD 1

#include <avr/pgmspace.h>
#include <Bounce2.h>
#include <Usb.h>
#include <usbh_midi.h>
#include <TM1637Display.h>

#if USE_SDCARD
#include <SD.h>
#define PATCH_FILE "THR10C.YDL"
#else
#include "patches.h"
#endif

// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

const uint8_t button_l_pin = 2;
const uint8_t button_r_pin = 3;
const uint8_t display_clk_pin = 5;
const uint8_t display_dio_pin = 6;
const uint8_t sdcard_ss_pin = 4;
// usbh_int_pin 9
// usbh_ss_pin 10

// This is displayed when no THR is connected.
const uint8_t seg_thr[] = {
    SEG_D | SEG_E | SEG_F | SEG_G, // t
    SEG_C | SEG_E | SEG_F | SEG_G, // h
    SEG_E | SEG_G,                 // r
    SEG_C | SEG_B                  // 1
};

USB Usb;
USBH_MIDI Midi(&Usb);
TM1637Display display(display_clk_pin, display_dio_pin);
Bounce debouncer_r = Bounce();
Bounce debouncer_l = Bounce();

void setup()
{
    Serial.begin(115200);

    // Buttons setup
    // Right button increases patch id.
    pinMode(button_r_pin, INPUT_PULLUP);
    debouncer_r.attach(button_r_pin);
    debouncer_r.interval(5);

    // Left button decreases patch id.
    pinMode(button_l_pin, INPUT_PULLUP);
    debouncer_l.attach(button_l_pin);
    debouncer_l.interval(5);

    // Display setup
    display.setBrightness(0x0f);
    display.setSegments(seg_thr);

#if USE_SDCARD
    // SD card setup
    if (!SD.begin(sdcard_ss_pin)) {
        Serial.println(F("Card failed, or not present."));
        while(1); //halt
    }
    Serial.println(F("Card initialized."));
#endif

    // USBH setup
    if (Usb.Init() == -1) {
        Serial.println(F("USB host init failed."));
        while(1); //halt
    }
    delay(200);
    Serial.println(F("USB initialized."));
}

void send_patch(uint8_t patch_id)
{
    uint8_t prefix[] = {
        0xf0, 0x43, 0x7d, 0x00, 0x02, 0x0c, 0x44, 0x54,
        0x41, 0x31, 0x41, 0x6c, 0x6c, 0x50, 0x00, 0x00,
        0x7f, 0x7f
    };
    uint8_t cs = 0x71; // prefix checksum
    const size_t patch_size = 0x100;
    size_t offset = 13 + (patch_id - 1) * (patch_size + 5);
    uint8_t chunk[16];
    uint8_t suffix[2];
#if USE_SDCARD
    File patchfile = SD.open(PATCH_FILE);
    if (patchfile) {
        patchfile.seek(offset);
#endif
        Midi.SendSysEx(prefix, sizeof(prefix));
        for (size_t j = 0; j < patch_size; j += sizeof(chunk)) {
            for (size_t i = 0; i < sizeof(chunk); i++) {
                if (j + i == patch_size - 1) {
                    // Last byte of patch has to be zero.
                    chunk[i] = 0;
                } else {
#if USE_SDCARD
                    chunk[i] = patchfile.read();
#else
                    chunk[i] = pgm_read_byte(patches + offset++);
#endif
                }
                cs += chunk[i];
            }
            // Split SysEx. THR doesn't seem to care even though
            // USBH_MIDI ends each fragment with a 0x6 or 0x7 Code
            // Index Number (CIN).
            Midi.SendSysEx(chunk, sizeof(chunk));
        }
        // Calculate check sum.
        suffix[0] = (~cs + 1) & 0x7f;
        // end SysEx
        suffix[1] = 0xf7;
        Midi.SendSysEx(suffix, sizeof(suffix));
#if USE_SDCARD
        patchfile.close();
    } else {
        Serial.println(F("File not present."));
    }
#endif
}

int read_buttons(void)
{
    debouncer_r.update();
    debouncer_l.update();
    return (debouncer_r.rose() ? 1: 0) + (debouncer_l.rose() ? -1: 0);
}

void loop() {
    static uint8_t patch_id = 1;
    static bool thr_connected = false;
#if USE_SDCARD
    const int npatches = 100;
#else
    const int npatches = (sizeof(patches) - 13) / 261;
#endif

    Usb.Task();
    if (Usb.getUsbTaskState() == USB_STATE_RUNNING) {
        if (thr_connected) {
            int button_state = read_buttons();
            if (button_state != 0) {
                patch_id = constrain(patch_id + button_state, 1, npatches);
                send_patch(patch_id);
                display.showNumberDec(patch_id, false);
            }
        } else {
            // A new device was connected. Assume it
            // is a THR5/10. Just don't connect anything else.
            delay(1000);
            thr_connected = true;
            send_patch(patch_id);
            display.showNumberDec(patch_id, false);
        }
    } else if (thr_connected) {
        thr_connected = false;
        display.setSegments(seg_thr);
    }
}
