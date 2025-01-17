/*****************************************************************************
 r playlistinfo.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2002-2008 the VideoLAN team
 * $Id: f0f84553d7805308cab210ff82333051b4c9ff11 $
 *
 * Authors: Benjamin Pracht <bigben at videolan dot org>
 *          Felix Paul Kühne <fkuehne at videolan dot org>
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

#include "intf.h"
#include "playlistinfo.h"
#include "playlist.h"

/*****************************************************************************
 * VLCPlaylistInfo Implementation
 *****************************************************************************/

@implementation VLCInfo

static VLCInfo *_o_sharedInstance = nil;

+ (VLCInfo *)sharedInstance
{
    return _o_sharedInstance ? _o_sharedInstance : [[self alloc] init];
}

- (id)init
{
    if( _o_sharedInstance ) {
        [self dealloc];
    } else {
        _o_sharedInstance = [super init];
        
        if( _o_sharedInstance != nil )
        {
            p_item = NULL;
            o_statUpdateTimer = nil;
            [self updatePanelWithItem: NULL];
            rootItem = [[VLCInfoTreeItem alloc] init];
        }
    }

    return _o_sharedInstance;
}

- (void)awakeFromNib
{
    [o_info_window setExcludedFromWindowsMenu: YES];
    [o_info_window setFloatingPanel: NO];

    [o_info_window setTitle: _NS("Media Information")];
    [o_uri_lbl setStringValue: _NS("Location")];
    [o_title_lbl setStringValue: _NS("Title")];
    [o_author_lbl setStringValue: _NS("Artist")];
    [o_saveMetaData_btn setStringValue: _NS("Save Metadata" )];

    [[o_tab_view tabViewItemAtIndex: 0] setLabel: _NS("General")];
    [[o_tab_view tabViewItemAtIndex: 1] setLabel: _NS("Codec Details")];
    [[o_tab_view tabViewItemAtIndex: 2] setLabel: _NS("Statistics")];
    [o_tab_view selectTabViewItemAtIndex: 0];

    /* constants defined in vlc_meta.h */
    [o_genre_lbl setStringValue: _NS(VLC_META_GENRE)];
    [o_copyright_lbl setStringValue: _NS(VLC_META_COPYRIGHT)];
    [o_collection_lbl setStringValue: _NS(VLC_META_ALBUM)];
    [o_seqNum_lbl setStringValue: _NS(VLC_META_TRACK_NUMBER)];
    [o_description_lbl setStringValue: _NS(VLC_META_DESCRIPTION)];
    [o_date_lbl setStringValue: _NS(VLC_META_DATE)];
    [o_language_lbl setStringValue: _NS(VLC_META_LANGUAGE)];
    [o_nowPlaying_lbl setStringValue: _NS(VLC_META_NOW_PLAYING)];
    [o_publisher_lbl setStringValue: _NS(VLC_META_PUBLISHER)];

    /* statistics */
    [o_input_box setTitle: _NS("Input")];
    [o_read_bytes_lbl setStringValue: _NS("Read at media")];
    [o_input_bitrate_lbl setStringValue: _NS("Input bitrate")];
    [o_demux_bytes_lbl setStringValue: _NS("Demuxed")];
    [o_demux_bitrate_lbl setStringValue: _NS("Stream bitrate")];

    [o_video_box setTitle: _NS("Video")];
    [o_video_decoded_lbl setStringValue: _NS("Decoded blocks")];
    [o_displayed_lbl setStringValue: _NS("Displayed frames")];
    [o_lost_frames_lbl setStringValue: _NS("Lost frames")];
	[o_fps_lbl setStringValue: _NS("Frames per Second")];

    [o_sout_box setTitle: _NS("Streaming")];
    [o_sent_packets_lbl setStringValue: _NS("Sent packets")];
    [o_sent_bytes_lbl setStringValue: _NS("Sent bytes")];
    [o_sent_bitrate_lbl setStringValue: _NS("Send rate")];

    [o_audio_box setTitle: _NS("Audio")];
    [o_audio_decoded_lbl setStringValue: _NS("Decoded blocks")];
    [o_played_abuffers_lbl setStringValue: _NS("Played buffers")];
    [o_lost_abuffers_lbl setStringValue: _NS("Lost buffers")];

    [o_info_window setInitialFirstResponder: o_uri_txt];
    [o_info_window setDelegate: self];

    /* We may be awoken from nib way after initialisation
     * Update ourselves */
    [self updatePanelWithItem:p_item];
}

- (void)stopTimers
{
    /* make sure that the timer is released in any case */
    if( o_statUpdateTimer && [o_statUpdateTimer isValid] )
        [o_statUpdateTimer invalidate];

    [rootItem release];

    [o_statUpdateTimer release];
    o_statUpdateTimer = nil;
}

- (void)dealloc
{
    /* make sure that the timer is released in any case */
    if( o_statUpdateTimer && [o_statUpdateTimer isValid] )
        [o_statUpdateTimer invalidate];

    [o_statUpdateTimer release];

    if( p_item ) vlc_gc_decref( p_item );

    [super dealloc];
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
    BOOL b_stats = config_GetInt(VLCIntf, "stats");
    if( b_stats )
    {
        if( o_statUpdateTimer )
        {
            [o_statUpdateTimer invalidate];
            [o_statUpdateTimer release];
            o_statUpdateTimer = nil;
        }
        o_statUpdateTimer = [NSTimer scheduledTimerWithTimeInterval: 1
            target: self selector: @selector(updateStatistics:)
            userInfo: nil repeats: YES];
        [o_statUpdateTimer fire];
        [o_statUpdateTimer retain];
    }
}

- (BOOL)windowShouldClose:(id)sender
{
    if( [o_statUpdateTimer isValid] )
        [o_statUpdateTimer invalidate];

    if( o_statUpdateTimer )
        [o_statUpdateTimer release];
    o_statUpdateTimer = nil;
    return YES;
}

- (void)initPanel
{
    BOOL b_stats = config_GetInt(VLCIntf, "stats");
    if( !b_stats )
    {
        if( [o_tab_view numberOfTabViewItems] > 2 )
            [o_tab_view removeTabViewItem: [o_tab_view tabViewItemAtIndex: 2]];
    }

    [self initMediaPanelStats];
    [o_info_window makeKeyAndOrderFront: self];
}

- (void)initMediaPanelStats
{
    //Initializing Input Variables
    [o_read_bytes_txt setStringValue: [NSString stringWithFormat:@"%8.0f kB", (float)0]];
    [o_input_bitrate_txt setStringValue: [NSString stringWithFormat:@"%6.0f kb/s", (float)0]];
    [o_demux_bytes_txt setStringValue: [NSString stringWithFormat:@"%8.0f kB", (float)0]];
    [o_demux_bitrate_txt setStringValue: [NSString stringWithFormat:@"%6.0f kb/s", (float)0]];
    
    //Initializing Video Variables
    [o_video_decoded_txt setIntValue:0];
    [o_displayed_txt setIntValue:0];
    [o_lost_frames_txt setIntValue:0];
    [o_fps_txt setFloatValue:0];

    //Initializing Output Variables
    [o_sent_packets_txt setIntValue: 0];
    [o_sent_bytes_txt setStringValue: [NSString stringWithFormat:@"%8.0f kB", (float)0]];
    [o_sent_bitrate_txt setStringValue: [NSString stringWithFormat:@"%6.0f kb/s", (float)0]];

    //Initializing Audio Variables
    [o_audio_decoded_txt setIntValue:0];
    [o_played_abuffers_txt setIntValue: 0];
    [o_lost_abuffers_txt setIntValue: 0];

}

- (void)updatePanelWithItem:(input_item_t *)_p_item;
{
    if( _p_item != p_item )
    {
        if( p_item ) vlc_gc_decref( p_item );
        [o_saveMetaData_btn setEnabled: NO];
        if( _p_item ) vlc_gc_incref( _p_item );
        p_item = _p_item;
    }

    if( !p_item )
    {
        /* Erase */
    #define SET( foo ) \
        [self setMeta: "" forLabel: o_##foo##_txt];
        SET( uri );
        SET( title );
        SET( author );
        SET( collection );
        SET( seqNum );
        SET( genre );
        SET( copyright );
        SET( publisher );
        SET( nowPlaying );
        SET( language );
        SET( date );
        SET( description );
    #undef SET
        [o_image_well setImage: [NSImage imageNamed: @"noart.png"]];
    }
    else
    {
        if( !input_item_IsPreparsed( p_item ) )
        {
            playlist_t * p_playlist = pl_Yield( VLCIntf );
            playlist_PreparseEnqueue( p_playlist, p_item );
            pl_Release( VLCIntf );
        }

        /* fill uri info */
        char * psz_url = input_item_GetURI( p_item );
        [o_uri_txt setStringValue: [NSString stringWithUTF8String: psz_url ? psz_url : ""  ]];
        free( psz_url );

        /* fill title info */
        char * psz_title = input_item_GetTitle( p_item );
        if( !psz_title )
            psz_title = input_item_GetName( p_item );
        [o_title_txt setStringValue: [NSString stringWithUTF8String: psz_title ? : ""  ]];
        free( psz_title );

    #define SET( foo, bar ) \
        char *psz_##foo = input_item_Get##bar ( p_item ); \
        [self setMeta: psz_##foo forLabel: o_##foo##_txt]; \
        FREENULL( psz_##foo );

        /* fill the other fields */
        SET( author, Artist );
        SET( collection, Album );
        SET( seqNum, TrackNum );
        SET( genre, Genre );
        SET( copyright, Copyright );
        SET( publisher, Publisher );
        SET( nowPlaying, NowPlaying );
        SET( language, Language );
        SET( date, Date );
        SET( description, Description );

    #undef SET

        char *psz_meta;
        NSImage *o_image;
        psz_meta = input_item_GetArtURL( p_item );
        if( psz_meta && !strncmp( psz_meta, "file://", 7 ) )
            o_image = [[NSImage alloc] initWithContentsOfFile: [NSString stringWithUTF8String: psz_meta+7]];
        else
            o_image = [[NSImage imageNamed: @"noart.png"] retain];
        [o_image_well setImage: o_image];
        [o_image release];
        FREENULL( psz_meta );
    }

    /* reload the advanced table */
    [rootItem refresh];
    [o_outline_view reloadData];

    /* update the stats once to display p_item change faster */
    [self updateStatistics: nil];

}

- (void)setMeta: (char *)psz_meta forLabel: (id)theItem
{
    if( psz_meta != NULL && *psz_meta)
        [theItem setStringValue: [NSString stringWithUTF8String:psz_meta]];
    else
        [theItem setStringValue: @""];
}

- (void)updateStatistics:(NSTimer*)theTimer
{
    if( !p_item || !p_item->p_stats )
    {
        [self initMediaPanelStats];
        return;
    }

    vlc_mutex_lock( &p_item->p_stats->lock );

    /* input */
    [o_read_bytes_txt setStringValue: [NSString stringWithFormat:
        @"%8.0f kB", (float)(p_item->p_stats->i_read_bytes)/1000]];
    [o_input_bitrate_txt setStringValue: [NSString stringWithFormat:
        @"%6.0f kb/s", (float)(p_item->p_stats->f_input_bitrate)*8000]];
    [o_demux_bytes_txt setStringValue: [NSString stringWithFormat:
        @"%8.0f kB", (float)(p_item->p_stats->i_demux_read_bytes)/1000]];
    [o_demux_bitrate_txt setStringValue: [NSString stringWithFormat:
        @"%6.0f kb/s", (float)(p_item->p_stats->f_demux_bitrate)*8000]];

    /* Video */
    [o_video_decoded_txt setIntValue: p_item->p_stats->i_decoded_video];
    [o_displayed_txt setIntValue: p_item->p_stats->i_displayed_pictures];
    [o_lost_frames_txt setIntValue: p_item->p_stats->i_lost_pictures];
    float f_fps = 0;
    /* FIXME: input_Control( p_item, INPUT_GET_VIDEO_FPS, &f_fps ); */
    [o_fps_txt setFloatValue: f_fps];

    /* Sout */
    [o_sent_packets_txt setIntValue: p_item->p_stats->i_sent_packets];
    [o_sent_bytes_txt setStringValue: [NSString stringWithFormat: @"%8.0f kB",
        (float)(p_item->p_stats->i_sent_bytes)/1000]];
    [o_sent_bitrate_txt setStringValue: [NSString stringWithFormat:
        @"%6.0f kb/s", (float)(p_item->p_stats->f_send_bitrate*8)*1000]];

    /* Audio */
    [o_audio_decoded_txt setIntValue: p_item->p_stats->i_decoded_audio];
    [o_played_abuffers_txt setIntValue: p_item->p_stats->i_played_abuffers];
    [o_lost_abuffers_txt setIntValue: p_item->p_stats->i_lost_abuffers];

    vlc_mutex_unlock( &p_item->p_stats->lock );
}

- (IBAction)metaFieldChanged:(id)sender
{
    [o_saveMetaData_btn setEnabled: YES];
}

- (IBAction)saveMetaData:(id)sender
{
    playlist_t * p_playlist = pl_Yield( VLCIntf );
    vlc_value_t val;

    if( !p_item ) goto error;

    meta_export_t p_export;
    p_export.p_item = p_item;

    /* we can write meta data only in a file */
    vlc_mutex_lock( &p_item->lock );
    int i_type = p_item->i_type;
    vlc_mutex_unlock( &p_item->lock );

    if( i_type != ITEM_TYPE_FILE )
        goto error;

    char *psz_uri_orig = input_item_GetURI( p_item );
    char *psz_uri = psz_uri_orig;
    if( !strncmp( psz_uri, "file://", 7 ) )
        psz_uri += 7; /* strlen("file://") = 7 */

    p_export.psz_file = strndup( psz_uri, PATH_MAX );
    free( psz_uri_orig );

    #define utf8( o_blub ) \
        [[o_blub stringValue] UTF8String]

    input_item_SetName( p_item, utf8( o_title_txt ) );
    input_item_SetTitle( p_item, utf8( o_title_txt ) );
    input_item_SetArtist( p_item, utf8( o_author_txt ) );
    input_item_SetAlbum( p_item, utf8( o_collection_txt ) );
    input_item_SetGenre( p_item, utf8( o_genre_txt ) );
    input_item_SetTrackNum( p_item, utf8( o_seqNum_txt ) );
    input_item_SetDate( p_item, utf8( o_date_txt ) );
    input_item_SetCopyright( p_item, utf8( o_copyright_txt ) );
    input_item_SetPublisher( p_item, utf8( o_publisher_txt ) );
    input_item_SetDescription( p_item, utf8( o_description_txt ) );
    input_item_SetLanguage( p_item, utf8( o_language_txt ) );

    PL_LOCK;
    p_playlist->p_private = &p_export;

    module_t *p_mod = module_Need( p_playlist, "meta writer", NULL, 0 );
    if( p_mod )
        module_Unneed( p_playlist, p_mod );
    PL_UNLOCK;

    val.b_bool = true;
    var_Set( p_playlist, "intf-change", val );
    [self updatePanelWithItem: p_item];

    pl_Release( VLCIntf );
    [o_saveMetaData_btn setEnabled: NO];
    return;

error:
    pl_Release( VLCIntf );
    NSRunAlertPanel(_NS("Error while saving meta"),
        _NS("VLC was unable to save the meta data."),
        _NS("OK"), nil, nil);
}

- (IBAction)downloadCoverArt:(id)sender
{
    playlist_t * p_playlist = pl_Yield( VLCIntf );
    if( p_item) playlist_AskForArtEnqueue( p_playlist, p_item );
    pl_Release( VLCIntf );
}

- (input_item_t *)item
{
    if( p_item ) vlc_gc_incref( p_item );
    return p_item;
}

@end

@implementation VLCInfo (NSMenuValidation)

- (BOOL)validateMenuItem:(NSMenuItem *)o_mi
{
    BOOL bEnabled = TRUE;

    if( [[o_mi title] isEqualToString: _NS("Information")] )
    {
        return ![[[VLCMain sharedInstance] getPlaylist] isSelectionEmpty];
    }

    return TRUE;
}

@end

@implementation VLCInfo (NSTableDataSource)

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
    return (item == nil) ? [rootItem numberOfChildren] : [item numberOfChildren];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    return ([item numberOfChildren] > 0);
}

- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
{
    return (item == nil) ? [rootItem childAtIndex:index] : (id)[item childAtIndex:index];
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"0"])
    {
        return (item == nil) ? @"" : (id)[item name];
    }
    else
    {
        return (item == nil) ? @"" : (id)[item value];
    }
}


@end

@implementation VLCInfoTreeItem

#define IsALeafNode ((id)-1)

- (id)initWithName: (NSString *)o_item_name value: (NSString *)o_item_value ID: (int)i_id
       parent:(VLCInfoTreeItem *)o_parent_item
{
    self = [super init];

    if( self != nil )
    {
        o_name = [o_item_name copy];
        o_value = [o_item_value copy];
        i_object_id = i_id;
        o_parent = o_parent_item;
        p_item = [[[VLCMain sharedInstance] getInfo] item];
        o_children = nil;
    }
    return( self );
}

- (id)init
{
    return [self initWithName:@"main" value:@"" ID:-1 parent:nil];
}

- (void)dealloc
{
    if( o_children != IsALeafNode ) [o_children release];
    [o_name release];
    [o_value release];
    if( p_item ) vlc_gc_decref( p_item );
    [super dealloc];
}

/* Creates and returns the array of children
 * Loads children incrementally */
- (NSArray *)children
{
    if( !p_item ) return nil;

    if (o_children == NULL)
    {
        int i;

        if( i_object_id == -1 )
        {
            vlc_mutex_lock( &p_item->lock );
            o_children = [[NSMutableArray alloc] initWithCapacity: p_item->i_categories];
            for (i = 0 ; i < p_item->i_categories ; i++)
            {
                NSString * name = [NSString stringWithUTF8String: p_item->pp_categories[i]->psz_name];
                VLCInfoTreeItem * item = [[VLCInfoTreeItem alloc] initWithName:name value:@"" ID:i parent:self];
                [item autorelease];
                [o_children addObject:item];
            }
            vlc_mutex_unlock( &p_item->lock );
        }
        else if( o_parent->i_object_id == -1 )
        {
            vlc_mutex_lock( &p_item->lock );
            info_category_t * cat = p_item->pp_categories[i_object_id];
            o_children = [[NSMutableArray alloc] initWithCapacity: cat->i_infos];
            for (i = 0 ; i < cat->i_infos ; i++)
            {
                NSString * name = [NSString stringWithUTF8String: cat->pp_infos[i]->psz_name];
                NSString * value = [NSString stringWithUTF8String: cat->pp_infos[i]->psz_value ? : ""];
                VLCInfoTreeItem * item = [[VLCInfoTreeItem alloc] initWithName:name value:value ID:i parent:self];
                [item autorelease];
                [o_children addObject:item];
            }
            vlc_mutex_unlock( &p_item->lock );
        }
        else
        {
            o_children = IsALeafNode;
        }
    }
    return o_children;
}

- (NSString *)name
{
    return [[o_name retain] autorelease];
}

- (NSString *)value
{
    return [[o_value retain] autorelease];
}

- (void)refresh
{
    if( p_item ) vlc_gc_decref( p_item );
    p_item = [[[VLCMain sharedInstance] getInfo] item];
    if( o_children != NULL )
    {
        [o_children release];
        o_children = NULL;
    }
}

- (VLCInfoTreeItem *)childAtIndex:(int)i_index {
    return [[self children] objectAtIndex:i_index];
}

- (int)numberOfChildren {

    id i_tmp = [self children];
    return ( i_tmp == IsALeafNode ) ? (-1) : (int)[i_tmp count];
}

@end

