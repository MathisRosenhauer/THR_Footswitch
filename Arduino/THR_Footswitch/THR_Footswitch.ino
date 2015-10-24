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
#include <SPI.h>
#include <SD.h>
#include <Usb.h>
#include <usbh_midi.h>
#include <TM1637Display.h>

#define PATCH_MAGIC "\xf0\x43\x7d\x00\x02\x0c\x44\x54\x41\x31\x41\x6c\x6c\x50\x00\x00\x7f\x7f"
#define PRESET_FILE_HEADER 6
#define PATCH_SIZE 0x100

#define BUTTON_PIN_1 2
#define BUTTON_PIN_2 3
#define CS_SDCARD 4
// Display IO
#define DIO 5
#define CLK 6
// CS_USBH 7

USB  Usb;
USBH_MIDI Midi(&Usb);
TM1637Display display(CLK, DIO);

uint16_t pid, vid;
uint8_t patch_id = 1;

Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();

void setup()
{
    vid = pid = 0;
    Serial.begin(115200);

    // Setup buttons
    pinMode(BUTTON_PIN_1,INPUT_PULLUP);
    debouncer1.attach(BUTTON_PIN_1);
    debouncer1.interval(5);
    pinMode(BUTTON_PIN_2,INPUT_PULLUP);
    debouncer2.attach(BUTTON_PIN_2);
    debouncer2.interval(5);
    display.setBrightness(0x0f);

    if (!SD.begin(CS_SDCARD)) {
        Serial.println("Card failed, or not present");
        return;
    }
    Serial.println("Card initialized.");

    //Workaround for non UHS2.0 Shield
    pinMode(7,OUTPUT);
    digitalWrite(7,HIGH);

    if (Usb.Init() == -1) {
        while(1); //halt
    }
    delay(200);
    Serial.println("USB initialized.");
}

void send_patch()
{
    char buf[20];
    int i, j;
    uint8_t prefix[] = PATCH_MAGIC;
    uint8_t line[16];
    uint8_t suffix[2];
    int cs = 0x371; // This is the static prefix part
    File patchfile;

    patchfile = SD.open("THR10C.YDL");
    if (patchfile) {
        patchfile.seek(0xd + (patch_id - 1) * (PATCH_SIZE + 5));
        Midi.SendSysEx(prefix, 18);
//        for (i = 0; i < 18; i++) {
//            sprintf(buf, " %02X", prefix[i]);
//            Serial.print(buf);
//        }
//        Serial.println("");

        for (j = 0; j < 16; j++) {
            for (i = 0; i < 16; i++) {
                if (j == 15 && i == 15)
                    line[i] = 0;
                else
                    line[i] = patchfile.read();
//                sprintf(buf, " %02X", line[i]);
//                Serial.print(buf);
                cs += line[i];
            }
            Midi.SendSysEx(line, 16);
            Serial.println("");
        }
        suffix[0] = (~cs + 1) & 0x7f;
        suffix[1] = 0xf7;
        Midi.SendSysEx(suffix, 2);
//        sprintf(buf, " cs: %02X", suffix[0]);
//        Serial.print(buf);
        patchfile.close();
    } else {
        Serial.println("File not present.");
    }
}

void loop() {
    int button1, button2;
    char buf[20];

    Usb.Task();
    if(Usb.getUsbTaskState() == USB_STATE_RUNNING) {
        if(Midi.vid != vid || Midi.pid != pid){
            sprintf(buf, "VID:%04X, PID:%04X", Midi.vid, Midi.pid);
            Serial.println(buf);
            vid = Midi.vid;
            pid = Midi.pid;
        }
    }

    debouncer1.update();
    debouncer2.update();

    button1 = debouncer1.fell();
    button2 = debouncer2.fell();

    if (button1 || button2) {
        if (button1 && patch_id < 100)
            patch_id++;
        if (button2 && patch_id > 1)
            patch_id--;
        send_patch();
        display.showNumberDec(patch_id, false);
    }
}
