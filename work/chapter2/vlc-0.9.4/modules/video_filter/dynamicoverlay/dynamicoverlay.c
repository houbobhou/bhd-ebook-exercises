/*****************************************************************************
 * dynamicoverlay.c : dynamic overlay plugin for vlc
 *****************************************************************************
 * Copyright (C) 2007 the VideoLAN team
 * $Id: 02e9a56cd3dbc7a9b5691a81260185462c5b0a8f $
 *
 * Author: Soren Bog <avacore@videolan.org>
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
#include <vlc_sout.h>
#include <vlc_vout.h>
#include <vlc_filter.h>
#include <vlc_osd.h>

#include <ctype.h>
#include <fcntl.h>

#include "dynamicoverlay.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int Create( vlc_object_t * );
static void Destroy( vlc_object_t * );
static subpicture_t *Filter( filter_t *, mtime_t );

static int AdjustCallback( vlc_object_t *p_this, char const *psz_var,
                           vlc_value_t oldval, vlc_value_t newval,
                           void *p_data );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

#define INPUT_TEXT N_("Input FIFO")
#define INPUT_LONGTEXT N_("FIFO which will be read for commands")

#define OUTPUT_TEXT N_("Output FIFO")
#define OUTPUT_LONGTEXT N_("FIFO which will be written to for responses")

vlc_module_begin();
    set_description( N_("Dynamic video overlay") );
    set_shortname( N_("Overlay" ));
    set_category( CAT_VIDEO );
    set_subcategory( SUBCAT_VIDEO_VFILTER );
    set_capability( "sub filter", 0 );

    add_file( "overlay-input", NULL, NULL, INPUT_TEXT, INPUT_LONGTEXT,
              false );
    add_file( "overlay-output", NULL, NULL, OUTPUT_TEXT, OUTPUT_LONGTEXT,
              false );

    add_shortcut( "overlay" );
    set_callbacks( Create, Destroy );
vlc_module_end();

static const char *const ppsz_filter_options[] = {
    "input", "output", NULL
};

/*****************************************************************************
 * Create: allocates adjust video thread output method
 *****************************************************************************
 * This function allocates and initializes a adjust vout method.
 *****************************************************************************/
static int Create( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;
    filter_sys_t *p_sys;

    /* Allocate structure */
    p_filter->p_sys = malloc( sizeof( filter_sys_t ) );
    if( p_filter->p_sys == NULL )
        return VLC_ENOMEM;
    p_sys = p_filter->p_sys;

    BufferInit( &p_sys->input );
    BufferInit( &p_sys->output );
    QueueInit( &p_sys->atomic );
    QueueInit( &p_sys->pending );
    QueueInit( &p_sys->processed );
    ListInit( &p_sys->overlays );

    p_sys->i_inputfd = -1;
    p_sys->i_outputfd = -1;
    p_sys->b_updated = true;
    p_sys->b_atomic = false;

    p_filter->pf_sub_filter = Filter;

    config_ChainParse( p_filter, "overlay-", ppsz_filter_options,
                       p_filter->p_cfg );

    p_sys->psz_inputfile = var_CreateGetStringCommand( p_filter,
                                                       "overlay-input" );
    p_sys->psz_outputfile = var_CreateGetStringCommand( p_filter,
                                                        "overlay-output" );

    var_AddCallback( p_filter, "overlay-input", AdjustCallback, p_sys );
    var_AddCallback( p_filter, "overlay-output", AdjustCallback, p_sys );

    RegisterCommand( p_filter );
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Destroy: destroy adjust video thread output method
 *****************************************************************************
 * Terminate an output method created by adjustCreateOutputMethod
 *****************************************************************************/
static void Destroy( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;

    BufferDestroy( &p_filter->p_sys->input );
    BufferDestroy( &p_filter->p_sys->output );
    QueueDestroy( &p_filter->p_sys->atomic );
    QueueDestroy( &p_filter->p_sys->pending );
    QueueDestroy( &p_filter->p_sys->processed );
    ListDestroy( &p_filter->p_sys->overlays );
    UnregisterCommand( p_filter );

    free( p_filter->p_sys->psz_inputfile );
    free( p_filter->p_sys->psz_outputfile );
    free( p_filter->p_sys );
}

/*****************************************************************************
 * Render: displays previously rendered output
 *****************************************************************************
 * This function send the currently rendered image to adjust modified image,
 * waits until it is displayed and switch the two rendering buffers, preparing
 * next frame.
 *****************************************************************************/
static subpicture_t *Filter( filter_t *p_filter, mtime_t date )
{
    filter_sys_t *p_sys = p_filter->p_sys;

    /* We might need to open these at any time. */
    if( p_sys->i_inputfd == -1 )
    {
        p_sys->i_inputfd = open( p_sys->psz_inputfile, O_RDONLY | O_NONBLOCK );
        if( p_sys->i_inputfd == -1 )
        {
            msg_Warn( p_filter, "Failed to grab input file: %s (%s)",
                      p_sys->psz_inputfile, strerror( errno ) );
        }
        else
        {
            msg_Info( p_filter, "Grabbed input file: %s",
                      p_sys->psz_inputfile );
        }
    }

    if( p_sys->i_outputfd == -1 )
    {
        p_sys->i_outputfd = open( p_sys->psz_outputfile,
                                  O_WRONLY | O_NONBLOCK );
        if( p_sys->i_outputfd == -1 )
        {
            if( errno != ENXIO )
            {
                msg_Warn( p_filter, "Failed to grab output file: %s (%s)",
                          p_sys->psz_outputfile, strerror( errno ) );
            }
        }
        else
        {
            msg_Info( p_filter, "Grabbed output file: %s",
                      p_sys->psz_outputfile );
        }
    }

    /* Read any waiting commands */
    if( p_sys->i_inputfd != -1 )
    {
        char p_buffer[1024];
        ssize_t i_len = read( p_sys->i_inputfd, p_buffer, 1024 );
        if( i_len == -1 )
        {
            /* We hit an error */
            if( errno != EAGAIN )
            {
                msg_Warn( p_filter, "Error on input file: %s",
                          strerror( errno ) );
                close( p_sys->i_inputfd );
                p_sys->i_inputfd = -1;
            }
        }
        else if( i_len == 0 )
        {
            /* We hit the end-of-file */
        }
        else
        {
            BufferAdd( &p_sys->input, p_buffer, i_len );
        }
    }

    /* Parse any complete commands */
    char *p_end, *p_cmd;
    while( ( p_end = memchr( p_sys->input.p_begin, '\n',
                             p_sys->input.i_length ) ) )
    {
        commanddesc_t *p_cur = NULL;
        bool b_found = false;
        size_t i_index = 0;

        *p_end = '\0';
        p_cmd = BufferGetToken( &p_sys->input );

        msg_Info( p_filter, "Search command: %s", p_cmd );
        for( i_index = 0; i_index < p_sys->i_commands; i_index++ )
        {
            p_cur = p_sys->pp_commands[i_index];
            if( !strncmp( p_cur->psz_command, p_cmd, strlen(p_cur->psz_command) ) )
            {
                p_cmd[strlen(p_cur->psz_command)] = '\0';
                b_found = true;
                break;
            }
        }

        if( !b_found )
        {
            /* No matching command */
            msg_Err( p_filter, "Got invalid command: %s", p_cmd );
            BufferPrintf( &p_sys->output, "FAILURE: %d Invalid Command\n", VLC_EGENERIC );
        }
        else
        {
            msg_Info( p_filter, "Got valid command: %s", p_cmd );

            command_t *p_cmddesc = malloc( sizeof( command_t ) );
            if( !p_cmddesc )
                return NULL;

            p_cmd = p_cmd + strlen(p_cur->psz_command) +1;
            p_cmddesc->p_command = p_cur;
            p_cmddesc->p_command->pf_parser( p_cmd, p_end,
                                             &p_cmddesc->params );

            if( ( p_cmddesc->p_command->b_atomic == true ) &&
                ( p_sys->b_atomic == true ) )
                QueueEnqueue( &p_sys->atomic, p_cmddesc );
            else
                QueueEnqueue( &p_sys->pending, p_cmddesc );
        }

        BufferDel( &p_sys->input, p_end - p_sys->input.p_begin + 1 );
    }

    /* Process any pending commands */
    command_t *p_command = NULL;
    while( (p_command = QueueDequeue( &p_sys->pending )) )
    {
        p_command->i_status =
            p_command->p_command->pf_execute( p_filter, &p_command->params,
                                              &p_command->results );
        QueueEnqueue( &p_sys->processed, p_command );
    }

    /* Output any processed commands */
    while( (p_command = QueueDequeue( &p_sys->processed )) )
    {
        if( p_command->i_status == VLC_SUCCESS )
        {
            const char *psz_success = "SUCCESS:";
            const char *psz_nl = "\n";
            BufferAdd( &p_sys->output, psz_success, 8 );
            p_command->p_command->pf_unparse( &p_command->results,
                                              &p_sys->output );
            BufferAdd( &p_sys->output, psz_nl, 1 );
        }
        else
        {
            BufferPrintf( &p_sys->output, "FAILURE: %d\n",
                          p_command->i_status );
        }
    }

    /* Try emptying the output buffer */
    if( p_sys->i_outputfd != -1 )
    {
        ssize_t i_len = write( p_sys->i_outputfd, p_sys->output.p_begin,
                              p_sys->output.i_length );
        if( i_len == -1 )
        {
            /* We hit an error */
            if( errno != EAGAIN )
            {
                msg_Warn( p_filter, "Error on output file: %s",
                          strerror( errno ) );
                close( p_sys->i_outputfd );
                p_sys->i_outputfd = -1;
            }
        }
        else
        {
            BufferDel( &p_sys->output, i_len );
        }
    }

    if( p_sys->b_updated == false )
        return NULL;

    subpicture_t *p_spu = NULL;
    overlay_t *p_overlay = NULL;

    p_spu = p_filter->pf_sub_buffer_new( p_filter );
    if( !p_spu )
    {
        msg_Err( p_filter, "cannot allocate subpicture" );
        return NULL;
    }

    p_spu->i_flags = OSD_ALIGN_LEFT | OSD_ALIGN_TOP;
    p_spu->i_x = 0;
    p_spu->i_y = 0;
    p_spu->b_absolute = true;
    p_spu->i_start = date;
    p_spu->i_stop = 0;
    p_spu->b_ephemer = true;

    subpicture_region_t **pp_region = &p_spu->p_region;
    while( (p_overlay = ListWalk( &p_sys->overlays )) )
    {
        msg_Dbg( p_filter, "Displaying overlay: %4.4s, %d, %d, %d",
                 (char*)&p_overlay->format.i_chroma, p_overlay->i_x, p_overlay->i_y,
                 p_overlay->i_alpha );

        if( p_overlay->format.i_chroma == VLC_FOURCC('T','E','X','T') )
        {
            *pp_region = p_spu->pf_create_region( VLC_OBJECT(p_filter),
                                                  &p_overlay->format );
            if( !*pp_region )
                break;
            (*pp_region)->psz_text = strdup( p_overlay->data.p_text );
            (*pp_region)->p_style = malloc( sizeof(struct text_style_t) );
            if( !(*pp_region)->p_style )
            {
                p_spu->pf_destroy_region( VLC_OBJECT(p_filter), (*pp_region) );
                *pp_region = NULL;
                break;
            }
            vlc_memcpy( (*pp_region)->p_style, &p_overlay->fontstyle,
                        sizeof(struct text_style_t) );
        }
        else
        {
            picture_t clone;
            if( vout_AllocatePicture( p_filter, &clone,
                                      p_overlay->format.i_chroma,
                                      p_overlay->format.i_width,
                                      p_overlay->format.i_height,
                                      p_overlay->format.i_aspect ) )
            {
                msg_Err( p_filter, "cannot allocate picture" );
                continue;
            }
            vout_CopyPicture( p_filter, &clone, p_overlay->data.p_pic );
            *pp_region = p_spu->pf_make_region( VLC_OBJECT(p_filter),
                                                &p_overlay->format,
                                                &clone );
            if( !*pp_region )
            {
                msg_Err( p_filter, "cannot allocate subpicture region" );
                continue;
            }
        }
        (*pp_region)->i_x = p_overlay->i_x;
        (*pp_region)->i_y = p_overlay->i_y;
        (*pp_region)->i_align = OSD_ALIGN_LEFT | OSD_ALIGN_TOP;
        (*pp_region)->i_alpha = p_overlay->i_alpha;
        pp_region = &(*pp_region)->p_next;
    }

    p_sys->b_updated = false;
    return p_spu;
}

static int AdjustCallback( vlc_object_t *p_this, char const *psz_var,
                           vlc_value_t oldval, vlc_value_t newval,
                           void *p_data )
{
    filter_sys_t *p_sys = (filter_sys_t *)p_data;
    VLC_UNUSED(p_this); VLC_UNUSED(oldval);

    if( !strncmp( psz_var, "overlay-input", 13 ) )
        p_sys->psz_inputfile = newval.psz_string;
    else if( !strncmp( psz_var, "overlay-output", 14 ) )
        p_sys->psz_outputfile = newval.psz_string;

    return VLC_EGENERIC;
}
