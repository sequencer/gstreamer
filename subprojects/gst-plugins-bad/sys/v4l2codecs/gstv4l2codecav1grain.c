/* GStreamer
 * Copyright (C) 2026 Jiuyang Liu <liu@jiuyang.me>
 *
 * AV1 film grain synthesis (AV1 specification 7.18.3), for stateless
 * decoders that output the reconstructed picture without the grain.
 *
 * The synthesis follows FFmpeg's libavcodec/aom_film_grain_template.c
 * (LGPL-2.1-or-later, Copyright (c) 2023 Niklas Haas), itself derived from
 * dav1d's C film grain implementation:
 *
 * Copyright © 2018, Niklas Haas
 * Copyright © 2018, VideoLAN and dav1d authors
 * Copyright © 2018, Two Orioles, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "gstv4l2codecav1grain.h"

#define GRAIN_WIDTH 82
#define GRAIN_HEIGHT 73
#define SUB_GRAIN_WIDTH 44
#define SUB_GRAIN_HEIGHT 38
#define FG_BLOCK_SIZE 32
#define SCALING_SIZE 4096

/* Gaussian_Sequence, AV1 specification 7.18.3.3 */
static const gint16 gaussian_sequence[2048] = {
    56,    568,   -180,  172,   124,   -84,   172,   -64,   -900,  24,   820,
    224,   1248,  996,   272,   -8,    -916,  -388,  -732,  -104,  -188, 800,
    112,   -652,  -320,  -376,  140,   -252,  492,   -168,  44,    -788, 588,
    -584,  500,   -228,  12,    680,   272,   -476,  972,   -100,  652,  368,
    432,   -196,  -720,  -192,  1000,  -332,  652,   -136,  -552,  -604, -4,
    192,   -220,  -136,  1000,  -52,   372,   -96,   -624,  124,   -24,  396,
    540,   -12,   -104,  640,   464,   244,   -208,  -84,   368,   -528, -740,
    248,   -968,  -848,  608,   376,   -60,   -292,  -40,   -156,  252,  -292,
    248,   224,   -280,  400,   -244,  244,   -60,   76,    -80,   212,  532,
    340,   128,   -36,   824,   -352,  -60,   -264,  -96,   -612,  416,  -704,
    220,   -204,  640,   -160,  1220,  -408,  900,   336,   20,    -336, -96,
    -792,  304,   48,    -28,   -1232, -1172, -448,  104,   -292,  -520, 244,
    60,    -948,  0,     -708,  268,   108,   356,   -548,  488,   -344, -136,
    488,   -196,  -224,  656,   -236,  -1128, 60,    4,     140,   276,  -676,
    -376,  168,   -108,  464,   8,     564,   64,    240,   308,   -300, -400,
    -456,  -136,  56,    120,   -408,  -116,  436,   504,   -232,  328,  844,
    -164,  -84,   784,   -168,  232,   -224,  348,   -376,  128,   568,  96,
    -1244, -288,  276,   848,   832,   -360,  656,   464,   -384,  -332, -356,
    728,   -388,  160,   -192,  468,   296,   224,   140,   -776,  -100, 280,
    4,     196,   44,    -36,   -648,  932,   16,    1428,  28,    528,  808,
    772,   20,    268,   88,    -332,  -284,  124,   -384,  -448,  208,  -228,
    -1044, -328,  660,   380,   -148,  -300,  588,   240,   540,   28,   136,
    -88,   -436,  256,   296,   -1000, 1400,  0,     -48,   1056,  -136, 264,
    -528,  -1108, 632,   -484,  -592,  -344,  796,   124,   -668,  -768, 388,
    1296,  -232,  -188,  -200,  -288,  -4,    308,   100,   -168,  256,  -500,
    204,   -508,  648,   -136,  372,   -272,  -120,  -1004, -552,  -548, -384,
    548,   -296,  428,   -108,  -8,    -912,  -324,  -224,  -88,   -112, -220,
    -100,  996,   -796,  548,   360,   -216,  180,   428,   -200,  -212, 148,
    96,    148,   284,   216,   -412,  -320,  120,   -300,  -384,  -604, -572,
    -332,  -8,    -180,  -176,  696,   116,   -88,   628,   76,    44,   -516,
    240,   -208,  -40,   100,   -592,  344,   -308,  -452,  -228,  20,   916,
    -1752, -136,  -340,  -804,  140,   40,    512,   340,   248,   184,  -492,
    896,   -156,  932,   -628,  328,   -688,  -448,  -616,  -752,  -100, 560,
    -1020, 180,   -800,  -64,   76,    576,   1068,  396,   660,   552,  -108,
    -28,   320,   -628,  312,   -92,   -92,   -472,  268,   16,    560,  516,
    -672,  -52,   492,   -100,  260,   384,   284,   292,   304,   -148, 88,
    -152,  1012,  1064,  -228,  164,   -376,  -684,  592,   -392,  156,  196,
    -524,  -64,   -884,  160,   -176,  636,   648,   404,   -396,  -436, 864,
    424,   -728,  988,   -604,  904,   -592,  296,   -224,  536,   -176, -920,
    436,   -48,   1176,  -884,  416,   -776,  -824,  -884,  524,   -548, -564,
    -68,   -164,  -96,   692,   364,   -692,  -1012, -68,   260,   -480, 876,
    -1116, 452,   -332,  -352,  892,   -1088, 1220,  -676,  12,    -292, 244,
    496,   372,   -32,   280,   200,   112,   -440,  -96,   24,    -644, -184,
    56,    -432,  224,   -980,  272,   -260,  144,   -436,  420,   356,  364,
    -528,  76,    172,   -744,  -368,  404,   -752,  -416,  684,   -688, 72,
    540,   416,   92,    444,   480,   -72,   -1416, 164,   -1172, -68,  24,
    424,   264,   1040,  128,   -912,  -524,  -356,  64,    876,   -12,  4,
    -88,   532,   272,   -524,  320,   276,   -508,  940,   24,    -400, -120,
    756,   60,    236,   -412,  100,   376,   -484,  400,   -100,  -740, -108,
    -260,  328,   -268,  224,   -200,  -416,  184,   -604,  -564,  -20,  296,
    60,    892,   -888,  60,    164,   68,    -760,  216,   -296,  904,  -336,
    -28,   404,   -356,  -568,  -208,  -1480, -512,  296,   328,   -360, -164,
    -1560, -776,  1156,  -428,  164,   -504,  -112,  120,   -216,  -148, -264,
    308,   32,    64,    -72,   72,    116,   176,   -64,   -272,  460,  -536,
    -784,  -280,  348,   108,   -752,  -132,  524,   -540,  -776,  116,  -296,
    -1196, -288,  -560,  1040,  -472,  116,   -848,  -1116, 116,   636,  696,
    284,   -176,  1016,  204,   -864,  -648,  -248,  356,   972,   -584, -204,
    264,   880,   528,   -24,   -184,  116,   448,   -144,  828,   524,  212,
    -212,  52,    12,    200,   268,   -488,  -404,  -880,  824,   -672, -40,
    908,   -248,  500,   716,   -576,  492,   -576,  16,    720,   -108, 384,
    124,   344,   280,   576,   -500,  252,   104,   -308,  196,   -188, -8,
    1268,  296,   1032,  -1196, 436,   316,   372,   -432,  -200,  -660, 704,
    -224,  596,   -132,  268,   32,    -452,  884,   104,   -1008, 424,  -1348,
    -280,  4,     -1168, 368,   476,   696,   300,   -8,    24,    180,  -592,
    -196,  388,   304,   500,   724,   -160,  244,   -84,   272,   -256, -420,
    320,   208,   -144,  -156,  156,   364,   452,   28,    540,   316,  220,
    -644,  -248,  464,   72,    360,   32,    -388,  496,   -680,  -48,  208,
    -116,  -408,  60,    -604,  -392,  548,   -840,  784,   -460,  656,  -544,
    -388,  -264,  908,   -800,  -628,  -612,  -568,  572,   -220,  164,  288,
    -16,   -308,  308,   -112,  -636,  -760,  280,   -668,  432,   364,  240,
    -196,  604,   340,   384,   196,   592,   -44,   -500,  432,   -580, -132,
    636,   -76,   392,   4,     -412,  540,   508,   328,   -356,  -36,  16,
    -220,  -64,   -248,  -60,   24,    -192,  368,   1040,  92,    -24,  -1044,
    -32,   40,    104,   148,   192,   -136,  -520,  56,    -816,  -224, 732,
    392,   356,   212,   -80,   -424,  -1008, -324,  588,   -1496, 576,  460,
    -816,  -848,  56,    -580,  -92,   -1372, -112,  -496,  200,   364,  52,
    -140,  48,    -48,   -60,   84,    72,    40,    132,   -356,  -268, -104,
    -284,  -404,  732,   -520,  164,   -304,  -540,  120,   328,   -76,  -460,
    756,   388,   588,   236,   -436,  -72,   -176,  -404,  -316,  -148, 716,
    -604,  404,   -72,   -88,   -888,  -68,   944,   88,    -220,  -344, 960,
    472,   460,   -232,  704,   120,   832,   -228,  692,   -508,  132,  -476,
    844,   -748,  -364,  -44,   1116,  -1104, -1056, 76,    428,   552,  -692,
    60,    356,   96,    -384,  -188,  -612,  -576,  736,   508,   892,  352,
    -1132, 504,   -24,   -352,  324,   332,   -600,  -312,  292,   508,  -144,
    -8,    484,   48,    284,   -260,  -240,  256,   -100,  -292,  -204, -44,
    472,   -204,  908,   -188,  -1000, -256,  92,    1164,  -392,  564,  356,
    652,   -28,   -884,  256,   484,   -192,  760,   -176,  376,   -524, -452,
    -436,  860,   -736,  212,   124,   504,   -476,  468,   76,    -472, 552,
    -692,  -944,  -620,  740,   -240,  400,   132,   20,    192,   -196, 264,
    -668,  -1012, -60,   296,   -316,  -828,  76,    -156,  284,   -768, -448,
    -832,  148,   248,   652,   616,   1236,  288,   -328,  -400,  -124, 588,
    220,   520,   -696,  1032,  768,   -740,  -92,   -272,  296,   448,  -464,
    412,   -200,  392,   440,   -200,  264,   -152,  -260,  320,   1032, 216,
    320,   -8,    -64,   156,   -1016, 1084,  1172,  536,   484,   -432, 132,
    372,   -52,   -256,  84,    116,   -352,  48,    116,   304,   -384, 412,
    924,   -300,  528,   628,   180,   648,   44,    -980,  -220,  1320, 48,
    332,   748,   524,   -268,  -720,  540,   -276,  564,   -344,  -208, -196,
    436,   896,   88,    -392,  132,   80,    -964,  -288,  568,   56,   -48,
    -456,  888,   8,     552,   -156,  -292,  948,   288,   128,   -716, -292,
    1192,  -152,  876,   352,   -600,  -260,  -812,  -468,  -28,   -120, -32,
    -44,   1284,  496,   192,   464,   312,   -76,   -516,  -380,  -456, -1012,
    -48,   308,   -156,  36,    492,   -156,  -808,  188,   1652,  68,   -120,
    -116,  316,   160,   -140,  352,   808,   -416,  592,   316,   -480, 56,
    528,   -204,  -568,  372,   -232,  752,   -344,  744,   -4,    324,  -416,
    -600,  768,   268,   -248,  -88,   -132,  -420,  -432,  80,    -288, 404,
    -316,  -1216, -588,  520,   -108,  92,    -320,  368,   -480,  -216, -92,
    1688,  -300,  180,   1020,  -176,  820,   -68,   -228,  -260,  436,  -904,
    20,    40,    -508,  440,   -736,  312,   332,   204,   760,   -372, 728,
    96,    -20,   -632,  -520,  -560,  336,   1076,  -64,   -532,  776,  584,
    192,   396,   -728,  -520,  276,   -188,  80,    -52,   -612,  -252, -48,
    648,   212,   -688,  228,   -52,   -260,  428,   -412,  -272,  -404, 180,
    816,   -796,  48,    152,   484,   -88,   -216,  988,   696,   188,  -528,
    648,   -116,  -180,  316,   476,   12,    -564,  96,    476,   -252, -364,
    -376,  -392,  556,   -256,  -576,  260,   -352,  120,   -16,   -136, -260,
    -492,  72,    556,   660,   580,   616,   772,   436,   424,   -32,  -324,
    -1268, 416,   -324,  -80,   920,   160,   228,   724,   32,    -516, 64,
    384,   68,    -128,  136,   240,   248,   -204,  -68,   252,   -932, -120,
    -480,  -628,  -84,   192,   852,   -404,  -288,  -132,  204,   100,  168,
    -68,   -196,  -868,  460,   1080,  380,   -80,   244,   0,     484,  -888,
    64,    184,   352,   600,   460,   164,   604,   -196,  320,   -64,  588,
    -184,  228,   12,    372,   48,    -848,  -344,  224,   208,   -200, 484,
    128,   -20,   272,   -468,  -840,  384,   256,   -720,  -520,  -464, -580,
    112,   -120,  644,   -356,  -208,  -608,  -528,  704,   560,   -424, 392,
    828,   40,    84,    200,   -152,  0,     -144,  584,   280,   -120, 80,
    -556,  -972,  -196,  -472,  724,   80,    168,   -32,   88,    160,  -688,
    0,     160,   356,   372,   -776,  740,   -128,  676,   -248,  -480, 4,
    -364,  96,    544,   232,   -1032, 956,   236,   356,   20,    -40,  300,
    24,    -676,  -596,  132,   1120,  -104,  532,   -1096, 568,   648,  444,
    508,   380,   188,   -376,  -604,  1488,  424,   24,    756,   -220, -192,
    716,   120,   920,   688,   168,   44,    -460,  568,   284,   1144, 1160,
    600,   424,   888,   656,   -356,  -320,  220,   316,   -176,  -724, -188,
    -816,  -628,  -348,  -228,  -380,  1012,  -452,  -660,  736,   928,  404,
    -696,  -72,   -268,  -892,  128,   184,   -344,  -780,  360,   336,  400,
    344,   428,   548,   -112,  136,   -228,  -216,  -820,  -516,  340,  92,
    -136,  116,   -300,  376,   -244,  100,   -316,  -520,  -284,  -12,  824,
    164,   -548,  -180,  -128,  116,   -924,  -828,  268,   -368,  -580, 620,
    192,   160,   0,     -1676, 1068,  424,   -56,   -360,  468,   -156, 720,
    288,   -528,  556,   -364,  548,   -148,  504,   316,   152,   -648, -620,
    -684,  -24,   -376,  -384,  -108,  -920,  -1032, 768,   180,   -264, -508,
    -1268, -260,  -60,   300,   -240,  988,   724,   -376,  -576,  -212, -736,
    556,   192,   1092,  -620,  -880,  376,   -56,   -4,    -216,  -32,  836,
    268,   396,   1332,  864,   -600,  100,   56,    -412,  -92,   356,  180,
    884,   -468,  -436,  292,   -388,  -804,  -704,  -840,  368,   -348, 140,
    -724,  1536,  940,   372,   112,   -372,  436,   -480,  1136,  296,  -32,
    -228,  132,   -48,   -220,  868,   -1016, -60,   -1044, -464,  328,  916,
    244,   12,    -736,  -296,  360,   468,   -376,  -108,  -92,   788,  368,
    -56,   544,   400,   -672,  -420,  728,   16,    320,   44,    -284, -380,
    -796,  488,   132,   204,   -596,  -372,  88,    -152,  -908,  -636, -572,
    -624,  -116,  -692,  -200,  -56,   276,   -88,   484,   -324,  948,  864,
    1000,  -456,  -184,  -276,  292,   -296,  156,   676,   320,   160,  908,
    -84,   -1236, -288,  -116,  260,   -372,  -644,  732,   -756,  -96,  84,
    344,   -520,  348,   -688,  240,   -84,   216,   -1044, -136,  -676, -396,
    -1500, 960,   -40,   176,   168,   1516,  420,   -504,  -344,  -364, -360,
    1216,  -940,  -380,  -212,  252,   -660,  -708,  484,   -444,  -152, 928,
    -120,  1112,  476,   -260,  560,   -148,  -344,  108,   -196,  228,  -288,
    504,   560,   -328,  -88,   288,   -1008, 460,   -228,  468,   -836, -196,
    76,    388,   232,   412,   -1168, -716,  -644,  756,   -172,  -356, -504,
    116,   432,   528,   48,    476,   -168,  -608,  448,   160,   -532, -272,
    28,    -676,  -12,   828,   980,   456,   520,   104,   -104,  256,  -344,
    -4,    -28,   -368,  -52,   -524,  -572,  -556,  -200,  768,   1124, -208,
    -512,  176,   232,   248,   -148,  -888,  604,   -600,  -304,  804,  -156,
    -212,  488,   -192,  -804,  -256,  368,   -360,  -916,  -328,  228,  -240,
    -448,  -472,  856,   -556,  -364,  572,   -12,   -156,  -368,  -340, 432,
    252,   -752,  -152,  288,   268,   -580,  -848,  -592,  108,   -76,  244,
    312,   -716,  592,   -80,   436,   360,   4,     -248,  160,   516,  584,
    732,   44,    -468,  -280,  -292,  -156,  -588,  28,    308,   912,  24,
    124,   156,   180,   -252,  944,   -924,  -772,  -520,  -428,  -624, 300,
    -212,  -1144, 32,    -724,  800,   -1128, -212,  -1288, -848,  180,  -416,
    440,   192,   -576,  -792,  -76,   -1080, 80,    -532,  -352,  -132, 380,
    -820,  148,   1112,  128,   164,   456,   700,   -924,  144,   -668, -384,
    648,   -832,  508,   552,   -52,   -100,  -656,  208,   -568,  748,  -88,
    680,   232,   300,   192,   -408,  -1012, -152,  -252,  -268,  272,  -876,
    -664,  -648,  -332,  -136,  16,    12,    1152,  -28,   332,   -536, 320,
    -672,  -460,  -316,  532,   -260,  228,   -40,   1052,  -816,  180,  88,
    -496,  -556,  -672,  -368,  428,   92,    356,   404,   -408,  252,  196,
    -176,  -556,  792,   268,   32,    372,   40,    96,    -332,  328,  120,
    372,   -900,  -40,   472,   -264,  -592,  952,   128,   656,   112,  664,
    -232,  420,   4,     -344,  -464,  556,   244,   -416,  -32,   252,  0,
    -412,  188,   -696,  508,   -476,  324,   -1096, 656,   -312,  560,  264,
    -136,  304,   160,   -64,   -580,  248,   336,   -720,  560,   -348, -288,
    -276,  -196,  -500,  852,   -544,  -236,  -1128, -992,  -776,  116,  56,
    52,    860,   884,   212,   -12,   168,   1020,  512,   -552,  924,  -148,
    716,   188,   164,   -340,  -520,  -184,  880,   -152,  -680,  -208, -1156,
    -300,  -528,  -472,  364,   100,   -744,  -1056, -32,   540,   280,  144,
    -676,  -32,   -232,  -280,  -224,  96,    568,   -76,   172,   148,  148,
    104,   32,    -296,  -32,   788,   -80,   32,    -16,   280,   288,  944,
    428,   -484
};

/* Every plane as 16-bit samples, whatever the bit depth. */
typedef struct
{
  const GstAV1FilmGrainParams *fg;
  gint bitdepth;
  gint sx, sy;
  gboolean is_id;
  gint ar_coeffs[3][GST_AV1_MAX_NUM_POS_LUMA];
} Grain;

static inline gint
get_random_number (gint bits, guint * state)
{
  guint r = *state;
  guint bit = ((r >> 0) ^ (r >> 1) ^ (r >> 3) ^ (r >> 12)) & 1;

  *state = (r >> 1) | (bit << 15);
  return (*state >> (16 - bits)) & ((1 << bits) - 1);
}

static inline gint
round2 (gint x, gint shift)
{
  return (x + ((1 << shift) >> 1)) >> shift;
}

static void
generate_grain_y (const Grain * g, gint16 buf[][GRAIN_WIDTH])
{
  const GstAV1FilmGrainParams *fg = g->fg;
  guint seed = fg->grain_seed;
  gint shift = 4 - (g->bitdepth - 8) + fg->grain_scale_shift;
  gint grain_ctr = 128 << (g->bitdepth - 8);
  gint grain_min = -grain_ctr, grain_max = grain_ctr - 1;
  gint ar_lag = fg->ar_coeff_lag;
  gint ar_shift = fg->ar_coeff_shift_minus_6 + 6;
  gint x, y;

  for (y = 0; y < GRAIN_HEIGHT; y++)
    for (x = 0; x < GRAIN_WIDTH; x++)
      buf[y][x] = round2 (gaussian_sequence[get_random_number (11, &seed)],
          shift);

  for (y = 3; y < GRAIN_HEIGHT; y++) {
    for (x = 3; x < GRAIN_WIDTH - 3; x++) {
      const gint *coeff = g->ar_coeffs[0];
      gint sum = 0, dx, dy;

      for (dy = -ar_lag; dy <= 0; dy++) {
        for (dx = -ar_lag; dx <= ar_lag; dx++) {
          if (!dx && !dy)
            break;
          sum += *(coeff++) * buf[y + dy][x + dx];
        }
      }

      buf[y][x] = CLAMP (buf[y][x] + round2 (sum, ar_shift), grain_min,
          grain_max);
    }
  }
}

static void
generate_grain_uv (const Grain * g, gint16 buf[][GRAIN_WIDTH],
    gint16 buf_y[][GRAIN_WIDTH], gint uv)
{
  const GstAV1FilmGrainParams *fg = g->fg;
  guint seed = fg->grain_seed ^ (uv ? 0x49d8 : 0xb524);
  gint shift = 4 - (g->bitdepth - 8) + fg->grain_scale_shift;
  gint grain_ctr = 128 << (g->bitdepth - 8);
  gint grain_min = -grain_ctr, grain_max = grain_ctr - 1;
  gint chroma_w = g->sx ? SUB_GRAIN_WIDTH : GRAIN_WIDTH;
  gint chroma_h = g->sy ? SUB_GRAIN_HEIGHT : GRAIN_HEIGHT;
  gint ar_lag = fg->ar_coeff_lag;
  gint ar_shift = fg->ar_coeff_shift_minus_6 + 6;
  gint x, y;

  for (y = 0; y < chroma_h; y++)
    for (x = 0; x < chroma_w; x++)
      buf[y][x] = round2 (gaussian_sequence[get_random_number (11, &seed)],
          shift);

  for (y = 3; y < chroma_h; y++) {
    for (x = 3; x < chroma_w - 3; x++) {
      const gint *coeff = g->ar_coeffs[1 + uv];
      gint sum = 0, dx, dy;

      for (dy = -ar_lag; dy <= 0; dy++) {
        for (dx = -ar_lag; dx <= ar_lag; dx++) {
          /* The current position takes the luma grain's contribution */
          if (!dx && !dy) {
            gint luma_x = ((x - 3) << g->sx) + 3;
            gint luma_y = ((y - 3) << g->sy) + 3;
            gint luma = 0, i, j;

            if (!fg->num_y_points)
              break;
            for (i = 0; i <= g->sy; i++)
              for (j = 0; j <= g->sx; j++)
                luma += buf_y[luma_y + i][luma_x + j];
            luma = round2 (luma, g->sx + g->sy);
            sum += luma * (*coeff);
            break;
          }
          sum += *(coeff++) * buf[y + dy][x + dx];
        }
      }

      buf[y][x] = CLAMP (buf[y][x] + round2 (sum, ar_shift), grain_min,
          grain_max);
    }
  }
}

/* The grain of block (bx, by) of the 2x2 neighbourhood: bx 1 is the block to
 * the left, by 1 the block above. */
static inline gint
sample_lut (gint16 lut[][GRAIN_WIDTH], gint offsets[2][2], gint sx, gint sy,
    gint bx, gint by, gint x, gint y)
{
  gint randval = offsets[bx][by];
  gint offx = 3 + (2 >> sx) * (3 + (randval >> 4));
  gint offy = 3 + (2 >> sy) * (3 + (randval & 0xf));

  return lut[offy + y + (FG_BLOCK_SIZE >> sy) * by]
      [offx + x + (FG_BLOCK_SIZE >> sx) * bx];
}

static void
row_seeds (const Grain * g, gint row, gint rows, guint seed[2])
{
  gint i;

  for (i = 0; i < rows; i++) {
    seed[i] = g->fg->grain_seed;
    seed[i] ^= (((row - i) * 37 + 178) & 0xff) << 8;
    seed[i] ^= (((row - i) * 173 + 105) & 0xff);
  }
}

static void
fgy_row (const Grain * g, guint16 * dst_row, const guint16 * src_row,
    gint stride, gint pw, const guint8 * scaling, gint16 lut[][GRAIN_WIDTH],
    gint bh, gint row)
{
  const GstAV1FilmGrainParams *fg = g->fg;
  static const gint w[2][2] = { {27, 17}, {17, 27} };
  gint rows = 1 + (fg->overlap_flag && row > 0);
  gint grain_ctr = 128 << (g->bitdepth - 8);
  gint grain_min = -grain_ctr, grain_max = grain_ctr - 1;
  gint scaling_shift = fg->grain_scaling_minus_8 + 8;
  gint min_value, max_value, bx;
  gint offsets[2][2] = { {0, 0}, {0, 0} };
  guint seed[2];

  if (fg->clip_to_restricted_range) {
    min_value = 16 << (g->bitdepth - 8);
    max_value = 235 << (g->bitdepth - 8);
  } else {
    min_value = 0;
    max_value = (1 << g->bitdepth) - 1;
  }

  row_seeds (g, row, rows, seed);

  for (bx = 0; bx < pw; bx += FG_BLOCK_SIZE) {
    gint bw = MIN (FG_BLOCK_SIZE, pw - bx);
    gint ystart = fg->overlap_flag && row ? MIN (2, bh) : 0;
    gint xstart = fg->overlap_flag && bx ? MIN (2, bw) : 0;
    gint i, x, y;

    if (fg->overlap_flag && bx)
      for (i = 0; i < rows; i++)
        offsets[1][i] = offsets[0][i];
    for (i = 0; i < rows; i++)
      offsets[0][i] = get_random_number (8, &seed[i]);

#define ADD_NOISE_Y(x, y, grain) G_STMT_START {                           \
      const guint16 *src = src_row + (y) * stride + (x) + bx;             \
      guint16 *dst = dst_row + (y) * stride + (x) + bx;                   \
      gint noise = round2 (scaling[*src] * (grain), scaling_shift);       \
      *dst = CLAMP (*src + noise, min_value, max_value);                  \
    } G_STMT_END

    for (y = ystart; y < bh; y++) {
      for (x = xstart; x < bw; x++)
        ADD_NOISE_Y (x, y, sample_lut (lut, offsets, 0, 0, 0, 0, x, y));

      /* Overlapped column */
      for (x = 0; x < xstart; x++) {
        gint grain = sample_lut (lut, offsets, 0, 0, 0, 0, x, y);
        gint old = sample_lut (lut, offsets, 0, 0, 1, 0, x, y);

        grain = CLAMP (round2 (old * w[x][0] + grain * w[x][1], 5),
            grain_min, grain_max);
        ADD_NOISE_Y (x, y, grain);
      }
    }

    for (y = 0; y < ystart; y++) {
      /* Overlapped row, without the corner */
      for (x = xstart; x < bw; x++) {
        gint grain = sample_lut (lut, offsets, 0, 0, 0, 0, x, y);
        gint old = sample_lut (lut, offsets, 0, 0, 0, 1, x, y);

        grain = CLAMP (round2 (old * w[y][0] + grain * w[y][1], 5),
            grain_min, grain_max);
        ADD_NOISE_Y (x, y, grain);
      }

      /* Doubly overlapped corner */
      for (x = 0; x < xstart; x++) {
        gint grain = sample_lut (lut, offsets, 0, 0, 0, 0, x, y);
        gint top = sample_lut (lut, offsets, 0, 0, 0, 1, x, y);
        gint old = sample_lut (lut, offsets, 0, 0, 1, 1, x, y);

        top = CLAMP (round2 (old * w[x][0] + top * w[x][1], 5), grain_min,
            grain_max);
        old = sample_lut (lut, offsets, 0, 0, 1, 0, x, y);
        grain = CLAMP (round2 (old * w[x][0] + grain * w[x][1], 5),
            grain_min, grain_max);
        grain = CLAMP (round2 (top * w[y][0] + grain * w[y][1], 5),
            grain_min, grain_max);
        ADD_NOISE_Y (x, y, grain);
      }
    }
#undef ADD_NOISE_Y
  }
}

static void
fguv_row (const Grain * g, guint16 * dst_row, const guint16 * src_row,
    gint stride, gint pw, const guint8 * scaling, gint16 lut[][GRAIN_WIDTH],
    gint bh, gint row, const guint16 * luma_row, gint luma_stride, gint uv)
{
  const GstAV1FilmGrainParams *fg = g->fg;
  static const gint w[2][2][2] = {
    {{27, 17}, {17, 27}},
    {{23, 22}},
  };
  gint sx = g->sx, sy = g->sy;
  gint rows = 1 + (fg->overlap_flag && row > 0);
  gint grain_ctr = 128 << (g->bitdepth - 8);
  gint grain_min = -grain_ctr, grain_max = grain_ctr - 1;
  gint bitdepth_max = (1 << g->bitdepth) - 1;
  gint scaling_shift = fg->grain_scaling_minus_8 + 8;
  gint mult = (uv ? fg->cr_mult : fg->cb_mult) - 128;
  gint luma_mult = (uv ? fg->cr_luma_mult : fg->cb_luma_mult) - 128;
  gint offset = (uv ? fg->cr_offset : fg->cb_offset) - 256;
  gint min_value, max_value, bx;
  gint offsets[2][2] = { {0, 0}, {0, 0} };
  guint seed[2];

  if (fg->clip_to_restricted_range) {
    min_value = 16 << (g->bitdepth - 8);
    max_value = (g->is_id ? 235 : 240) << (g->bitdepth - 8);
  } else {
    min_value = 0;
    max_value = bitdepth_max;
  }

  row_seeds (g, row, rows, seed);

  for (bx = 0; bx < pw; bx += FG_BLOCK_SIZE >> sx) {
    gint bw = MIN (FG_BLOCK_SIZE >> sx, pw - bx);
    gint ystart = fg->overlap_flag && row ? MIN (2 >> sy, bh) : 0;
    gint xstart = fg->overlap_flag && bx ? MIN (2 >> sx, bw) : 0;
    gint i, x, y;

    if (fg->overlap_flag && bx)
      for (i = 0; i < rows; i++)
        offsets[1][i] = offsets[0][i];
    for (i = 0; i < rows; i++)
      offsets[0][i] = get_random_number (8, &seed[i]);

#define ADD_NOISE_UV(x, y, grain) G_STMT_START {                          \
      const guint16 *luma = luma_row + ((y) << sy) * luma_stride +        \
          ((bx + (x)) << sx);                                             \
      const guint16 *src = src_row + (y) * stride + bx + (x);             \
      guint16 *dst = dst_row + (y) * stride + bx + (x);                   \
      gint avg = luma[0], val, noise;                                     \
      if (sx)                                                             \
        avg = (avg + luma[1] + 1) >> 1;                                   \
      val = avg;                                                          \
      if (!fg->chroma_scaling_from_luma)                                  \
        val = CLAMP (((avg * luma_mult + *src * mult) >> 6) +             \
            offset * (1 << (g->bitdepth - 8)), 0, bitdepth_max);          \
      noise = round2 (scaling[val] * (grain), scaling_shift);             \
      *dst = CLAMP (*src + noise, min_value, max_value);                  \
    } G_STMT_END

    for (y = ystart; y < bh; y++) {
      for (x = xstart; x < bw; x++)
        ADD_NOISE_UV (x, y, sample_lut (lut, offsets, sx, sy, 0, 0, x, y));

      /* Overlapped column */
      for (x = 0; x < xstart; x++) {
        gint grain = sample_lut (lut, offsets, sx, sy, 0, 0, x, y);
        gint old = sample_lut (lut, offsets, sx, sy, 1, 0, x, y);

        grain = CLAMP (round2 (old * w[sx][x][0] + grain * w[sx][x][1], 5),
            grain_min, grain_max);
        ADD_NOISE_UV (x, y, grain);
      }
    }

    for (y = 0; y < ystart; y++) {
      /* Overlapped row, without the corner */
      for (x = xstart; x < bw; x++) {
        gint grain = sample_lut (lut, offsets, sx, sy, 0, 0, x, y);
        gint old = sample_lut (lut, offsets, sx, sy, 0, 1, x, y);

        grain = CLAMP (round2 (old * w[sy][y][0] + grain * w[sy][y][1], 5),
            grain_min, grain_max);
        ADD_NOISE_UV (x, y, grain);
      }

      /* Doubly overlapped corner */
      for (x = 0; x < xstart; x++) {
        gint top = sample_lut (lut, offsets, sx, sy, 0, 1, x, y);
        gint old = sample_lut (lut, offsets, sx, sy, 1, 1, x, y);
        gint grain = sample_lut (lut, offsets, sx, sy, 0, 0, x, y);

        top = CLAMP (round2 (old * w[sx][x][0] + top * w[sx][x][1], 5),
            grain_min, grain_max);
        old = sample_lut (lut, offsets, sx, sy, 1, 0, x, y);
        grain = CLAMP (round2 (old * w[sx][x][0] + grain * w[sx][x][1], 5),
            grain_min, grain_max);
        grain = CLAMP (round2 (top * w[sy][y][0] + grain * w[sy][y][1], 5),
            grain_min, grain_max);
        ADD_NOISE_UV (x, y, grain);
      }
    }
#undef ADD_NOISE_UV
  }
}

static void
generate_scaling (const Grain * g, const guint8 * values,
    const guint8 * scalings, gint num, guint8 scaling[SCALING_SIZE])
{
  gint shift_x = g->bitdepth - 8;
  gint scaling_size = 1 << g->bitdepth;
  gint max_value, i, x;

  if (num == 0) {
    memset (scaling, 0, scaling_size);
    return;
  }

  max_value = values[num - 1] << shift_x;

  /* The entries before the first point take its value */
  memset (scaling, scalings[0], values[0] << shift_x);

  /* Linear interpolation between the points */
  for (i = 0; i < num - 1; i++) {
    gint bx = values[i], by = scalings[i];
    gint dx = values[i + 1] - bx, dy = scalings[i + 1] - by;
    gint delta = dy * ((0x10000 + (dx >> 1)) / dx);
    gint d = 0x8000;

    for (x = 0; x < dx; x++) {
      scaling[(bx + x) << shift_x] = by + (d >> 16);
      d += delta;
    }
  }

  /* The entries after the last point take its value */
  memset (&scaling[max_value], scalings[num - 1], scaling_size - max_value);

  /* Above 8 bits, interpolate between the 8-bit positions */
  for (i = 0; i < num - 1; i++) {
    gint pad = 1 << shift_x, rnd = pad >> 1;
    gint bx = values[i] << shift_x;
    gint dx = (values[i + 1] << shift_x) - bx;

    for (x = 0; x < dx; x += pad) {
      gint range = scaling[bx + x + pad] - scaling[bx + x];
      gint n, r;

      for (n = 1, r = rnd; n < pad; n++) {
        r += range;
        scaling[bx + x + n] = scaling[bx + x] + (r >> shift_x);
      }
    }
  }
}

/* Sample access for the formats the decoder outputs */
typedef enum
{
  SAMPLES_8,                    /* 8 bits per byte */
  SAMPLES_16,                   /* MSB-aligned in 16-bit little endian */
  SAMPLES_10LE32,               /* three per little endian 32-bit word */
} SampleKind;

static SampleKind
sample_kind (GstVideoFormat format)
{
  switch (format) {
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV16:
    case GST_VIDEO_FORMAT_NV24:
    case GST_VIDEO_FORMAT_GRAY8:
      return SAMPLES_8;
    case GST_VIDEO_FORMAT_NV12_10LE32:
    case GST_VIDEO_FORMAT_NV16_10LE32:
    case GST_VIDEO_FORMAT_NV24_10LE32:
      return SAMPLES_10LE32;
    default:
      return SAMPLES_16;
  }
}

gboolean
gst_v4l2_codec_av1_format_has_grain (GstVideoFormat format)
{
  switch (format) {
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV16:
    case GST_VIDEO_FORMAT_NV24:
    case GST_VIDEO_FORMAT_GRAY8:
    case GST_VIDEO_FORMAT_NV12_10LE32:
    case GST_VIDEO_FORMAT_NV16_10LE32:
    case GST_VIDEO_FORMAT_NV24_10LE32:
    case GST_VIDEO_FORMAT_P010_10LE:
    case GST_VIDEO_FORMAT_P012_LE:
    case GST_VIDEO_FORMAT_P210_10LE:
    case GST_VIDEO_FORMAT_P212_LE:
    case GST_VIDEO_FORMAT_P410_10LE:
    case GST_VIDEO_FORMAT_P412_LE:
    case GST_VIDEO_FORMAT_GRAY16_LE:
      return TRUE;
    default:
      return FALSE;
  }
}

static void
read_samples (SampleKind kind, gint shift, const guint8 * line, gint n,
    guint16 * dst)
{
  gint i;

  for (i = 0; i < n; i++) {
    switch (kind) {
      case SAMPLES_8:
        dst[i] = line[i];
        break;
      case SAMPLES_16:
        dst[i] = GST_READ_UINT16_LE (line + 2 * i) >> shift;
        break;
      case SAMPLES_10LE32:
        dst[i] = (GST_READ_UINT32_LE (line + 4 * (i / 3)) >> (10 * (i % 3)))
            & 0x3ff;
        break;
    }
  }
}

static void
write_samples (SampleKind kind, gint shift, guint8 * line, gint n,
    const guint16 * src)
{
  gint i;

  for (i = 0; i < n; i++) {
    switch (kind) {
      case SAMPLES_8:
        line[i] = src[i];
        break;
      case SAMPLES_16:
        GST_WRITE_UINT16_LE (line + 2 * i, src[i] << shift);
        break;
      case SAMPLES_10LE32:
        if (i % 3 == 2 || i == n - 1) {
          guint32 word = 0;
          gint j;

          for (j = i - i % 3; j <= i; j++)
            word |= (guint32) src[j] << (10 * (j % 3));
          GST_WRITE_UINT32_LE (line + 4 * (i / 3), word);
        }
        break;
    }
  }
}

/**
 * gst_v4l2_codec_av1_apply_film_grain:
 * @fg: the frame's film grain parameters, with apply_grain set
 * @bit_depth: the sequence's BitDepth
 * @matrix_identity: the sequence's matrix_coefficients is MC_IDENTITY
 * @src: the decoded picture
 * @dest: the output, of the same format and at most the size of @src
 *
 * Writes @src with the film grain of @fg added into @dest.
 */
void
gst_v4l2_codec_av1_apply_film_grain (const GstAV1FilmGrainParams * fg,
    guint bit_depth, gboolean matrix_identity, const GstVideoFrame * src,
    GstVideoFrame * dest)
{
  const GstVideoFormatInfo *finfo = dest->info.finfo;
  SampleKind kind = sample_kind (GST_VIDEO_FRAME_FORMAT (dest));
  gint shift = kind == SAMPLES_16 ? 16 - bit_depth : 0;
  gboolean mono = GST_VIDEO_FORMAT_INFO_IS_GRAY (finfo);
  gint w = GST_VIDEO_FRAME_WIDTH (dest), h = GST_VIDEO_FRAME_HEIGHT (dest);
  gint16 (*lut)[GRAIN_HEIGHT + 1][GRAIN_WIDTH];
  guint8 (*scaling)[SCALING_SIZE];
  guint16 *y_in, *y_out, *uv_in[2] = { NULL, NULL }, *uv_out[2] = { NULL,
    NULL }, *line;
  gint y_stride = w + 1, cw = 0, ch = 0, rows, row, x, y, i, n;
  Grain g = {
    .fg = fg,
    .bitdepth = bit_depth,
    .is_id = matrix_identity,
  };

  if (!mono) {
    g.sx = GST_VIDEO_FORMAT_INFO_W_SUB (finfo, 1);
    g.sy = GST_VIDEO_FORMAT_INFO_H_SUB (finfo, 1);
    cw = (w + g.sx) >> g.sx;
    ch = (h + g.sy) >> g.sy;
  }

  /* ar_coeffs_*_plus_128: numPosLuma entries, the chroma ones followed by
   * the luma contribution */
  n = 2 * fg->ar_coeff_lag * (fg->ar_coeff_lag + 1);
  for (i = 0; i < n + 1; i++) {
    g.ar_coeffs[0][i] = fg->ar_coeffs_y_plus_128[i] - 128;
    g.ar_coeffs[1][i] = fg->ar_coeffs_cb_plus_128[i] - 128;
    g.ar_coeffs[2][i] = fg->ar_coeffs_cr_plus_128[i] - 128;
  }

  lut = g_malloc0 (sizeof (*lut) * 3);
  scaling = g_malloc0 (sizeof (*scaling) * 3);
  y_in = g_new (guint16, y_stride * h);
  y_out = g_new (guint16, y_stride * h);
  line = g_new (guint16, 2 * MAX (w, 1));

  for (y = 0; y < h; y++) {
    read_samples (kind, shift, GST_VIDEO_FRAME_PLANE_DATA (src, 0) +
        y * GST_VIDEO_FRAME_PLANE_STRIDE (src, 0), w, y_in + y * y_stride);
    /* The chroma of an odd last column averages the last luma twice */
    y_in[y * y_stride + w] = y_in[y * y_stride + w - 1];
  }
  memcpy (y_out, y_in, sizeof (guint16) * y_stride * h);

  if (!mono) {
    for (i = 0; i < 2; i++) {
      uv_in[i] = g_new (guint16, cw * ch);
      uv_out[i] = g_new (guint16, cw * ch);
    }
    for (y = 0; y < ch; y++) {
      read_samples (kind, shift, GST_VIDEO_FRAME_PLANE_DATA (src, 1) +
          y * GST_VIDEO_FRAME_PLANE_STRIDE (src, 1), 2 * cw, line);
      for (x = 0; x < cw; x++) {
        uv_in[0][y * cw + x] = line[2 * x];
        uv_in[1][y * cw + x] = line[2 * x + 1];
      }
    }
    for (i = 0; i < 2; i++)
      memcpy (uv_out[i], uv_in[i], sizeof (guint16) * cw * ch);
  }

  generate_grain_y (&g, lut[0]);
  if (!mono) {
    if (fg->num_cb_points || fg->chroma_scaling_from_luma)
      generate_grain_uv (&g, lut[1], lut[0], 0);
    if (fg->num_cr_points || fg->chroma_scaling_from_luma)
      generate_grain_uv (&g, lut[2], lut[0], 1);
  }

  if (fg->num_y_points || fg->chroma_scaling_from_luma)
    generate_scaling (&g, fg->point_y_value, fg->point_y_scaling,
        fg->num_y_points, scaling[0]);
  if (fg->num_cb_points)
    generate_scaling (&g, fg->point_cb_value, fg->point_cb_scaling,
        fg->num_cb_points, scaling[1]);
  if (fg->num_cr_points)
    generate_scaling (&g, fg->point_cr_value, fg->point_cr_scaling,
        fg->num_cr_points, scaling[2]);

  rows = (h + FG_BLOCK_SIZE - 1) / FG_BLOCK_SIZE;
  for (row = 0; row < rows; row++) {
    gint bh = MIN (h - row * FG_BLOCK_SIZE, FG_BLOCK_SIZE);
    gint cbh = (bh + g.sy) >> g.sy;
    const guint16 *luma = y_in + row * FG_BLOCK_SIZE * y_stride;
    gint uv_off = (row * FG_BLOCK_SIZE >> g.sy) * cw;

    if (fg->num_y_points)
      fgy_row (&g, y_out + row * FG_BLOCK_SIZE * y_stride,
          y_in + row * FG_BLOCK_SIZE * y_stride, y_stride, w, scaling[0],
          lut[0], bh, row);

    if (mono)
      continue;

    for (i = 0; i < 2; i++) {
      gint num_points = i ? fg->num_cr_points : fg->num_cb_points;

      if (fg->chroma_scaling_from_luma)
        fguv_row (&g, uv_out[i] + uv_off, uv_in[i] + uv_off, cw, cw,
            scaling[0], lut[1 + i], cbh, row, luma, y_stride, i);
      else if (num_points)
        fguv_row (&g, uv_out[i] + uv_off, uv_in[i] + uv_off, cw, cw,
            scaling[1 + i], lut[1 + i], cbh, row, luma, y_stride, i);
    }
  }

  for (y = 0; y < h; y++)
    write_samples (kind, shift, GST_VIDEO_FRAME_PLANE_DATA (dest, 0) +
        y * GST_VIDEO_FRAME_PLANE_STRIDE (dest, 0), w, y_out + y * y_stride);

  if (!mono) {
    for (y = 0; y < ch; y++) {
      for (x = 0; x < cw; x++) {
        line[2 * x] = uv_out[0][y * cw + x];
        line[2 * x + 1] = uv_out[1][y * cw + x];
      }
      write_samples (kind, shift, GST_VIDEO_FRAME_PLANE_DATA (dest, 1) +
          y * GST_VIDEO_FRAME_PLANE_STRIDE (dest, 1), 2 * cw, line);
    }
    for (i = 0; i < 2; i++) {
      g_free (uv_in[i]);
      g_free (uv_out[i]);
    }
  }

  g_free (line);
  g_free (y_out);
  g_free (y_in);
  g_free (scaling);
  g_free (lut);
}
