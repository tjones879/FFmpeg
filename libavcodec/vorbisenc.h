/*
 * Vorbis encoder
 * Copyright (C) 2017 Tyler Jones
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVCODEC_VORBISENC_H
#define AVCODEC_VORBISENC_H

#define MAX_CHANNELS     2
#define MAX_CODEBOOK_DIM 8

#define MAX_FLOOR_CLASS_DIM  4
#define NUM_FLOOR_PARTITIONS 8
#define MAX_FLOOR_VALUES     (MAX_FLOOR_CLASS_DIM*NUM_FLOOR_PARTITIONS+2)

#define RESIDUE_SIZE           1600
#define RESIDUE_PART_SIZE      32
#define NUM_RESIDUE_PARTITIONS (RESIDUE_SIZE/RESIDUE_PART_SIZE)

#include "vorbis.h"
#include "vorbispsy.h"
#include "audio_frame_queue.h"
#include "libavutil/float_dsp.h"
#include "libavfilter/bufferqueue.h"
#include "fft.h"

typedef struct vorbis_enc_codebook {
    int nentries;
    uint8_t *lens;
    uint32_t *codewords;
    int ndimensions;
    float min;
    float delta;
    int seq_p;
    int lookup;
    int *quantlist;
    float *dimensions;
    float *pow2;
} vorbis_enc_codebook;

typedef struct vorbis_enc_floor_class {
    int dim;
    int subclass;
    int masterbook;
    int *books;
} vorbis_enc_floor_class;

typedef struct vorbis_enc_floor {
    int partitions;
    int *partition_to_class;
    int nclasses;
    vorbis_enc_floor_class *classes;
    int multiplier;
    int rangebits;
    int values;
    vorbis_floor1_entry *list;
} vorbis_enc_floor;

typedef struct vorbis_enc_residue {
    int type;
    int begin;
    int end;
    int partition_size;
    int classifications;
    int classbook;
    int8_t (*books)[8];
    float (*maxes)[2];
} vorbis_enc_residue;

typedef struct vorbis_enc_mapping {
    int submaps;
    int *mux;
    int *floor;
    int *residue;
    int coupling_steps;
    int *magnitude;
    int *angle;
} vorbis_enc_mapping;

typedef struct vorbis_enc_mode {
    int blockflag;
    int mapping;
} vorbis_enc_mode;

typedef struct vorbis_enc_context {
    int channels;
    int sample_rate;
    int log2_blocksize[2];
    int blockflags[3]; ///< Flags used for the previous, current, next windows
    int transient;     ///< Negative if a series of transients are not being encoded
    int num_transient; ///< Number of short blocks for each frame
    FFTContext mdct[2];
    const float *win[2];
    int have_saved;
    float *saved;
    float *samples;
    float *floor;  // also used for tmp values for mdct
    float *coeffs; // also used for residue after floor
    float *scratch; //< Used for temp values for psy model and window application
    float quality;

    AudioFrameQueue afq;
    struct FFBufQueue bufqueue;

    int ncodebooks;
    vorbis_enc_codebook *codebooks;

    int nfloors;
    vorbis_enc_floor *floors;

    int nresidues;
    vorbis_enc_residue *residues;

    int nmappings;
    vorbis_enc_mapping *mappings;

    int nmodes;
    vorbis_enc_mode *modes;

    int64_t next_pts;

    AVFloatDSPContext *fdsp;
    VorbisPsyContext *vpctx;
} vorbis_enc_context;

#endif /* AVCODEC_VORBISENC_H */
