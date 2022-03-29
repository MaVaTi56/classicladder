/* Classic Ladder Project */
/* Copyright (C) 2001-2021 Marc Le Douarain */
/* http://www.sourceforge.net/projects/classicladder */
/* http://sites.google.com/site/classicladder */
/* December 2019 */
/* ------------------------------ */
/* Variables browser - GTK window */
/* ------------------------------ */
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
#include <libintl.h> // i18n
#include <locale.h> // i18n
#include "classicladder.h"
#include "global.h"
#include "classicladder_gtk.h"
#include "vars_names.h"
#include "symbols.h"
#include "vars_browser_gtk.h"

static GtkWidget *VarsBrowserDialogBox;
static GtkWidget  *VarsBrowserScrolledWin;
static GtkListStore *ListStore;
static GtkWidget * pComboTypeVar;
static GtkWidget *ListViewVariables;
static int TypeVarForPreselectionInCombo = -1; // no variable already entered
static int OffsetVarForPreselectionInList = -1; // no variable already entered
static int IndexTypeVarForPreselectionInCombo = 0; // index in the combo for type selection

enum
{
	TYPE_VAR, //hidden column
	OFFSET_VAR, //hidden column
	VAR_NAME,
	SYMBOL,
	COMMENT,
	NBR_INFOS
};

void RefreshVarsListForCurrentType( int IndexInTypeVar )
{
	GtkTreeIter   iter;
	int ScanVar;
	char BuffVarName[ 100 ];
	
	// put (back) vertical lift at the top position...
	GtkAdjustment * pGtkAdjust = gtk_scrolled_window_get_vadjustment( GTK_SCROLLED_WINDOW(VarsBrowserScrolledWin) );
	if ( pGtkAdjust )
		gtk_adjustment_set_value( pGtkAdjust, 0.0 );

	gtk_list_store_clear( ListStore );

	for ( ScanVar=0; ScanVar<TableConvIdVarName[IndexInTypeVar].iSize1; ScanVar++ )
	{
		StrSymbol * pSymbol;
		// Acquire an iterator
		gtk_list_store_append( ListStore, &iter );

		BuffVarName[0] = '%';
		sprintf( &BuffVarName[1], TableConvIdVarName[IndexInTypeVar].StringBaseVarName, ScanVar );
		pSymbol = ConvVarNameInSymbolPtr( BuffVarName );
		
		// fill the element
		gtk_list_store_set( ListStore, &iter,
					TYPE_VAR, 0,
					OFFSET_VAR, ScanVar,
                    VAR_NAME, BuffVarName,
                    SYMBOL, pSymbol?pSymbol->Symbol:"",
                    COMMENT, pSymbol?pSymbol->Comment:"",
                    -1);
	}

	//if any current variable, select it per-default in the variables list
	if( IndexTypeVarForPreselectionInCombo==IndexInTypeVar && OffsetVarForPreselectionInList!=-1 )
	{
		GtkTreePath * PathToSelect;
		char BuffLine[ 20 ];
		sprintf( BuffLine, "%d", OffsetVarForPreselectionInList );
		PathToSelect = gtk_tree_path_new_from_string(BuffLine);
		if( PathToSelect )
		{
			printf("path to select var per-default '%s'...\n",BuffLine);
			GtkTreeSelection * selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ListViewVariables ) );
			gtk_tree_selection_select_path(selection,PathToSelect);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW( ListViewVariables ), PathToSelect, NULL, TRUE/*use_align*/, 0.3, 0 );
			gtk_tree_path_free(PathToSelect);
		}
	}
}

static gint ComboBox_changed_event(GtkWidget *widget, int NotUsed)
{
	RefreshVarsListForCurrentType( gtk_combo_box_get_active( GTK_COMBO_BOX( pComboTypeVar ) ) );
	return TRUE;
}	
static gboolean ListViewVariables_button_press_event( GtkWidget *widget, GdkEventButton *event )
{
//	printf("clicked on list\n");
	if( event->type==GDK_2BUTTON_PRESS )
	{
		printf("double-clicked on list, will now simulate 'OK' response !\n");
		gtk_dialog_response( GTK_DIALOG(VarsBrowserDialogBox), GTK_RESPONSE_OK );
	}
	return FALSE; // to let gtk do the normal selection on its side...
}

void OpenBrowserVarsToReplace( GtkWidget *widget, GtkWidget * pEntryWidget )
{
	if( OpenBrowserVars( pEntryWidget, FALSE/*AddMode*/ ) )
	{
		// for free vars display, signal required to be taken into account (simulate as an "enter" key)...
		g_signal_emit_by_name( pEntryWidget, "activate" );
	}
}
//used in arithm expressions edit
void OpenBrowserVarsToAdd( GtkWidget *widget, GtkWidget * pEntryWidget )
{
	OpenBrowserVars( pEntryWidget, TRUE/*AddMode*/ );
}
//used in properties, to replace or add if entry actually ending with '[' character...
void OpenBrowserVarsToReplaceOrAddIndex( GtkWidget *widget, GtkWidget * pEntryWidget )
{
	char cDoAnAdd = FALSE;
	char * pVarName = (char *)gtk_entry_get_text(GTK_ENTRY(pEntryWidget));
	if( pVarName[0]!='\0' )
	{
		if( pVarName[strlen( pVarName )-1]=='[' )
		{
			printf("-->Detect currently ending with '[', so doing an adding in vars browser\n");
			cDoAnAdd = TRUE;
		}
	}
	if( OpenBrowserVars( pEntryWidget, cDoAnAdd/*AddMode*/ ) )
	{
		if( cDoAnAdd )
			// for free vars display, signal required to be taken into account (simulate as an "enter" key)...
			g_signal_emit_by_name( pEntryWidget, "activate" );
	}
}

//if AddMode=TRUE => add variable selected, else at FALSE => replace with variable selected
char OpenBrowserVars( GtkWidget * pEntryWidget, char AddMode )
{
	GtkWidget *DialogContentAreaVBox;
	GtkWidget * pLabel;
	long ScanCol;
	char * ColName[] = { "HiddenColTypeVar!", "HiddenColOffsetVar!", N_("Variable"), N_("Symbol name"), N_("Comment") };
	char BuffVarType[ 100 ];
	int ScanVarTypeLine = 0;
//////	char GrabEntryWidgetFocus = FALSE;

	char VarSelectionDone = FALSE;
	TypeVarForPreselectionInCombo = -1; // no variable already entered
	OffsetVarForPreselectionInList = -1; // no variable already entered
	IndexTypeVarForPreselectionInCombo = 0; // index in the combo for type selection
	
	VarsBrowserDialogBox = gtk_dialog_new_with_buttons( _("Variables browser"),
		GTK_WINDOW( MainSectionWindow ),
		GTK_DIALOG_MODAL,
		GTK_STOCK_OK,GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
		NULL);
	
	// it is already a vbox !!!
	DialogContentAreaVBox = gtk_dialog_get_content_area(GTK_DIALOG(VarsBrowserDialogBox));
	
	pLabel = gtk_label_new( _("Vars type selection :") );
	gtk_box_pack_start(GTK_BOX(DialogContentAreaVBox), pLabel, FALSE, FALSE, 0);

	// for replace mode, get current type/offset of the variable in the widget, to select per-default the current variable in the list !
	if( !AddMode )
	{
		int VarTypeConvert,VarOffsetConvert;
		char * pVarName = (char *)gtk_entry_get_text(GTK_ENTRY(pEntryWidget));
		if (TextParserForAVar(pVarName , &VarTypeConvert, &VarOffsetConvert, NULL, FALSE/*PartialNames*/))
		{
			TypeVarForPreselectionInCombo = VarTypeConvert;
			OffsetVarForPreselectionInList = VarOffsetConvert;
printf("converted var %d/%d for preselection combo / list\n",TypeVarForPreselectionInCombo,OffsetVarForPreselectionInList);
		}
	}

	/* Create a combo for type of vars list */
	pComboTypeVar = gtk_combo_box_new_text();
	do
	{
		char * pScanPercent = BuffVarType+1;
		char PercentFound = FALSE;
		sprintf( BuffVarType, "%c%s (%s-%s) : %s", '%', TableConvIdVarName[ScanVarTypeLine].StringBaseVarName,
			(TEST_VAR_IS_A_BOOL(TableConvIdVarName[ScanVarTypeLine].iTypeVar,0))?(_("Bool")):(_("Integer")),
			TableConvIdVarName[ScanVarTypeLine].cReadWriteAccess?(_("R/W")):(_("R")),
			TableConvIdVarName[ScanVarTypeLine].StringVarUsage );
		do
		{
			if( *pScanPercent=='%' )
			{
				PercentFound = TRUE;
				*pScanPercent++ = 'x';
				*pScanPercent = 'x';
			}
			pScanPercent++;
		}
		while( !PercentFound && *pScanPercent!='\0');
		gtk_combo_box_append_text( MY_GTK_COMBO_BOX(pComboTypeVar), BuffVarType );
		if( TypeVarForPreselectionInCombo!=-1 && TableConvIdVarName[ScanVarTypeLine].iTypeVar==TypeVarForPreselectionInCombo )
		{
			IndexTypeVarForPreselectionInCombo = ScanVarTypeLine;
			printf("found index types combo for preselection = %d\n",ScanVarTypeLine);
		}
		ScanVarTypeLine++;
	}
	while( TableConvIdVarName[ScanVarTypeLine].StringBaseVarName );
	gtk_combo_box_set_active( GTK_COMBO_BOX( pComboTypeVar ), IndexTypeVarForPreselectionInCombo );
	gtk_box_pack_start (GTK_BOX(DialogContentAreaVBox), pComboTypeVar, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT( pComboTypeVar ), "changed", 
							GTK_SIGNAL_FUNC(ComboBox_changed_event), (void *)NULL );

	/* Create a list-model and the view. */
	ListStore = gtk_list_store_new( NBR_INFOS, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
	ListViewVariables = gtk_tree_view_new_with_model ( GTK_TREE_MODEL(ListStore) );

	/* Add the columns to the view. */
	for (ScanCol=2; ScanCol<NBR_INFOS; ScanCol++)
	{
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
//		g_object_set(renderer, "editable", TRUE, NULL);
//TODO? gtk_entry_set_max_length(GTK_ENTRY(  ),9);
//		g_signal_connect( G_OBJECT(renderer), "edited", G_CALLBACK(Callback_TextEdited), (gpointer)ScanCol );
		column = gtk_tree_view_column_new_with_attributes( gettext(ColName[ ScanCol ]), renderer, "text", ScanCol, NULL );
		gtk_tree_view_append_column( GTK_TREE_VIEW(ListViewVariables), column );
		gtk_tree_view_column_set_resizable( column, TRUE );
//////		gtk_tree_view_column_set_sort_column_id( column, ScanCol );
	}
//	avail since gtk v2.10...?
	gtk_tree_view_set_grid_lines( GTK_TREE_VIEW(ListViewVariables), GTK_TREE_VIEW_GRID_LINES_BOTH );

	VarsBrowserScrolledWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (VarsBrowserScrolledWin),
                                    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	// here we add the view to the scrolled !
	gtk_container_add(GTK_CONTAINER(VarsBrowserScrolledWin), ListViewVariables);
	gtk_box_pack_start(GTK_BOX(DialogContentAreaVBox), VarsBrowserScrolledWin, TRUE, TRUE, 0);

	// connect to double-click event in the list to directly validate as a click on "OK" button...
	gtk_signal_connect( GTK_OBJECT(ListViewVariables), "button_press_event",
						GTK_SIGNAL_FUNC(ListViewVariables_button_press_event), NULL );

	RefreshVarsListForCurrentType( IndexTypeVarForPreselectionInCombo/*TypeVar*/ );


gtk_window_set_default_size (GTK_WINDOW (VarsBrowserDialogBox), -1, 600);
//gtk_window_resize (GTK_WINDOW (VarsBrowserDialogBox), 300, 450);

	gtk_widget_show_all(DialogContentAreaVBox);
	switch (gtk_dialog_run(GTK_DIALOG(VarsBrowserDialogBox)))
	{
		case GTK_RESPONSE_OK:
		{
			GtkTreeSelection * selection;
			GtkTreeModel * model;
			GtkTreeIter iter;
			selection = gtk_tree_view_get_selection( GTK_TREE_VIEW( ListViewVariables ) );
			if ( gtk_tree_selection_get_selected( selection, &model, &iter) )
			{
				int Selected0,Selected1;
				char * OtherVarName = NULL;
				gchar * pBuffVar;
				gtk_tree_model_get( model, &iter, 0/*ColumnNbr*/, &Selected0, 1/*ColumnNbr*/, &Selected1, -1/*EndOfList*/ );
				gtk_tree_model_get( model, &iter, 2/*ColumnNbr*/, &pBuffVar, -1/*EndOfList*/ );
				printf("selected: %d/%d - %s\n", Selected0,Selected1,pBuffVar);
				if( Preferences.DisplaySymbolsInMainWindow )
					OtherVarName = ConvVarNameToSymbol( pBuffVar );
				if( AddMode )
				{
					if( gtk_entry_get_text( GTK_ENTRY(pEntryWidget) )[0]=='#' )
					{
						gtk_entry_set_text(GTK_ENTRY(pEntryWidget),OtherVarName?OtherVarName:pBuffVar);
					}
					else
					{
						char * pVarToInsert = OtherVarName?OtherVarName:pBuffVar;
						gint EntryPosition = gtk_editable_get_position( GTK_EDITABLE(pEntryWidget) );
						gtk_editable_insert_text( GTK_EDITABLE(pEntryWidget), pVarToInsert, -1, &EntryPosition );
						gtk_editable_set_position( GTK_EDITABLE(pEntryWidget), EntryPosition+strlen(pVarToInsert)-1 );
					}
				}
				else
				{
					gtk_entry_set_text(GTK_ENTRY(pEntryWidget),OtherVarName?OtherVarName:pBuffVar);
				}
				g_free( pBuffVar );
				VarSelectionDone = TRUE;
			}
			else
			{
				printf("No variable selected...\n");
			}
			break;
		}
	}

	gtk_widget_destroy(VarsBrowserDialogBox);
//////	if( GrabEntryWidgetFocus )
//////		gtk_widget_grab_focus( pEntryWidget );
	return VarSelectionDone;
}


