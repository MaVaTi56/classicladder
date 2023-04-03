/* Classic Ladder Project */
/* Copyright (C) 2001-2023 Marc Le Douarain */
/* http://www.sourceforge.net/projects/classicladder */
/* http://sites.google.com/site/classicladder */
/* August 2002 */
/* -------------------------- */
/* Sections manager (GTK part)*/
/* -------------------------- */
/* This library is free software; you can redistribute it and/or */
/* modify it under the terms of the GNU Lesser General Public */
/* License as published by the Free Software Foundation; either */
/* version 2.1 of the License, or (at your option) any later version. */

/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU */
/* Lesser General Public License for more details. */

/* You should have received a copy of the GNU Lesser General Public */
/* License along with this library; if not, write to the Free Software */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h> // i18n
#include <locale.h> // i18n
#include "classicladder.h"
#include "global.h"
#include "manager.h"
#include "classicladder_gtk.h"
#include "manager_gtk.h"
#include "edit_gtk.h"
#include "menu_and_toolbar_gtk.h"
#include "preferences.h"

GtkWidget *ManagerWindow;
GtkWidget *ListViewSections;

static GtkListStore *ListStore;
GtkWidget * ToolBarWidget;

GtkWidget *AddSectionWindow;
GtkWidget * EditName;
GtkWidget * CycleLanguage;
GtkWidget * CycleSubRoutineNbr;

//char * pNameSectionSelected;
int SectionNbrSelected = -1;

char ModifyNotAdding = FALSE;
static char BlockTreeViewCursorChangedSignal = FALSE;

enum
{
	SECTION_NUMBER, //hidden but useful later to get directly section number array
	SECTION_NAME,
	SECTION_LANGUAGE,
	SECTION_TYPE,
	SECTION_DEBUG,
	SECTION_NBR_INFOS
};

void RefreshSectionSelected( )
{
	SetSectionSelected( SectionNbrSelected );
	EditorButtonsAccordingSectionType( );
	EnableDisableMenusAccordingSectionType( );
	UpdateVScrollBar( TRUE/*AutoSelectCurrentRung*/ );
}

void ManagerDisplaySections( char ForgetSectionSelected, char RefreshComboSectionLists )
{
	StrSection * pSection;
	GtkTreeIter   iter;
	int NumSec;
	int ComboIndex = 0;
	char BufferForSRx[ 10 ];
char buffer_debug[ 50 ];
//	pNameSectionSelected = NULL;
	if ( ForgetSectionSelected )
		SectionNbrSelected = -1;
printf("%s(): init...\n",__FUNCTION__);
	BlockTreeViewCursorChangedSignal = TRUE;
	BlockComboCurrentChangedSignal = TRUE;
	gtk_list_store_clear( ListStore );
	// for combo in main window
	if( RefreshComboSectionLists )
		gtk_list_store_clear( GTK_LIST_STORE (gtk_combo_box_get_model( GTK_COMBO_BOX(WidgetComboCurrentSection) )) );
	for ( NumSec=0; NumSec<NBR_SECTIONS; NumSec++ )
	{
		pSection = &SectionArray[ NumSec ];
		if ( pSection->Used )
		{
			// for combo in main window
			if( RefreshComboSectionLists )
				gtk_combo_box_append_text( MY_GTK_COMBO_BOX(WidgetComboCurrentSection), pSection->Name );

			if ( pSection->Language == SECTION_IN_LADDER )
				strcpy( BufferForSRx, _("Main") );
			if ( pSection->Language == SECTION_IN_SEQUENTIAL )
				strcpy( BufferForSRx, "---" );
			if ( pSection->SubRoutineNumber>=0 )
				sprintf( BufferForSRx, "SR%d", pSection->SubRoutineNumber );
sprintf( buffer_debug, "(%d) F=%d, L=%d, P=%d", NumSec, pSection->FirstRung, pSection->LastRung, pSection->SequentialPage );

			// Acquire an iterator
			gtk_list_store_append( ListStore, &iter );

			// fill the element
			gtk_list_store_set( ListStore, &iter,
					SECTION_NUMBER, NumSec, // hidden but useful later to get directly section number array
					SECTION_NAME, pSection->Name,
				SECTION_LANGUAGE, ( pSection->Language == SECTION_IN_SEQUENTIAL )?(_("Sequential")):(_("Ladder")),
				SECTION_TYPE, BufferForSRx,
				SECTION_DEBUG, buffer_debug,
					-1);
printf("%s(): added %s...\n",__FUNCTION__,pSection->Name);
			if ( SectionNbrSelected==-1 || SectionNbrSelected==NumSec )
			{
printf("%s(): select... before=%d scan=%d\n", __FUNCTION__, SectionNbrSelected, NumSec );
				GtkTreeSelection * selection;
				selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ListViewSections ) );
				gtk_tree_selection_select_iter( selection, &iter );
				SectionNbrSelected = NumSec;
				// for combo in main window to select in correspondence
				if( RefreshComboSectionLists )
					gtk_combo_box_set_active( GTK_COMBO_BOX( WidgetComboCurrentSection ), ComboIndex );
				RefreshSectionSelected( );
			}
			ComboIndex++;
		}
	}
	BlockTreeViewCursorChangedSignal = FALSE;
	BlockComboCurrentChangedSignal = FALSE;
}

void TreeViewCursorChangedSignal( GtkTreeView * treeview, gpointer user_data )
{
	if ( !BlockTreeViewCursorChangedSignal )
	{
		printf(":: Manage TreeView : cursor-changed signal !\n");
		GtkTreeSelection * selection;
		GtkTreeModel * model;
		GtkTreeIter iter;
		selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ListViewSections ) );
		if ( gtk_tree_selection_get_selected( selection, &model, &iter) )
		{
			int SecNumberSelected;
			// first get line number selected for combo activate
			GtkTreePath* PathLineNumberSelected = gtk_tree_model_get_path ( model, &iter );
			if( PathLineNumberSelected )
			{
				char * pathstring = gtk_tree_path_to_string( PathLineNumberSelected );
				if( pathstring )
				{
					printf(":: section line path selected: %s\n",pathstring);
					// for combo in main window to select in correspondence
					BlockComboCurrentChangedSignal = TRUE;
					gtk_combo_box_set_active( GTK_COMBO_BOX( WidgetComboCurrentSection ), atoi( pathstring ) );
					BlockComboCurrentChangedSignal = FALSE;
					free( pathstring );
				}
				gtk_tree_path_free( PathLineNumberSelected );
			}
			gtk_tree_model_get( model, &iter, 0/*ColumnNbr*/, &SecNumberSelected, -1/*EndOfList*/ );
			printf(":: section number selected: %d\n", SecNumberSelected);
			SectionNbrSelected = SecNumberSelected;
			RefreshSectionSelected( );
		}
		else
		{
			printf(":: No selection...\n");
		}
	}
}

//added for combo event from main window (parameter = index defined sections list, not directly section number array !)
void ChangeSectionSelectedFromComboIndex( int SectionIndexToSelect )
{
	int NumSec = 0;
	int ComboIndex = 0;
	int FoundSec = -1;
	do
	{
		if ( SectionArray[ NumSec ].Used )
		{
			if( SectionIndexToSelect==ComboIndex )
				FoundSec = NumSec;
			ComboIndex++;
		}
		NumSec++;
	}
	while( NumSec<NBR_SECTIONS && FoundSec==-1 );
	if( FoundSec!=-1 )
	{
		SectionNbrSelected = FoundSec;
		ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, FALSE/*RefreshComboSectionLists*/ );
	}
	else
	{
		printf("!!! Error in %s() no section found for index %d !!!\n", __FUNCTION__, SectionIndexToSelect);
	}
}

//added by Heli for search
void ChangeSectionSelectedInManager( int SectionToSelect )
{
	SectionNbrSelected = SectionToSelect;
	ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
}
void SelectNextSectionInManager(void)
{
	int SearchSectionNext = GetPrevNextSection( SectionNbrSelected, TRUE/*NextSearch*/ );
	if ( SearchSectionNext!=-1 )
	{
		SectionNbrSelected = SearchSectionNext;
		ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
	}
}

void SelectPreviousSectionInManager(void)
{
	int SearchSectionPrev = GetPrevNextSection( SectionNbrSelected, FALSE/*NextSearch*/ );
	if ( SearchSectionPrev!=-1 )
	{
		SectionNbrSelected = SearchSectionPrev;
		ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
	}
}

void ButtonAddSectionDoneClickSignal( )
{
	char * pSectionNameEntered = (char *)gtk_entry_get_text( GTK_ENTRY(EditName) );
	// verify if name already exists...
	if ( VerifyIfSectionNameAlreadyExist( pSectionNameEntered ) )
	{
		ShowMessageBoxError( _("This section name already exists or is incorrect !!!") );
	}
	else
	{
		if ( ModifyNotAdding )
		{
			ModifySectionProperties( SectionNbrSelected /*pNameSectionSelected*/, pSectionNameEntered );
			gtk_widget_hide( AddSectionWindow );
			ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
		}
		else
		{
//			char SubNbrValue[ 10 ];
			int SubNbr = -1;
//			char BuffLanguage[ 30 ];
			int Language = SECTION_IN_LADDER;
			// get language type
//			strcpy( BuffLanguage , (char *)gtk_entry_get_text((GtkEntry *)((GtkCombo *)CycleLanguage)->entry) );
//			if ( strcmp( BuffLanguage, "Sequential" )==0 )
//				Language = SECTION_IN_SEQUENTIAL;
			Language = gtk_combo_box_get_active( GTK_COMBO_BOX( CycleLanguage ) );
			// get if main or sub-routine (and which number if sub, used in the 'C'all coils)
//			strcpy( SubNbrValue , (char *)gtk_entry_get_text((GtkEntry *)((GtkCombo *)CycleSubRoutineNbr)->entry) );
//			if ( SubNbrValue[ 0 ]=='S' && SubNbrValue[ 1 ]=='R' )
//				SubNbr = atoi( &SubNbrValue[2] );
			SubNbr = gtk_combo_box_get_active( GTK_COMBO_BOX( CycleSubRoutineNbr ) )-1;
		
			if ( SubNbr>=0 && VerifyIfSubRoutineNumberExist( SubNbr ))
			{
				ShowMessageBoxError( _("This sub-routine number for calls is already defined !!!") );
			}
			else
			{
				// create the new section
				int NewSectionNbrAllocated = AddSection( pSectionNameEntered , Language , SubNbr );
				if ( NewSectionNbrAllocated==-1 )
				{
					ShowMessageBoxError( _("Failed to add a new section. Full?") );
				}
				else
				{
					SectionNbrSelected = NewSectionNbrAllocated;
					ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
				}
				gtk_widget_hide( AddSectionWindow );
			}
		}
	}
}
void ButtonAddClickSignal( )
{
	if ( InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when connected to a remote target...") );
	}
	else if ( InfosGene->LadderState==STATE_RUN && !InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when program running...") );
	}
	else
	{
		// we open the requester to add a new section...
		gtk_entry_set_text( GTK_ENTRY(EditName), "" );
		gtk_combo_box_set_active( GTK_COMBO_BOX( CycleLanguage ), 0 );
		gtk_widget_set_sensitive( CycleLanguage, TRUE );
		gtk_combo_box_set_active( GTK_COMBO_BOX( CycleSubRoutineNbr ), 0 );
		gtk_widget_set_sensitive( CycleSubRoutineNbr, TRUE );
		gtk_widget_grab_focus( EditName );
		gtk_window_set_title( GTK_WINDOW(AddSectionWindow), _("Add a new section...") );
		gtk_widget_show( AddSectionWindow );
		ModifyNotAdding = FALSE;
	}
}
void ButtonPropertiesClickSignal( )
{
	if ( InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when connected to a remote target...") );
	}
	else
	{
	// we open the requester to modify some properties of the current section...
//	gtk_entry_set_text( GTK_ENTRY(EditName), pNameSectionSelected );
//	int NumSec = SearchSectionWithName( pNameSectionSelected );
//	if ( NumSec>=0 )
	if ( SectionNbrSelected!=-1 )
	{
//		char Buff[10];
		StrSection * pSection = &SectionArray[ SectionNbrSelected /*NumSec*/ ];
printf("NumSectionCurrentlySelected=%d\n", SectionNbrSelected);

		// we open the requester to modify some properties of the current section...
		gtk_entry_set_text( GTK_ENTRY(EditName), pSection->Name );

//		gtk_entry_set_text((GtkEntry*)((GtkCombo *)CycleLanguage)->entry,pSection->Language== SECTION_IN_LADDER?"Ladder":"Sequential");
		gtk_combo_box_set_active( GTK_COMBO_BOX( CycleLanguage ), pSection->Language );
//		strcpy( Buff, "Main");
//		if ( pSection->SubRoutineNumber>=0 )
//			sprintf( Buff, "SR%d", pSection->SubRoutineNumber );
//		gtk_entry_set_text((GtkEntry*)((GtkCombo *)CycleSubRoutineNbr)->entry,Buff);
		gtk_combo_box_set_active( GTK_COMBO_BOX( CycleSubRoutineNbr ), pSection->SubRoutineNumber+1 );

		gtk_widget_set_sensitive( CycleLanguage, FALSE );
		gtk_widget_set_sensitive( CycleSubRoutineNbr, FALSE );
		gtk_widget_grab_focus( EditName );
		gtk_window_set_title( GTK_WINDOW(AddSectionWindow), _("Modify current section") );
		gtk_widget_show( AddSectionWindow );
		ModifyNotAdding = TRUE;
	}
	else
	{
		printf("Internal error, no current section selected !!!\n");
	}
	}
}
gint AddSectionWindowDeleteEvent( GtkWidget * widget, GdkEvent * event, gpointer data )
{
	// we just want to hide the window
	gtk_widget_hide( AddSectionWindow );
	// we do not want that the window be destroyed.
	return TRUE;
}

void DeleteCurrentSection( )
{
	DelSection( SectionNbrSelected );
	SectionNbrSelected = -1; // to select another section during list display
	ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
}

void ButtonDelClickSignal( )
{
	if ( InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when connected to a remote target...") );
	}
	else if ( InfosGene->LadderState==STATE_RUN && !InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when program running...") );
	}
	else
	{
//		if (pNameSectionSelected )
		if ( SectionNbrSelected!=-1 )
		{
			if ( NbrSectionsDefined( )>1 )
			{
				ShowConfirmationBox( _("Delete"), _("Do you really want to delete the section ?"), DeleteCurrentSection);	
			}
			else
			{
				ShowMessageBoxError( _("You can not delete the last section...") );
			}
		}
	}
}

void ButtonMoveUpClickSignal( )
{
	if ( InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when connected to a remote target...") );
	}
	else if ( InfosGene->LadderState==STATE_RUN && !InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when program running...") );
	}
	else
	{
		int PrevSectNbr = GetPrevNextSection( SectionNbrSelected, FALSE/*NextSearch*/ );
		if ( PrevSectNbr!=-1 )
		{
			SwapSections( SectionNbrSelected, PrevSectNbr );
			SectionNbrSelected = PrevSectNbr;
		}
		else
		{
			ShowMessageBoxError( _("This section is already executed the first !") );
		}
		ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
	}
}
void ButtonMoveDownClickSignal( )
{
	if ( InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when connected to a remote target...") );
	}
	else if ( InfosGene->LadderState==STATE_RUN && !InfosGUI->TargetMonitor.RemoteConnected )
	{
		ShowMessageBoxError( _("Not possible when program running...") );
	}
	else
	{
		int PrevSectNbr = GetPrevNextSection( SectionNbrSelected, TRUE/*NextSearch*/ );
		if ( PrevSectNbr!=-1 )
		{
			SwapSections( SectionNbrSelected, PrevSectNbr );
			SectionNbrSelected = PrevSectNbr;
		}
		else
		{
			ShowMessageBoxError( _("This section is already executed the latest !") );
		}
		ManagerDisplaySections( FALSE/*ForgetSectionSelected*/, TRUE/*RefreshComboSectionLists*/ );
	}
}

gint ManagerWindowDeleteEvent( GtkWidget * widget, GdkEvent * event, gpointer data )
{
// Here, we must only toggle the menu check that will call itself the function below to close the window ...
//	gtk_widget_hide( ManagerWindow );
	SetToggleMenuForSectionsManagerWindow( FALSE/*OpenedWin*/ );
	// we do not want that the window be destroyed.
	return TRUE;
}
// called per toggle action menu, or at startup (if window saved open or not)...
void OpenManagerWindow( GtkAction * ActionOpen, gboolean OpenIt )
{
	if ( ActionOpen!=NULL )
		OpenIt = gtk_toggle_action_get_active( GTK_TOGGLE_ACTION(ActionOpen) );
	if ( OpenIt )
	{
		RestoreWindowPosiPrefs( "Manager", ManagerWindow );
		gtk_widget_show( ManagerWindow );
		gtk_window_present( GTK_WINDOW(ManagerWindow) );
	}
	else
	{
		RememberWindowPosiPrefs( "Manager", ManagerWindow, FALSE/*SaveWindowSize*/ );
		gtk_widget_hide( ManagerWindow );
	}
}
void RememberManagerWindowPrefs( void )
{
//ForGTK3	char WindowIsOpened = GTK_WIDGET_VISIBLE( GTK_WINDOW(ManagerWindow) );
	char WindowIsOpened = MY_GTK_WIDGET_VISIBLE( ManagerWindow );
	RememberWindowOpenPrefs( "Manager", WindowIsOpened );
	if ( WindowIsOpened )
		RememberWindowPosiPrefs( "Manager", ManagerWindow, FALSE/*SaveWindowSize*/ );
}
void CloseManagerWindowForEnd( void )
{
	gtk_widget_hide( ManagerWindow );
}
void OpenManagerWindowFromPopup( void )
{
	SetToggleMenuForSectionsManagerWindow( TRUE );
	gtk_window_present( GTK_WINDOW(ManagerWindow) );
}
void ManagerEnableActionsSectionsList( char cState )
{
	gtk_widget_set_sensitive( ListViewSections, cState );
	gtk_widget_set_sensitive( ToolBarWidget , cState );
}

void AddSectionWindowInit( )
{
	GtkWidget *vbox;
	GtkWidget * ButtonOk;
	GtkWidget * hbox[ 3 ];
	GtkWidget * Lbl[ 3 ];
//	GList *LanguageItems = NULL;
//	GList *SubRoutinesNbrItems = NULL;
	int NumSub;
	char BuffNumSub[ 10 ];
//	char * ArrayNumSub[ ] = { "SR0", "SR1", "SR2", "SR3", "SR4", "SR5", "SR6", "SR7", "SR8", "SR9" };
	int Line;
//	LanguageItems = g_list_append( LanguageItems, "Ladder" );
//#ifdef SEQUENTIAL_SUPPORT
//	LanguageItems = g_list_append( LanguageItems, "Sequential" );
//#endif

//	SubRoutinesNbrItems = g_list_append( SubRoutinesNbrItems, "Main" );
//	for ( NumSub=0; NumSub<10; NumSub++ )
//	{
//		SubRoutinesNbrItems = g_list_append( SubRoutinesNbrItems, ArrayNumSub[ NumSub ] );
//	}

	AddSectionWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	vbox = MY_GTK_NEW_BOX (MY_GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (AddSectionWindow), vbox);
	gtk_widget_show (vbox);

	for ( Line = 0; Line<3; Line++ )
	{
		char * text;
		hbox[ Line ] = MY_GTK_NEW_BOX (MY_GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_container_add (GTK_CONTAINER (vbox), hbox[ Line ]);
		gtk_widget_show (hbox[ Line ]);

		switch( Line )
		{
			case 1 : text = _("Language"); break;
			case 2 : text = _("Main/Sub-Routine"); break;
			default: text = _("Name"); break;
		}
		Lbl[ Line ] = gtk_label_new( text );
		gtk_box_pack_start (GTK_BOX (hbox[ Line ]), Lbl[ Line ], FALSE, FALSE, 0);
		gtk_widget_show ( Lbl[ Line ]);

		switch( Line )
		{
			case 0:
				EditName = gtk_entry_new();
				gtk_entry_set_max_length( GTK_ENTRY(EditName),LGT_SECTION_NAME-1 );
				gtk_box_pack_start( GTK_BOX (hbox[Line]), EditName, TRUE, TRUE, 0 );
				gtk_widget_show( EditName );
				break;
			case 1:
//				CycleLanguage = gtk_combo_new();
//				gtk_combo_set_value_in_list(GTK_COMBO(CycleLanguage), TRUE /*val*/, FALSE /*ok_if_empty*/);
//				gtk_combo_set_popdown_strings(GTK_COMBO(CycleLanguage),LanguageItems);
				CycleLanguage = gtk_combo_box_new_text( );
				gtk_combo_box_append_text( MY_GTK_COMBO_BOX(CycleLanguage), _("Ladder") );
#ifdef SEQUENTIAL_SUPPORT
				gtk_combo_box_append_text( MY_GTK_COMBO_BOX(CycleLanguage), _("Sequential") );
#endif
				gtk_box_pack_start( GTK_BOX (hbox[Line]), CycleLanguage, TRUE, TRUE, 0 );
				gtk_widget_show( CycleLanguage );
				break;
			case 2:
//				CycleSubRoutineNbr = gtk_combo_new();
//				gtk_combo_set_value_in_list(GTK_COMBO(CycleSubRoutineNbr), TRUE /*val*/, FALSE /*ok_if_empty*/);
//				gtk_combo_set_popdown_strings(GTK_COMBO(CycleSubRoutineNbr),SubRoutinesNbrItems);
				CycleSubRoutineNbr = gtk_combo_box_new_text( );
				gtk_combo_box_append_text( MY_GTK_COMBO_BOX(CycleSubRoutineNbr), _("Main") );
				for ( NumSub=0; NumSub<10; NumSub++ )
				{
					sprintf( BuffNumSub, "SR%d", NumSub );
					gtk_combo_box_append_text( MY_GTK_COMBO_BOX(CycleSubRoutineNbr), BuffNumSub );
				}
				gtk_box_pack_start( GTK_BOX (hbox[Line]), CycleSubRoutineNbr, TRUE, TRUE, 0 );
				gtk_widget_show( CycleSubRoutineNbr );
				break;
		}
	}

	ButtonOk = gtk_button_new_with_label( _("Ok") );
	gtk_box_pack_start (GTK_BOX (vbox), ButtonOk, TRUE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT (ButtonOk), "clicked",
		GTK_SIGNAL_FUNC(ButtonAddSectionDoneClickSignal), 0);
	gtk_widget_show (ButtonOk);
	gtk_window_set_modal(GTK_WINDOW(AddSectionWindow),TRUE);
	gtk_window_set_position(GTK_WINDOW(AddSectionWindow),GTK_WIN_POS_CENTER);
	gtk_signal_connect( GTK_OBJECT(AddSectionWindow), "delete_event",
		GTK_SIGNAL_FUNC(AddSectionWindowDeleteEvent), 0 );
}

GtkUIManager * ManageruiManager;

static GtkActionEntry ManagerEntriesArray[ ] =
{	{ "AddSection", GTK_STOCK_ADD, N_("Add section"),  NULL, N_("Add New Section"), G_CALLBACK( ButtonAddClickSignal ) },
	{ "DelSection", GTK_STOCK_DELETE, N_("Delete section"),  NULL, N_("Delete Section"), G_CALLBACK( ButtonDelClickSignal ) },
	{ "MoveUpSection", GTK_STOCK_GO_UP,  N_("Move up"),  NULL, N_("Priority order Move up"), G_CALLBACK( ButtonMoveUpClickSignal ) },
	{ "MoveDownSection", GTK_STOCK_GO_DOWN, N_("Move down"),  NULL, N_("Priority order Move down"), G_CALLBACK( ButtonMoveDownClickSignal ) },
	{ "PropertiesSection", GTK_STOCK_PROPERTIES, N_("Properties"), NULL, N_("Section Properties"), G_CALLBACK( ButtonPropertiesClickSignal ) } 
};
static const gchar *Manager_ui_strings = 
"<ui>"
"  <toolbar name='ToolBar'>"
"		<toolitem action='AddSection' />"
"		<toolitem action='DelSection' />"
"		<separator />"
"		<toolitem action='MoveUpSection' />"
"		<toolitem action='MoveDownSection' />"
"		<separator />"
"		<toolitem action='PropertiesSection' />"
"	</toolbar>"
"</ui>";

void ManagerInitGtk()
{
	GtkWidget *vbox;
//	GtkWidget *hbox;
	GtkWidget *ScrollWin;
	long ScanCol;
	GtkCellRenderer   *renderer;
	char * ColName[ ] = { N_("Nbr"), N_("Section Name"), N_("Language"), N_("Type"), N_("debug") };

//	pNameSectionSelected = NULL;

	ManagerWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title ( (GtkWindow *)ManagerWindow, _("Sections Manager") );

	vbox = MY_GTK_NEW_BOX (MY_GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (ManagerWindow), vbox);
	gtk_widget_show (vbox);

	GError *error = NULL;
	GtkActionGroup * MgrActionGroup;
	ManageruiManager = gtk_ui_manager_new( );
	
	MgrActionGroup = gtk_action_group_new( "ManagerActionGroup" );

	// for i18n menus !
	gtk_action_group_set_translation_domain( MgrActionGroup, "classicladder" );

	gtk_action_group_add_actions( MgrActionGroup, ManagerEntriesArray, G_N_ELEMENTS( ManagerEntriesArray ), NULL );
	
	gtk_ui_manager_insert_action_group( ManageruiManager, MgrActionGroup, 0 );
	if (!gtk_ui_manager_add_ui_from_string( ManageruiManager, Manager_ui_strings, -1/*length*/, &error ))
	{
		g_message ("Manager failed to build gtk menus: %s", error->message);
		g_error_free (error);
	}
	ToolBarWidget = gtk_ui_manager_get_widget( ManageruiManager, "/ToolBar" );
	gtk_toolbar_set_style( GTK_TOOLBAR(ToolBarWidget), GTK_TOOLBAR_ICONS );
//	gtk_toolbar_set_style( GTK_TOOLBAR(ToolBarWidget), GTK_TOOLBAR_BOTH );
	gtk_box_pack_start( GTK_BOX(vbox), ToolBarWidget, FALSE, FALSE, 0 );
//GTK3 now default size for the window! gtk_widget_set_usize((GtkWidget *)ToolBarWidget,475/*ToSeeAllButtons! 367*/,0);

	/* Create a list-model and the view. */
	ListStore = gtk_list_store_new( SECTION_NBR_INFOS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
	ListViewSections = gtk_tree_view_new_with_model( GTK_TREE_MODEL(ListStore) );

	/* Add the columns to the view. */
	for (ScanCol=1; ScanCol<SECTION_NBR_INFOS; ScanCol++)
	{
		GtkTreeViewColumn *column;
		renderer = gtk_cell_renderer_text_new();
//		g_object_set(renderer, "editable", TRUE, NULL);
//TODO? gtk_entry_set_max_length(GTK_ENTRY(  ),9);
//		g_signal_connect( G_OBJECT(renderer), "edited", G_CALLBACK(Callback_TextEdited), (gpointer)ScanCol );
		column = gtk_tree_view_column_new_with_attributes( gettext(ColName[ ScanCol ]), renderer, "text", ScanCol, NULL );
		gtk_tree_view_append_column( GTK_TREE_VIEW(ListViewSections), column );
		gtk_tree_view_column_set_resizable( column, TRUE );
//////		gtk_tree_view_column_set_sort_column_id( column, ScanCol );
	}
	gtk_tree_view_set_grid_lines( GTK_TREE_VIEW(ListViewSections), GTK_TREE_VIEW_GRID_LINES_BOTH );

	// Create a Scrolled Window that will contain the GtkTreeView
	ScrollWin = gtk_scrolled_window_new (NULL, NULL);
	// here we add the view to the scrolled !
	gtk_container_add(GTK_CONTAINER(ScrollWin), ListViewSections);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ScrollWin),
                                    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX (vbox), ScrollWin, TRUE, TRUE, 0);
	gtk_widget_show( ListViewSections );
	gtk_widget_show( ScrollWin );

	gtk_signal_connect(GTK_OBJECT (ListViewSections), "cursor-changed",
		GTK_SIGNAL_FUNC(TreeViewCursorChangedSignal), 0);

//v0.9.20	ManagerDisplaySections( );
	gtk_signal_connect( GTK_OBJECT(ManagerWindow), "delete_event",
		GTK_SIGNAL_FUNC(ManagerWindowDeleteEvent), 0 );
	gtk_window_set_icon_name (GTK_WINDOW( ManagerWindow ), GTK_STOCK_DND_MULTIPLE);
//RestoreWindowPosiPrefs( "Manager", ManagerWindow );

gtk_window_set_default_size ( GTK_WINDOW(ManagerWindow), -1, 130);

//gtk_widget_show (ManagerWindow);

	AddSectionWindowInit( );
}

