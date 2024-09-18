/* Copyright (C) 2001-2023 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  39 Mesa Street, Suite 108A, San Francisco,
   CA 94129, USA, for further information.
*/


/* Conversion between device color spaces for Ghostscript */
#include "gx.h"
#include "gsdcolor.h"		/* for gxcmap.h */
#include "gxdcconv.h"		/* interface */
#include "gxdevice.h"		/* for gxcmap.h */
#include "gxcmap.h"
#include "gxfarith.h"
#include "gxlum.h"
#include "gxgstate.h"
#include "gsstate.h"            /* for gs_currentcpsimode */

/*
 * The CMYK to RGB algorithms specified by Adobe are, e.g.,
 *      R = 1.0 - min(1.0, C + K)
 *      C = max(0.0, min(1.0, 1 - R - UCR))
 * We got better results on displays with
 *      R = (1.0 - C) * (1.0 - K)
 *      C = max(0.0, min(1.0, 1 - R / (1 - UCR)))
 * For PLRM compatibility, we use the Adobe algorithms by default,
 * but what Adobe says and what they do are two different things.
 * Testing on CPSI shows that they use the 'better' algorithm.
 */

/* ------ Color space conversion ------ */

/* Only 4 of the 6 conversions are implemented here; */
/* the other 2 (Gray to RGB/CMYK) are trivial. */

/* Convert RGB to Gray. */
frac
color_rgb_to_gray(frac r, frac g, frac b, const gs_gstate * pgs)
{
    return (r * (unsigned long)lum_red_weight +
            g * (unsigned long)lum_green_weight +
            b * (unsigned long)lum_blue_weight +
            (lum_all_weights / 2))
        / lum_all_weights;
}

/* Convert RGB to CMYK. */
/* Note that this involves black generation and undercolor removal. */
void
color_rgb_to_cmyk(frac r, frac g, frac b, const gs_gstate * pgs,
                  frac cmyk[4], gs_memory_t *mem)
{
    frac c = frac_1 - r, m = frac_1 - g, y = frac_1 - b;
    frac k = (c < m ? min(c, y) : min(m, y));

    /*
     * The default UCR and BG functions are pretty arbitrary,
     * but they must agree with the ones in gs_init.ps.
     */
    frac bg =
        (pgs == NULL ? k : pgs->black_generation == NULL ? frac_0 :
         gx_map_color_frac(pgs, k, black_generation));
    signed_frac ucr =
        (pgs == NULL ? k : pgs->undercolor_removal == NULL ? frac_0 :
         gx_map_color_frac(pgs, k, undercolor_removal));

    if (ucr == frac_1)
        cmyk[0] = cmyk[1] = cmyk[2] = 0;
    else if (ucr == frac_0)
        cmyk[0] = c, cmyk[1] = m, cmyk[2] = y;
    else {
        if (!gs_currentcpsimode(mem)) {
            /* C = max(0.0, min(1.0, 1 - R - UCR)), etc. */
            signed_frac not_ucr = (ucr < 0 ? frac_1 + ucr : frac_1);

            cmyk[0] = (c < ucr ? frac_0 : c > not_ucr ? frac_1 : c - ucr);
            cmyk[1] = (m < ucr ? frac_0 : m > not_ucr ? frac_1 : m - ucr);
            cmyk[2] = (y < ucr ? frac_0 : y > not_ucr ? frac_1 : y - ucr);
        } else {
            /* Adobe CPSI method */
            /* C = max(0.0, min(1.0, 1 - R / (1 - UCR))), etc. */
            float denom = frac2float(frac_1 - ucr);		/* unscaled */
            float v;

            v = (float)frac_1 - r / denom;	/* unscaled */
            cmyk[0] =
                (is_fneg(v) ? frac_0 : v >= (float)frac_1 ? frac_1 : (frac) v);
            v = (float)frac_1 - g / denom;	/* unscaled */
            cmyk[1] =
                (is_fneg(v) ? frac_0 : v >= (float)frac_1 ? frac_1 : (frac) v);
            v = (float)frac_1 - b / denom;	/* unscaled */
            cmyk[2] =
                (is_fneg(v) ? frac_0 : v >= (float)frac_1 ? frac_1 : (frac) v);
        }
    }
    cmyk[3] = bg;
    if_debug7m('c', mem, "[c]RGB 0x%x,0x%x,0x%x -> CMYK 0x%x,0x%x,0x%x,0x%x\n",
               r, g, b, cmyk[0], cmyk[1], cmyk[2], cmyk[3]);
}

/* Convert CMYK to Gray. */
frac
color_cmyk_to_gray(frac c, frac m, frac y, frac k, const gs_gstate * pgs)
{
    frac not_gray = color_rgb_to_gray(c, m, y, pgs);

    return (not_gray > frac_1 - k ?	/* gray + k > 1.0 */
            frac_0 : frac_1 - (not_gray + k));
}

/* Convert CMYK to RGB. */
void
color_cmyk_to_rgb(frac c, frac m, frac y, frac k, const gs_gstate * pgs,
                  frac rgb[3], gs_memory_t *mem)
{
    switch (k) {
        case frac_0:
            rgb[0] = frac_1 - c;
            rgb[1] = frac_1 - m;
            rgb[2] = frac_1 - y;
            break;
        case frac_1:
            rgb[0] = rgb[1] = rgb[2] = frac_0;
            break;
        default:
            if (!gs_currentcpsimode(mem)) {
                /* R = 1.0 - min(1.0, C + K), etc. */
                frac not_k = frac_1 - k;

                rgb[0] = (c > not_k ? frac_0 : not_k - c);
                rgb[1] = (m > not_k ? frac_0 : not_k - m);
                rgb[2] = (y > not_k ? frac_0 : not_k - y);
            } else {
                /* R = (1.0 - C) * (1.0 - K), etc. */
                ulong not_k = frac_1 - k;

                /* Compute not_k * (frac_1 - v) / frac_1 efficiently. */
                ulong prod;

#define deduct_black(v)\
  (prod = (frac_1 - (v)) * not_k, frac_1_quo(prod))
                rgb[0] = deduct_black(c);
                rgb[1] = deduct_black(m);
                rgb[2] = deduct_black(y);
#undef deduct_black
            }
    }
    if_debug7m('c', mem, "[c]CMYK 0x%x,0x%x,0x%x,0x%x -> RGB 0x%x,0x%x,0x%x\n",
               c, m, y, k, rgb[0], rgb[1], rgb[2]);
}
