/*****************************************************************************
 * preferences_widgets.cpp : Widgets for preferences displays
 ****************************************************************************
 * Copyright (C) 2006-2007 the VideoLAN team
 * $Id: 920196dd63a8d9c771ffe015044a08c708e71819 $
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

/**
 * Todo:
 *  - Finish implementation (see WX, there might be missing a
 *    i_action handler for IntegerLists, but I don't see any module using it...
 *  - Improvements over WX
 *      - Validator for modulelist
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "components/preferences_widgets.hpp"
#include "util/customwidgets.hpp"
#include <vlc_keys.h>

#include <QString>
#include <QVariant>
#include <QGridLayout>
#include <QSlider>
#include <QFileDialog>
#include <QGroupBox>
#include <QTreeWidgetItem>
#include <QSignalMapper>
#include <QDialogButtonBox>

#define MINWIDTH_BOX 90
#define LAST_COLUMN 10

QString formatTooltip(const QString & tooltip)
{
    QString formatted =
    "<html><head><meta name=\"qrichtext\" content=\"1\" />"
    "<style type=\"text/css\"> p, li { white-space: pre-wrap; } </style></head>"
    "<body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; "
    "font-style:normal; text-decoration:none;\">"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; "
    "margin-right:0px; -qt-block-indent:0; text-indent:0px;\">" +
    tooltip +
    "</p></body></html>";
    return formatted;
}

ConfigControl *ConfigControl::createControl( vlc_object_t *p_this,
                                             module_config_t *p_item,
                                             QWidget *parent )
{
    int i = 0;
    return createControl( p_this, p_item, parent, NULL, i );
}

ConfigControl *ConfigControl::createControl( vlc_object_t *p_this,
                                             module_config_t *p_item,
                                             QWidget *parent,
                                             QGridLayout *l, int &line )
{
    ConfigControl *p_control = NULL;

    switch( p_item->i_type )
    {
    case CONFIG_ITEM_MODULE:
        p_control = new ModuleConfigControl( p_this, p_item, parent, false,
                                             l, line );
        break;
    case CONFIG_ITEM_MODULE_CAT:
        p_control = new ModuleConfigControl( p_this, p_item, parent, true,
                                             l, line );
        break;
    case CONFIG_ITEM_MODULE_LIST:
        p_control = new ModuleListConfigControl( p_this, p_item, parent, false,
                                             l, line );
        break;
    case CONFIG_ITEM_MODULE_LIST_CAT:
        p_control = new ModuleListConfigControl( p_this, p_item, parent, true,
                                             l, line );
        /* Special Hack for a bug in video-filter */
        if( qobject_cast<ModuleListConfigControl *>( p_control )->groupBox == NULL )
            return NULL;
        break;
    case CONFIG_ITEM_STRING:
        if( !p_item->i_list )
            p_control = new StringConfigControl( p_this, p_item, parent,
                                                 l, line, false );
        else
            p_control = new StringListConfigControl( p_this, p_item,
                                            parent, false, l, line );
        break;
    case CONFIG_ITEM_PASSWORD:
        if( !p_item->i_list )
            p_control = new StringConfigControl( p_this, p_item, parent,
                                                 l, line, true );
        else
            p_control = new StringListConfigControl( p_this, p_item,
                                            parent, true, l, line );
        break;
    case CONFIG_ITEM_INTEGER:
        if( p_item->i_list )
            p_control = new IntegerListConfigControl( p_this, p_item,
                                            parent, false, l, line );
        else if( p_item->min.i || p_item->max.i )
            p_control = new IntegerRangeConfigControl( p_this, p_item, parent,
                                                       l, line );
        else
            p_control = new IntegerConfigControl( p_this, p_item, parent,
                                                  l, line );
        break;
    case CONFIG_ITEM_FILE:
        p_control = new FileConfigControl( p_this, p_item, parent, l,
                                                line, false );
        break;
    case CONFIG_ITEM_DIRECTORY:
        p_control = new DirectoryConfigControl( p_this, p_item, parent, l,
                                                line, false );
        break;
#if 0
    case CONFIG_ITEM_FONT:
        p_control = new FontConfigControl( p_this, p_item, parent, l,
                                           line, false );
        break;
#endif
    case CONFIG_ITEM_KEY:
        p_control = new KeySelectorControl( p_this, p_item, parent, l, line );
        break;
    case CONFIG_ITEM_BOOL:
        p_control = new BoolConfigControl( p_this, p_item, parent, l, line );
        break;
    case CONFIG_ITEM_FLOAT:
        if( p_item->min.f || p_item->max.f )
            p_control = new FloatRangeConfigControl( p_this, p_item, parent,
                                                     l, line );
        else
            p_control = new FloatConfigControl( p_this, p_item, parent,
                                                  l, line );
        break;
    default:
        break;
    }
    return p_control;
}

void ConfigControl::doApply( intf_thread_t *p_intf )
{
    switch( getType() )
    {
        case CONFIG_ITEM_INTEGER:
        case CONFIG_ITEM_BOOL:
        {
            VIntConfigControl *vicc = qobject_cast<VIntConfigControl *>(this);
            assert( vicc );
            config_PutInt( p_intf, vicc->getName(), vicc->getValue() );
            break;
        }
        case CONFIG_ITEM_FLOAT:
        {
            VFloatConfigControl *vfcc =
                                    qobject_cast<VFloatConfigControl *>(this);
            assert( vfcc );
            config_PutFloat( p_intf, vfcc->getName(), vfcc->getValue() );
            break;
        }
        case CONFIG_ITEM_STRING:
        {
            VStringConfigControl *vscc =
                            qobject_cast<VStringConfigControl *>(this);
            assert( vscc );
            config_PutPsz( p_intf, vscc->getName(), qtu( vscc->getValue() ) );
            break;
        }
        case CONFIG_ITEM_KEY:
        {
            KeySelectorControl *ksc = qobject_cast<KeySelectorControl *>(this);
            assert( ksc );
            ksc->doApply();
        }
    }
}

/**************************************************************************
 * String-based controls
 *************************************************************************/

/*********** String **************/
StringConfigControl::StringConfigControl( vlc_object_t *_p_this,
                                          module_config_t *_p_item,
                                          QWidget *_parent, QGridLayout *l,
                                          int &line, bool pwd ) :
                           VStringConfigControl( _p_this, _p_item, _parent )
{
    label = new QLabel( qtr(p_item->psz_text) );
    text = new QLineEdit( qfu(p_item->value.psz) );
    if( pwd ) text->setEchoMode( QLineEdit::Password );
    finish();

    if( !l )
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget( label, 0 ); layout->insertSpacing( 1, 10 );
        layout->addWidget( text, LAST_COLUMN );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( label, line, 0 );
        l->setColumnMinimumWidth( 1, 10 );
        l->addWidget( text, line, LAST_COLUMN );
    }
}

StringConfigControl::StringConfigControl( vlc_object_t *_p_this,
                                   module_config_t *_p_item,
                                   QLabel *_label, QLineEdit *_text, bool pwd ):
                           VStringConfigControl( _p_this, _p_item )
{
    text = _text;
    label = _label;
    finish( );
}

void StringConfigControl::finish()
{
    text->setText( qfu(p_item->value.psz) );
    text->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}

/*********** File **************/
FileConfigControl::FileConfigControl( vlc_object_t *_p_this,
                                          module_config_t *_p_item,
                                          QWidget *_parent, QGridLayout *l,
                                          int &line, bool pwd ) :
                           VStringConfigControl( _p_this, _p_item, _parent )
{
    label = new QLabel( qtr(p_item->psz_text) );
    text = new QLineEdit( qfu(p_item->value.psz) );
    browse = new QPushButton( qtr( "Browse..." ) );
    QHBoxLayout *textAndButton = new QHBoxLayout();
    textAndButton->setMargin( 0 );
    textAndButton->addWidget( text, 2 );
    textAndButton->addWidget( browse, 0 );

    BUTTONACT( browse, updateField() );

    finish();

    if( !l )
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget( label, 0 );
        layout->insertSpacing( 1, 10 );
        layout->addLayout( textAndButton, LAST_COLUMN );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( label, line, 0 );
        l->setColumnMinimumWidth( 1, 10 );
        l->addLayout( textAndButton, line, LAST_COLUMN );
    }
}


FileConfigControl::FileConfigControl( vlc_object_t *_p_this,
                                   module_config_t *_p_item,
                                   QLabel *_label, QLineEdit *_text,
                                   QPushButton *_button, bool pwd ):
                           VStringConfigControl( _p_this, _p_item )
{
    browse = _button;
    text = _text;
    label = _label;

    BUTTONACT( browse, updateField() );

    finish( );
}

void FileConfigControl::updateField()
{
    QString file = QFileDialog::getOpenFileName( NULL,
                  qtr( "Select File" ), qfu( config_GetHomeDir() ) );
    if( file.isNull() ) return;
    text->setText( toNativeSeparators( file ) );
}

void FileConfigControl::finish()
{
    text->setText( qfu(p_item->value.psz) );
    text->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}

/********* String / Directory **********/
DirectoryConfigControl::DirectoryConfigControl( vlc_object_t *_p_this,
                        module_config_t *_p_item, QWidget *_p_widget,
                        QGridLayout *_p_layout, int& _int, bool _pwd ) :
     FileConfigControl( _p_this, _p_item, _p_widget, _p_layout, _int, _pwd)
{}

DirectoryConfigControl::DirectoryConfigControl( vlc_object_t *_p_this,
                        module_config_t *_p_item, QLabel *_p_label,
                        QLineEdit *_p_line, QPushButton *_p_button, bool _pwd ):
     FileConfigControl( _p_this, _p_item, _p_label, _p_line, _p_button, _pwd)
{}

void DirectoryConfigControl::updateField()
{
    QString dir = QFileDialog::getExistingDirectory( NULL,
                      qtr( "Select Directory" ),
                      text->text().isEmpty() ?
                        qfu( config_GetHomeDir() ) : text->text(),
                  QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );

    if( dir.isNull() ) return;
    text->setText( toNativeSepNoSlash( dir ) );
}

#if 0
#include <QFontDialog>

/********* String / Font **********/
FontConfigControl::FontConfigControl( vlc_object_t *_p_this,
                        module_config_t *_p_item, QWidget *_p_widget,
                        QGridLayout *_p_layout, int& _int, bool _pwd ) :
     FileConfigControl( _p_this, _p_item, _p_widget, _p_layout, _int, _pwd)
{}

FontConfigControl::FontConfigControl( vlc_object_t *_p_this,
                        module_config_t *_p_item, QLabel *_p_label,
                        QLineEdit *_p_line, QPushButton *_p_button, bool _pwd ):
     FileConfigControl( _p_this, _p_item, _p_label, _p_line, _p_button, _pwd)
{}

void FontConfigControl::updateField()
{
    bool ok;
    QFont font = QFontDialog::getFont( &ok, QFont( text->text() ), NULL );
    if( !ok ) return;
    text->setText( font.family() );
}
#endif

/********* String / choice list **********/
StringListConfigControl::StringListConfigControl( vlc_object_t *_p_this,
               module_config_t *_p_item, QWidget *_parent, bool bycat,
               QGridLayout *l, int &line) :
               VStringConfigControl( _p_this, _p_item, _parent )
{
    label = new QLabel( qtr(p_item->psz_text) );
    combo = new QComboBox();
    combo->setMinimumWidth( MINWIDTH_BOX );
    combo->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

    module_config_t *p_module_config = config_FindConfig( p_this, p_item->psz_name );
    if(p_module_config && p_module_config->pf_update_list)
    {
       vlc_value_t val;
       val.psz_string = strdup(p_module_config->value.psz);

       p_module_config->pf_update_list(p_this, p_item->psz_name, val, val, NULL);

       // assume in any case that dirty was set to true
       // because lazy programmes will use the same callback for
       // this, like the one behind the refresh push button?
       p_module_config->b_dirty = false;

       free( val.psz_string );
    }

    finish( p_module_config, bycat );
    if( !l )
    {
        l = new QGridLayout();
        l->addWidget( label, 0, 0 ); l->addWidget( combo, 0, LAST_COLUMN );
        widget->setLayout( l );
    }
    else
    {
        l->addWidget( label, line, 0 );
        l->addWidget( combo, line, LAST_COLUMN, Qt::AlignRight );
    }

    if( p_item->i_action )
    {
        QSignalMapper *signalMapper = new QSignalMapper(this);

        /* Some stringLists like Capture listings have action associated */
        for( int i = 0; i < p_item->i_action; i++ )
        {
            QPushButton *button =
                new QPushButton( qfu( p_item->ppsz_action_text[i] ));
            CONNECT( button, clicked(), signalMapper, map() );
            signalMapper->setMapping( button, i );
            l->addWidget( button, line, LAST_COLUMN - p_item->i_action + i,
                    Qt::AlignRight );
        }
        CONNECT( signalMapper, mapped( int ),
                this, actionRequested( int ) );
    }
}

void StringListConfigControl::actionRequested( int i_action )
{
    /* Supplementary check for boundaries */
    if( i_action < 0 || i_action >= p_item->i_action ) return;

    module_config_t *p_module_config = config_FindConfig( p_this, getName() );
    if(!p_module_config) return;

    vlc_value_t val;
    val.psz_string =
        qtu( (combo->itemData( combo->currentIndex() ).toString() ) );

    p_module_config->ppf_action[i_action]( p_this, getName(), val, val, 0 );

    if( p_module_config->b_dirty )
    {
        combo->clear();
        finish( p_module_config, true );
        p_module_config->b_dirty = false;
    }
}
StringListConfigControl::StringListConfigControl( vlc_object_t *_p_this,
                module_config_t *_p_item, QLabel *_label, QComboBox *_combo,
                bool bycat ) : VStringConfigControl( _p_this, _p_item )
{
    combo = _combo;
    label = _label;

    module_config_t *p_module_config = config_FindConfig( p_this, getName() );

    finish( p_module_config, bycat );
}

void StringListConfigControl::finish(module_config_t *p_module_config, bool bycat )
{
    combo->setEditable( false );

    if(!p_module_config) return;

    for( int i_index = 0; i_index < p_module_config->i_list; i_index++ )
    {
        combo->addItem( qfu((p_module_config->ppsz_list_text &&
                            p_module_config->ppsz_list_text[i_index])?
                            p_module_config->ppsz_list_text[i_index] :
                            p_module_config->ppsz_list[i_index] ),
                   QVariant( qfu(p_module_config->ppsz_list[i_index] )) );
        if( p_item->value.psz && !strcmp( p_module_config->value.psz,
                                          p_module_config->ppsz_list[i_index] ) )
            combo->setCurrentIndex( combo->count() - 1 );
    }
    combo->setToolTip( formatTooltip(qtr(p_module_config->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_module_config->psz_longtext)) );
}

QString StringListConfigControl::getValue()
{
    return combo->itemData( combo->currentIndex() ).toString();
}

void setfillVLCConfigCombo( const char *configname, intf_thread_t *p_intf,
                        QComboBox *combo, QWidget *parent )
{
    module_config_t *p_config =
                      config_FindConfig( VLC_OBJECT(p_intf), configname );
    if( p_config )
    {
       if(p_config->pf_update_list)
        {
            vlc_value_t val;
            val.i_int = p_config->value.i;
            p_config->pf_update_list(VLC_OBJECT(p_intf), configname, val, val, NULL);
            // assume in any case that dirty was set to true
            // because lazy programmes will use the same callback for
            // this, like the one behind the refresh push button?
            p_config->b_dirty = false;
        }

        for ( int i_index = 0; i_index < p_config->i_list; i_index++ )
        {
            combo->addItem( qfu( p_config->ppsz_list_text[i_index] ),
                    QVariant( p_config->pi_list[i_index] ) );
            if( p_config->value.i == p_config->pi_list[i_index] )
            {
                combo->setCurrentIndex( i_index );
            }
        }
        combo->setToolTip( qfu( p_config->psz_longtext ) );
    }
}

/********* Module **********/
ModuleConfigControl::ModuleConfigControl( vlc_object_t *_p_this,
               module_config_t *_p_item, QWidget *_parent, bool bycat,
               QGridLayout *l, int &line) :
               VStringConfigControl( _p_this, _p_item, _parent )
{
    label = new QLabel( qtr(p_item->psz_text) );
    combo = new QComboBox();
    combo->setMinimumWidth( MINWIDTH_BOX );
    finish( bycat );
    if( !l )
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget( label ); layout->addWidget( combo, LAST_COLUMN );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( label, line, 0 );
        l->addWidget( combo, line, LAST_COLUMN, Qt::AlignRight );
    }
}

ModuleConfigControl::ModuleConfigControl( vlc_object_t *_p_this,
                module_config_t *_p_item, QLabel *_label, QComboBox *_combo,
                bool bycat ) : VStringConfigControl( _p_this, _p_item )
{
    combo = _combo;
    label = _label;
    finish( bycat );
}

void ModuleConfigControl::finish( bool bycat )
{
    vlc_list_t *p_list;
    module_t *p_parser;

    combo->setEditable( false );

    /* build a list of available modules */
    p_list = vlc_list_find( p_this, VLC_OBJECT_MODULE, FIND_ANYWHERE );
    combo->addItem( qtr("Default") );
    for( int i_index = 0; i_index < p_list->i_count; i_index++ )
    {
        p_parser = (module_t *)p_list->p_values[i_index].p_object ;

        if( bycat )
        {
            if( !strcmp( module_GetObjName( p_parser ), "main" ) ) continue;

            unsigned confsize;
            module_config_t *p_config;

            p_config = module_GetConfig (p_parser, &confsize);
             for (size_t i = 0; i < confsize; i++)
            {
                /* Hack: required subcategory is stored in i_min */
                const module_config_t *p_cfg = p_config + i;
                if( p_cfg->i_type == CONFIG_SUBCATEGORY &&
                    p_cfg->value.i == p_item->min.i )
                    combo->addItem( qtr( module_GetLongName( p_parser )),
                                    QVariant( module_GetObjName( p_parser ) ) );
                if( p_item->value.psz && !strcmp( p_item->value.psz,
                                                  module_GetObjName( p_parser ) ) )
                    combo->setCurrentIndex( combo->count() - 1 );
            }
            module_PutConfig (p_config);
        }
        else if( module_IsCapable( p_parser, p_item->psz_type ) )
        {
            combo->addItem( qtr(module_GetLongName( p_parser ) ),
                            QVariant( module_GetObjName( p_parser ) ) );
            if( p_item->value.psz && !strcmp( p_item->value.psz,
                                              module_GetObjName( p_parser ) ) )
                combo->setCurrentIndex( combo->count() - 1 );
        }
    }
    vlc_list_release( p_list );
    combo->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}

QString ModuleConfigControl::getValue()
{
    return combo->itemData( combo->currentIndex() ).toString();
}

/********* Module list **********/
ModuleListConfigControl::ModuleListConfigControl( vlc_object_t *_p_this,
        module_config_t *_p_item, QWidget *_parent, bool bycat,
        QGridLayout *l, int &line) :
    VStringConfigControl( _p_this, _p_item, _parent )
{
    groupBox = NULL;
    /* Special Hack */
    if( !p_item->psz_text ) return;

    groupBox = new QGroupBox ( qtr(p_item->psz_text), _parent );
    text = new QLineEdit;
    QGridLayout *layoutGroupBox = new QGridLayout( groupBox );

    finish( bycat );

    int boxline = 0;
    for( QVector<checkBoxListItem*>::iterator it = modules.begin();
            it != modules.end(); it++ )
    {
        layoutGroupBox->addWidget( (*it)->checkBox, boxline++, 0 );
    }
    layoutGroupBox->addWidget( text, boxline, 0 );

    if( !l )
    {
        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget( groupBox, line, 0 );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( groupBox, line, 0, 1, -1 );
    }

    text->setToolTip( formatTooltip( qtr( p_item->psz_longtext) ) );
}

ModuleListConfigControl::~ModuleListConfigControl()
{
    for( QVector<checkBoxListItem*>::iterator it = modules.begin();
            it != modules.end(); it++ )
    {
        delete *it;
    }
    delete groupBox;
}

#define CHECKBOX_LISTS \
{ \
       QCheckBox *cb = new QCheckBox( qtr( module_GetLongName( p_parser ) ) );\
       checkBoxListItem *cbl = new checkBoxListItem; \
\
       CONNECT( cb, stateChanged( int ), this, onUpdate( int ) );\
       cb->setToolTip( formatTooltip( qtr( module_GetHelp( p_parser ))));\
       cbl->checkBox = cb; \
\
       cbl->psz_module = strdup( module_GetObjName( p_parser ) ); \
       modules.push_back( cbl ); \
\
       if( p_item->value.psz && strstr( p_item->value.psz, cbl->psz_module ) ) \
            cbl->checkBox->setChecked( true ); \
}


void ModuleListConfigControl::finish( bool bycat )
{
    vlc_list_t *p_list;
    module_t *p_parser;

    /* build a list of available modules */
    p_list = vlc_list_find( p_this, VLC_OBJECT_MODULE, FIND_ANYWHERE );
    for( int i_index = 0; i_index < p_list->i_count; i_index++ )
    {
        p_parser = (module_t *)p_list->p_values[i_index].p_object ;

        if( bycat )
        {
            if( !strcmp( module_GetObjName( p_parser ), "main" ) ) continue;

            unsigned confsize;
            module_config_t *p_config = module_GetConfig (p_parser, &confsize);

            for (size_t i = 0; i < confsize; i++)
            {
                module_config_t *p_cfg = p_config + i;
                /* Hack: required subcategory is stored in i_min */
                if( p_cfg->i_type == CONFIG_SUBCATEGORY &&
                        p_cfg->value.i == p_item->min.i )
                {
                    CHECKBOX_LISTS;
                }
            }
            module_PutConfig (p_config);
        }
        else if( module_IsCapable( p_parser, p_item->psz_type ) )
        {
            CHECKBOX_LISTS;
        }
    }
    vlc_list_release( p_list );
    text->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
    assert( groupBox );
    groupBox->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}
#undef CHECKBOX_LISTS

QString ModuleListConfigControl::getValue()
{
    assert( text );
    return text->text();
}

void ModuleListConfigControl::hide()
{
    for( QVector<checkBoxListItem*>::iterator it = modules.begin();
         it != modules.end(); it++ )
    {
        (*it)->checkBox->hide();
    }
    groupBox->hide();
}

void ModuleListConfigControl::show()
{
    for( QVector<checkBoxListItem*>::iterator it = modules.begin();
         it != modules.end(); it++ )
    {
        (*it)->checkBox->show();
    }
    groupBox->show();
}


void ModuleListConfigControl::onUpdate( int value )
{
    text->clear();
    bool first = true;

    for( QVector<checkBoxListItem*>::iterator it = modules.begin();
         it != modules.end(); it++ )
    {
        if( (*it)->checkBox->isChecked() )
        {
            if( first )
            {
                text->setText( text->text() + (*it)->psz_module );
                first = false;
            }
            else
            {
                text->setText( text->text() + ":" + (*it)->psz_module );
            }
        }
    }
}

/**************************************************************************
 * Integer-based controls
 *************************************************************************/

/*********** Integer **************/
IntegerConfigControl::IntegerConfigControl( vlc_object_t *_p_this,
                                            module_config_t *_p_item,
                                            QWidget *_parent, QGridLayout *l,
                                            int &line ) :
                           VIntConfigControl( _p_this, _p_item, _parent )
{
    label = new QLabel( qtr(p_item->psz_text) );
    spin = new QSpinBox; spin->setMinimumWidth( MINWIDTH_BOX );
    spin->setAlignment( Qt::AlignRight );
    spin->setMaximumWidth( MINWIDTH_BOX );
    finish();

    if( !l )
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget( label, 0 ); layout->addWidget( spin, LAST_COLUMN );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( label, line, 0 );
        l->addWidget( spin, line, LAST_COLUMN, Qt::AlignRight );
    }
}
IntegerConfigControl::IntegerConfigControl( vlc_object_t *_p_this,
                                            module_config_t *_p_item,
                                            QLabel *_label, QSpinBox *_spin ) :
                                      VIntConfigControl( _p_this, _p_item )
{
    spin = _spin;
    label = _label;
    finish();
}

void IntegerConfigControl::finish()
{
    spin->setMaximum( 2000000000 );
    spin->setMinimum( -2000000000 );
    spin->setValue( p_item->value.i );
    spin->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}

int IntegerConfigControl::getValue()
{
    return spin->value();
}

/********* Integer range **********/
IntegerRangeConfigControl::IntegerRangeConfigControl( vlc_object_t *_p_this,
                                            module_config_t *_p_item,
                                            QWidget *_parent, QGridLayout *l,
                                            int &line ) :
            IntegerConfigControl( _p_this, _p_item, _parent, l, line )
{
    finish();
}

IntegerRangeConfigControl::IntegerRangeConfigControl( vlc_object_t *_p_this,
                                            module_config_t *_p_item,
                                            QLabel *_label, QSpinBox *_spin ) :
            IntegerConfigControl( _p_this, _p_item, _label, _spin )
{
    finish();
}

void IntegerRangeConfigControl::finish()
{
    spin->setMaximum( p_item->max.i );
    spin->setMinimum( p_item->min.i );
}

IntegerRangeSliderConfigControl::IntegerRangeSliderConfigControl(
                                            vlc_object_t *_p_this,
                                            module_config_t *_p_item,
                                            QLabel *_label, QSlider *_slider ):
                    VIntConfigControl( _p_this, _p_item )
{
    slider = _slider;
    label = _label;
    slider->setMaximum( p_item->max.i );
    slider->setMinimum( p_item->min.i );
    slider->setValue( p_item->value.i );
    slider->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}

int IntegerRangeSliderConfigControl::getValue()
{
        return slider->value();
}


/********* Integer / choice list **********/
IntegerListConfigControl::IntegerListConfigControl( vlc_object_t *_p_this,
               module_config_t *_p_item, QWidget *_parent, bool bycat,
               QGridLayout *l, int &line) :
               VIntConfigControl( _p_this, _p_item, _parent )
{
    label = new QLabel( qtr(p_item->psz_text) );
    combo = new QComboBox();
    combo->setMinimumWidth( MINWIDTH_BOX );

    module_config_t *p_module_config = config_FindConfig( p_this, p_item->psz_name );
    if(p_module_config && p_module_config->pf_update_list)
    {
       vlc_value_t val;
       val.i_int = p_module_config->value.i;

       p_module_config->pf_update_list(p_this, p_item->psz_name, val, val, NULL);

       // assume in any case that dirty was set to true
       // because lazy programmes will use the same callback for
       // this, like the one behind the refresh push button?
       p_module_config->b_dirty = false;
    }


    finish( p_module_config, bycat );
    if( !l )
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget( label ); layout->addWidget( combo, LAST_COLUMN );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( label, line, 0 );
        l->addWidget( combo, line, LAST_COLUMN, Qt::AlignRight );
    }

    if( p_item->i_action )
    {
        QSignalMapper *signalMapper = new QSignalMapper(this);

        /* Some stringLists like Capture listings have action associated */
        for( int i = 0; i < p_item->i_action; i++ )
        {
            QPushButton *button =
                new QPushButton( qfu( p_item->ppsz_action_text[i] ));
            CONNECT( button, clicked(), signalMapper, map() );
            signalMapper->setMapping( button, i );
            l->addWidget( button, line, LAST_COLUMN - p_item->i_action + i,
                    Qt::AlignRight );
        }
        CONNECT( signalMapper, mapped( int ),
                this, actionRequested( int ) );
    }

}
IntegerListConfigControl::IntegerListConfigControl( vlc_object_t *_p_this,
                module_config_t *_p_item, QLabel *_label, QComboBox *_combo,
                bool bycat ) : VIntConfigControl( _p_this, _p_item )
{
    combo = _combo;
    label = _label;

    module_config_t *p_module_config = config_FindConfig( p_this, getName() );

    finish( p_module_config, bycat );
}

void IntegerListConfigControl::finish(module_config_t *p_module_config, bool bycat )
{
    combo->setEditable( false );

    if(!p_module_config) return;

    for( int i_index = 0; i_index < p_module_config->i_list; i_index++ )
    {
        combo->addItem( qtr(p_module_config->ppsz_list_text[i_index] ),
                        QVariant( p_module_config->pi_list[i_index] ) );
        if( p_module_config->value.i == p_module_config->pi_list[i_index] )
            combo->setCurrentIndex( combo->count() - 1 );
    }
    combo->setToolTip( formatTooltip(qtr(p_module_config->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_module_config->psz_longtext)) );
}

void IntegerListConfigControl::actionRequested( int i_action )
{
    /* Supplementary check for boundaries */
    if( i_action < 0 || i_action >= p_item->i_action ) return;

    module_config_t *p_module_config = config_FindConfig( p_this, getName() );
    if(!p_module_config) return;


    vlc_value_t val;
    val.i_int = combo->itemData( combo->currentIndex() ).toInt();

    p_module_config->ppf_action[i_action]( p_this, getName(), val, val, 0 );

    if( p_module_config->b_dirty )
    {
        combo->clear();
        finish( p_module_config, true );
        p_module_config->b_dirty = false;
    }
}

int IntegerListConfigControl::getValue()
{
    return combo->itemData( combo->currentIndex() ).toInt();
}

/*********** Boolean **************/
BoolConfigControl::BoolConfigControl( vlc_object_t *_p_this,
                                      module_config_t *_p_item,
                                      QWidget *_parent, QGridLayout *l,
                                      int &line ) :
                    VIntConfigControl( _p_this, _p_item, _parent )
{
    checkbox = new QCheckBox( qtr(p_item->psz_text) );
    finish();

    if( !l )
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget( checkbox, 0 );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( checkbox, line, 0 );
    }
}
BoolConfigControl::BoolConfigControl( vlc_object_t *_p_this,
                                      module_config_t *_p_item,
                                      QLabel *_label,
                                      QCheckBox *_checkbox,
                                      bool bycat ) :
                   VIntConfigControl( _p_this, _p_item )
{
    checkbox = _checkbox;
    finish();
}

void BoolConfigControl::finish()
{
    checkbox->setCheckState( p_item->value.i == true ? Qt::Checked
                                                        : Qt::Unchecked );
    checkbox->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}

int BoolConfigControl::getValue()
{
    return checkbox->checkState() == Qt::Checked ? true : false;
}

/**************************************************************************
 * Float-based controls
 *************************************************************************/

/*********** Float **************/
FloatConfigControl::FloatConfigControl( vlc_object_t *_p_this,
                                        module_config_t *_p_item,
                                        QWidget *_parent, QGridLayout *l,
                                        int &line ) :
                    VFloatConfigControl( _p_this, _p_item, _parent )
{
    label = new QLabel( qtr(p_item->psz_text) );
    spin = new QDoubleSpinBox;
    spin->setMinimumWidth( MINWIDTH_BOX );
    spin->setMaximumWidth( MINWIDTH_BOX );
    spin->setAlignment( Qt::AlignRight );
    finish();

    if( !l )
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addWidget( label, 0 ); layout->addWidget( spin, LAST_COLUMN );
        widget->setLayout( layout );
    }
    else
    {
        l->addWidget( label, line, 0 );
        l->addWidget( spin, line, LAST_COLUMN, Qt::AlignRight );
    }
}

FloatConfigControl::FloatConfigControl( vlc_object_t *_p_this,
                                        module_config_t *_p_item,
                                        QLabel *_label,
                                        QDoubleSpinBox *_spin ) :
                    VFloatConfigControl( _p_this, _p_item )
{
    spin = _spin;
    label = _label;
    finish();
}

void FloatConfigControl::finish()
{
    spin->setMaximum( 2000000000. );
    spin->setMinimum( -2000000000. );
    spin->setSingleStep( 0.1 );
    spin->setValue( (double)p_item->value.f );
    spin->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
    if( label )
        label->setToolTip( formatTooltip(qtr(p_item->psz_longtext)) );
}

float FloatConfigControl::getValue()
{
    return (float)spin->value();
}

/*********** Float with range **************/
FloatRangeConfigControl::FloatRangeConfigControl( vlc_object_t *_p_this,
                                        module_config_t *_p_item,
                                        QWidget *_parent, QGridLayout *l,
                                        int &line ) :
                FloatConfigControl( _p_this, _p_item, _parent, l, line )
{
    finish();
}

FloatRangeConfigControl::FloatRangeConfigControl( vlc_object_t *_p_this,
                                        module_config_t *_p_item,
                                        QLabel *_label,
                                        QDoubleSpinBox *_spin ) :
                FloatConfigControl( _p_this, _p_item, _label, _spin )
{
    finish();
}

void FloatRangeConfigControl::finish()
{
    spin->setMaximum( (double)p_item->max.f );
    spin->setMinimum( (double)p_item->min.f );
}


/**********************************************************************
 * Key selector widget
 **********************************************************************/
KeySelectorControl::KeySelectorControl( vlc_object_t *_p_this,
                                      module_config_t *_p_item,
                                      QWidget *_parent, QGridLayout *l,
                                      int &line ) :
                                ConfigControl( _p_this, _p_item, _parent )

{
    QWidget *keyContainer = new QWidget;
    QGridLayout *gLayout = new QGridLayout( keyContainer );

    label = new QLabel(
            qtr( "Select an action to change the associated hotkey") );

    /* Deactivated for now
    QLabel *searchLabel = new QLabel( qtr( "Search" ) );
    QLineEdit *actionSearch = new QLineEdit;*/

    table = new QTreeWidget;
    table->setColumnCount(2);
    table->headerItem()->setText( 0, qtr( "Action" ) );
    table->headerItem()->setText( 1, qtr( "Shortcut" ) );

    shortcutValue = new KeyShortcutEdit;
    shortcutValue->setReadOnly(true);

    QPushButton *clearButton = new QPushButton( qtr( "Clear" ) );
    QPushButton *setButton = new QPushButton( qtr( "Set" ) );
    setButton->setDefault( true );
    finish();

    gLayout->addWidget( label, 0, 0, 1, 4 );
  /* deactivated for now
    gLayout->addWidget( searchLabel, 1, 0, 1, 2 );
    gLayout->addWidget( actionSearch, 1, 2, 1, 2 ); */
    gLayout->addWidget( table, 2, 0, 1, 4 );
    gLayout->addWidget( clearButton, 3, 0, 1, 1 );
    gLayout->addWidget( shortcutValue, 3, 1, 1, 2 );
    gLayout->addWidget( setButton, 3, 3, 1, 1 );

    l->addWidget( keyContainer, line, 0, 1, 2 );

    CONNECT( clearButton, clicked(), shortcutValue, clear() );
    CONNECT( clearButton, clicked(), this, setTheKey() );
    BUTTONACT( setButton, setTheKey() );
}

void KeySelectorControl::finish()
{
    if( label )
        label->setToolTip( formatTooltip( qtr( p_item->psz_longtext ) ) );

    /* Fill the table */
    table->setColumnCount( 2 );
    table->setAlternatingRowColors( true );

    /* Get the main Module */
    module_t *p_main = module_Find( p_this, "main" );
    assert( p_main );

    /* Access to the module_config_t */
    unsigned confsize;
    module_config_t *p_config;

    p_config = module_GetConfig (p_main, &confsize);

    for (size_t i = 0; i < confsize; i++)
    {
        module_config_t *p_item = p_config + i;

        /* If we are a key option not empty */
        if( p_item->i_type & CONFIG_ITEM && p_item->psz_name
            && strstr( p_item->psz_name , "key-" )
            && !EMPTY_STR( p_item->psz_text ) )
        {
            /*
               Each tree item has:
                - QString text in column 0
                - QString name in data of column 0
                - KeyValue in String in column 1
                - KeyValue in int in column 1
             */
            QTreeWidgetItem *treeItem = new QTreeWidgetItem();
            treeItem->setText( 0, qtr( p_item->psz_text ) );
            treeItem->setData( 0, Qt::UserRole,
                               QVariant( qfu( p_item->psz_name ) ) );
            treeItem->setText( 1, VLCKeyToString( p_item->value.i ) );
            treeItem->setData( 1, Qt::UserRole, QVariant( p_item->value.i ) );
            table->addTopLevelItem( treeItem );
        }
    }
    module_PutConfig (p_config);
    module_Put (p_main);

    table->resizeColumnToContents( 0 );

    CONNECT( table, itemDoubleClicked( QTreeWidgetItem *, int ),
             this, selectKey( QTreeWidgetItem * ) );
    CONNECT( table, itemSelectionChanged (),
             this, select1Key() );

    CONNECT( shortcutValue, pressed(), this, selectKey() );
}

/* Show the key selected from the table in the keySelector */
void KeySelectorControl::select1Key()
{
    QTreeWidgetItem *keyItem = table->currentItem();
    shortcutValue->setText( keyItem->text( 1 ) );
    shortcutValue->setValue( keyItem->data( 1, Qt::UserRole ).toInt() );
}

void KeySelectorControl::selectKey( QTreeWidgetItem *keyItem )
{
    /* This happens when triggered by ClickEater */
    if( keyItem == NULL ) keyItem = table->currentItem();

    /* This can happen when nothing is selected on the treeView
       and the shortcutValue is clicked */
    if( !keyItem ) return;

    /* Launch a small dialog to ask for a new key */
    KeyInputDialog *d = new KeyInputDialog( table, keyItem->text( 0 ), widget );
    d->exec();

    if( d->result() == QDialog::Accepted )
    {
        int newValue = d->keyValue;
        shortcutValue->setText( VLCKeyToString( newValue ) );
        shortcutValue->setValue( newValue );

        if( d->conflicts )
        {
            QTreeWidgetItem *it;
            for( int i = 0; i < table->topLevelItemCount() ; i++ )
            {
                it = table->topLevelItem(i);
                if( ( keyItem != it )
                        && ( it->data( 1, Qt::UserRole ).toInt() == newValue ) )
                {
                    it->setData( 1, Qt::UserRole, QVariant( -1 ) );
                    it->setText( 1, qtr( "Unset" ) );
                }
            }
            /* We already made an OK once. */
            setTheKey();
        }
    }
    delete d;
}

void KeySelectorControl::setTheKey()
{
    table->currentItem()->setText( 1, shortcutValue->text() );
    table->currentItem()->setData( 1, Qt::UserRole, shortcutValue->getValue() );
}

void KeySelectorControl::doApply()
{
    QTreeWidgetItem *it;
    for( int i = 0; i < table->topLevelItemCount() ; i++ )
    {
        it = table->topLevelItem(i);
        if( it->data( 1, Qt::UserRole ).toInt() >= 0 )
            config_PutInt( p_this,
                           qtu( it->data( 0, Qt::UserRole ).toString() ),
                           it->data( 1, Qt::UserRole ).toInt() );
    }
}

KeyInputDialog::KeyInputDialog( QTreeWidget *_table,
                                QString keyToChange,
                                QWidget *_parent ) :
                                QDialog( _parent ), keyValue(0)
{
    setModal( true );
    conflicts = false;

    table = _table;
    setWindowTitle( qtr( "Hotkey for " ) + keyToChange );

    vLayout = new QVBoxLayout( this );
    selected = new QLabel( qtr( "Press the new keys for " ) + keyToChange );
    vLayout->addWidget( selected , Qt::AlignCenter );

    buttonBox = new QDialogButtonBox;
    QPushButton *ok = new QPushButton( qtr("OK") );
    QPushButton *cancel = new QPushButton( qtr("Cancel") );
    buttonBox->addButton( ok, QDialogButtonBox::AcceptRole );
    buttonBox->addButton( cancel, QDialogButtonBox::RejectRole );
    ok->setDefault( true );

    vLayout->addWidget( buttonBox );
    buttonBox->hide();

    CONNECT( buttonBox, accepted(), this, accept() );
    CONNECT( buttonBox, rejected(), this, reject() );
}

void KeyInputDialog::checkForConflicts( int i_vlckey )
{
     QList<QTreeWidgetItem *> conflictList =
         table->findItems( VLCKeyToString( i_vlckey ), Qt::MatchExactly, 1 );

    if( conflictList.size() )
    {
        QLabel *warning = new QLabel(
          qtr("Warning: the key is already assigned to \"") +
          conflictList[0]->text( 0 ) + "\"" );
        vLayout->insertWidget( 1, warning );
        buttonBox->show();

        conflicts = true;
    }
    else accept();
}

void KeyInputDialog::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Tab ||
        e->key() == Qt::Key_Shift ||
        e->key() == Qt::Key_Control ||
        e->key() == Qt::Key_Meta ||
        e->key() == Qt::Key_Alt ||
        e->key() == Qt::Key_AltGr )
        return;
    int i_vlck = qtEventToVLCKey( e );
    selected->setText( qtr( "Key: " ) + VLCKeyToString( i_vlck ) );
    checkForConflicts( i_vlck );
    keyValue = i_vlck;
}

void KeyInputDialog::wheelEvent( QWheelEvent *e )
{
    int i_vlck = qtWheelEventToVLCKey( e );
    selected->setText( qtr( "Key: " ) + VLCKeyToString( i_vlck ) );
    checkForConflicts( i_vlck );
    keyValue = i_vlck;
}

void KeyShortcutEdit::mousePressEvent( QMouseEvent *)
{
    emit pressed();
}

