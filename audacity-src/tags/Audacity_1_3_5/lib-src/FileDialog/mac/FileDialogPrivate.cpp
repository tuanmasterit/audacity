/////////////////////////////////////////////////////////////////////////////
// Name:        filedlg.cpp
// Purpose:     wxFileDialog
// Author:      Stefan Csomor
// Modified by: Leland Lucius
// Created:     1998-01-01
// RCS-ID:      $Id: FileDialogPrivate.cpp,v 1.2 2008-03-10 22:20:18 richardash1981 Exp $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
//
// Modified for Audacity to support an additional button on Save dialogs
//
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#include "wx/app.h"
#include "wx/utils.h"
#include "wx/dialog.h"
#include "wx/filedlg.h"
#include "wx/intl.h"
#include "wx/tokenzr.h"
#include "wx/filename.h"

#include "../FileDialog.h"

#ifndef __DARWIN__
#include "PLStringFuncs.h"
#endif

IMPLEMENT_CLASS(FileDialog, wxFileDialogBase)

// begin wxmac

#include "wx/mac/private.h"

#ifndef __DARWIN__
#include <Navigation.h>
#endif

extern bool gUseNavServices ;

#define kCustom 'cstm'
#define kChoice 1
#define kButton 2

static ControlID kChoiceID = { kCustom, kChoice };
static ControlID kButtonID = { kCustom, kButton };

// the data we need to pass to our standard file hook routine
// includes a pointer to the dialog, a pointer to the standard
// file reply record (so we can inspect the current selection)
// and a copy of the "previous" file spec of the reply record
// so we can see if the selection has changed

struct CustomData {
    FileDialog     *me;
    Rect           bounds;
    ControlRef     userpane;
    ControlRef     choice;
    ControlRef     button;
    MenuRef        menu;
    wxArrayString  name;
    wxArrayString  extensions;
    wxArrayLong    filtermactypes;
    wxString       defaultLocation;
    int            currentfilter;
    bool           saveMode;
};

typedef struct CustomData
CustomData, *CustomDataPtr;

static pascal void    NavEventProc(
                                   NavEventCallbackMessage        inSelector,
                                   NavCBRecPtr                    ioParams,
                                   NavCallBackUserData            ioUserData);

static NavEventUPP    sStandardNavEventFilter = NewNavEventUPP(NavEventProc);

static void HandleAdjustRect(NavCBRecPtr callBackParms, CustomData * data)
{
    Rect rect;
    
    if (!data->userpane)
    {
        return;
    }
    
    GetControlBounds(data->userpane, &rect);
    
    SInt16 w = rect.right - rect.left;
    SInt16 h = rect.bottom - rect.top;
    SInt16 cw = callBackParms->customRect.right - callBackParms->customRect.left;
    SInt16 ch = callBackParms->customRect.bottom - callBackParms->customRect.top;
    
    MoveControl(data->userpane,
                callBackParms->customRect.left + ((cw-w) / 2),
                callBackParms->customRect.top + ((ch-h) / 2));
    
    return;
}

static void HandleCarbonCustomizeEvent(NavCBRecPtr callBackParms, CustomData * data)
{
    if ((callBackParms->customRect.right == 0) && (callBackParms->customRect.bottom == 0))
    {
        callBackParms->customRect.right = callBackParms->customRect.left +
        (data->bounds.right - data->bounds.left);
        callBackParms->customRect.bottom = callBackParms->customRect.top +
            (data->bounds.bottom - data->bounds.top);
    }
}

static void HandleCustomMouseDown(NavCBRecPtr callBackParms, CustomData *data)
{
	EventRecord *evt = callBackParms->eventData.eventDataParms.event;
	Point where = evt->where;
    
	GlobalToLocal(&where);
    
	ControlRef control = FindControlUnderMouse(where, callBackParms->window, NULL);
    
	if (control != NULL)
    {
        ControlID cid;
        GetControlID(control, &cid);
        
        HandleControlClick(control, where, evt->modifiers, (ControlActionUPP)-1L);
        
        if (cid.signature == kCustom)
        {
            switch (cid.id)
            {
                case kChoice:
                {
                    MenuRef menu;
                    UInt32 v;
                    menu = GetControlPopupMenuRef(control);
                    v = GetControl32BitValue(control) - 1;
                    const size_t numFilters = data->extensions.GetCount();
                    
                    if (v < numFilters)
                    {
                        data->currentfilter = v;
                        if (data->saveMode)
                        {
                            int i = data->currentfilter;
                            
                            wxString extension =  data->extensions[i].AfterLast('.') ;
                            extension.MakeLower() ;
                            wxString sfilename ;
                            
                            wxMacCFStringHolder cfString(NavDialogGetSaveFileName(callBackParms->context), false);
                            sfilename = cfString.AsString() ;
#if 0
                            int pos = sfilename.Find('.', true) ;
                            if (pos != wxNOT_FOUND)
                            {
                                sfilename = sfilename.Left(pos + 1) + extension;
                                cfString.Assign(sfilename, wxFONTENCODING_DEFAULT);
                                NavDialogSetSaveFileName(callBackParms->context, cfString);
                            }
#endif
                        }
                    }
                    
                    break;
                }
                    
                case kButton:
                {
                    data->me->ClickButton(GetControl32BitValue(data->choice) - 1);
                    
                    break;
                }
            }
        }
    }
}

static void HandleNormalEvents(NavCBRecPtr callBackParms, CustomData *data)
{
	switch (callBackParms->eventData.eventDataParms.event->what)
    {
		case mouseDown:
        {
			HandleCustomMouseDown(callBackParms, data);
			break;
        }
    }			
}

static void HandleStartEvent(NavCBRecPtr callBackParms, CustomData *data)
{
    CreateUserPaneControl(callBackParms->window, &data->bounds, kControlSupportsEmbedding, &data->userpane);
    EmbedControl(data->choice, data->userpane);
    EmbedControl(data->button, data->userpane);
    
    NavCustomControl(callBackParms->context, kNavCtlAddControl, data->userpane);
    
    HandleAdjustRect(callBackParms, data);
    
    if (data && !(data->defaultLocation).IsEmpty()) {
        // Set default location for the modern Navigation APIs
        // Apple Technical Q&A 1151
        FSSpec theFSSpec;
        wxMacFilename2FSSpec(data->defaultLocation, &theFSSpec);
        AEDesc theLocation = {typeNull, NULL};
        if (noErr == ::AECreateDesc(typeFSS, &theFSSpec, sizeof(FSSpec), &theLocation))
            ::NavCustomControl(callBackParms->context, kNavCtlSetLocation, (void *) &theLocation);
    }
}

static pascal void NavEventProc(NavEventCallbackMessage inSelector,
                                NavCBRecPtr ioParams,
                                NavCallBackUserData ioUserData)
{
    CustomData * data = (CustomData *) ioUserData ;
    
    switch (inSelector)
    {
		case kNavCBCustomize:
        {
			HandleCarbonCustomizeEvent(ioParams, data);
			break;
        }
            
        case kNavCBStart:
        {
            HandleStartEvent(ioParams, data);
            break;
        }
            
		case kNavCBAdjustRect:
        {
            HandleAdjustRect(ioParams, data);
			break;
        }
            
		case kNavCBEvent:
        {
			HandleNormalEvents(ioParams, data);
			break;
        }
    }
}

static void MakeUserDataRec(CustomData *myData, const wxString& filter)
{
    myData->currentfilter = 0 ;
    
    if (!filter.IsEmpty())
    {
        wxString filter2(filter) ;
        int filterIndex = 0;
        bool isName = true ;
        wxString current ;
        for( unsigned int i = 0; i < filter2.Len() ; i++ )
        {
            if( filter2.GetChar(i) == wxT('|') )
            {
                if( isName ) {
                    myData->name.Add( current ) ;
                }
                else {
                    myData->extensions.Add( current.MakeUpper() ) ;
                    ++filterIndex ;
                }
                isName = !isName ;
                current = wxEmptyString ;
            }
            else
            {
                current += filter2.GetChar(i) ;
            }
        }
        // we allow for compatibility reason to have a single filter expression (like *.*) without
        // an explanatory text, in that case the first part is name and extension at the same time
        
        wxASSERT_MSG( filterIndex == 0 || !isName , wxT("incorrect format of format string") ) ;
        if ( current.IsEmpty() )
            myData->extensions.Add( myData->name[filterIndex] ) ;
        else
            myData->extensions.Add( current.MakeUpper() ) ;
        if ( filterIndex == 0 || isName )
            myData->name.Add( current.MakeUpper() ) ;
        
        ++filterIndex ;
        
        const size_t extCount = myData->extensions.GetCount();
        for ( size_t i = 0 ; i < extCount; i++ )
        {
            wxUint32 fileType;
            wxUint32 creator;
            wxString extension = myData->extensions[i];
            
            if (extension.GetChar(0) == '*')
                extension = extension.Mid(1);	// Remove leading *
            
            if (extension.GetChar(0) == '.')
            {
                extension = extension.Mid(1);	// Remove leading .
            }
            
            if (wxFileName::MacFindDefaultTypeAndCreator( extension, &fileType, &creator ))
            {
                myData->filtermactypes.Add( (OSType)fileType );
            }
            else
            {
                myData->filtermactypes.Add( '****' ) ;		// We'll fail safe if it's not recognized
            }
        }
    }
}

static Boolean CheckFile( const wxString &filename , OSType type , CustomDataPtr data)
{
    wxString file(filename) ;
    file.MakeUpper() ;
    
    if ( data->extensions.GetCount() > 0 )
    {
        //for ( int i = 0 ; i < data->numfilters ; ++i )
        int i = data->currentfilter ;
        if ( data->extensions[i].Right(2) == wxT(".*") )
            return true ;
        
        {
            if ( type == (OSType)data->filtermactypes[i] )
                return true ;
            
            wxStringTokenizer tokenizer( data->extensions[i] , wxT(";") ) ;
            while( tokenizer.HasMoreTokens() )
            {
                wxString extension = tokenizer.GetNextToken() ;
                if ( extension.GetChar(0) == '*' )
                    extension = extension.Mid(1) ;
                
                if ( file.Len() >= extension.Len() && extension == file.Right(extension.Len() ) )
                    return true ;
            }
        }
        return false ;
    }
    return true ;
}

// end wxmac

FileDialog::FileDialog(wxWindow *parent, const wxString& message,
                       const wxString& defaultDir, const wxString& defaultFileName, const wxString& wildCard,
                       long style, const wxPoint& pos)
:wxFileDialogBase(parent, message, defaultDir, defaultFileName, wildCard, style, pos)
{
    wxASSERT_MSG( NavServicesAvailable() , wxT("Navigation Services are not running") ) ;
    m_callback = NULL;
    m_cbdata = NULL;
    m_dialogStyle = style;
}

static pascal Boolean CrossPlatformFilterCallback (
                                                   AEDesc *theItem,
                                                   void *info,
                                                   void *callBackUD,
                                                   NavFilterModes filterMode
                                                   )
{
    bool display = true;
    CustomDataPtr data = (CustomDataPtr) callBackUD ;
    
    if (filterMode == kNavFilteringBrowserList)
    {
        NavFileOrFolderInfo* theInfo = (NavFileOrFolderInfo*) info ;
        if ( !theInfo->isFolder )
        {
            if (theItem->descriptorType == typeFSS )
            {
                FSSpec    spec;
                memcpy( &spec , *theItem->dataHandle , sizeof(FSSpec) ) ;
                wxString file = wxMacMakeStringFromPascal( spec.name ) ;
                display = CheckFile( file , theInfo->fileAndFolder.fileInfo.finderInfo.fdType , data ) ;
            }
            else if ( theItem->descriptorType == typeFSRef )
            {
                FSRef fsref ;
                memcpy( &fsref , *theItem->dataHandle , sizeof(FSRef) ) ;
                wxString file = wxMacFSRefToPath( &fsref ) ;
                display = CheckFile( file , theInfo->fileAndFolder.fileInfo.finderInfo.fdType , data ) ;
            }
        }
    }
    
    return display;
}

int FileDialog::ShowModal()
{
    OSErr err;
    NavDialogCreationOptions dialogCreateOptions;
    // set default options
    ::NavGetDefaultDialogCreationOptions(&dialogCreateOptions);
    
    // this was always unset in the old code
    dialogCreateOptions.optionFlags &= ~kNavSelectDefaultLocation;
    
    wxMacCFStringHolder message(m_message, m_font.GetEncoding());
    dialogCreateOptions.windowTitle = message;
    
    wxMacCFStringHolder defaultFileName(m_fileName, m_font.GetEncoding());
    dialogCreateOptions.saveFileName = defaultFileName;
    
    
    NavDialogRef dialog;
    NavObjectFilterUPP navFilterUPP = NULL;
    CustomData myData;
    
    SetRect(&myData.bounds, 0, 0, 0, 0);
    myData.me = this;
    myData.defaultLocation = m_dir;
    myData.userpane = NULL;
    myData.choice = NULL;
    myData.button = NULL;
    myData.saveMode = false;
    
    Rect r;
    SInt16 base;
    SInt16 margin = 3;
    SInt16 gap = 0;
    
    MakeUserDataRec(&myData , m_wildCard);
    myData.currentfilter = m_filterIndex;
    size_t numFilters = myData.extensions.GetCount();
    if (numFilters)
    {
        CreateNewMenu(0, 0, &myData.menu);
        
        for ( size_t i = 0 ; i < numFilters ; ++i )
        {
            ::AppendMenuItemTextWithCFString(myData.menu,
                                             wxMacCFStringHolder(myData.name[i],
                                                                 m_font.GetEncoding()),
                                             0,
                                             i,
                                             NULL);
        }
        
        SetRect(&r, 0, margin, 0, 0);
        CreatePopupButtonControl(NULL, &r, CFSTR("Format:"), -12345, false, 50, teJustLeft, normal, &myData.choice);
        SetControlID(myData.choice, &kChoiceID);
        SetControlPopupMenuRef(myData.choice, myData.menu);
        SetControl32BitMinimum(myData.choice, 1);
        SetControl32BitMaximum(myData.choice, myData.name.GetCount());
        SetControl32BitValue(myData.choice, myData.currentfilter + 1);
        GetBestControlRect(myData.choice, &r, &base);
        SizeControl(myData.choice, r.right - r.left, r.bottom - r.top);
        UnionRect(&myData.bounds, &r, &myData.bounds);
        gap = 15;
    }

    if (!m_buttonlabel.IsEmpty())
    {
        wxMacCFStringHolder cfString(wxStripMenuCodes(m_buttonlabel).c_str(), wxFONTENCODING_DEFAULT);
        SetRect(&r, myData.bounds.right + gap, margin, 0, 0);
        CreatePushButtonControl(NULL, &r, cfString, &myData.button);
        SetControlID(myData.button, &kButtonID);
        GetBestControlRect(myData.button, &r, &base);
        SizeControl(myData.button, r.right - r.left, r.bottom - r.top);
        UnionRect(&myData.bounds, &r, &myData.bounds);
    }
    
    // Expand bounding rectangle to include a top and bottom margin
    myData.bounds.top -= margin;
    myData.bounds.bottom += margin;
    
    dialogCreateOptions.optionFlags |= kNavNoTypePopup;
    
    if (m_dialogStyle & wxSAVE)
    {
        myData.saveMode = true;
        
        if (!numFilters)
        {
            dialogCreateOptions.optionFlags |= kNavNoTypePopup;
        }
        dialogCreateOptions.optionFlags |= kNavDontAutoTranslate;
        dialogCreateOptions.optionFlags |= kNavDontAddTranslateItems;
        
        // The extension is important
        if (numFilters < 2)
            dialogCreateOptions.optionFlags |= kNavPreserveSaveFileExtension;
        
#if TARGET_API_MAC_OSX
        if (!(m_dialogStyle & wxOVERWRITE_PROMPT))
        {
	        dialogCreateOptions.optionFlags |= kNavDontConfirmReplacement;
        }
#endif
        err = ::NavCreatePutFileDialog(&dialogCreateOptions,
                                       // Suppresses the 'Default' (top) menu item
                                       kNavGenericSignature, kNavGenericSignature,
                                       sStandardNavEventFilter,
                                       &myData, // for defaultLocation
                                       &dialog);
    }
    else
    {
        
        //let people select bundles/programs in dialogs
        dialogCreateOptions.optionFlags |= kNavSupportPackages;
        
        navFilterUPP = NewNavObjectFilterUPP(CrossPlatformFilterCallback);
        err = ::NavCreateGetFileDialog(&dialogCreateOptions,
                                       NULL, // NavTypeListHandle
                                       sStandardNavEventFilter,
                                       NULL, // NavPreviewUPP
                                       navFilterUPP,
                                       (void *) &myData, // inClientData
                                       &dialog);
    }
    
    if (err == noErr)
        err = ::NavDialogRun(dialog);
    
    // clean up filter related data, etc.
    if (navFilterUPP)
        ::DisposeNavObjectFilterUPP(navFilterUPP);
    
    if (err != noErr)
        return wxID_CANCEL;
    
    NavReplyRecord navReply;
    err = ::NavDialogGetReply(dialog, &navReply);
    if (err == noErr && navReply.validRecord)
    {
        AEKeyword   theKeyword;
        DescType    actualType;
        Size        actualSize;
        FSRef       theFSRef;
        wxString thePath ;
        
        m_filterIndex = myData.currentfilter;
        
        long count;
        ::AECountItems(&navReply.selection , &count);
        for (long i = 1; i <= count; ++i)
        {
            err = ::AEGetNthPtr(&(navReply.selection), i, typeFSRef, &theKeyword, &actualType,
                                &theFSRef, sizeof(theFSRef), &actualSize);
            if (err != noErr)
                break;
            
            if (m_dialogStyle & wxSAVE)
                thePath = wxMacFSRefToPath( &theFSRef , navReply.saveFileName ) ;
            else
                thePath = wxMacFSRefToPath( &theFSRef ) ;
            
            if (!thePath)
            {
                ::NavDisposeReply(&navReply);
                return wxID_CANCEL;
            }
            
            m_path = ConvertSlashInFileName(thePath);
            m_paths.Add(m_path);
            m_fileName = wxFileNameFromPath(m_path);
            m_fileNames.Add(m_fileName);
        }
        // set these to the first hit
        m_path = m_paths[0];
        m_fileName = wxFileNameFromPath(m_path);
        m_dir = wxPathOnly(m_path);
    }
    ::NavDisposeReply(&navReply);
    
    return (err == noErr) ? wxID_OK : wxID_CANCEL;
}

wxString FileDialog::ConvertSlashInFileName(const wxString& filePath)
{
#if TARGET_API_MAC_OSX
   wxString path = filePath;
   wxString filename;
   wxString newPath = filePath;
   int pathLen = 1;
   while (!wxDirExists(wxPathOnly(newPath)) && ! path.IsEmpty()) {
      path = newPath.BeforeLast('/');
      filename = newPath.AfterLast('/');
      newPath = path;
      newPath += ':';
      newPath += filename;
   }
   return newPath;
#else
   return filePath;
#endif
}
