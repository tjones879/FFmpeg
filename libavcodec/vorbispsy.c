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

#include <math.h>

#include "avcodec.h"
#include "libavutil/attributes.h"
#include "vorbispsy.h"

/**
 * Generate the coefficients for a highpass biquad filter
 *
 * @param filter Instance of biquad filter to be initialized
 * @param Fs     Input's sampling frequency
 * @param Fc     Critical frequency for samples to be passed
 * @param Q      Quality factor
 */
static av_cold void biquad_filter_init(IIRFilter *filter, int Fs, int Fc, float Q)
{
    float k = tan(M_PI * Fc / Fs);
    float normalize = 1 / (1 + k / Q + k * k);

    filter->b[0] = normalize;
    filter->b[1] = -2 * normalize;
    filter->b[2] = normalize;

    filter->a[0] = 1;
    filter->a[1] = 2 * (k * k - 1) * normalize;
    filter->a[2] = (1 - k / Q + k * k) * normalize;
}

/**
 * Direct Form II implementation for a second order digital filter
 *
 * @param filter Filter to be applied to input samples
 * @param in     Single input sample to be filtered
 * @param delay  Array of IIR feedback values
 * @return       Filtered sample
 */
static float apply_filter(IIRFilter *filter, float in, float *delay)
{
    float ret, w;

    w   = filter->a[0] * in - filter->a[1] * delay[0] - filter->a[2] * delay[1];
    ret = filter->b[0] * w  + filter->b[1] * delay[0] + filter->b[2] * delay[1];

    delay[1] = delay[0];
    delay[0] = w;

    return ret;
}

/**
 * Calculate the variance of a block of samples
 *
 * @param in     Array of input samples
 * @param length Number of input samples being analyzed
 * @return       The variance for the current block
 */
static float variance(const float *in, int length)
{
    int i;
    float mean = 0.0f, square_sum = 0.0f;

    for (i = 0; i < length; i++) {
        mean += in[i];
        square_sum += in[i] * in[i];
    }

    mean /= length;
    return (square_sum - length * mean * mean) / (length - 1);
}

av_cold int ff_psy_vorbis_init(VorbisPsyContext *vpctx, int sample_rate,
                               int channels, int blocks)
{
    int crit_freq;
    float Q[2] = {.54, 1.31}; // Quality values for maximally flat cascaded filters

    vpctx->filter_delay = av_mallocz_array(4 * channels, sizeof(vpctx->filter_delay[0]));
    if (!vpctx->filter_delay)
        return AVERROR(ENOMEM);

    crit_freq = sample_rate / 4;
    biquad_filter_init(&vpctx->filter[0], sample_rate, crit_freq, Q[0]);
    biquad_filter_init(&vpctx->filter[1], sample_rate, crit_freq, Q[1]);

    vpctx->variance = av_mallocz_array(channels * blocks, sizeof(vpctx->variance[0]));
    if (!vpctx->variance)
        return AVERROR(ENOMEM);

    vpctx->preecho_thresh = 100.0f;

    return 0;
}

int ff_psy_vorbis_block_frame(VorbisPsyContext *vpctx, float *audio,
                              int ch, int frame_size, int block_size)
{
    int i, block_flag = 1;
    int blocks = frame_size / block_size;
    float last_var;
    const float eps = 1e-4;
    float *var = vpctx->variance + ch * blocks;

    for (i = 0; i < frame_size; i++) {
        apply_filter(&vpctx->filter[0], audio[i], vpctx->filter_delay + 4 * ch);
        apply_filter(&vpctx->filter[1], audio[i], vpctx->filter_delay + 4 * ch + 2);
    }

    for (i = 0; i < blocks; i++) {
        last_var = var[i];
        var[i] = variance(audio + i * block_size, block_size);

        /* A small constant is added to the threshold in order to prevent false
         * transients from being detected when quiet sounds follow near-silence */
        if (var[i] > vpctx->preecho_thresh * last_var + eps)
            block_flag = 0;
    }

    return block_flag;
}

av_cold void ff_psy_vorbis_close(VorbisPsyContext *vpctx)
{
    if (vpctx) {
        if (vpctx->filter_delay)
            av_freep(&vpctx->filter_delay);

        if (vpctx->variance)
            av_freep(&vpctx->variance);

        av_freep(&vpctx);
    }
}
