/*
 * SpanDSP - a series of DSP components for telephony
 *
 * adsi_tests.c - tests for analogue display service handling.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2003 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: adsi_tests.c,v 1.48 2008/11/30 10:17:31 steveu Exp $
 */

/*! \page adsi_tests_page ADSI tests
\section adsi_tests_page_sec_1 What does it do?
These tests exercise the ADSI module, for all supported standards. A transmit
and a receive instance of the ADSI module are connected together. A quantity
of messages is passed between these instances, and checked for accuracy at
the receiver. Since the FSK modems used for this are exercised fully by other
tests, these tests do not include line modelling.

\section adsi_tests_page_sec_2 How does it work?
*/

/* Enable the following definition to enable direct probing into the FAX structures */
//#define WITH_SPANDSP_INTERNALS

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <audiofile.h>

//#if defined(WITH_SPANDSP_INTERNALS)
#define SPANDSP_EXPOSE_INTERNAL_STRUCTURES
//#endif

#include "spandsp.h"
#include "spandsp-sim.h"

#define OUT_FILE_NAME   "adsi.wav"

#define BLOCK_LEN       160

char *decode_test_file = NULL;

int errors = 0;

adsi_rx_state_t *rx_adsi;
adsi_tx_state_t *tx_adsi;

int current_standard = 0;
int good_message_received;

static int adsi_create_message(adsi_tx_state_t *s, uint8_t *msg)
{
    const char *t;
    int len;
    static int cycle = 0;

    len = 0;
    switch (current_standard)
    {
    case ADSI_STANDARD_CLASS:
        if (cycle > 3)
            cycle = 0;
        switch (cycle)
        {
        case 0:
            len = adsi_add_field(s, msg, -1, CLASS_MDMF_CALLERID, NULL, 0);
            /* Date and time as MMDDHHMM */
            len = adsi_add_field(s, msg, len, MCLASS_DATETIME, (uint8_t *) "10011750", 8);
            len = adsi_add_field(s, msg, len, MCLASS_CALLER_NUMBER, (uint8_t *) "12345678", 8);
            len = adsi_add_field(s, msg, len, MCLASS_DIALED_NUMBER, (uint8_t *) "87654321", 8);
            len = adsi_add_field(s, msg, len, MCLASS_CALLER_NAME, (uint8_t *) "Chan Dai Man", 15);
            break;
        case 1:
            len = adsi_add_field(s, msg, -1, CLASS_SDMF_MSG_WAITING, NULL, 0);
            /* Active */
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "\x42", 1);
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "\x42", 1);
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "\x42", 1);
            break;
        case 2:
            len = adsi_add_field(s, msg, -1, CLASS_SDMF_MSG_WAITING, NULL, 0);
            /* Inactive */
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "\x6F", 1);
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "\x6F", 1);
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "\x6F", 1);
            break;
        case 3:
            len = adsi_add_field(s, msg, -1, CLASS_SDMF_CALLERID, NULL, 0);
            /* Date and time as MMDDHHMM */
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "10011750", 8);
            len = adsi_add_field(s, msg, len, 0, (uint8_t *) "6095551212", 10);
            break;
        }
        break;
    case ADSI_STANDARD_CLIP:
        if (cycle > 4)
            cycle = 0;
        switch (cycle)
        {
        case 0:
            len = adsi_add_field(s, msg, -1, CLIP_MDMF_CALLERID, NULL, 0);
            len = adsi_add_field(s, msg, len, CLIP_CALLTYPE, (uint8_t *) "\x81", 1);
            /* Date and time as MMDDHHMM */
            len = adsi_add_field(s, msg, len, CLIP_DATETIME, (uint8_t *) "10011750", 8);
            len = adsi_add_field(s, msg, len, CLIP_DIALED_NUMBER, (uint8_t *) "12345678", 8);
            len = adsi_add_field(s, msg, len, CLIP_CALLER_NUMBER, (uint8_t *) "87654321", 8);
            len = adsi_add_field(s, msg, len, CLIP_CALLER_NAME, (uint8_t *) "Chan Dai Man", 15);
            break;
        case 1:
            len = adsi_add_field(s, msg, -1, CLIP_MDMF_MSG_WAITING, NULL, 0);
            /* Inactive */
            len = adsi_add_field(s, msg, len, CLIP_VISUAL_INDICATOR, (uint8_t *) "\x00", 1);
            break;
        case 2:
            len = adsi_add_field(s, msg, -1, CLIP_MDMF_MSG_WAITING, NULL, 0);
            /* Active */
            len = adsi_add_field(s, msg, len, CLIP_VISUAL_INDICATOR, (uint8_t *) "\xFF", 1);
            len = adsi_add_field(s, msg, len, CLIP_NUM_MSG, (uint8_t *) "\x05", 1);
            break;
        case 3:
            len = adsi_add_field(s, msg, -1, CLIP_MDMF_SMS, NULL, 0);
            /* Active */
            len = adsi_add_field(s, msg, len, CLIP_DISPLAY_INFO, (uint8_t *) "\x00" "ABC", 4);
            break;
        case 4:
            len = adsi_add_field(s, msg, -1, CLIP_MDMF_CALLERID, NULL, 0);
            len = adsi_add_field(s, msg, len, CLIP_NUM_MSG, (uint8_t *) "\x03", 1);
            break;
        }
        break;
    case ADSI_STANDARD_ACLIP:
        if (cycle > 0)
            cycle = 0;
        switch (cycle)
        {
        case 0:
            len = adsi_add_field(s, msg, -1, ACLIP_MDMF_CALLERID, NULL, 0);
            /* Date and time as MMDDHHMM */
            len = adsi_add_field(s, msg, len, ACLIP_DATETIME, (uint8_t *) "10011750", 8);
            len = adsi_add_field(s, msg, len, ACLIP_DIALED_NUMBER, (uint8_t *) "12345678", 8);
            len = adsi_add_field(s, msg, len, ACLIP_CALLER_NUMBER, (uint8_t *) "87654321", 8);
            len = adsi_add_field(s, msg, len, ACLIP_CALLER_NAME, (uint8_t *) "Chan Dai Man", 15);
            break;
        }
        break;
    case ADSI_STANDARD_JCLIP:
        len = adsi_add_field(s, msg, -1, JCLIP_MDMF_CALLERID, NULL, 0);
        len = adsi_add_field(s, msg, len, JCLIP_CALLER_NUMBER, (uint8_t *) "12345678", 8);
        len = adsi_add_field(s, msg, len, JCLIP_CALLER_NUM_DES, (uint8_t *) "215", 3);
        len = adsi_add_field(s, msg, len, JCLIP_DIALED_NUMBER, (uint8_t *) "87654321", 8);
        len = adsi_add_field(s, msg, len, JCLIP_DIALED_NUM_DES, (uint8_t *) "215", 3);
        break;
    case ADSI_STANDARD_CLIP_DTMF:
        if (cycle > 4)
            cycle = 0;
        switch (cycle)
        {
        case 0:
            len = adsi_add_field(s, msg, -1, CLIP_DTMF_C_TERMINATED, NULL, 0);
            len = adsi_add_field(s, msg, len, CLIP_DTMF_C_CALLER_NUMBER, (uint8_t *) "12345678", 8);
            len = adsi_add_field(s, msg, len, CLIP_DTMF_C_ABSENCE, (uint8_t *) "10", 2);
            len = adsi_add_field(s, msg, len, CLIP_DTMF_C_REDIRECT_NUMBER, (uint8_t *) "87654321", 8);
            break;
        case 1:
            len = adsi_add_field(s, msg, -1, CLIP_DTMF_HASH_TERMINATED, NULL, 0);
            len = adsi_add_field(s, msg, len, CLIP_DTMF_HASH_CALLER_NUMBER, (uint8_t *) "12345678", 8);
            break;
        case 2:
            len = adsi_add_field(s, msg, -1, CLIP_DTMF_HASH_TERMINATED, NULL, 0);
            len = adsi_add_field(s, msg, len, CLIP_DTMF_HASH_ABSENCE, (uint8_t *) "1", 1);
            break;
        case 3:
            /* Test the D<number>C format, used in Taiwan and Kuwait */
            len = adsi_add_field(s, msg, -1, CLIP_DTMF_HASH_TERMINATED, NULL, 0);
            len = adsi_add_field(s, msg, len, CLIP_DTMF_HASH_ABSENCE, (uint8_t *) "12345678", 8);
            break;
        case 4:
            /* Test the <number># format, with no header */
            len = adsi_add_field(s, msg, -1, CLIP_DTMF_HASH_TERMINATED, NULL, 0);
            len = adsi_add_field(s, msg, len, CLIP_DTMF_HASH_UNSPECIFIED, (uint8_t *) "12345678", 8);
            break;
        }
        break;
    case ADSI_STANDARD_TDD:
        t = "The quick Brown Fox Jumps Over The Lazy dog 0123456789!@#$%^&*()";
        len = adsi_add_field(s, msg, -1, 0, (uint8_t *) t, strlen(t));
        break;
    }
    cycle++;
    return len;
}
/*- End of function --------------------------------------------------------*/

static void put_adsi_msg(void *user_data, const uint8_t *msg, int len)
{
    int i;
    int l;
    uint8_t field_type;
    const uint8_t *field_body;
    int field_len;
    int message_type;
    uint8_t body[256];
    
    printf("Good message received (%d bytes)\n", len);
    good_message_received = TRUE;
    for (i = 0;  i < len;  i++)
    {
        printf("%02x ", msg[i]);
        if ((i & 0xF) == 0xF)
            printf("\n");
    }
    printf("\n");
    l = -1;
    message_type = -1;
    printf("Message breakdown\n");
    do
    {
        l = adsi_next_field(rx_adsi, msg, len, l, &field_type, &field_body, &field_len);
        if (l > 0)
        {
            if (field_body)
            {
                memcpy(body, field_body, field_len);
                body[field_len] = '\0';
                printf("Field type 0x%x, len %d, '%s' - ", field_type, field_len, body);
                switch (current_standard)
                {
                case ADSI_STANDARD_CLASS:
                    switch (message_type)
                    {
                    case CLASS_SDMF_CALLERID:
                        break;
                    case CLASS_MDMF_CALLERID:
                        switch (field_type)
                        {
                        case MCLASS_DATETIME:
                            printf("Date and time (MMDDHHMM)");
                            break;
                        case MCLASS_CALLER_NUMBER:
                            printf("Caller's number");
                            break;
                        case MCLASS_DIALED_NUMBER:
                            printf("Dialed number");
                            break;
                        case MCLASS_ABSENCE1:
                            printf("Caller's number absent: 'O' or 'P'");
                            break;
                        case MCLASS_REDIRECT:
                            printf("Call forward: universal ('0'), on busy ('1'), or on unanswered ('2')");
                            break;
                        case MCLASS_QUALIFIER:
                            printf("Long distance: 'L'");
                            break;
                        case MCLASS_CALLER_NAME:
                            printf("Caller's name");
                            break;
                        case MCLASS_ABSENCE2:
                            printf("Caller's name absent: 'O' or 'P'");
                            break;
                        }
                        break;
                    case CLASS_SDMF_MSG_WAITING:
                        break;
                    case CLASS_MDMF_MSG_WAITING:
                        switch (field_type)
                        {
                        case MCLASS_VISUAL_INDICATOR:
                            printf("Message waiting/not waiting");
                            break;
                        }
                        break;
                    }
                    break;
                case ADSI_STANDARD_CLIP:
                    switch (message_type)
                    {
                    case CLIP_MDMF_CALLERID:
                    case CLIP_MDMF_MSG_WAITING:
                    case CLIP_MDMF_CHARGE_INFO:
                    case CLIP_MDMF_SMS:
                        switch (field_type)
                        {
                        case CLIP_DATETIME:
                            printf("Date and time (MMDDHHMM)");
                            break;
                        case CLIP_CALLER_NUMBER:
                            printf("Caller's number");
                            break;
                        case CLIP_DIALED_NUMBER:
                            printf("Dialed number");
                            break;
                        case CLIP_ABSENCE1:
                            printf("Caller's number absent");
                            break;
                        case CLIP_CALLER_NAME:
                            printf("Caller's name");
                            break;
                        case CLIP_ABSENCE2:
                            printf("Caller's name absent");
                            break;
                        case CLIP_VISUAL_INDICATOR:
                            printf("Visual indicator");
                            break;
                        case CLIP_MESSAGE_ID:
                            printf("Message ID");
                            break;
                        case CLIP_CALLTYPE:
                            printf("Voice call, ring-back-when-free call, or msg waiting call");
                            break;
                        case CLIP_NUM_MSG:
                            printf("Number of messages");
                            break;
#if 0
                        case CLIP_REDIR_NUMBER:
                            printf("Redirecting number");
                            break;
#endif
                        case CLIP_CHARGE:
                            printf("Charge");
                            break;
                        case CLIP_DURATION:
                            printf("Duration of the call");
                            break;
                        case CLIP_ADD_CHARGE:
                            printf("Additional charge");
                            break;
                        case CLIP_DISPLAY_INFO:
                            printf("Display information");
                            break;
                        case CLIP_SERVICE_INFO:
                            printf("Service information");
                            break;
                        }
                        break;
                    }
                    break;
                case ADSI_STANDARD_ACLIP:
                    switch (message_type)
                    {
                    case ACLIP_SDMF_CALLERID:
                        break;
                    case ACLIP_MDMF_CALLERID:
                        switch (field_type)
                        {
                        case ACLIP_DATETIME:
                            printf("Date and time (MMDDHHMM)");
                            break;
                        case ACLIP_CALLER_NUMBER:
                            printf("Caller's number");
                            break;
                        case ACLIP_DIALED_NUMBER:
                            printf("Dialed number");
                            break;
                        case ACLIP_NUMBER_ABSENCE:
                            printf("Caller's number absent: 'O' or 'P'");
                            break;
                        case ACLIP_REDIRECT:
                            printf("Call forward: universal, on busy, or on unanswered");
                            break;
                        case ACLIP_QUALIFIER:
                            printf("Long distance call: 'L'");
                            break;
                        case ACLIP_CALLER_NAME:
                            printf("Caller's name");
                            break;
                        case ACLIP_NAME_ABSENCE:
                            printf("Caller's name absent: 'O' or 'P'");
                            break;
                        }
                        break;
                    }
                    break;
                case ADSI_STANDARD_JCLIP:
                    switch (message_type)
                    {
                    case JCLIP_MDMF_CALLERID:
                        switch (field_type)
                        {
                        case JCLIP_CALLER_NUMBER:
                            printf("Caller's number");
                            break;
                        case JCLIP_CALLER_NUM_DES:
                            printf("Caller's number data extension signal");
                            break;
                        case JCLIP_DIALED_NUMBER:
                            printf("Dialed number");
                            break;
                        case JCLIP_DIALED_NUM_DES:
                            printf("Dialed number data extension signal");
                            break;
                        case JCLIP_ABSENCE:
                            printf("Caller's number absent: 'C', 'O', 'P' or 'S'");
                            break;
                        }
                        break;
                    }
                    break;
                case ADSI_STANDARD_CLIP_DTMF:
                    switch (message_type)
                    {
                    case CLIP_DTMF_HASH_TERMINATED:
                        switch (field_type)
                        {
                        case CLIP_DTMF_HASH_CALLER_NUMBER:
                            printf("Caller's number");
                            break;
                        case CLIP_DTMF_HASH_ABSENCE:
                            printf("Caller's number absent: private (1), overseas (2) or not available (3)");
                            break;
                        case CLIP_DTMF_HASH_UNSPECIFIED:
                            printf("Unspecified");
                            break;
                        }
                        break;
                    case CLIP_DTMF_C_TERMINATED:
                        switch (field_type)
                        {
                        case CLIP_DTMF_C_CALLER_NUMBER:
                            printf("Caller's number");
                            break;
                        case CLIP_DTMF_C_REDIRECT_NUMBER:
                            printf("Redirect number");
                            break;
                        case CLIP_DTMF_C_ABSENCE:
                            printf("Caller's number absent");
                            break;
                        }
                        break;
                    }
                    break;
                case ADSI_STANDARD_TDD:
                    if (len != 59
                        ||
                        memcmp(msg, "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG 0123456789#$*()", 59))
                    {
                        printf("\n");
                        printf("String error\n");
                        exit(2);
                    }
                    break;
                }
            }
            else
            {
                printf("Message type 0x%x - ", field_type);
                message_type = field_type;
                switch (current_standard)
                {
                case ADSI_STANDARD_CLASS:
                    switch (message_type)
                    {
                    case CLASS_SDMF_CALLERID:
                        printf("Single data message caller ID");
                        break;
                    case CLASS_MDMF_CALLERID:
                        printf("Multiple data message caller ID");
                        break;
                    case CLASS_SDMF_MSG_WAITING:
                        printf("Single data message message waiting");
                        break;
                    case CLASS_MDMF_MSG_WAITING:
                        printf("Multiple data message message waiting");
                        break;
                    default:
                        printf("Unknown");
                        break;
                    }
                    break;
                case ADSI_STANDARD_CLIP:
                    switch (message_type)
                    {
                    case CLIP_MDMF_CALLERID:
                        printf("Multiple data message caller ID");
                        break;
                    case CLIP_MDMF_MSG_WAITING:
                        printf("Multiple data message message waiting");
                        break;
                    case CLIP_MDMF_CHARGE_INFO:
                        printf("Multiple data message charge info");
                        break;
                    case CLIP_MDMF_SMS:
                        printf("Multiple data message SMS");
                        break;
                    default:
                        printf("Unknown");
                        break;
                    }
                    break;
                case ADSI_STANDARD_ACLIP:
                    switch (message_type)
                    {
                    case ACLIP_SDMF_CALLERID:
                        printf("Single data message caller ID frame");
                        break;
                    case ACLIP_MDMF_CALLERID:
                        printf("Multiple data message caller ID frame");
                        break;
                    default:
                        printf("Unknown");
                        break;
                    }
                    break;
                case ADSI_STANDARD_JCLIP:
                    switch (message_type)
                    {
                    case JCLIP_MDMF_CALLERID:
                        printf("Multiple data message caller ID frame");
                        break;
                    default:
                        printf("Unknown");
                        break;
                    }
                    break;
                case ADSI_STANDARD_CLIP_DTMF:
                    switch (message_type)
                    {
                    case CLIP_DTMF_HASH_TERMINATED:
                        printf("# terminated");
                        break;
                    case CLIP_DTMF_C_TERMINATED:
                        printf("C terminated");
                        break;
                    default:
                        printf("Unknown");
                        break;
                    }
                    break;
                case ADSI_STANDARD_TDD:
                    printf("Unknown");
                    break;
                }
            }
            printf("\n");
        }
    }
    while (l > 0);
    if (l < -1)
    {
        /* This message appears corrupt */
        printf("Bad message contents\n");
        exit(2);
    }
    printf("\n");
}
/*- End of function --------------------------------------------------------*/

int main(int argc, char *argv[])
{
    int16_t amp[BLOCK_LEN];
    uint8_t adsi_msg[256 + 42];
    int adsi_msg_len;
    AFfilehandle inhandle;
    AFfilehandle outhandle;
    int outframes;
    int len;
    int i;
    int push;
    int log_audio;
    int short_preamble;
    int test_standard;
    int first_standard;
    int last_standard;
    int opt;

    log_audio = FALSE;
    decode_test_file = NULL;
    test_standard = -1;
    short_preamble = FALSE;
    while ((opt = getopt(argc, argv, "d:lps:")) != -1)
    {
        switch (opt)
        {
        case 'd':
            decode_test_file = optarg;
            break;
        case 'l':
            log_audio = TRUE;
            break;
        case 'p':
            short_preamble = TRUE;
            break;
        case 's':
            if (strcasecmp("CLASS", optarg) == 0)
                test_standard = ADSI_STANDARD_CLASS;
            else if (strcasecmp("CLIP", optarg) == 0)
                test_standard = ADSI_STANDARD_CLIP;
            else if (strcasecmp("A-CLIP", optarg) == 0)
                test_standard = ADSI_STANDARD_ACLIP;
            else if (strcasecmp("J-CLIP", optarg) == 0)
                test_standard = ADSI_STANDARD_JCLIP;
            else if (strcasecmp("CLIP-DTMF", optarg) == 0)
                test_standard = ADSI_STANDARD_CLIP_DTMF;
            else if (strcasecmp("TDD", optarg) == 0)
                test_standard = ADSI_STANDARD_TDD;
            else
                test_standard = atoi(optarg);
            break;
        default:
            //usage();
            exit(2);
            break;
        }
    }
    outhandle = AF_NULL_FILEHANDLE;
    
#if 0
    /* This part tests internal static routines in the ADSI module. It can
       only be run with a modified version of the ADSI module, which makes
       the routines visible. */
    /* Check the character encode/decode cycle */
    current_standard = ADSI_STANDARD_TDD;
    tx_adsi = adsi_tx_init(NULL, ADSI_STANDARD_TDD);
    rx_adsi = adsi_rx_init(NULL, ADSI_STANDARD_TDD, put_adsi_msg, NULL);
    s = "The quick Brown Fox Jumps Over The Lazy dog 0123456789!@#$%^&*()";
    while ((ch = *s++))
    {
        xx = adsi_encode_baudot(tx_adsi, ch);
        if ((xx & 0x3E0))
        {
            yy = adsi_decode_baudot(rx_adsi, (xx >> 5) & 0x1F);
            if (yy)
                printf("%c", yy);
        }
        yy = adsi_decode_baudot(rx_adsi, xx & 0x1F);
        if (yy)
            printf("%c", yy);
    }
    adsi_tx_free(tx_adsi);
    adsi_rx_free(rx_adsi);
    printf("\n");
#endif

    if (decode_test_file)
    {
        /* We will decode the audio from a wave file. */
        if ((inhandle = afOpenFile_telephony_read(decode_test_file, 1)) == AF_NULL_FILEHANDLE)
        {
            fprintf(stderr, "    Cannot open wave file '%s'\n", decode_test_file);
            exit(2);
        }
        if (test_standard < 0)
            current_standard = ADSI_STANDARD_CLASS;
        else
            current_standard = test_standard;

        rx_adsi = adsi_rx_init(NULL, current_standard, put_adsi_msg, NULL);
#if 0
        span_log_set_level(rx_adsi.logging, SPAN_LOG_SHOW_SEVERITY | SPAN_LOG_SHOW_PROTOCOL | SPAN_LOG_FLOW);
        span_log_set_tag(rx_adsi.logging, "ADSI");
#endif
        for (;;)
        {
            len = afReadFrames(inhandle,
                               AF_DEFAULT_TRACK,
                               amp,
                               BLOCK_LEN);
            if (len == 0)
                break;
            adsi_rx(rx_adsi, amp, len);
        }
        if (afCloseFile(inhandle) != 0)
        {
            fprintf(stderr, "    Cannot close wave file '%s'\n", decode_test_file);
            exit(2);
        }
        adsi_rx_free(rx_adsi);
    }
    else
    {
        if (log_audio)
        {
            if ((outhandle = afOpenFile_telephony_write(OUT_FILE_NAME, 1)) == AF_NULL_FILEHANDLE)
            {
                fprintf(stderr, "    Cannot create wave file '%s'\n", OUT_FILE_NAME);
                exit(2);
            }
        }
        /* Go through all the standards */
        /* This assumes standard 0 is NULL, and TDD is the last in the list */
        if (test_standard < 0)
        {
            first_standard = ADSI_STANDARD_CLASS;
            last_standard = ADSI_STANDARD_TDD;
        }
        else
        {
            first_standard =
            last_standard = test_standard;
        }
        for (current_standard = first_standard;  current_standard <= last_standard;  current_standard++)
        {
            printf("Testing %s\n", adsi_standard_to_str(current_standard));
            tx_adsi = adsi_tx_init(NULL, current_standard);
            if (short_preamble)
                adsi_tx_set_preamble(tx_adsi, 50, 20, 5, -1);
            rx_adsi = adsi_rx_init(NULL, current_standard, put_adsi_msg, NULL);

            /* Fake an OK condition for the first message test */
            good_message_received = TRUE;
            push = 0;
            for (i = 0;  i < 100000;  i++)
            {
                if (push == 0)
                {
                    if ((len = adsi_tx(tx_adsi, amp, BLOCK_LEN)) == 0)
                        push = 10;
                }
                else
                {
                    len = 0;
                    /* Push a little silence through, to flush things out */
                    if (--push == 0)
                    {
                        if (!good_message_received)
                        {
                            printf("No message received %s (%d)\n", adsi_standard_to_str(current_standard), i);
                            exit(2);
                        }
                        good_message_received = FALSE;
                        adsi_msg_len = adsi_create_message(tx_adsi, adsi_msg);
                        adsi_msg_len = adsi_tx_put_message(tx_adsi, adsi_msg, adsi_msg_len);
                    }
                }
                if (len < BLOCK_LEN)
                {
                    memset(&amp[len], 0, sizeof(int16_t)*(BLOCK_LEN - len));
                    len = BLOCK_LEN;
                }
                if (log_audio)
                {
                    outframes = afWriteFrames(outhandle,
                                              AF_DEFAULT_TRACK,
                                              amp,
                                              len);
                    if (outframes != len)
                    {
                        fprintf(stderr, "    Error writing wave file\n");
                        exit(2);
                    }
                }
                adsi_rx(rx_adsi, amp, len);
            }
            adsi_rx_free(rx_adsi);
            adsi_tx_free(tx_adsi);
        }
        if (log_audio)
        {
            if (afCloseFile(outhandle) != 0)
            {
                fprintf(stderr, "    Cannot close wave file '%s'\n", OUT_FILE_NAME);
                exit(2);
            }
        }
    }
    
    printf("Tests passed.\n");
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
