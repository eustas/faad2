/*
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: in_mp4.c,v 1.14 2002/08/14 17:55:20 menno Exp $
**/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <faad.h>
#include <mp4.h>

#include "resource.h"
#include "in2.h"
#include "utils.h"
#include "config.h"
#include "aacinfo.h"

static long priority_table[] = {
    0,
    THREAD_PRIORITY_HIGHEST,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_LOWEST
};
static int res_id_table[] = {
    IDC_16BITS,
    IDC_24BITS,
    IDC_32BITS,
    0,
    IDC_16BITS_DITHERED
};
static int res_table[] = {
    16,
    24,
    32,
    0,
    16
};
static char info_fn[_MAX_PATH];

// post this to the main window at end of file (after playback has stopped)
#define WM_WA_AAC_EOF WM_USER+2

typedef struct state
{
    /* general stuff */
    faacDecHandle hDecoder;
    int samplerate;
    unsigned char channels;
    int decode_pos_ms; // current decoding position, in milliseconds
    int paused; // are we paused?
    int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.
    char filename[_MAX_PATH];
    int filetype; /* 0: MP4; 1: AAC */
    int last_frame;

    /* MP4 stuff */
    MP4FileHandle mp4file;
    int mp4track;
    MP4SampleId numSamples;
    MP4SampleId sampleId;

    /* AAC stuff */
    FILE *aacfile;
    long filesize;
    long bytes_read;
    long bytes_into_buffer;
    long bytes_consumed;
    unsigned char *buffer;
    long seconds;
    unsigned long *seek_table;
    int seek_table_len;
    faadAACInfo aacInfo;
} state;

static state mp4state;

static In_Module module; // the output module (declared near the bottom of this file)

static int killPlayThread;
static int PlayThreadAlive = 0; // 1=play thread still running
HANDLE play_thread_handle = INVALID_HANDLE_VALUE; // the handle to the decode thread

/* Function definitions */
DWORD WINAPI MP4PlayThread(void *b); // the decode thread procedure
DWORD WINAPI AACPlayThread(void *b); // the decode thread procedure

static void show_error(HWND hwnd, char *message, ...)
{
    if (m_show_errors)
        MessageBox(hwnd, message, "Error", MB_OK);
}

static void config_init()
{
	char *p=INI_FILE;
	GetModuleFileName(NULL,INI_FILE,_MAX_PATH);
	while (*p) p++;
	while (p >= INI_FILE && *p != '.') p--;
	strcpy(p+1,"ini");
}

void config_read()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];

	config_init();

    strcpy(show_errors, "1");
    strcpy(priority, "3");
    strcpy(resolution, "0");

    RS(priority);
	RS(resolution);
	RS(show_errors);

    m_priority = atoi(priority);
    m_resolution = atoi(resolution);
    m_show_errors = atoi(show_errors);
}

void config_write()
{
    char priority[10];
    char resolution[10];
    char show_errors[10];

    itoa(m_priority, priority, 10);
    itoa(m_resolution, resolution, 10);
    itoa(m_show_errors, show_errors, 10);

    WS(priority);
	WS(resolution);
	WS(show_errors);
}

void init()
{
    config_read();
}

void quit()
{
}

BOOL CALLBACK mp4_info_dialog_proc(HWND hwndDlg, UINT message,
                                   WPARAM wParam, LPARAM lParam)
{
    MP4FileHandle file;
    int tracks, i;

    switch (message) {
    case WM_INITDIALOG:
        file = MP4Read(info_fn, 0);

        if (!file)
            return FALSE;

        tracks = MP4GetNumberOfTracks(file, NULL, 0);

        if (tracks == 0)
        {
            SetDlgItemText(hwndDlg, IDC_INFOTEXT, "No tracks found");
        } else {
            char *file_info;
            char *info_text = malloc(1024*sizeof(char));
            info_text[0] = '\0';

            for (i = 0; i < tracks; i++)
            {
                file_info = MP4Info(file, i+1);
                lstrcat(info_text, file_info);
            }
            SetDlgItemText(hwndDlg, IDC_INFOTEXT, info_text);
            free(info_text);
        }

        MP4Close(file);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

/* returns the name of the object type */
char *get_ot_string(int ot)
{
    switch (ot)
    {
    case 0:
        return "Main";
    case 1:
        return "LC";
    case 2:
        return "SSR";
    case 3:
        return "LTP";
    }
    return NULL;
}

BOOL CALLBACK aac_info_dialog_proc(HWND hwndDlg, UINT message,
                                   WPARAM wParam, LPARAM lParam)
{
    faadAACInfo aacInfo;
    char *info_text;

    switch (message) {
    case WM_INITDIALOG:
        info_text = malloc(1024*sizeof(char));

        get_AAC_format(info_fn, &aacInfo, NULL, NULL, 0);

        sprintf(info_text, "%s AAC %s, %d sec, %d kbps, %d Hz",
            (aacInfo.version==2)?"MPEG-2":"MPEG-4", get_ot_string(aacInfo.object_type),
            aacInfo.length/1000, (int)(aacInfo.bitrate/1000.0+0.5), aacInfo.sampling_rate);

        SetDlgItemText(hwndDlg, IDC_INFOTEXT, info_text);

        free(info_text);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDOK:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

int infoDlg(char *fn, HWND hwndParent)
{
    if(StringComp(fn + strlen(fn) - 3, "aac", 3) == 0)
    {
        lstrcpy(info_fn, fn);

        DialogBox(module.hDllInstance, MAKEINTRESOURCE(IDD_INFO),
            hwndParent, aac_info_dialog_proc);
    } else {
        lstrcpy(info_fn, fn);

        DialogBox(module.hDllInstance, MAKEINTRESOURCE(IDD_INFO),
            hwndParent, mp4_info_dialog_proc);
    }

    return 0;
}

BOOL CALLBACK config_dialog_proc(HWND hwndDlg, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    int i;

    switch (message) {
    case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_SETRANGE, TRUE, MAKELONG(1,5)); 
		SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_SETPOS, TRUE, m_priority);
        SendMessage(GetDlgItem(hwndDlg, res_id_table[m_resolution]), BM_SETCHECK, BST_CHECKED, 0);
        if (m_show_errors)
            SendMessage(GetDlgItem(hwndDlg, IDC_ERROR), BM_SETCHECK, BST_CHECKED, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        case IDOK:
            m_show_errors = SendMessage(GetDlgItem(hwndDlg, IDC_ERROR), BM_GETCHECK, 0, 0);
            m_priority = SendMessage(GetDlgItem(hwndDlg, IDC_PRIORITY), TBM_GETPOS, 0, 0);
            for (i = 0; i < 3; i++)
            {
                int set = SendMessage(GetDlgItem(hwndDlg, res_id_table[i]), BM_GETCHECK, 0, 0);
                if (set)
                {
                    m_resolution = i;
                    break;
                }
            }

            /* save config */
            config_write();
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

void config(HWND hwndParent)
{
    DialogBox(module.hDllInstance, MAKEINTRESOURCE(IDD_CONFIG),
        hwndParent, config_dialog_proc);

    return;
}

void about(HWND hwndParent)
{
    MessageBox(hwndParent,
        "AudioCoding.com MPEG-4 General Audio player.\n"
        "Visit the website for more info.\n"
        "Copyright 2002 AudioCoding.com",
        "About",
        MB_OK);
}

int isourfile(char *fn)
{
    if(StringComp(fn + strlen(fn) - 3, "mp4", 3) == 0)
    {
        return 1;
    }
    if(StringComp(fn + strlen(fn) - 3, "aac", 3) == 0)
    {
        return 1;
    }

    return 0;
}

int play(char *fn)
{
    int maxlatency;
    int thread_id;
    int avg_bitrate;
    unsigned char *buffer;
    int buffer_size;
    faacDecConfigurationPtr config;

    mp4state.channels = 0;
    mp4state.samplerate = 0;
    mp4state.filetype = 0;
    mp4state.seek_table_len = 0;

    if (mp4state.seek_table)
    {
        free(mp4state.seek_table);
        mp4state.seek_table = NULL;
    }

    strcpy(mp4state.filename, fn);

    if(StringComp(fn + strlen(fn) - 3, "aac", 3) == 0)
        mp4state.filetype = 1;

    mp4state.hDecoder = faacDecOpen();
    if (!mp4state.hDecoder)
    {
        show_error(module.hMainWindow, "Unable to open decoder library.");
        return -1;
    }

    if (mp4state.filetype)
    {
        long pos, tmp, read;

        get_AAC_format(mp4state.filename, &mp4state.aacInfo,
            &mp4state.seek_table, &mp4state.seek_table_len, 1);

        mp4state.aacfile = fopen(mp4state.filename, "rb");
        if (!mp4state.aacfile)
        {
            show_error(module.hMainWindow, "Unable to open file.");
            faacDecClose(mp4state.hDecoder);
            return -1;
        }

        pos = ftell(mp4state.aacfile);
        fseek(mp4state.aacfile, 0, SEEK_END);
        mp4state.filesize = ftell(mp4state.aacfile);
        fseek(mp4state.aacfile, pos, SEEK_SET);

        if (!(mp4state.buffer=(unsigned char*)malloc(768*48)))
        {
            show_error(module.hMainWindow, "Memory allocation error.");
            faacDecClose(mp4state.hDecoder);
            fclose(mp4state.aacfile);
            return -1;
        }
        memset(mp4state.buffer, 0, 768*48);

        if (mp4state.filesize < 768*48)
            tmp = mp4state.filesize;
        else
            tmp = 768*48;
        read = fread(mp4state.buffer, 1, tmp, mp4state.aacfile);
        if (read == tmp)
        {
            mp4state.bytes_read = read;
            mp4state.bytes_into_buffer = read;
        } else {
            show_error(module.hMainWindow, "Error reading from file.");
            faacDecClose(mp4state.hDecoder);
            fclose(mp4state.aacfile);
            return -1;
        }

        if ((mp4state.bytes_consumed=faacDecInit(mp4state.hDecoder,
            mp4state.buffer, &mp4state.samplerate, &mp4state.channels)) < 0)
        {
            show_error(module.hMainWindow, "Can't initialize library.");
            faacDecClose(mp4state.hDecoder);
            fclose(mp4state.aacfile);
            return -1;
        }
        mp4state.bytes_into_buffer -= mp4state.bytes_consumed;

        avg_bitrate = mp4state.aacInfo.bitrate;

        if (mp4state.aacInfo.headertype == 2)
            module.is_seekable = 1;
        else
            module.is_seekable = 0;
    } else {
        mp4state.mp4file = MP4Read(mp4state.filename, 0);
        if (!mp4state.mp4file)
        {
            show_error(module.hMainWindow, "Unable to open file.");
            faacDecClose(mp4state.hDecoder);
            return -1;
        }

        if ((mp4state.mp4track = GetAACTrack(mp4state.mp4file)) < 0)
        {
            show_error(module.hMainWindow, "Unsupported Audio track type.");
            faacDecClose(mp4state.hDecoder);
            MP4Close(mp4state.mp4file);
            return -1;
        }

        buffer = NULL;
        buffer_size = 0;
        MP4GetTrackESConfiguration(mp4state.mp4file, mp4state.mp4track,
            &buffer, &buffer_size);
        if (!buffer)
        {
            faacDecClose(mp4state.hDecoder);
            MP4Close(mp4state.mp4file);
            return -1;
        }

        if(faacDecInit2(mp4state.hDecoder, buffer, buffer_size,
            &mp4state.samplerate, &mp4state.channels) < 0)
        {
            /* If some error initializing occured, skip the file */
            faacDecClose(mp4state.hDecoder);
            MP4Close(mp4state.mp4file);
            return -1;
        }
        free(buffer);

        avg_bitrate = MP4GetTrackIntegerProperty(mp4state.mp4file, mp4state.mp4track,
            "mdia.minf.stbl.stsd.mp4a.esds.decConfigDescr.avgBitrate");

        mp4state.numSamples = MP4GetTrackNumberOfSamples(mp4state.mp4file, mp4state.mp4track);
        mp4state.sampleId = 1;

        module.is_seekable = 1;
    }

    config = faacDecGetCurrentConfiguration(mp4state.hDecoder);
    config->outputFormat = m_resolution + 1;
    faacDecSetConfiguration(mp4state.hDecoder, config);

    if (mp4state.channels == 0)
    {
        show_error(module.hMainWindow, "Number of channels not supported for playback.");
        faacDecClose(mp4state.hDecoder);
        if (mp4state.filetype)
            fclose(mp4state.aacfile);
        else
            MP4Close(mp4state.mp4file);
        return -1;
    }

    maxlatency = module.outMod->Open(mp4state.samplerate, mp4state.channels,
        res_table[m_resolution], -1,-1);
    if (maxlatency < 0) // error opening device
    {
        faacDecClose(mp4state.hDecoder);
        if (mp4state.filetype)
            fclose(mp4state.aacfile);
        else
            MP4Close(mp4state.mp4file);
        return -1;
    }

    mp4state.paused        =  0;
    mp4state.decode_pos_ms =  0;
    mp4state.seek_needed   = -1;

    // initialize vis stuff
    module.SAVSAInit(maxlatency, mp4state.samplerate);
    module.VSASetInfo(mp4state.samplerate, mp4state.channels);

    module.SetInfo((avg_bitrate + 500)/1000, mp4state.samplerate/1000, mp4state.channels, 1);

    module.outMod->SetVolume(-666); // set the output plug-ins default volume

    killPlayThread = 0;

    if (mp4state.filetype)
    {
        if((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AACPlayThread,
            (void *)&killPlayThread, 0, &thread_id)) == NULL)
        {
            show_error(module.hMainWindow, "Cannot create playback thread");
            faacDecClose(mp4state.hDecoder);
            fclose(mp4state.aacfile);
            return -1;
        }
    } else {
        if((play_thread_handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MP4PlayThread,
            (void *)&killPlayThread, 0, &thread_id)) == NULL)
        {
            show_error(module.hMainWindow, "Cannot create playback thread");
            faacDecClose(mp4state.hDecoder);
            MP4Close(mp4state.mp4file);
            return -1;
        }
    }

    if (m_priority != 3)
        SetThreadPriority(play_thread_handle, priority_table[m_priority]);

    return 0;
}

void pause()
{
    mp4state.paused = 1;
    module.outMod->Pause(1);
}

void unpause()
{
    mp4state.paused = 0;
    module.outMod->Pause(0);
}

int ispaused()
{
    return mp4state.paused;
}

void setvolume(int volume)
{
    module.outMod->SetVolume(volume);
}

void setpan(int pan)
{
    module.outMod->SetPan(pan);
}

void stop()
{
    killPlayThread = 1;

    if (play_thread_handle != INVALID_HANDLE_VALUE)
    {
        if (WaitForSingleObject(play_thread_handle, INFINITE) == WAIT_TIMEOUT)
            TerminateThread(play_thread_handle,0);
        CloseHandle(play_thread_handle);
        play_thread_handle = INVALID_HANDLE_VALUE;
    }

    faacDecClose(mp4state.hDecoder);
    if (mp4state.filetype)
        fclose(mp4state.aacfile);
    else
        MP4Close(mp4state.mp4file);

    if (mp4state.seek_table)
    {
        free(mp4state.seek_table);
        mp4state.seek_table = NULL;
        mp4state.seek_table_len = 0;
    }

    module.outMod->Close();
    module.SAVSADeInit();
}

int getsonglength(char *fn)
{
    long msDuration = 0;

    if(StringComp(fn + strlen(fn) - 3, "mp4", 3) == 0)
    {
        int track;
        MP4Duration length;
        MP4FileHandle file;

        file = MP4Read(fn, 0);
        if (!file)
            return 0;

        if ((track = GetAACTrack(file)) < 0)
        {
            MP4Close(file);
            return -1;
        }

        length = MP4GetTrackDuration(file, track);

        msDuration = MP4ConvertFromTrackDuration(file, track,
            length, MP4_MSECS_TIME_SCALE);

        MP4Close(file);

        return msDuration;
    } else {
        faadAACInfo aacInfo;
        get_AAC_format(fn, &aacInfo, NULL, NULL, 0);

        return aacInfo.length;
    }
}

int getlength()
{
    if (!mp4state.filetype)
    {
        int track;
        long msDuration;
        MP4Duration length;

        if ((track = GetAACTrack(mp4state.mp4file)) < 0)
        {
            return -1;
        }

        length = MP4GetTrackDuration(mp4state.mp4file, track);

        msDuration = MP4ConvertFromTrackDuration(mp4state.mp4file, track,
            length, MP4_MSECS_TIME_SCALE);

        return msDuration;
    } else {
        return mp4state.aacInfo.length;
    }
    return 0;
}

int getoutputtime()
{
    return mp4state.decode_pos_ms+(module.outMod->GetOutputTime()-module.outMod->GetWrittenTime());
}

void setoutputtime(int time_in_ms)
{
    mp4state.seek_needed = time_in_ms;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
    if (!filename || !*filename)  /* currently playing file */
    {
        if (length_in_ms)
            *length_in_ms = getlength();

        if (title)
        {
            char *tmp = PathFindFileName(mp4state.filename);
            strcpy(title, tmp);
        }
    } else {
        if (length_in_ms)
            *length_in_ms = getsonglength(filename);

        if (title)
        {
            char *tmp = PathFindFileName(filename);
            strcpy(title, tmp);
        }
    }
}

void eq_set(int on, char data[10], int preamp)
{
}

DWORD WINAPI MP4PlayThread(void *b)
{
    int done = 0;
    int l;

    void *sample_buffer;
    unsigned char *buffer;
    int buffer_size;
    faacDecFrameInfo frameInfo;

	PlayThreadAlive = 1;
    mp4state.last_frame = 0;

    while (!*((int *)b))
    {
        /* seeking */
        if (mp4state.seek_needed != -1)
        {
            MP4Duration duration;

            module.outMod->Flush(mp4state.decode_pos_ms);
            duration = MP4ConvertToTrackDuration(mp4state.mp4file,
                mp4state.mp4track, mp4state.seek_needed, MP4_MSECS_TIME_SCALE);
            mp4state.sampleId = MP4GetSampleIdFromTime(mp4state.mp4file,
                mp4state.mp4track, duration, 0);

            mp4state.decode_pos_ms = mp4state.seek_needed;
			mp4state.seek_needed = -1;
        }

        if (done)
        {
            module.outMod->CanWrite();

            if (!module.outMod->IsPlaying())
            {
                PostMessage(module.hMainWindow, WM_WA_AAC_EOF, 0, 0);
                PlayThreadAlive = 0;
                return 0;
            }

            Sleep(10);
        } else if (module.outMod->CanWrite() >=
            ((1024*mp4state.channels*sizeof(short))<<(module.dsp_isactive()?1:0)))
        {
            if (mp4state.last_frame)
            {
                done = 1;
            } else {
                int rc;

                /* get acces unit from MP4 file */
                buffer = NULL;
                buffer_size = 0;

                rc = MP4ReadSample(mp4state.mp4file, mp4state.mp4track,
                    mp4state.sampleId++, &buffer, &buffer_size,
                    NULL, NULL, NULL, NULL);
                if (rc == 0 || buffer == NULL)
                {
                    mp4state.last_frame = 1;
                    sample_buffer = NULL;
                    frameInfo.samples = 0;
                } else {
                    sample_buffer = faacDecDecode(mp4state.hDecoder, &frameInfo, buffer);
                }
                if (frameInfo.error > 0)
                {
                    show_error(module.hMainWindow, faacDecGetErrorMessage(frameInfo.error));
                    mp4state.last_frame = 1;
                }
                if (mp4state.sampleId >= mp4state.numSamples)
                    mp4state.last_frame = 1;

                if (buffer) free(buffer);

                if (!killPlayThread && (frameInfo.samples > 0))
                {
                    if (res_table[m_resolution] == 24)
                    {
                        /* convert libfaad output (3 bytes packed in 4) */
                        char *temp_buffer = convert3in4to3in3(sample_buffer, frameInfo.samples);
                        memcpy((void*)sample_buffer, (void*)temp_buffer, frameInfo.samples*3);
                        free(temp_buffer);
                    }

                    module.SAAddPCMData(sample_buffer, mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    module.VSAAddPCMData(sample_buffer, mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    mp4state.decode_pos_ms += (1024*1000)/mp4state.samplerate;

                    if (module.dsp_isactive())
                    {
                        l = module.dsp_dosamples((short*)sample_buffer,
                            frameInfo.samples*sizeof(short)/mp4state.channels/(res_table[m_resolution]/8),
                            res_table[m_resolution],
                            mp4state.channels,mp4state.samplerate)*(mp4state.channels*(res_table[m_resolution]/8));
                    } else {
                        l = frameInfo.samples*(res_table[m_resolution]/8);
                    }

                    module.outMod->Write(sample_buffer, l);
                }
            }
        } else {
            Sleep(10);
        }
    }

	PlayThreadAlive = 0;
	
    return 0;
}

int aac_seek(int pos_ms)
{
    int read;
    int offset_sec = (int)((float)pos_ms / 1000.0 + 0.5);

    fseek(mp4state.aacfile, mp4state.seek_table[offset_sec], SEEK_SET);

    mp4state.bytes_read = mp4state.seek_table[offset_sec];
    mp4state.bytes_consumed = 0;

    read = fread(mp4state.buffer, 1, 768*48, mp4state.aacfile);
    mp4state.bytes_read += read;
    mp4state.bytes_into_buffer = read;

    return 0;
}

DWORD WINAPI AACPlayThread(void *b)
{
    int done = 0;
    int l;

    void *sample_buffer;
    faacDecFrameInfo frameInfo;

    PlayThreadAlive = 1;
    mp4state.last_frame = 0;

    while (!*((int *)b))
    {
        /* seeking */
        if (mp4state.seek_needed != -1)
        {
            int ms;

            /* Round off to a second */
            ms = mp4state.seek_needed - (mp4state.seek_needed%1000);
            module.outMod->Flush(mp4state.decode_pos_ms);
            aac_seek(ms);
            mp4state.decode_pos_ms = ms;
            mp4state.seek_needed = -1;
        }

        if (done)
        {
            module.outMod->CanWrite();

            if (!module.outMod->IsPlaying())
            {
                PostMessage(module.hMainWindow, WM_WA_AAC_EOF, 0, 0);
                PlayThreadAlive = 0;
                return 0;
            }

            Sleep(10);
        } else if (module.outMod->CanWrite() >=
            ((1024*mp4state.channels*sizeof(short))<<(module.dsp_isactive()?1:0)))
        {
            if (mp4state.last_frame)
            {
                done = 1;
            } else {
                long tmp, read;
                unsigned char *buffer = mp4state.buffer;

                do
                {
                    if (mp4state.bytes_consumed > 0)
                    {
                        if (mp4state.bytes_into_buffer)
                        {
                            memcpy(buffer, buffer+mp4state.bytes_consumed,
                                mp4state.bytes_into_buffer);
                        }

                        if (mp4state.bytes_read < mp4state.filesize)
                        {
                            if (mp4state.bytes_read + mp4state.bytes_consumed < mp4state.filesize)
                                tmp = mp4state.bytes_consumed;
                            else
                                tmp = mp4state.filesize - mp4state.bytes_read;
                            read = fread(buffer + mp4state.bytes_into_buffer, 1, tmp, mp4state.aacfile);
                            if (read == tmp)
                            {
                                mp4state.bytes_read += read;
                                mp4state.bytes_into_buffer += read;
                            }
                        } else {
                            if (mp4state.bytes_into_buffer)
                            {
                                memset(buffer + mp4state.bytes_into_buffer, 0,
                                    mp4state.bytes_consumed);
                            }
                        }

                        mp4state.bytes_consumed = 0;
                    }

                    if (mp4state.bytes_into_buffer < 1)
                    {
                        if (mp4state.bytes_read < mp4state.filesize)
                        {
                            show_error(module.hMainWindow, faacDecGetErrorMessage(frameInfo.error));
                            mp4state.last_frame = 1;
                        } else {
                            mp4state.last_frame = 1;
                        }
                    }

                    sample_buffer = faacDecDecode(mp4state.hDecoder, &frameInfo, buffer);

                    mp4state.bytes_consumed += frameInfo.bytesconsumed;
                    mp4state.bytes_into_buffer -= mp4state.bytes_consumed;
                } while (!frameInfo.samples && !frameInfo.error);

                if (!killPlayThread && (frameInfo.samples > 0))
                {
                    if (res_table[m_resolution] == 24)
                    {
                        /* convert libfaad output (3 bytes packed in 4 bytes) */
                        char *temp_buffer = convert3in4to3in3(sample_buffer, frameInfo.samples);
                        memcpy((void*)sample_buffer, (void*)temp_buffer, frameInfo.samples*3);
                        free(temp_buffer);
                    }

                    module.SAAddPCMData(sample_buffer, mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    module.VSAAddPCMData(sample_buffer, mp4state.channels, res_table[m_resolution],
                        mp4state.decode_pos_ms);
                    mp4state.decode_pos_ms += (1024*1000)/mp4state.samplerate;

                    if (module.dsp_isactive())
                    {
                        l = module.dsp_dosamples((short*)sample_buffer,
                            frameInfo.samples*sizeof(short)/mp4state.channels/(res_table[m_resolution]/8),
                            res_table[m_resolution],
                            mp4state.channels,mp4state.samplerate)*(mp4state.channels*(res_table[m_resolution]/8));
                    } else {
                        l = frameInfo.samples*(res_table[m_resolution]/8);
                    }

                    module.outMod->Write(sample_buffer, l);
                }
            }
        } else {
            Sleep(10);
        }
    }

    if (mp4state.seek_table)
    {
        free(mp4state.seek_table);
        mp4state.seek_table = NULL;
        mp4state.seek_table_len = 0;
    }

	PlayThreadAlive = 0;
	
    return 0;
}

static In_Module module =
{
    IN_VER,
    "AudioCoding.com MPEG-4 General Audio player: " FAAD2_VERSION,
    0,  // hMainWindow
    0,  // hDllInstance
    "MP4\0MPEG-4 Files (*.MP4)\0AAC\0AAC Files (*.AAC)\0"
    ,
    1, // is_seekable
    1, // uses output
    config,
    about,
    init,
    quit,
    getfileinfo,
    infoDlg,
    isourfile,
    play,
    pause,
    unpause,
    ispaused,
    stop,

    getlength,
    getoutputtime,
    setoutputtime,

    setvolume,
    setpan,

    0,0,0,0,0,0,0,0,0, // vis stuff


    0,0, // dsp

    eq_set,

    NULL,       // setinfo

    0 // out_mod
};

__declspec(dllexport) In_Module* winampGetInModule2()
{
    return &module;
}
