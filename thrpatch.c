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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

/*  F0: Begin system exclusive
 *  43: YAMAHA ID
 *  7D: Device number
 *  00: ?
 */
#define MESSAGE_START "\xf0\x43\x7d\x00"

/* Seems to be the same for all patches */
#define PATCH_MAGIC "DTA1AllP\x00\x00\x7f\x7f"

#define HEADER_SIZE 6
#define DATA_SIZE 0x10c
#define TRAILER_SIZE 2
#define SYSEX_SIZE (HEADER_SIZE + DATA_SIZE + TRAILER_SIZE)

#define PATCH_SIZE 0x100

void usage(void)
{
    fprintf(stderr, "NAME\n\tthrpatch - upload settings from a ");
    fprintf(stderr, ".YDL file to a Yamaha THR10/5\n\n");
    fprintf(stderr, "SYNOPSIS\n\tthrpatch [OPTION]... FILE\n");
    fprintf(stderr, "\nOPTIONS\n");
    fprintf(stderr, "\t-h\n\t\tprint this text\n");
    fprintf(stderr, "\t-l\n\t\tlist all patches in file\n");
    fprintf(stderr, "\t-d device\n\t\tdump patch to MIDI device\n");
    fprintf(stderr, "\t-o file\n\t\tdump patch into .syx file\n");
}

int main(int argc, char *argv[])
{
    int i, cs, err;
    int pid  = -1;
    int list = 0;
    char *fn, *ofn = 0, *device = 0;
    char *presets, *pi, *p;
    FILE *fp;
    size_t fsize;
    snd_rawmidi_t *handle = 0;

    i = 1;
    while ((i < argc) && (argv[i][0] == '-')) {
        switch (argv[i][1]) {
        case 'h':
            usage();
            return 0;
            break;
        case 'l':
            list = 1;
            break;
        case 'd':
            if (i + 1 < argc)
                device = argv[++i];
            break;
        case 'o':
            if (i + 1 < argc)
                ofn = argv[++i];
            break;
        case 'p':
            if (i + 1 < argc)
                pid = atoi(argv[++i]);
            break;
        default:
            usage();
            return 1;
        }
        i++;
    }
    if (argc - i < 1) {
        usage();
        return 1;
    }

    fn = argv[i];
    fp = fopen(fn, "rb");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: cannot open preset file %s.\n", fn);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    presets = malloc(fsize + 1);
    if (presets == NULL) {
        fprintf(stderr, "ERROR: Out of emory.\n");
        return 1;
    }

    fread(presets, fsize, 1, fp);
    fclose(fp);

    if (strncmp("DTAB01d", presets, 7)) {
        fprintf(stderr, "ERROR: file %s is not a YDL preset file.\n", fn);
        free(presets);
        return 1;
    }

    if (list) {
        pi = presets + 0xd;
        pid = 0;
        while (pi - presets <= fsize - PATCH_SIZE) {
            printf("%i: %.96s\n", pid++, pi);
            pi += PATCH_SIZE + 5;
        }
    }

    if (pid >= 0) {
        p = malloc(SYSEX_SIZE);
        if (p == NULL) {
            fprintf(stderr, "ERROR: Out of emory.\n");
            return 1;
        }

        /* Header */
        memcpy(p, MESSAGE_START, 4);
        p[4] = DATA_SIZE >> 7;
        p[5] = DATA_SIZE & 0x7f;
        pi = p + HEADER_SIZE;

        /* Assemble data section */
        memcpy(pi, PATCH_MAGIC, 0xc);
        memcpy(pi + 0xc, presets + 0xd + pid * (PATCH_SIZE + 5), PATCH_SIZE);
        /* Last byte of patch has to be zero. No idea why. */
        pi[DATA_SIZE - 1] = 0;

        /* Calculate checksum */
        cs = 0;
        for (i = 0; i < DATA_SIZE; i++)
            cs += pi[i];
        pi += DATA_SIZE;
        *pi++ = (~cs + 1) & 0x7f;

        /* End sysex */
        *pi = 0xf7;

        if (device) {
            err = snd_rawmidi_open(NULL, &handle, device, 0);
            if (err) {
                fprintf(stderr,
                        "ERROR: snd_rawmidi_open %s failed: %d\n",
                        device, err);
                free(presets);
                free(p);
                return 1;
            }
            snd_rawmidi_write(handle, p, SYSEX_SIZE);
            snd_rawmidi_drain(handle);
            snd_rawmidi_close(handle);
        }

        if (ofn) {
            fp = fopen(ofn, "wb");
            if (fp == NULL) {
                fprintf(stderr, "ERROR: cannot open output file %s.\n", ofn);
                return 1;
            }
            fwrite(p, SYSEX_SIZE, 1, fp);
            fclose(fp);
        }
        free(p);
    }

    free(presets);
    return 0;
}
