/*****************************************************************************
 * bluescreen.c : Bluescreen (weather channel like) video filter for vlc
 *****************************************************************************
 * Copyright (C) 2005-2007 the VideoLAN team
 * $Id: 2499f55d9b016180d56d29c1a85010dd8182f00e $
 *
 * Authors: Antoine Cellerier <dionoea at videolan tod org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout.h>

#include "vlc_filter.h"

#define BLUESCREEN_HELP N_( \
    "This effect, also known as \"greenscreen\" or \"chroma key\" blends " \
    "the \"blue parts\" of the foreground image of the mosaic on the " \
    "background (like weather forcasts). You can choose the \"key\" " \
    "color for blending (blyyue by default)." )

#define BLUESCREENU_TEXT N_("Bluescreen U value")
#define BLUESCREENU_LONGTEXT N_( \
        "\"U\" value for the bluescreen key color " \
        "(in YUV values). From 0 to 255. Defaults to 120 for blue." )
#define BLUESCREENV_TEXT N_("Bluescreen V value")
#define BLUESCREENV_LONGTEXT N_( \
        "\"V\" value for the bluescreen key color " \
        "(in YUV values). From 0 to 255. Defaults to 90 for blue." )
#define BLUESCREENUTOL_TEXT N_("Bluescreen U tolerance")
#define BLUESCREENUTOL_LONGTEXT N_( \
        "Tolerance of the bluescreen blender " \
        "on color variations for the U plane. A value between 10 and 20 " \
        "seems sensible." )
#define BLUESCREENVTOL_TEXT N_("Bluescreen V tolerance")
#define BLUESCREENVTOL_LONGTEXT N_( \
        "Tolerance of the bluescreen blender " \
        "on color variations for the V plane. A value between 10 and 20 " \
        "seems sensible." )

#define CFG_PREFIX "bluescreen-"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  Create      ( vlc_object_t * );
static void Destroy     ( vlc_object_t * );

static picture_t *Filter( filter_t *, picture_t * );
static int BluescreenCallback( vlc_object_t *, char const *,
                               vlc_value_t, vlc_value_t, void * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin();
    set_description( N_("Bluescreen video filter") );
    set_shortname( N_("Bluescreen" ));
    set_help( BLUESCREEN_HELP );
    set_category( CAT_VIDEO );
    set_subcategory( SUBCAT_VIDEO_VFILTER );
    set_capability( "video filter2", 0 );
    add_shortcut( "bluescreen" );
    set_callbacks( Create, Destroy );

    add_integer_with_range( CFG_PREFIX "u", 120, 0, 255, NULL,
                            BLUESCREENU_TEXT, BLUESCREENU_LONGTEXT, false );
    add_integer_with_range( CFG_PREFIX "v", 90, 0, 255, NULL,
                            BLUESCREENV_TEXT, BLUESCREENV_LONGTEXT, false );
    add_integer_with_range( CFG_PREFIX "ut", 17, 0, 255, NULL,
                            BLUESCREENUTOL_TEXT, BLUESCREENUTOL_LONGTEXT,
                            false );
    add_integer_with_range( CFG_PREFIX "vt", 17, 0, 255, NULL,
                            BLUESCREENVTOL_TEXT, BLUESCREENVTOL_LONGTEXT,
                            false );
vlc_module_end();

static const char *const ppsz_filter_options[] = {
    "u", "v", "ut", "vt", NULL
};

struct filter_sys_t
{
    int i_u, i_v, i_ut, i_vt;
    uint8_t *p_at;
};

static int Create( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;
    filter_sys_t *p_sys;

    if( p_filter->fmt_in.video.i_chroma != VLC_FOURCC('Y','U','V','A') )
    {
        msg_Err( p_filter,
                 "Unsupported input chroma \"%4s\". "
                 "Bluescreen can only use \"YUVA\".",
                 (char*)&p_filter->fmt_in.video.i_chroma );
        return VLC_EGENERIC;
    }

    /* Allocate structure */
    p_filter->p_sys = malloc( sizeof( filter_sys_t ) );
    if( p_filter->p_sys == NULL )
        return VLC_ENOMEM;
    p_sys = p_filter->p_sys;

    config_ChainParse( p_filter, CFG_PREFIX, ppsz_filter_options,
                       p_filter->p_cfg );

#define GET_VAR( name, min, max )                                           \
    p_sys->i_##name = __MIN( max, __MAX( min,                               \
        var_CreateGetIntegerCommand( p_filter, CFG_PREFIX #name ) ) );      \
    var_AddCallback( p_filter, CFG_PREFIX #name, BluescreenCallback, p_sys );

    GET_VAR( u, 0x00, 0xff );
    GET_VAR( v, 0x00, 0xff );
    GET_VAR( ut, 0x00, 0xff );
    GET_VAR( vt, 0x00, 0xff );
    p_sys->p_at = NULL;

    p_filter->pf_video_filter = Filter;

    return VLC_SUCCESS;
}

static void Destroy( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;

    free( p_filter->p_sys->p_at );
    free( p_filter->p_sys );
}

static picture_t *Filter( filter_t *p_filter, picture_t *p_pic )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    int i,j;
    int i_lines = p_pic->p[ A_PLANE ].i_lines;
    int i_pitch = p_pic->p[ A_PLANE ].i_pitch;
    uint8_t *p_a = p_pic->p[ A_PLANE ].p_pixels;
    uint8_t *p_at;
    uint8_t *p_u = p_pic->p[ U_PLANE ].p_pixels;
    uint8_t *p_v = p_pic->p[ V_PLANE ].p_pixels;
    uint8_t umin, umax, vmin, vmax;

    if( p_pic->format.i_chroma != VLC_FOURCC('Y','U','V','A') )
    {
        msg_Err( p_filter,
                 "Unsupported input chroma \"%4s\". "
                 "Bluescreen can only use \"YUVA\".",
                 (char*)&p_pic->format.i_chroma );
        return NULL;
    }

    p_sys->p_at = realloc( p_sys->p_at, i_lines * i_pitch * sizeof( uint8_t ) );
    p_at = p_sys->p_at;

    umin = p_sys->i_u - p_sys->i_ut >= 0x00 ? p_sys->i_u - p_sys->i_ut : 0x00;
    umax = p_sys->i_u + p_sys->i_ut <= 0xff ? p_sys->i_u + p_sys->i_ut : 0xff;
    vmin = p_sys->i_v - p_sys->i_vt >= 0x00 ? p_sys->i_v - p_sys->i_vt : 0x00;
    vmax = p_sys->i_v + p_sys->i_vt <= 0xff ? p_sys->i_v + p_sys->i_vt : 0xff;

    for( i = 0; i < i_lines*i_pitch; i++ )
    {
        if(    p_u[i] < umax && p_u[i] > umin
            && p_v[i] < vmax && p_v[i] > vmin )
        {
            p_at[i] = 0x00;
        }
        else
        {
            p_at[i] = 0xff;
        }
    }
    /* Gaussian convolution to make it look cleaner */
    vlc_memset( p_a, 0, 2 * i_pitch );
    for( i = 2; i < i_lines - 2; i++ )
    {
        p_a[i*i_pitch] = 0x00;
        p_a[i*i_pitch+1] = 0x00;
        for( j = 2; j < i_pitch - 2; j ++ )
        {
            p_a[i*i_pitch+j] = (uint8_t)((
              /* 2 rows up */
                ( p_at[(i-2)*i_pitch+j-2]<<1 )
              + ( p_at[(i-2)*i_pitch+j-1]<<2 )
              + ( p_at[(i-2)*i_pitch+j]<<2 )
              + ( p_at[(i-2)*i_pitch+j+1]<<2 )
              + ( p_at[(i-2)*i_pitch+j+2]<<1 )
              /* 1 row up */
              + ( p_at[(i-1)*i_pitch+j-2]<<2 )
              + ( p_at[(i-1)*i_pitch+j-1]<<3 )
              + ( p_at[(i-1)*i_pitch+j]*12 )
              + ( p_at[(i-1)*i_pitch+j+1]<<3 )
              + ( p_at[(i-1)*i_pitch+j+2]<<2 )
              /* */
              + ( p_at[i*i_pitch+j-2]<<2 )
              + ( p_at[i*i_pitch+j-1]*12 )
              + ( p_at[i*i_pitch+j]<<4 )
              + ( p_at[i*i_pitch+j+1]*12 )
              + ( p_at[i*i_pitch+j+2]<<2 )
              /* 1 row down */
              + ( p_at[(i+1)*i_pitch+j-2]<<2 )
              + ( p_at[(i+1)*i_pitch+j-1]<<3 )
              + ( p_at[(i+1)*i_pitch+j]*12 )
              + ( p_at[(i+1)*i_pitch+j+1]<<3 )
              + ( p_at[(i+1)*i_pitch+j+2]<<2 )
              /* 2 rows down */
              + ( p_at[(i+2)*i_pitch+j-2]<<1 )
              + ( p_at[(i+2)*i_pitch+j-1]<<2 )
              + ( p_at[(i+2)*i_pitch+j]<<2 )
              + ( p_at[(i+2)*i_pitch+j+1]<<2 )
              + ( p_at[(i+2)*i_pitch+j+2]<<1 )
              )/152);
              if( p_a[i*i_pitch+j] < 0xbf ) p_a[i*i_pitch+j] = 0x00;
        }
    }
    return p_pic;
}

/*****************************************************************************
* Callback to update params on the fly
*****************************************************************************/
static int BluescreenCallback( vlc_object_t *p_this, char const *psz_var,
                               vlc_value_t oldval, vlc_value_t newval,
                               void *p_data )
{
    VLC_UNUSED(p_this); VLC_UNUSED(oldval);
    filter_sys_t *p_sys = (filter_sys_t *) p_data;

#define VAR_IS( a ) !strcmp( psz_var, CFG_PREFIX a )
    if( VAR_IS( "u" ) )
    {
        p_sys->i_u = __MAX( 0, __MIN( 255, newval.i_int ) );
    }
    else if( VAR_IS( "v" ) )
    {
        p_sys->i_v = __MAX( 0, __MIN( 255, newval.i_int ) );
    }
    else if( VAR_IS( "ut" ) )
    {
        p_sys->i_ut = __MAX( 0, __MIN( 255, newval.i_int ) );
    }
    else if( VAR_IS( "vt" ) )
    {
        p_sys->i_vt = __MAX( 0, __MIN( 255, newval.i_int ) );
    }

    return VLC_SUCCESS;
}
