/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
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
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: mp4ff.c,v 1.6 2003/12/04 21:29:52 menno Exp $
**/

#include <stdlib.h>
#include <string.h>
#include "mp4ffint.h"

mp4ff_t *mp4ff_open_read(mp4ff_callback_t *f)
{
    mp4ff_t *ff = malloc(sizeof(mp4ff_t));

    memset(ff, 0, sizeof(mp4ff_t));

    ff->stream = f;

    parse_atoms(ff);

    return ff;
}

#ifdef USE_TAGGING
mp4ff_t *mp4ff_open_edit(mp4ff_callback_t *f)
{
    mp4ff_t *ff = malloc(sizeof(mp4ff_t));

    memset(ff, 0, sizeof(mp4ff_t));

    ff->stream = f;

    parse_atoms(ff);

    /* copy moov atom to end of the file */
    if (ff->last_atom != ATOM_MOOV)
    {
        char *free_data = "free";
        char *moov_data;

        moov_data = (unsigned char*)malloc(ff->moov_size);

        /* read the moov atom */
        mp4ff_set_position(ff, ff->moov_offset);
        mp4ff_read_data(ff, moov_data, ff->moov_size);

        /* rename old moov to free */
        mp4ff_set_position(ff, ff->moov_offset + 4);
        mp4ff_write_data(ff, free_data, 4);

        /* write old moov at end of file */
        mp4ff_set_position(ff, ff->file_size);
        mp4ff_write_data(ff, moov_data, ff->moov_size);

        free(moov_data);
    }

    return ff;
}
#endif

void mp4ff_close(mp4ff_t *ff)
{
    int32_t i;

    for (i = 0; i < ff->total_tracks; i++)
    {
        if (ff->track[i])
        {
            if (ff->track[i]->stsz_table)
                free(ff->track[i]->stsz_table);
            if (ff->track[i]->stts_sample_count)
                free(ff->track[i]->stts_sample_count);
            if (ff->track[i]->stts_sample_delta)
                free(ff->track[i]->stts_sample_delta);
            if (ff->track[i]->stsc_first_chunk)
                free(ff->track[i]->stsc_first_chunk);
            if (ff->track[i]->stsc_samples_per_chunk)
                free(ff->track[i]->stsc_samples_per_chunk);
            if (ff->track[i]->stsc_sample_desc_index)
                free(ff->track[i]->stsc_sample_desc_index);
            if (ff->track[i]->stco_chunk_offset)
                free(ff->track[i]->stco_chunk_offset);
            if (ff->track[i]->decoderConfig)
                free(ff->track[i]->decoderConfig);
            free(ff->track[i]);
        }
    }

#ifdef USE_TAGGING
    mp4ff_tag_delete(&(ff->tags));
#endif

    if (ff) free(ff);
}

static void mp4ff_track_add(mp4ff_t *f)
{
    f->total_tracks++;

    f->track[f->total_tracks - 1] = malloc(sizeof(mp4ff_track_t));

    memset(f->track[f->total_tracks - 1], 0, sizeof(mp4ff_track_t));
}

/* parse atoms that are sub atoms of other atoms */
static int32_t parse_sub_atoms(mp4ff_t *f, const uint64_t total_size)
{
    uint64_t size;
    uint8_t atom_type = 0;
    uint64_t counted_size = 0;
    uint8_t header_size = 0;

    while (counted_size < total_size)
    {
        size = mp4ff_atom_read_header(f, &atom_type, &header_size);
        counted_size += size;

        /* check for end of file */
        if (size == 0)
            break;

        /* we're starting to read a new track, update index,
         * so that all data and tables get written in the right place
         */
        if (atom_type == ATOM_TRAK)
        {
            mp4ff_track_add(f);
        }

        /* parse subatoms */
        if (atom_type < SUBATOMIC)
        {
            parse_sub_atoms(f, size-header_size);
        } else {
            mp4ff_atom_read(f, size, atom_type);
        }
    }

    return 0;
}

/* parse root atoms */
static int32_t parse_atoms(mp4ff_t *f)
{
    uint64_t size;
    uint8_t atom_type = 0;
    uint8_t header_size = 0;

    f->file_size = 0;

    while ((size = mp4ff_atom_read_header(f, &atom_type, &header_size)) != 0)
    {
        f->file_size += size;
        f->last_atom = atom_type;

        if (atom_type == ATOM_MDAT && f->moov_read)
        {
            /* moov atom is before mdat, we can stop reading when mdat is encountered */
            /* file position will stay at beginning of mdat data */
//            break;
        }

        if (atom_type == ATOM_MOOV && size > header_size)
        {
            f->moov_read = 1;
            f->moov_offset = mp4ff_position(f)-header_size;
            f->moov_size = size;
        }

        /* parse subatoms */
        if (atom_type < SUBATOMIC)
        {
            parse_sub_atoms(f, size-header_size);
        } else {
            /* skip this atom */
            mp4ff_set_position(f, mp4ff_position(f)+size-header_size);
        }
    }

    return 0;
}


int32_t mp4ff_get_sample_duration(const mp4ff_t *f, const int32_t track, const int32_t sample)
{
    int32_t i, ci = 0, co = 0;

    for (i = 0; i < f->track[track]->stts_entry_count; i++)
    {
        int32_t j;
        for (j = 0; j < f->track[track]->stts_sample_count[i]; j++)
        {
            if (co == sample)
                return f->track[track]->stts_sample_delta[ci];
            co++;
        }
        ci++;
    }

    return 0;
}

int32_t mp4ff_read_sample(mp4ff_t *f, const int32_t track, const int32_t sample,
                          uint8_t **audio_buffer,  uint32_t *bytes)
{
    int32_t result = 0;

    *bytes = mp4ff_audio_frame_size(f, track, sample);

    *audio_buffer = (uint8_t*)malloc(*bytes);

    mp4ff_set_sample_position(f, track, sample);

    result = mp4ff_read_data(f, *audio_buffer, *bytes);

    if (!result)
        return 0;

    return *bytes;
}

int32_t mp4ff_get_decoder_config(const mp4ff_t *f, const int32_t track,
                                 uint8_t** ppBuf, uint32_t* pBufSize)
{
    if (track >= f->total_tracks)
    {
        *ppBuf = NULL;
        *pBufSize = 0;
        return 1;
    }

    if (f->track[track]->decoderConfig == NULL || f->track[track]->decoderConfigLen == 0)
    {
        *ppBuf = NULL;
        *pBufSize = 0;
    } else {
        *ppBuf = malloc(f->track[track]->decoderConfigLen);
        if (*ppBuf == NULL)
        {
            *pBufSize = 0;
            return 1;
        }
        memcpy(*ppBuf, f->track[track]->decoderConfig, f->track[track]->decoderConfigLen);
        *pBufSize = f->track[track]->decoderConfigLen;
    }

    return 0;
}

int32_t mp4ff_total_tracks(const mp4ff_t *f)
{
    return f->total_tracks;
}

int32_t mp4ff_time_scale(const mp4ff_t *f, const int32_t track)
{
    return f->track[track]->sampleRate;
}

int32_t mp4ff_num_samples(const mp4ff_t *f, const int32_t track)
{
    int32_t i;
    int32_t total = 0;

    for (i = 0; i < f->track[track]->stts_entry_count; i++)
    {
        total += f->track[track]->stts_sample_count[i];
    }
    return total;
}
