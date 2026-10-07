/* GStreamer
 * Copyright (C) 2026 Jiuyang Liu <liu@jiuyang.me>
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

#ifndef __GST_V4L2_CODEC_AV1_GRAIN_H__
#define __GST_V4L2_CODEC_AV1_GRAIN_H__

#include <gst/video/video.h>
#include <gst/codecparsers/gstav1parser.h>

G_BEGIN_DECLS

gboolean gst_v4l2_codec_av1_format_has_grain (GstVideoFormat format);

void     gst_v4l2_codec_av1_apply_film_grain (const GstAV1FilmGrainParams * fg,
                                              guint bit_depth,
                                              gboolean matrix_identity,
                                              const GstVideoFrame * src,
                                              GstVideoFrame * dest);

G_END_DECLS

#endif /* __GST_V4L2_CODEC_AV1_GRAIN_H__ */
