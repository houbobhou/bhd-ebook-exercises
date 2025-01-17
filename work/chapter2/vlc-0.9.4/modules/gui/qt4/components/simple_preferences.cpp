/*****************************************************************************
 * simple_preferences.cpp : "Simple preferences"
 ****************************************************************************
 * Copyright (C) 2006-2008 the VideoLAN team
 * $Id: d196990ea3053b2025487ac35cceb0085db89f7e $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Antoine Cellerier <dionoea@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "components/simple_preferences.hpp"
#include "components/preferences_widgets.hpp"

#include <vlc_config_cat.h>
#include <vlc_configuration.h>

#include <QString>
#include <QFont>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>

#include <QtAlgorithms>

#include <string>

#define ICON_HEIGHT 64
#define BUTTON_HEIGHT 74

/*********************************************************************
 * The List of categories
 *********************************************************************/
SPrefsCatList::SPrefsCatList( intf_thread_t *_p_intf, QWidget *_parent ) :
                                  QWidget( _parent ), p_intf( _p_intf )
{
    QVBoxLayout *layout = new QVBoxLayout();

    QButtonGroup *buttonGroup = new QButtonGroup( this );
    buttonGroup->setExclusive ( true );
    CONNECT( buttonGroup, buttonClicked ( int ),
            this, switchPanel( int ) );

#define ADD_CATEGORY( button, label, icon, numb )                           \
    QToolButton * button = new QToolButton( this );                         \
    button->setIcon( QIcon( ":/pixmaps/prefs/" #icon ) );                   \
    button->setIconSize( QSize( ICON_HEIGHT , ICON_HEIGHT ) );              \
    button->setText( label );                                               \
    button->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );              \
    button->resize( BUTTON_HEIGHT , BUTTON_HEIGHT);                         \
    button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding) ;  \
    button->setAutoRaise( true );                                           \
    button->setCheckable( true );                                           \
    buttonGroup->addButton( button, numb );                                 \
    layout->addWidget( button );

    ADD_CATEGORY( SPrefsInterface, qtr("Interface"),
                  spref_cone_Interface_64.png, 0 );
    ADD_CATEGORY( SPrefsAudio, qtr("Audio"), spref_cone_Audio_64.png, 1 );
    ADD_CATEGORY( SPrefsVideo, qtr("Video"), spref_cone_Video_64.png, 2 );
    ADD_CATEGORY( SPrefsSubtitles, qtr("Subtitles && OSD"),
                  spref_cone_Subtitles_64.png, 3 );
    ADD_CATEGORY( SPrefsInputAndCodecs, qtr("Input && Codecs"),
                  spref_cone_Input_64.png, 4 );
    ADD_CATEGORY( SPrefsHotkeys, qtr("Hotkeys"), spref_cone_Hotkeys_64.png, 5 );

    SPrefsInterface->setChecked( true );
    layout->setMargin( 0 );
    layout->setSpacing( 1 );

    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    setLayout( layout );

}

void SPrefsCatList::switchPanel( int i )
{
    emit currentItemChanged( i );
}

/*********************************************************************
 * The Panels
 *********************************************************************/
SPrefsPanel::SPrefsPanel( intf_thread_t *_p_intf, QWidget *_parent,
                          int _number ) : QWidget( _parent ), p_intf( _p_intf )
{
    module_config_t *p_config;
    ConfigControl *control;
    number = _number;

#define CONFIG_GENERIC( option, type, label, qcontrol )                   \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                           p_config, label, ui.qcontrol, false );         \
                controls.append( control );                               \
            }

#define CONFIG_GENERIC2( option, type, label, qcontrol )                   \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                           p_config, label, qcontrol, false );         \
                controls.append( control );                               \
            }


#define CONFIG_GENERIC_NO_BOOL( option, type, label, qcontrol )           \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                           p_config, label, ui.qcontrol );                \
                controls.append( control );                               \
            }

#define CONFIG_GENERIC_FILE( option, type, label, qcontrol, qbutton )         \
                p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
                if( p_config )                                                \
                {                                                             \
                    control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                               p_config, label, qcontrol, qbutton,      \
                            false );                                          \
                    controls.append( control );                               \
                }

#define START_SPREFS_CAT( name , label )    \
        case SPrefs ## name:                \
        {                                   \
            Ui::SPrefs ## name ui;      \
            ui.setupUi( panel );            \
            panel_label->setText( label );

#define END_SPREFS_CAT      \
            break;          \
        }

    QVBoxLayout *panel_layout = new QVBoxLayout();
    QWidget *panel = new QWidget();
    panel_layout->setMargin( 3 );

    // Title Label
    QLabel *panel_label = new QLabel;
    QFont labelFont = QApplication::font( static_cast<QWidget*>(0) );
    labelFont.setPointSize( labelFont.pointSize() + 6 );
    labelFont.setFamily( "Verdana" );
    panel_label->setFont( labelFont );

    // Title <hr>
    QFrame *title_line = new QFrame;
    title_line->setFrameShape(QFrame::HLine);
    title_line->setFrameShadow(QFrame::Sunken);

    QFont italicFont = QApplication::font( static_cast<QWidget*>(0) );
    italicFont.setItalic( true );

    switch( number )
    {
        /******************************
         * VIDEO Panel Implementation *
         ******************************/
        START_SPREFS_CAT( Video , qtr("General Video Settings") );
            CONFIG_GENERIC( "video", Bool, NULL, enableVideo );

            CONFIG_GENERIC( "fullscreen", Bool, NULL, fullscreen );
            CONFIG_GENERIC( "overlay", Bool, NULL, overlay );
            CONFIG_GENERIC( "video-on-top", Bool, NULL, alwaysOnTop );
            CONFIG_GENERIC( "video-deco", Bool, NULL, windowDecorations );
            CONFIG_GENERIC( "skip-frames" , Bool, NULL, skipFrames );
            CONFIG_GENERIC( "vout", Module, NULL, outputModule );

#ifdef WIN32
            CONFIG_GENERIC( "directx-wallpaper" , Bool , NULL, wallpaperMode );
            CONFIG_GENERIC( "directx-device", StringList, NULL,
                            dXdisplayDevice );
            CONFIG_GENERIC( "directx-hw-yuv", Bool, NULL, hwYUVBox );
#else
            ui.directXBox->setVisible( false );
            ui.hwYUVBox->setVisible( false );
#endif

            CONFIG_GENERIC_FILE( "snapshot-path", Directory, NULL,
                                 ui.snapshotsDirectory, ui.snapshotsDirectoryBrowse );
            CONFIG_GENERIC( "snapshot-prefix", String, NULL, snapshotsPrefix );
            CONFIG_GENERIC( "snapshot-sequential", Bool, NULL,
                            snapshotsSequentialNumbering );
            CONFIG_GENERIC( "snapshot-format", StringList, NULL,
                            snapshotsFormat );
         END_SPREFS_CAT;

        /******************************
         * AUDIO Panel Implementation *
         ******************************/
        START_SPREFS_CAT( Audio, qtr("General Audio Settings") );

            CONFIG_GENERIC( "audio", Bool, NULL, enableAudio );

#define audioCommon( name ) \
            QWidget * name ## Control = new QWidget( ui.outputAudioBox ); \
            QHBoxLayout * name ## Layout = new QHBoxLayout( name ## Control); \
            name ## Layout->setMargin( 0 ); \
            name ## Layout->setSpacing( 0 ); \
            QLabel * name ## Label = new QLabel( qtr( "Device:" ), name ## Control ); \
            name ## Label->setMinimumSize(QSize(100, 0)); \
            name ## Layout->addWidget( name ## Label ); \

#define audioControl( name) \
            audioCommon( name ) \
            QComboBox * name ## Device = new QComboBox( name ## Control ); \
            name ## Layout->addWidget( name ## Device ); \
            name ## Label->setBuddy( name ## Device ); \
            outputAudioLayout->addWidget( name ## Control, outputAudioLayout->rowCount(), 0, 1, -1 );

#define audioControl2( name) \
            audioCommon( name ) \
            QLineEdit * name ## Device = new QLineEdit( name ## Control ); \
            name ## Layout->addWidget( name ## Device ); \
            name ## Label->setBuddy( name ## Device ); \
            QPushButton * name ## Browse = new QPushButton( qtr( "Browse..." ), name ## Control); \
            name ## Layout->addWidget( name ## Browse ); \
            outputAudioLayout->addWidget( name ## Control, outputAudioLayout->rowCount(), 0, 1, -1 );

            /* hide if necessary */
            ui.lastfm_user_edit->hide();
            ui.lastfm_user_label->hide();
            ui.lastfm_pass_edit->hide();
            ui.lastfm_pass_label->hide();

            /* Build if necessary */
            QGridLayout * outputAudioLayout = qobject_cast<QGridLayout *>(ui.outputAudioBox->layout());
#ifdef WIN32
            audioControl( DirectX );
            optionWidgets.append( DirectXControl );
            CONFIG_GENERIC2( "directx-audio-device", IntegerList,
                    DirectXLabel, DirectXDevice );
#else
            if( module_Exists( p_intf, "alsa" ) )
            {
                audioControl( alsa );
                optionWidgets.append( alsaControl );

                CONFIG_GENERIC2( "alsadev" , StringList , alsaLabel,
                                alsaDevice );
            }
            else
                optionWidgets.append( NULL );
            if( module_Exists( p_intf, "oss" ) )
            {
                audioControl2( OSS );
                optionWidgets.append( OSSControl );
                CONFIG_GENERIC_FILE( "dspdev" , File , OSSLabel, OSSDevice,
                                 OSSBrowse );
            }
            else
                optionWidgets.append( NULL );
#endif

            /* General Audio Options */
            CONFIG_GENERIC_NO_BOOL( "volume" , IntegerRangeSlider, NULL,
                                     defaultVolume );
            CONNECT( ui.defaultVolume, valueChanged( int ),
                    this, updateAudioVolume( int ) );

            CONFIG_GENERIC( "audio-language" , String , NULL,
                            preferredAudioLanguage );

            CONFIG_GENERIC( "spdif", Bool, NULL, spdifBox );
            CONFIG_GENERIC( "qt-autosave-volume", Bool, NULL, saveVolBox );
            CONFIG_GENERIC( "force-dolby-surround" , IntegerList , NULL,
                            detectionDolby );

            CONFIG_GENERIC( "headphone-dolby" , Bool , NULL, headphoneEffect );

            CONFIG_GENERIC_NO_BOOL( "norm-max-level" , Float , NULL,
                                    volNormSpin );
            CONFIG_GENERIC( "audio-visual" , Module , NULL, visualisation);

            /* Audio Output Specifics */
            CONFIG_GENERIC( "aout", Module, NULL, outputModule );

            CONNECT( ui.outputModule, currentIndexChanged( int ),
                     this, updateAudioOptions( int ) );

            /* File output exists on all platforms */
            CONFIG_GENERIC_FILE( "audiofile-file" , File , ui.fileLabel,
                                 ui.fileName, ui.fileBrowseButton );

            optionWidgets.append( ui.fileControl );
            optionWidgets.append( ui.outputModule );
            optionWidgets.append( ui.volNormBox );
            /*Little mofification of ui.volumeValue to compile with Qt < 4.3 */
#if HAS_QT43
            ui.volumeValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
#endif
            optionWidgets.append( ui.volumeValue );
            updateAudioOptions( ui.outputModule->currentIndex() );

            /* LastFM */
            if( module_Exists( p_intf, "audioscrobbler" ) )
            {
                CONFIG_GENERIC( "lastfm-username", String, ui.lastfm_user_label,
                        lastfm_user_edit );
                CONFIG_GENERIC( "lastfm-password", String, ui.lastfm_pass_label,
                        lastfm_pass_edit );

                if( config_ExistIntf( VLC_OBJECT( p_intf ), "audioscrobbler" ) )
                    ui.lastfm->setChecked( true );
                else
                    ui.lastfm->setChecked( false );
                CONNECT( ui.lastfm, stateChanged( int ), this ,
                        lastfm_Changed( int ) );
            }
            else
                ui.lastfm->hide();

            /* Normalizer */

            CONNECT( ui.volNormBox, toggled( bool ), ui.volNormSpin,
                     setEnabled( bool ) );
            char* psz = config_GetPsz( p_intf, "audio-filter" );
            qs_filter = qfu( psz );
            free( psz );
            bool b_normalizer = ( qs_filter.contains( "volnorm" ) );
            {
                ui.volNormBox->setChecked( b_normalizer );
                ui.volNormSpin->setEnabled( b_normalizer );
            }

            /* Volume Label */
            updateAudioVolume( ui.defaultVolume->value() ); // First time init

        END_SPREFS_CAT;

        /* Input and Codecs Panel Implementation */
        START_SPREFS_CAT( InputAndCodecs, qtr("Input & Codecs Settings") );

            /* Disk Devices */
            {
                ui.DVDDevice->setToolTip(
                    qtr( "If this property is blank, different values\n"
                         "for DVD, VCD, and CDDA are set.\n"
                         "You can define a unique one or configure them \n"
                         "individually in the advanced preferences." ) );
                char *psz_dvddiscpath = config_GetPsz( p_intf, "dvd" );
                char *psz_vcddiscpath = config_GetPsz( p_intf, "vcd" );
                char *psz_cddadiscpath = config_GetPsz( p_intf, "cd-audio" );
                if( psz_dvddiscpath && psz_vcddiscpath && psz_cddadiscpath )
                if( !strcmp( psz_cddadiscpath, psz_dvddiscpath ) &&
                    !strcmp( psz_dvddiscpath, psz_vcddiscpath ) )
                {
                    ui.DVDDevice->setText( qfu( psz_dvddiscpath ) );
                }
                free( psz_cddadiscpath );
                free( psz_dvddiscpath );
                free( psz_vcddiscpath );
            }

            CONFIG_GENERIC_NO_BOOL( "server-port", Integer, NULL, UDPPort );
            CONFIG_GENERIC( "http-proxy", String , NULL, proxy );
            CONFIG_GENERIC_NO_BOOL( "ffmpeg-pp-q", Integer, NULL, PostProcLevel );
            CONFIG_GENERIC( "avi-index", IntegerList, NULL, AviRepair );
            CONFIG_GENERIC( "rtsp-tcp", Bool, NULL, RTSP_TCPBox );
#ifdef WIN32
            CONFIG_GENERIC( "prefer-system-codecs", Bool, NULL, systemCodecBox );
#else
            ui.systemCodecBox->hide();
#endif
            /* Access Filters */
            char* psz = config_GetPsz( p_intf, "access-filter" );
            qs_filter = qfu( psz );
            free( psz );
            ui.timeshiftBox->setChecked( qs_filter.contains( "timeshift" ) );
            ui.dumpBox->setChecked( qs_filter.contains( "dump" ) );
            ui.recordBox->setChecked( qs_filter.contains( "record" ) );
            ui.bandwidthBox->setChecked( qs_filter.contains( "bandwidth" ) );

            optionWidgets.append( ui.recordBox );
            optionWidgets.append( ui.dumpBox );
            optionWidgets.append( ui.bandwidthBox );
            optionWidgets.append( ui.timeshiftBox );
            optionWidgets.append( ui.DVDDevice );
            optionWidgets.append( ui.cachingCombo );

            /* Caching */
            /* Add the things to the ComboBox */
            #define addToCachingBox( str, cachingNumber ) \
                ui.cachingCombo->addItem( qtr(str), QVariant( cachingNumber ) );
            addToCachingBox( N_("Custom"), CachingCustom );
            addToCachingBox( N_("Lowest latency"), CachingLowest );
            addToCachingBox( N_("Low latency"), CachingLow );
            addToCachingBox( N_("Normal"), CachingNormal );
            addToCachingBox( N_("High latency"), CachingHigh );
            addToCachingBox( N_("Higher latency"), CachingHigher );

#define TestCaC( name ) \
    b_cache_equal =  b_cache_equal && \
     ( i_cache == config_GetInt( p_intf, name ) )

#define TestCaCi( name, int ) \
    b_cache_equal = b_cache_equal &&  \
    ( ( i_cache * int ) == config_GetInt( p_intf, name ) )
            /* Select the accurate value of the ComboBox */
            bool b_cache_equal = true;
            int i_cache = config_GetInt( p_intf, "file-caching");

            TestCaC( "udp-caching" );
            if (module_Exists (p_intf, "dvdread"))
                TestCaC( "dvdread-caching" );
            if (module_Exists (p_intf, "dvdnav"))
                TestCaC( "dvdnav-caching" );
            TestCaC( "tcp-caching" );
            TestCaC( "fake-caching" ); TestCaC( "cdda-caching" );
            TestCaC( "screen-caching" ); TestCaC( "vcd-caching" );
            #ifdef WIN32
            TestCaC( "dshow-caching" );
            #else
            if (module_Exists (p_intf, "v4l"))
                TestCaC( "v4l-caching" );
            if (module_Exists (p_intf, "access_jack"))
                TestCaC( "jack-input-caching" );
            if (module_Exists (p_intf, "v4l2"))
                TestCaC( "v4l2-caching" );
            if (module_Exists (p_intf, "pvr"))
                TestCaC( "pvr-caching" );
            #endif
            TestCaCi( "rtsp-caching", 4 ); TestCaCi( "ftp-caching", 2 );
            TestCaCi( "http-caching", 4 );
            if (module_Exists (p_intf, "access_realrtsp"))
                TestCaCi( "realrtsp-caching", 10 );
            TestCaCi( "mms-caching", 19 );
            if( b_cache_equal ) ui.cachingCombo->setCurrentIndex(
                ui.cachingCombo->findData( QVariant( i_cache ) ) );

        END_SPREFS_CAT;
        /*******************
         * Interface Panel *
         *******************/
        START_SPREFS_CAT( Interface, qtr("Interface Settings") );
            ui.defaultLabel->setFont( italicFont );
            ui.skinsLabel->setFont( italicFont );

#if defined( WIN32 )
            CONFIG_GENERIC( "language", StringList, NULL, language );
            BUTTONACT( ui.assoButton, assoDialog() );
#else
            ui.language->hide();
            ui.languageLabel->hide();
            ui.assoName->hide();
            ui.assoButton->hide();
#endif

            /* interface */
            char *psz_intf = config_GetPsz( p_intf, "intf" );
            if( psz_intf )
            {
                if( strstr( psz_intf, "skin" ) )
                    ui.skins->setChecked( true );
                else if( strstr( psz_intf, "qt" ) )
                    ui.qt4->setChecked( true );
            }
            free( psz_intf );

            optionWidgets.append( ui.skins );
            optionWidgets.append( ui.qt4 );

            CONFIG_GENERIC( "qt-display-mode", IntegerList, NULL,
                            displayModeBox );
            CONFIG_GENERIC( "embedded-video", Bool, NULL, embedVideo );
            CONFIG_GENERIC( "qt-fs-controller", Bool, NULL, fsController );
            CONFIG_GENERIC( "qt-system-tray", Bool, NULL, systrayBox );
            CONFIG_GENERIC_FILE( "skins2-last", File, NULL, ui.fileSkin,
                    ui.skinBrowse );

            CONFIG_GENERIC( "album-art", IntegerList, ui.artFetchLabel,
                                                      artFetcher );

            /* UPDATE options */
#ifdef UPDATE_CHECK
            CONFIG_GENERIC( "qt-updates-notif", Bool, NULL, updatesBox );
            CONFIG_GENERIC_NO_BOOL( "qt-updates-days", Integer, NULL,
                    updatesDays );
            CONNECT( ui.updatesBox, toggled( bool ),
                     ui.updatesDays, setEnabled( bool ) );
#else
            ui.updatesBox->hide();
            ui.updatesDays->hide();
#endif
            /* ONE INSTANCE options */
#if defined( WIN32 ) || defined( HAVE_DBUS ) || defined(__APPLE__)
            CONFIG_GENERIC( "one-instance", Bool, NULL, OneInterfaceMode );
            CONFIG_GENERIC( "playlist-enqueue", Bool, NULL,
                    EnqueueOneInterfaceMode );
#else
            ui.OneInterfaceBox->hide();
#endif
        END_SPREFS_CAT;

        START_SPREFS_CAT( Subtitles, qtr("Subtitles & On Screen Display Settings") );
            CONFIG_GENERIC( "osd", Bool, NULL, OSDBox);

            CONFIG_GENERIC( "subsdec-encoding", StringList, NULL, encoding );
            CONFIG_GENERIC( "sub-language", String, NULL, preferredLanguage );
            CONFIG_GENERIC_FILE( "freetype-font", File, NULL, ui.font,
                            ui.fontBrowse );
            CONFIG_GENERIC( "freetype-color", IntegerList, NULL, fontColor );
            CONFIG_GENERIC( "freetype-rel-fontsize", IntegerList, NULL,
                            fontSize );
            CONFIG_GENERIC( "freetype-effect", IntegerList, NULL, effect );
            CONFIG_GENERIC_NO_BOOL( "sub-margin", Integer, NULL, subsPosition );

        END_SPREFS_CAT;

        case SPrefsHotkeys:
        {
            p_config = config_FindConfig( VLC_OBJECT(p_intf), "key-fullscreen" );

            QGridLayout *gLayout = new QGridLayout;
            panel->setLayout( gLayout );
            int line = 0;

            control = new KeySelectorControl( VLC_OBJECT(p_intf), p_config ,
                                                this, gLayout, line );

            panel_label->setText( qtr( "Configure Hotkeys" ) );
            controls.append( control );

            break;
        }
    }

    panel_layout->addWidget( panel_label );
    panel_layout->addWidget( title_line );
    panel_layout->addWidget( panel );
    if( number != SPrefsHotkeys ) panel_layout->addStretch( 2 );

    setLayout( panel_layout );
}

void SPrefsPanel::updateAudioOptions( int number)
{
    QString value = qobject_cast<QComboBox *>(optionWidgets[audioOutCoB])
                                            ->itemData( number ).toString();
#ifdef WIN32
    optionWidgets[directxW]->setVisible( ( value == "directx" ) );
#else
    /* optionWidgets[ossW] can be NULL */
    if( optionWidgets[ossW] )
        optionWidgets[ossW]->setVisible( ( value == "oss" ) );
    /* optionWidgets[alsaW] can be NULL */
    if( optionWidgets[alsaW] )
        optionWidgets[alsaW]->setVisible( ( value == "alsa" ) );
#endif
    optionWidgets[fileW]->setVisible( ( value == "aout_file" ) );
}


SPrefsPanel::~SPrefsPanel()
{
    qDeleteAll( controls ); controls.clear();
}

void SPrefsPanel::updateAudioVolume( int volume )
{
    qobject_cast<QSpinBox *>(optionWidgets[volLW])
        ->setValue( volume * 100 / 256 );
}


/* Function called from the main Preferences dialog on each SPrefs Panel */
void SPrefsPanel::apply()
{
    /* Generic save for ever panel */
    QList<ConfigControl *>::Iterator i;
    for( i = controls.begin() ; i != controls.end() ; i++ )
    {
        ConfigControl *c = qobject_cast<ConfigControl *>(*i);
        c->doApply( p_intf );
    }

    switch( number )
    {
    case SPrefsInputAndCodecs:
    {
        /* Device default selection */
        char *psz_devicepath =
              qtu( qobject_cast<QLineEdit *>(optionWidgets[inputLE] )->text() );
        if( !EMPTY_STR( psz_devicepath ) )
        {
            config_PutPsz( p_intf, "dvd", psz_devicepath );
            config_PutPsz( p_intf, "vcd", psz_devicepath );
            config_PutPsz( p_intf, "cd-audio", psz_devicepath );
        }

        /* Access filters */
#define saveBox( name, box ) {\
        if( box->isChecked() ) { \
            if( b_first ) { \
                qs_filter.append( name ); \
                b_first = false; \
            } \
            else qs_filter.append( ":" ).append( name ); \
        } }

        bool b_first = true;
        qs_filter.clear();
        saveBox( "record", qobject_cast<QCheckBox *>(optionWidgets[recordChB]) );
        saveBox( "dump", qobject_cast<QCheckBox *>(optionWidgets[dumpChB]) );
        saveBox( "timeshift", qobject_cast<QCheckBox *>(optionWidgets[timeshiftChB]) );
        saveBox( "bandwidth", qobject_cast<QCheckBox *>(optionWidgets[bandwidthChB] ) );
        config_PutPsz( p_intf, "access-filter", qtu( qs_filter ) );

#define CaCi( name, int ) config_PutInt( p_intf, name, int * i_comboValue )
#define CaC( name ) CaCi( name, 1 )
        /* Caching */
        QComboBox *cachingCombo = qobject_cast<QComboBox *>(optionWidgets[cachingCoB]);
        int i_comboValue = cachingCombo->itemData( cachingCombo->currentIndex() ).toInt();
        if( i_comboValue )
        {
            CaC( "udp-caching" );
            if (module_Exists (p_intf, "dvdread" ))
                CaC( "dvdread-caching" );
            if (module_Exists (p_intf, "dvdnav" ))
                CaC( "dvdnav-caching" );
            CaC( "tcp-caching" ); CaC( "vcd-caching" );
            CaC( "fake-caching" ); CaC( "cdda-caching" ); CaC( "file-caching" );
            CaC( "screen-caching" );
            CaCi( "rtsp-caching", 4 ); CaCi( "ftp-caching", 2 );
            CaCi( "http-caching", 4 );
            if (module_Exists (p_intf, "access_realrtsp" ))
                CaCi( "realrtsp-caching", 10 );
            CaCi( "mms-caching", 19 );
            #ifdef WIN32
            CaC( "dshow-caching" );
            #else
            if (module_Exists (p_intf, "v4l" ))
                CaC( "v4l-caching" );
            if (module_Exists (p_intf, "access_jack" ))
            CaC( "jack-input-caching" );
            if (module_Exists (p_intf, "v4l2" ))
                CaC( "v4l2-caching" );
            if (module_Exists (p_intf, "pvr" ))
                CaC( "pvr-caching" );
            #endif
            //CaCi( "dv-caching" ) too short...
        }
        break;
    }

    /* Interfaces */
    case SPrefsInterface:
    {
        if( qobject_cast<QRadioButton *>(optionWidgets[skinRB])->isChecked() )
            config_PutPsz( p_intf, "intf", "skins2" );
        if( qobject_cast<QRadioButton *>(optionWidgets[qtRB])->isChecked() )
            config_PutPsz( p_intf, "intf", "qt4" );
        break;
    }

    case SPrefsAudio:
    {
        bool b_normChecked =
            qobject_cast<QCheckBox *>(optionWidgets[normalizerChB])->isChecked();
        if( qs_filter.isEmpty() )
        {
            /* the psz_filter is already empty, so we just append it needed */
            if( b_normChecked ) qs_filter = "volnorm";
        }
        else /* Not Empty */
        {
            if( qs_filter.contains( "volnorm" ) )
            {
                /* The qs_filter not empty and contains "volnorm"
                   that we have to remove */
                if( !b_normChecked )
                {
                    /* Ugly :D */
                    qs_filter.remove( "volnorm:" );
                    qs_filter.remove( ":volnorm" );
                    qs_filter.remove( "volnorm" );
                }
            }
            else /* qs_filter not empty, but doesn't have volnorm inside */
                if( b_normChecked ) qs_filter.append( ":volnorm" );
        }
        config_PutPsz( p_intf, "audio-filter", qtu( qs_filter ) );
        break;
    }
    }
}

void SPrefsPanel::clean()
{}

void SPrefsPanel::lastfm_Changed( int i_state )
{
    if( i_state == Qt::Checked )
        config_AddIntf( VLC_OBJECT( p_intf ), "audioscrobbler" );
    else if( i_state == Qt::Unchecked )
        config_RemoveIntf( VLC_OBJECT( p_intf ), "audioscrobbler" );
}

#ifdef WIN32
#include <QDialogButtonBox>
#include <QHeaderView>
#include "util/registry.hpp"

bool SPrefsPanel::addType( const char * psz_ext, QTreeWidgetItem* current,
                           QTreeWidgetItem* parent, QVLCRegistry *qvReg )
{
    bool b_temp;
    const char* psz_VLC = "VLC";
    current = new QTreeWidgetItem( parent, QStringList( psz_ext ) );

    if( strstr( qvReg->ReadRegistryString( psz_ext, "", ""  ), psz_VLC ) )
    {
        current->setCheckState( 0, Qt::Checked );
        b_temp = false;
    }
    else
    {
        current->setCheckState( 0, Qt::Unchecked );
        b_temp = true;
    }
    listAsso.append( current );
    return b_temp;
}

void SPrefsPanel::assoDialog()
{
    QDialog *d = new QDialog( this );
    QGridLayout *assoLayout = new QGridLayout( d );

    QTreeWidget *filetypeList = new QTreeWidget;
    assoLayout->addWidget( filetypeList, 0, 0, 1, 4 );
    filetypeList->header()->hide();

    QVLCRegistry * qvReg = new QVLCRegistry( HKEY_CLASSES_ROOT );

    QTreeWidgetItem *audioType = new QTreeWidgetItem( QStringList( qtr( "Audio Files" ) ) );
    QTreeWidgetItem *videoType = new QTreeWidgetItem( QStringList( qtr( "Video Files" ) ) );
    QTreeWidgetItem *otherType = new QTreeWidgetItem( QStringList( qtr( "Playlist Files" ) ) );

    filetypeList->addTopLevelItem( audioType );
    filetypeList->addTopLevelItem( videoType );
    filetypeList->addTopLevelItem( otherType );

    audioType->setExpanded( true ); audioType->setCheckState( 0, Qt::Unchecked );
    videoType->setExpanded( true ); videoType->setCheckState( 0, Qt::Unchecked );
    otherType->setExpanded( true ); otherType->setCheckState( 0, Qt::Unchecked );

    QTreeWidgetItem *currentItem;

    int i_temp = 0;
#define aTa( name ) i_temp += addType( name, currentItem, audioType, qvReg )
#define aTv( name ) i_temp += addType( name, currentItem, videoType, qvReg )
#define aTo( name ) i_temp += addType( name, currentItem, otherType, qvReg )

    aTa( ".a52" ); aTa( ".aac" ); aTa( ".ac3" ); aTa( ".dts" ); aTa( ".flac" );
    aTa( ".m4a" ); aTa( ".m4p" ); aTa( ".mka" ); aTa( ".mod" ); aTa( ".mp1" );
    aTa( ".mp2" ); aTa( ".mp3" ); aTa( ".oma" ); aTa( ".ogg" ); aTa( ".spx" );
    aTa( ".wav" ); aTa( ".wma" ); aTa( ".xm" );
    audioType->setCheckState( 0, ( i_temp > 0 ) ?
                              ( ( i_temp == audioType->childCount() ) ?
                               Qt::Checked : Qt::PartiallyChecked )
                            : Qt::Unchecked );

    i_temp = 0;
    aTv( ".asf" ); aTv( ".avi" ); aTv( ".divx" ); aTv( ".dv" ); aTv( ".flv" );
    aTv( ".gxf" ); aTv( ".m1v" ); aTv( ".m2v" ); aTv( ".m2ts" ); aTv( ".m4v" );
    aTv( ".mkv" ); aTv( ".mov" ); aTv( ".mp2" ); aTv( ".mp4" ); aTv( ".mpeg" );
    aTv( ".mpeg1" ); aTv( ".mpeg2" ); aTv( ".mpeg4" ); aTv( ".mpg" );
    aTv( ".mts" ); aTv( ".mxf" ); aTv( ".ogm" ); aTv( ".ts" );
    aTv( ".vob" ); aTv( ".wmv" );
    videoType->setCheckState( 0, ( i_temp > 0 ) ?
                              ( ( i_temp == audioType->childCount() ) ?
                               Qt::Checked : Qt::PartiallyChecked )
                            : Qt::Unchecked );

    i_temp = 0;
    aTo( ".asx" ); aTo( ".b4s" ); aTo( ".m3u" ); aTo( ".pls" ); aTo( ".vlc" );
    aTo( ".xspf" );
    otherType->setCheckState( 0, ( i_temp > 0 ) ?
                              ( ( i_temp == audioType->childCount() ) ?
                               Qt::Checked : Qt::PartiallyChecked )
                            : Qt::Unchecked );

    QDialogButtonBox *buttonBox = new QDialogButtonBox( d );
    QPushButton *closeButton = new QPushButton( qtr( "&Apply" ) );
    QPushButton *clearButton = new QPushButton( qtr( "&Cancel" ) );
    buttonBox->addButton( closeButton, QDialogButtonBox::AcceptRole );
    buttonBox->addButton( clearButton, QDialogButtonBox::ActionRole );

    assoLayout->addWidget( buttonBox, 1, 2, 1, 2 );

    CONNECT( closeButton, clicked(), this, saveAsso() );
    CONNECT( clearButton, clicked(), d, reject() );
    d->resize( 300, 400 );
    d->exec();
    delete d;
    delete qvReg;
    listAsso.clear();
}

void addAsso( QVLCRegistry *qvReg, char *psz_ext )
{
    std::string s_path( "VLC" ); s_path += psz_ext;
    std::string s_path2 = s_path;

    /* Save a backup if already assigned */
    char *psz_value = qvReg->ReadRegistryString( psz_ext, "", ""  );

    if( psz_value && strlen( psz_value ) > 0 )
        qvReg->WriteRegistryString( psz_ext, "VLC.backup", psz_value );
    delete psz_value;

    /* Put a "link" to VLC.EXT as default */
    qvReg->WriteRegistryString( psz_ext, "", s_path.c_str() );

    /* Create the needed Key if they weren't done in the installer */
    if( !qvReg->RegistryKeyExists( s_path.c_str() ) )
    {
        qvReg->WriteRegistryString( psz_ext, "", s_path.c_str() );
        qvReg->WriteRegistryString( s_path.c_str(), "", "Media file" );
        qvReg->WriteRegistryString( s_path.append( "\\shell" ).c_str() , "", "Play" );

        /* Get the installer path */
        QVLCRegistry *qvReg2 = new QVLCRegistry( HKEY_LOCAL_MACHINE );
        std::string str_temp; str_temp.assign(
            qvReg2->ReadRegistryString( "Software\\VideoLAN\\VLC", "", "" ) );

        if( str_temp.size() )
        {
            qvReg->WriteRegistryString( s_path.append( "\\Play\\command" ).c_str(),
                "", str_temp.append(" --started-from-file \"%1\"" ).c_str() );

            qvReg->WriteRegistryString( s_path2.append( "\\DefaultIcon" ).c_str(),
                        "", str_temp.append(",0").c_str() );
        }
        delete qvReg2;
    }
}

void delAsso( QVLCRegistry *qvReg, char *psz_ext )
{
    char psz_VLC[] = "VLC";
    char *psz_value = qvReg->ReadRegistryString( psz_ext, "", ""  );

    if( psz_value && !strcmp( strcat( psz_VLC, psz_ext ), psz_value ) )
    {
        free( psz_value );
        psz_value = qvReg->ReadRegistryString( psz_ext, "VLC.backup", "" );
        if( psz_value )
            qvReg->WriteRegistryString( psz_ext, "", psz_value );

        qvReg->DeleteKey( psz_ext, "VLC.backup" );
    }
    delete( psz_value );
}
void SPrefsPanel::saveAsso()
{
    QVLCRegistry * qvReg;
    for( int i = 0; i < listAsso.size(); i ++ )
    {
        qvReg  = new QVLCRegistry( HKEY_CLASSES_ROOT );
        if( listAsso[i]->checkState( 0 ) > 0 )
        {
            addAsso( qvReg, qtu( listAsso[i]->text( 0 ) ) );
        }
        else
        {
            delAsso( qvReg, qtu( listAsso[i]->text( 0 ) ) );
        }
    }
    /* Gruik ? Naaah */
    qobject_cast<QDialog *>(listAsso[0]->treeWidget()->parent())->accept();
    delete qvReg;
}

#endif /* WIN32 */

