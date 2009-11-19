///////////////////////////////////////////////////////////////////////////////
// Name:        MadSearchDialog.cpp
// Description:
// Author:      madedit@gmail.com
// Licence:     GPL
///////////////////////////////////////////////////////////////////////////////

#include "MadSearchDialog.h"
#include "MadReplaceDialog.h"

#include "MadEdit/MadEdit.h"

//Do not add custom headers.
//wx-dvcpp designer will remove them
////Header Include Start
////Header Include End


#include "../images/down.xpm"

MadSearchDialog *g_SearchDialog=NULL;

//----------------------------------------------------------------------------
// MadSearchDialog
//----------------------------------------------------------------------------
     //Add Custom Events only in the appropriate Block.
    // Code added in  other places will be removed by wx-dvcpp
    ////Event Table Start
BEGIN_EVENT_TABLE(MadSearchDialog,wxDialog)
	////Manual Code Start
	EVT_BUTTON(ID_WXBITMAPBUTTONRECENTFINDTEXT, MadSearchDialog::WxBitmapButtonRecentFindTextClick)
	EVT_MENU_RANGE(ID_RECENTFINDTEXT1, ID_RECENTFINDTEXT20, MadSearchDialog::OnRecentFindText)
	////Manual Code End
	
	EVT_CLOSE(MadSearchDialog::MadSearchDialogClose)
	EVT_KEY_DOWN(MadSearchDialog::MadSearchDialogKeyDown)
	EVT_ACTIVATE(MadSearchDialog::MadSearchDialogActivate)
	EVT_BUTTON(ID_WXBUTTONCLOSE,MadSearchDialog::WxButtonCloseClick)
	EVT_BUTTON(ID_WXBUTTONREPLACE,MadSearchDialog::WxButtonReplaceClick)
	EVT_BUTTON(ID_WXBUTTONCOUNT,MadSearchDialog::WxButtonCountClick)
	EVT_BUTTON(ID_WXBUTTONFINDPREV,MadSearchDialog::WxButtonFindPrevClick)
	EVT_BUTTON(ID_WXBUTTONFINDNEXT,MadSearchDialog::WxButtonFindNextClick)
	EVT_CHECKBOX(ID_WXCHECKBOXSEARCHINSELECTION,MadSearchDialog::WxCheckBoxSearchInSelectionClick)
	EVT_CHECKBOX(ID_WXCHECKBOXFINDHEX,MadSearchDialog::WxCheckBoxFindHexClick)
END_EVENT_TABLE()
    ////Event Table End



MadSearchDialog::MadSearchDialog( wxWindow *parent, wxWindowID id, const wxString &title, const wxPoint &position, const wxSize& size, long style )
    : wxDialog( parent, id, title, position, size, style)
{
    CreateGUIControls();
}

MadSearchDialog::~MadSearchDialog()
{

}

//static int gs_MinX=0;

static void ResizeItem(wxBoxSizer* sizer, wxWindow *item, int ax, int ay)
{
    int x, y;
    wxString str=item->GetLabel();
    item->GetTextExtent(str, &x, &y);
    item->SetSize(x+=ax, y+=ay);
    sizer->SetItemMinSize(item, x, y);

    //wxPoint pos=item->GetPosition();
    //if(pos.x + x > gs_MinX) gs_MinX = pos.x + x;
}

void MadSearchDialog::CreateGUIControls(void)
{
    //do not set FontName, it is not exist on all platforms
    #define wxFont(p0,p1,p2,p3,p4,p5) wxFont(wxDEFAULT,wxDEFAULT,p2,p3,p4)

    //Do not add custom Code here
    //wx-devcpp designer will remove them.
    //Add the custom code before or after the Blocks
    ////GUI Items Creation Start

	WxBoxSizer1 = new wxBoxSizer(wxHORIZONTAL);
	this->SetSizer(WxBoxSizer1);
	this->SetAutoLayout(true);

	WxBoxSizer2 = new wxBoxSizer(wxVERTICAL);
	WxBoxSizer1->Add(WxBoxSizer2, 0, wxALIGN_CENTER | wxALL, 0);

	WxBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
	WxBoxSizer2->Add(WxBoxSizer4, 0, wxALIGN_CENTER | wxALL, 0);

	WxBoxSizer5 = new wxBoxSizer(wxVERTICAL);
	WxBoxSizer2->Add(WxBoxSizer5, 0, wxALIGN_LEFT | wxALIGN_TOP | wxALL, 0);

	WxCheckBoxMoveFocus = new wxCheckBox(this, ID_WXCHECKBOXMOVEFOCUS, _("&Move Focus to Editor Window"), wxPoint(25, 2), wxSize(300, 22), 0, wxDefaultValidator, _("WxCheckBoxMoveFocus"));
	WxCheckBoxMoveFocus->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer5->Add(WxCheckBoxMoveFocus,0,wxALIGN_LEFT | wxALL,2);

	WxCheckBoxCaseSensitive = new wxCheckBox(this, ID_WXCHECKBOXCASESENSITIVE, _("&Case Sensitive"), wxPoint(25, 28), wxSize(300, 22), 0, wxDefaultValidator, _("WxCheckBoxCaseSensitive"));
	WxCheckBoxCaseSensitive->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer5->Add(WxCheckBoxCaseSensitive,0,wxALIGN_LEFT | wxALL,2);

	WxCheckBoxWholeWord = new wxCheckBox(this, ID_WXCHECKBOXWHOLEWORD, _("&Whole Word Only"), wxPoint(25, 54), wxSize(300, 22), 0, wxDefaultValidator, _("WxCheckBoxWholeWord"));
	WxCheckBoxWholeWord->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer5->Add(WxCheckBoxWholeWord,0,wxALIGN_LEFT | wxALL,2);

	WxCheckBoxRegex = new wxCheckBox(this, ID_WXCHECKBOXREGEX, _("Use Regular E&xpressions"), wxPoint(25, 80), wxSize(300, 22), 0, wxDefaultValidator, _("WxCheckBoxRegex"));
	WxCheckBoxRegex->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer5->Add(WxCheckBoxRegex,0,wxALIGN_LEFT | wxALL,2);

	WxCheckBoxFindHex = new wxCheckBox(this, ID_WXCHECKBOXFINDHEX, _("Find &Hex String (Example: BE 00 3A or BE003A)"), wxPoint(25, 106), wxSize(300, 22), 0, wxDefaultValidator, _("WxCheckBoxFindHex"));
	WxCheckBoxFindHex->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer5->Add(WxCheckBoxFindHex,0,wxALIGN_LEFT | wxALL,2);

	WxBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
	WxBoxSizer5->Add(WxBoxSizer6, 0, wxALIGN_LEFT | wxALL, 0);

	WxCheckBoxSearchInSelection = new wxCheckBox(this, ID_WXCHECKBOXSEARCHINSELECTION, _("Search In &Selection"), wxPoint(2, 2), wxSize(120, 22), 0, wxDefaultValidator, _("WxCheckBoxSearchInSelection"));
	WxCheckBoxSearchInSelection->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer6->Add(WxCheckBoxSearchInSelection,0,wxALIGN_LEFT | wxALL,2);

	WxStaticTextFrom = new wxStaticText(this, ID_WXSTATICTEXTFROM, _("From:"), wxPoint(126, 4), wxDefaultSize, 0, _("WxStaticTextFrom"));
	WxStaticTextFrom->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer6->Add(WxStaticTextFrom,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,2);

	WxEditFrom = new wxTextCtrl(this, ID_WXEDITFROM, _(""), wxPoint(160, 2), wxSize(80, 22), 0, wxDefaultValidator, _("WxEditFrom"));
	WxEditFrom->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer6->Add(WxEditFrom,0,wxALIGN_LEFT | wxALL,2);

	WxStaticTextTo = new wxStaticText(this, ID_WXSTATICTEXTTO, _("To:"), wxPoint(244, 4), wxDefaultSize, 0, _("WxStaticTextTo"));
	WxStaticTextTo->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer6->Add(WxStaticTextTo,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,2);

	WxEditTo = new wxTextCtrl(this, ID_WXEDITTO, _(""), wxPoint(268, 2), wxSize(80, 22), 0, wxDefaultValidator, _("WxEditTo"));
	WxEditTo->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer6->Add(WxEditTo,0,wxALIGN_LEFT | wxALL,2);

	WxBoxSizer3 = new wxBoxSizer(wxVERTICAL);
	WxBoxSizer1->Add(WxBoxSizer3, 0, wxALIGN_TOP | wxALL, 0);

	WxButtonFindNext = new wxButton(this, ID_WXBUTTONFINDNEXT, _("Find &Next"), wxPoint(2, 2), wxSize(100, 28), 0, wxDefaultValidator, _("WxButtonFindNext"));
	WxButtonFindNext->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer3->Add(WxButtonFindNext,0,wxALIGN_CENTER | wxALL,2);

	WxButtonFindPrev = new wxButton(this, ID_WXBUTTONFINDPREV, _("Find &Previous"), wxPoint(2, 34), wxSize(100, 28), 0, wxDefaultValidator, _("WxButtonFindPrev"));
	WxButtonFindPrev->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer3->Add(WxButtonFindPrev,0,wxALIGN_CENTER | wxALL,2);

	WxButtonCount = new wxButton(this, ID_WXBUTTONCOUNT, _("C&ount"), wxPoint(2, 66), wxSize(100, 28), 0, wxDefaultValidator, _("WxButtonCount"));
	WxButtonCount->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer3->Add(WxButtonCount,0,wxALIGN_CENTER | wxALL,2);

	WxButtonReplace = new wxButton(this, ID_WXBUTTONREPLACE, _("&Replace >>"), wxPoint(2, 98), wxSize(100, 28), 0, wxDefaultValidator, _("WxButtonReplace"));
	WxButtonReplace->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer3->Add(WxButtonReplace,0,wxALIGN_CENTER | wxALL,2);

	WxButtonClose = new wxButton(this, ID_WXBUTTONCLOSE, _("Close"), wxPoint(2, 128), wxSize(100, 28), 0, wxDefaultValidator, _("WxButtonClose"));
	WxButtonClose->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _("MS Sans Serif")));
	WxBoxSizer3->Add(WxButtonClose,0,wxALIGN_CENTER | wxALL,2);

	WxPopupMenuRecentFindText = new wxMenu(_(""));

	SetTitle(_("Search"));
	SetIcon(wxNullIcon);
	
	GetSizer()->Layout();
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	
    ////GUI Items Creation End

    //restore wxFont
    #undef wxFont

    this->SetPosition(wxPoint(300,100));

    int bw, bh;
    WxButtonFindNext->GetSize(&bw, &bh);

    m_FindText=new MadEdit(this, ID_MADEDIT, wxPoint(0, 0), wxSize(400, bh));
    m_FindText->SetSingleLineMode(true);
    m_FindText->SetEncoding(wxT("UTF-32LE"));
    m_FindText->SetFixedWidthMode(false);
    m_FindText->SetRecordCaretMovements(false);
    m_FindText->SetInsertSpacesInsteadOfTab(false);
    m_FindText->SetWantTab(false);
    m_FindText->LoadDefaultSyntaxScheme();

    WxBoxSizer4->Add(m_FindText,0,wxALIGN_CENTER_HORIZONTAL | wxALL,2);
    WxBoxSizer4->SetItemMinSize(m_FindText, 400, bh);

    wxBitmap WxBitmapButtonRecentFindText_BITMAP (down_xpm);
    WxBitmapButtonRecentFindText = new wxBitmapButton(this, ID_WXBITMAPBUTTONRECENTFINDTEXT, WxBitmapButtonRecentFindText_BITMAP, wxPoint(0,0), wxSize(bh,bh), wxBU_AUTODRAW, wxDefaultValidator, _("WxBitmapButtonRecentFindText"));
    WxBoxSizer4->Add(WxBitmapButtonRecentFindText,0,wxALIGN_CENTER_HORIZONTAL | wxALL,2);

    // resize checkbox
    ResizeItem(WxBoxSizer5, WxCheckBoxMoveFocus, 25, 4);
    ResizeItem(WxBoxSizer5, WxCheckBoxCaseSensitive, 25, 4);
    ResizeItem(WxBoxSizer5, WxCheckBoxWholeWord, 25, 4);
    ResizeItem(WxBoxSizer5, WxCheckBoxRegex, 25, 4);
    ResizeItem(WxBoxSizer5, WxCheckBoxFindHex, 25, 4);
    ResizeItem(WxBoxSizer6, WxCheckBoxSearchInSelection, 25, 4);
    ResizeItem(WxBoxSizer6, WxStaticTextFrom, 2, 2);
    ResizeItem(WxBoxSizer6, WxStaticTextTo, 2, 2);

    GetSizer()->Fit(this);

    // connect to KeyDown event handler
    m_FindText->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxBitmapButtonRecentFindText->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxCheckBoxMoveFocus->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxCheckBoxCaseSensitive->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxCheckBoxWholeWord->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxCheckBoxRegex->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxCheckBoxFindHex->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxCheckBoxSearchInSelection->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxEditFrom->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxEditTo->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxButtonFindNext->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxButtonFindPrev->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxButtonCount->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));
    WxButtonClose->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MadSearchDialog::MadSearchDialogKeyDown));


    m_RecentFindText=new wxFileHistory(20, ID_RECENTFINDTEXT1);
    m_RecentFindText->UseMenu(WxPopupMenuRecentFindText);

    wxConfigBase *m_Config=wxConfigBase::Get(false);
    wxString oldpath=m_Config->GetPath();
    m_Config->SetPath(wxT("/RecentFindText"));
    m_RecentFindText->Load(*m_Config);
    m_Config->SetPath(oldpath);

    if(m_RecentFindText->GetCount()>0)
    {
        wxString text=m_RecentFindText->GetHistoryFile(0);
        if(!text.IsEmpty())
        {
            m_FindText->SetText(text);
        }
    }

    SetDefaultItem(WxButtonFindNext);
}

void MadSearchDialog::MadSearchDialogClose(wxCloseEvent& event)
{
    // --> Don't use Close with a Dialog,
    // use Destroy instead.

    if(event.CanVeto())
    {
        event.Veto();
        Show(false);
        return;
    }

    g_SearchDialog=NULL;
    Destroy();
}


/*
 * WxButtonCloseClick
 */
void MadSearchDialog::WxButtonCloseClick(wxCommandEvent& event)
{
    Show(false);
}

/*
 * WxButtonFindNextClick
 */
void MadSearchDialog::WxButtonFindNextClick(wxCommandEvent& event)
{
    extern MadEdit *g_ActiveMadEdit;

    if(g_ActiveMadEdit==NULL)
        return;

    wxString text;
    m_FindText->GetText(text, true);

    if(text.Len()>0)
    {
        m_RecentFindText->AddFileToHistory(text);

        MadSearchResult sr;
        wxFileOffset selend = g_ActiveMadEdit->GetSelectionEndPos();

        wxInt64 from = 0, to = 0;
        wxFileOffset rangeFrom = -1, rangeTo = -1;
        if(WxCheckBoxSearchInSelection->IsChecked())
        {
            if(!StrToInt64(WxEditFrom->GetValue(), from))
            {
                wxMessageBox(_("The value of 'From' is incorrect."), wxT("MadEdit"), wxOK|wxICON_WARNING);
                return;
            }
            if(!StrToInt64(WxEditTo->GetValue(), to))
            {
                wxMessageBox(_("The value of 'To' is incorrect."), wxT("MadEdit"), wxOK|wxICON_WARNING);
                return;
            }

            rangeTo = to;
            wxFileOffset caretpos = g_ActiveMadEdit->GetCaretPosition();
            if(caretpos <= from || caretpos > to)
                rangeFrom = from;
        }

        for(;;)
        {
            if(WxCheckBoxFindHex->GetValue())
            {
                sr=g_ActiveMadEdit->FindHexNext(text, rangeFrom, rangeTo);
            }
            else
            {
                sr=g_ActiveMadEdit->FindTextNext(text,
                    WxCheckBoxRegex->GetValue(),
                    WxCheckBoxCaseSensitive->GetValue(),
                    WxCheckBoxWholeWord->GetValue(),
                    rangeFrom, rangeTo);
            }

            if(sr != SR_NO)
            {
                if(sr == SR_YES && g_ActiveMadEdit->GetCaretPosition() == selend)
                {
                    selend = -1;
                    continue;
                }
                break;
            }

            wxString msg(_("Cannot find the matched string."));
            msg += wxT("\n\n");
            msg += WxCheckBoxSearchInSelection->IsChecked()?
                _("Do you want to find from begin of selection?"):
                _("Do you want to find from begin of file?");

            if(wxCANCEL == wxMessageBox(msg, _("Find Next"), wxOK|wxCANCEL|wxICON_QUESTION ))
            {
                break;
            }
            rangeFrom = WxCheckBoxSearchInSelection->IsChecked()? from : 0;
        }
    }

    if(WxCheckBoxMoveFocus->GetValue())
    {
        ((wxFrame*)wxTheApp->GetTopWindow())->Raise();
        g_ActiveMadEdit->SetFocus();
    }
}

/*
 * WxButtonFindPrevClick
 */
void MadSearchDialog::WxButtonFindPrevClick(wxCommandEvent& event)
{
    extern MadEdit *g_ActiveMadEdit;

    if(g_ActiveMadEdit==NULL)
        return;

    wxString text;
    m_FindText->GetText(text, true);

    if(text.Len()>0)
    {
        m_RecentFindText->AddFileToHistory(text);

        MadSearchResult sr;
        wxFileOffset selbeg = g_ActiveMadEdit->GetSelectionBeginPos();

        wxInt64 from = 0, to = 0;
        wxFileOffset rangeFrom = -1, rangeTo = -1;
        if(WxCheckBoxSearchInSelection->IsChecked())
        {
            if(!StrToInt64(WxEditFrom->GetValue(), from))
            {
                wxMessageBox(_("The value of 'From' is incorrect."), wxT("MadEdit"), wxOK|wxICON_WARNING);
                return;
            }
            if(!StrToInt64(WxEditTo->GetValue(), to))
            {
                wxMessageBox(_("The value of 'To' is incorrect."), wxT("MadEdit"), wxOK|wxICON_WARNING);
                return;
            }

            rangeFrom = from;
            wxFileOffset caretpos = g_ActiveMadEdit->GetCaretPosition();
            if(caretpos < from || caretpos >= to)
                rangeTo = to;
        }

        for(;;)
        {
            if(WxCheckBoxFindHex->GetValue())
            {
                sr=g_ActiveMadEdit->FindHexPrevious(text, rangeTo, rangeFrom);
            }
            else
            {
                sr=g_ActiveMadEdit->FindTextPrevious(text,
                    WxCheckBoxRegex->GetValue(),
                    WxCheckBoxCaseSensitive->GetValue(),
                    WxCheckBoxWholeWord->GetValue(),
                    rangeTo, rangeFrom);
            }

            if(sr!=SR_NO)
            {
                if(sr == SR_YES && g_ActiveMadEdit->GetCaretPosition() == selbeg)
                {
                    selbeg = -1;
                    continue;
                }
                break;
            }

            wxString msg(_("Cannot find the matched string."));
            msg += wxT("\n\n");
            msg += WxCheckBoxSearchInSelection->IsChecked()?
                _("Do you want to find from end of selection?"):
                _("Do you want to find from end of file?");

            if(wxCANCEL==wxMessageBox(msg, _("Find Previous"), wxOK|wxCANCEL|wxICON_QUESTION ))
            {
                break;
            }
            rangeTo = WxCheckBoxSearchInSelection->IsChecked()? to: g_ActiveMadEdit->GetFileSize();
        }
    }

    if(WxCheckBoxMoveFocus->GetValue())
    {
        ((wxFrame*)wxTheApp->GetTopWindow())->Raise();
        g_ActiveMadEdit->SetFocus();
    }
}


/*
 * MadSearchDialogKeyDown
 */
void MadSearchDialog::MadSearchDialogKeyDown(wxKeyEvent& event)
{
    int key=event.GetKeyCode();

    //g_SearchDialog->SetTitle(wxString()<<key);

    switch(key)
    {
    case WXK_ESCAPE:
        g_SearchDialog->Show(false);
        return;
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        if((wxButton*)this!=g_SearchDialog->WxButtonFindNext &&
           (wxButton*)this!=g_SearchDialog->WxButtonFindPrev &&
           (wxButton*)this!=g_SearchDialog->WxButtonClose)
        {
            wxCommandEvent e;
            g_SearchDialog->WxButtonFindNextClick(e);
            return; // no skip
        }
        break;
    case WXK_DOWN:
        if((MadEdit*)this==g_SearchDialog->m_FindText)
        {
            int x,y,w,h;
            g_SearchDialog->m_FindText->GetPosition(&x, &y);
            g_SearchDialog->m_FindText->GetSize(&w, &h);
            g_SearchDialog->PopupMenu(g_SearchDialog->WxPopupMenuRecentFindText, x, y+h);
            return;
        }
        break;
    }

    extern wxAcceleratorEntry g_AccelFindNext, g_AccelFindPrev;
    int flags=wxACCEL_NORMAL;
    if(event.m_altDown) flags|=wxACCEL_ALT;
    if(event.m_shiftDown) flags|=wxACCEL_SHIFT;
    if(event.m_controlDown) flags|=wxACCEL_CTRL;

    if(g_AccelFindNext.GetKeyCode()==key && g_AccelFindNext.GetFlags()==flags)
    {
        wxCommandEvent e;
        g_SearchDialog->WxButtonFindNextClick(e);
        return; // no skip
    }

    if(g_AccelFindPrev.GetKeyCode()==key && g_AccelFindPrev.GetFlags()==flags)
    {
        wxCommandEvent e;
        g_SearchDialog->WxButtonFindPrevClick(e);
        return; // no skip
    }

    event.Skip();
}

/*
 * WxBitmapButtonRecentFindTextClick
 */
void MadSearchDialog::WxBitmapButtonRecentFindTextClick(wxCommandEvent& event)
{
    PopupMenu(WxPopupMenuRecentFindText);
}

void MadSearchDialog::OnRecentFindText(wxCommandEvent& event)
{
    int idx=event.GetId()-ID_RECENTFINDTEXT1;
    wxString text=m_RecentFindText->GetHistoryFile(idx);
    if(!text.IsEmpty())
    {
        m_FindText->SetText(text);
        m_FindText->SetFocus();
    }
}

void MadSearchDialog::ReadWriteSettings(bool bRead)
{
    wxConfigBase *m_Config=wxConfigBase::Get(false);
    wxString oldpath=m_Config->GetPath();

    if(bRead)
    {
        bool bb;
        m_Config->Read(wxT("/MadEdit/SearchMoveFocus"), &bb, false);
        WxCheckBoxMoveFocus->SetValue(bb);

        m_Config->Read(wxT("/MadEdit/SearchCaseSensitive"), &bb, false);
        WxCheckBoxCaseSensitive->SetValue(bb);

        m_Config->Read(wxT("/MadEdit/SearchWholeWord"), &bb, false);
        WxCheckBoxWholeWord->SetValue(bb);

        m_Config->Read(wxT("/MadEdit/SearchRegex"), &bb, false);
        WxCheckBoxRegex->SetValue(bb);

        m_Config->Read(wxT("/MadEdit/SearchHex"), &bb, false);
        WxCheckBoxFindHex->SetValue(bb);
        UpdateCheckBoxByCBHex(bb);

        m_Config->Read(wxT("/MadEdit/SearchInSelection"), &bb, false);
        WxCheckBoxSearchInSelection->SetValue(bb);
        UpdateSearchInSelection(bb);

        wxString str;
        m_Config->Read(wxT("/MadEdit/SearchFrom"), &str, wxEmptyString);
        WxEditFrom->SetValue(str);
        m_Config->Read(wxT("/MadEdit/SearchTo"), &str, wxEmptyString);
        WxEditTo->SetValue(str);
    }
    else
    {
        m_Config->Write(wxT("/MadEdit/SearchMoveFocus"), WxCheckBoxMoveFocus->GetValue());
        m_Config->Write(wxT("/MadEdit/SearchCaseSensitive"), WxCheckBoxCaseSensitive->GetValue());
        m_Config->Write(wxT("/MadEdit/SearchWholeWord"), WxCheckBoxWholeWord->GetValue());
        m_Config->Write(wxT("/MadEdit/SearchRegex"), WxCheckBoxRegex->GetValue());
        m_Config->Write(wxT("/MadEdit/SearchHex"), WxCheckBoxFindHex->GetValue());

        m_Config->Write(wxT("/MadEdit/SearchInSelection"), WxCheckBoxSearchInSelection->GetValue());
        m_Config->Write(wxT("/MadEdit/SearchFrom"), WxEditFrom->GetValue());
        m_Config->Write(wxT("/MadEdit/SearchTo"), WxEditTo->GetValue());
    }

    m_Config->SetPath(oldpath);
}

void MadSearchDialog::UpdateCheckBoxByCBHex(bool check)
{
    if(check)
    {
        WxCheckBoxCaseSensitive->Disable();
        WxCheckBoxWholeWord->Disable();
        WxCheckBoxRegex->Disable();
    }
    else
    {
        WxCheckBoxCaseSensitive->Enable();
        WxCheckBoxWholeWord->Enable();
        WxCheckBoxRegex->Enable();
    }
}

/*
 * WxCheckBoxFindHexClick
 */
void MadSearchDialog::WxCheckBoxFindHexClick(wxCommandEvent& event)
{
    UpdateCheckBoxByCBHex(event.IsChecked());
}

/*
 * MadSearchDialogActivate
 */
void MadSearchDialog::MadSearchDialogActivate(wxActivateEvent& event)
{
    ReadWriteSettings(event.GetActive());
}


/*
 * WxButtonReplaceClick
 */
void MadSearchDialog::WxButtonReplaceClick(wxCommandEvent& event)
{
    wxString fname;
    int fsize;
    m_FindText->GetFont(fname, fsize);
    g_ReplaceDialog->m_FindText->SetFont(fname, 14);
    g_ReplaceDialog->m_ReplaceText->SetFont(fname, 14);

    wxString text;
    m_FindText->GetText(text, true);
    g_ReplaceDialog->m_FindText->SetText(text);

    this->Show(false);
    g_ReplaceDialog->Show();
    g_ReplaceDialog->SetFocus();
    g_ReplaceDialog->Raise();

    g_ReplaceDialog->UpdateCheckBoxByCBHex(g_ReplaceDialog->WxCheckBoxFindHex->GetValue());

    g_ReplaceDialog->m_FindText->SelectAll();
    g_ReplaceDialog->m_FindText->SetFocus();

}

void MadSearchDialog::UpdateSearchInSelection(bool check)
{
    WxEditFrom->Enable(check);
    WxEditTo->Enable(check);

    extern MadEdit *g_ActiveMadEdit;
    if(check && g_ActiveMadEdit!=NULL)
    {
        WxEditFrom->SetValue(wxLongLong(g_ActiveMadEdit->GetSelectionBeginPos()).ToString());
        WxEditTo->SetValue(wxLongLong(g_ActiveMadEdit->GetSelectionEndPos()).ToString());
    }
}


void MadSearchDialog::WxCheckBoxSearchInSelectionClick(wxCommandEvent& event)
{
    UpdateSearchInSelection(event.IsChecked());
}

void MadSearchDialog::WxButtonCountClick(wxCommandEvent& event)
{
    extern MadEdit *g_ActiveMadEdit;

    if(g_ActiveMadEdit==NULL)
        return;

    int count = 0;
    wxString text;
    m_FindText->GetText(text, true);

    if(text.Len()>0)
    {
        m_RecentFindText->AddFileToHistory(text);

        wxInt64 from = 0, to = 0;
        wxFileOffset rangeFrom = -1, rangeTo = -1;
        if(WxCheckBoxSearchInSelection->IsChecked())
        {
            if(!StrToInt64(WxEditFrom->GetValue(), from))
            {
                wxMessageBox(_("The value of 'From' is incorrect."), wxT("MadEdit"), wxOK|wxICON_WARNING);
                return;
            }
            if(!StrToInt64(WxEditTo->GetValue(), to))
            {
                wxMessageBox(_("The value of 'To' is incorrect."), wxT("MadEdit"), wxOK|wxICON_WARNING);
                return;
            }

            rangeTo = to;
            rangeFrom = from;
        }

        if(WxCheckBoxFindHex->GetValue())
        {
            count=g_ActiveMadEdit->FindHexAll(text, false, NULL, NULL, rangeFrom, rangeTo);
        }
        else
        {
            count=g_ActiveMadEdit->FindTextAll(text,
                WxCheckBoxRegex->GetValue(),
                WxCheckBoxCaseSensitive->GetValue(),
                WxCheckBoxWholeWord->GetValue(),
                false,
                NULL, NULL,
                rangeFrom, rangeTo);
        }
    }

    if(count >= 0)
    {
        wxString msg;
        msg.Printf(_("'%s' was found %d times."), text.c_str(), count);
        wxMessageBox(msg, wxT("MadEdit"), wxOK);
    }
}
