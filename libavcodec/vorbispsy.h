/*
 * Vorbis encoder psychoacoustic model
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

/**
 * @file
 * Vorbis psychoacoustic model
 */

#ifndef AVCODEC_VORBISPSY_H
#define AVCODEC_VORBISPSY_H

#include "libavutil/attributes.h"

/**
 * Second order IIR Filter
 */
typedef struct IIRFilter {
    float b[3]; ///< Normalized cofficients for numerator of transfer function
    float a[3]; ///< Normalized coefficiets for denominator of transfer function
} IIRFilter;

typedef struct VorbisPsyContext {
    IIRFilter filter[2];
    float *filter_delay;  ///< Direct Form II delay registers for each channel
    float *variance;      ///< Saved variances from previous sub-blocks for each channel
    float preecho_thresh; ///< Threshold for determining prescence of a preecho
} VorbisPsyContext;

/**
 * Initializes the psychoacoustic model context
 *
 * @param vpctx       Uninitialized pointer to the model context
 * @param sample_rate Input audio sample rate
 * @param channels    Number of channels being analyzed
 * @param blocks      Number of short blocks for every frame of input
 * @return            0 on success, negative on failure
 */
av_cold int ff_psy_vorbis_init(VorbisPsyContext *vpctx, int sample_rate,
                               int channels, int blocks);

/**
 * Suggest the type of block to use for encoding the current frame
 *
 * Each frame of input is passed through a highpass filter to remove dominant
 * low-frequency waveforms and the variance of each short block of input is
 * then calculated. If the variance over this block is significantly more than
 * blocks from the previous frame, a transient signal is likely present.
 *
 * @param audio      Pointer to the current channel's input samples
 * @param ch         Current channel being analyzed
 * @param frame_size Size of a full frame, i.e. the size of the long block
 * @param block_size Size of the short block
 * @return           The correct blockflag to use for encoding, 0 short and 1 long
 */
int ff_psy_vorbis_block_frame(VorbisPsyContext *vpctx, float *audio,
                              int ch, int frame_size, int block_size);
/**
 * Closes and frees the memory used by the psychoacoustic model
 */
av_cold void ff_psy_vorbis_close(VorbisPsyContext *vpctx);
#endif /* AVCODEC_VORBISPSY_H */
