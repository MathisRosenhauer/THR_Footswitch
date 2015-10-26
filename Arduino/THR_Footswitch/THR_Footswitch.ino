/*
 * Copyright 2015
 * Mathis Rosenhauer
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
  Sketch for an Arduino Uno (other types may work as well) with an USB
  host shield and SD card adapter. A Yamaha THR10/5 is connected to
  the USB host shield. The SD card holds a patch file as used with the
  THR editor. Patches are read from the SD card and sent to the
  THR. Two switches are used to navigate through the patch file.
*/

#include <Bounce2.h>
#include <SD.h>
#include <Usb.h>
#include <usbh_midi.h>
#include <TM1637Display.h>

// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

const uint8_t button1_pin = 2;
const uint8_t button2_pin = 3;
const uint8_t sdcard_ss_pin = 4;
const uint8_t display_dio_pin = 5;
const uint8_t display_clk_pin = 6;
// usbh_ss_pin 10
// usbh_int_pin 9

USB Usb;
USBH_MIDI Midi(&Usb);
TM1637Display display(display_clk_pin, display_dio_pin);
Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();

void setup()
{
    Serial.begin(115200);

    // Buttons setup
    pinMode(button1_pin,INPUT_PULLUP);
    debouncer1.attach(button1_pin);
    debouncer1.interval(5);
    pinMode(button2_pin,INPUT_PULLUP);
    debouncer2.attach(button2_pin);
    debouncer2.interval(5);
    display.setBrightness(0x0f);

    // SD card setup
    if (!SD.begin(sdcard_ss_pin)) {
        Serial.println("Card failed, or not present");
        return;
    }
    Serial.println("Card initialized.");

    // Workaround for non UHS2.0 Shield
    pinMode(7,OUTPUT);
    digitalWrite(7,HIGH);

    // USBH setup
    if (Usb.Init() == -1) {
        while(1); //halt
    }
    delay(200);
    Serial.println("USB initialized.");
}

void send_patch(uint8_t patch_id)
{
    int i, j;
    uint8_t prefix[] = {
        0xf0, 0x43, 0x7d, 0x00, 0x02, 0x0c, 0x44, 0x54,
        0x41, 0x31, 0x41, 0x6c, 0x6c, 0x50, 0x00, 0x00,
        0x7f, 0x7f
    };
    int cs = 0x371; // prefix checksum
    const int patch_size = 0x100;
    uint8_t line[16];
    uint8_t suffix[2];
    File patchfile;

    patchfile = SD.open("THR10C.YDL");
    if (patchfile) {
        Midi.SendSysEx(prefix, sizeof(prefix));

        patchfile.seek(0xd + (patch_id - 1) * (patch_size + 5));
        for (j = 0; j < patch_size; j += sizeof(line)) {
            for (i = 0; i < sizeof(line); i++) {
                if (j + i == patch_size - 1)
                    line[i] = 0;
                else
                    line[i] = patchfile.read();
                cs += line[i];
            }
            Midi.SendSysEx(line, sizeof(line));
        }
        suffix[0] = (~cs + 1) & 0x7f;
        suffix[1] = 0xf7;
        Midi.SendSysEx(suffix, sizeof(suffix));
        patchfile.close();
    } else {
        Serial.println("File not present.");
    }
}

int read_buttons(void)
{
    debouncer1.update();
    debouncer2.update();
    return (debouncer1.rose() ? 1: 0) + (debouncer2.rose() ? -1: 0);
}

void loop() {
    static uint8_t patch_id = 1;
    static uint16_t pid = 0;
    static uint16_t vid = 0;
    static bool thr_connected = false;
    int button_state;

    Usb.Task();
    if(Usb.getUsbTaskState() == USB_STATE_RUNNING) {
        if(Midi.vid != vid || Midi.pid != pid){
            vid = Midi.vid;
            pid = Midi.pid;
            thr_connected = true;
            send_patch(patch_id);
            display.showNumberDec(patch_id, false);
        }
    }

    button_state = read_buttons();
    if (button_state != 0 && thr_connected) {
        patch_id = constrain(patch_id + button_state, 0, 100);
        send_patch(patch_id);
        display.showNumberDec(patch_id, false);
    }
}
