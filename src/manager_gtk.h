void ManagerDisplaySections( char ForgetSectionSelected, char RefreshComboSectionLists );
void ChangeSectionSelectedFromComboIndex( int SectionIndexToSelect );
void ChangeSectionSelectedInManager( int SectionToSelect );
void SelectNextSectionInManager(void);
void SelectPreviousSectionInManager(void);
// if this file included and gtk.h not included before... (this is the case in search.c !)
#ifdef __GTK_H__
void OpenManagerWindow( GtkAction * ActionOpen, gboolean OpenIt );
#endif
void RememberManagerWindowPrefs( void );
void OpenManagerWindowFromPopup( void );
void ManagerEnableActionsSectionsList( char cState );
void ManagerInitGtk();
