///////////////////////////////////////////////////////////////////////////////
// Name:        MadEdit/MadEditAdvanced.cpp
// Description: advanced functions of MadEdit
// Author:      madedit@gmail.com
// Licence:     GPL
///////////////////////////////////////////////////////////////////////////////

#include "MadEdit.h"
#include "TradSimp.h"

#include <algorithm>
#include <vector>
using std::vector;

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#endif

//==============================================================================

void MadEdit::ToggleBOM()
{
    MadEncodingType type=GetEncodingType();
    if(IsReadOnly() || !IsTextFile() || type==etSingleByte || type==etDoubleByte)
        return;

    size_t len=m_Lines->m_LineList.begin()->m_RowIndices[0].m_Start;

    if(len!=0)    // remove BOM
    {
        MadDeleteUndoData *dudata = new MadDeleteUndoData;

        dudata->m_Pos = 0;
        dudata->m_Size = len;

        MadLineIterator lit = DeleteInsertData(dudata->m_Pos, dudata->m_Size, &dudata->m_Data, 0, NULL);

        MadUndo *undo = m_UndoBuffer->Add();
        undo->m_CaretPosBefore = m_CaretPos.pos;
        undo->m_Undos.push_back(dudata);

        m_Lines->Reformat(lit, lit);

        if((m_CaretPos.pos-=len)<0) m_CaretPos.pos=0;

        undo->m_CaretPosAfter = m_CaretPos.pos;

        UpdateCaretByPos(m_CaretPos, m_ActiveRowUChars, m_ActiveRowWidths, m_CaretRowUCharPos);

        // update selection pos
        if(m_Selection)
        {
            if((m_SelectionBegin->pos-=len)<0) m_SelectionBegin->pos=0;
            if((m_SelectionEnd->pos-=len)<0)   m_SelectionEnd->pos=0;
            UpdateSelectionPos();
        }
    }
    else            // add BOM
    {
        MadBlock blk(m_Lines->m_MemData, -1, 0);
        ucs4_t bom=0xFEFF;
        UCStoBlock(&bom, 1, blk);

        MadInsertUndoData *insud = new MadInsertUndoData;
        insud->m_Pos = 0;
        insud->m_Size = blk.m_Size;

        insud->m_Data.push_back(blk);

        MadUndo *undo = m_UndoBuffer->Add();
        undo->m_CaretPosBefore = m_CaretPos.pos;
        undo->m_CaretPosAfter = m_CaretPos.pos+blk.m_Size;
        undo->m_Undos.push_back(insud);

        MadLineIterator lit = DeleteInsertData(insud->m_Pos, 0, NULL, insud->m_Size, &insud->m_Data);

        m_Lines->Reformat(lit, lit);

        m_CaretPos.pos=undo->m_CaretPosAfter;
        UpdateCaretByPos(m_CaretPos, m_ActiveRowUChars, m_ActiveRowWidths, m_CaretRowUCharPos);

        if(m_Selection)
        {
            m_SelectionBegin->pos+=blk.m_Size;
            m_SelectionEnd->pos+=blk.m_Size;
            UpdateSelectionPos();
        }
    }

    if(m_EditMode==emHexMode)
    {
        m_RepaintAll=true;
        Refresh(false);
    }

    m_Modified = true;

    DoSelectionChanged();
    DoStatusChanged();
}


wxString *ConvertTextToNewString(const wxString& text, MadConvertChineseFlag flag)
{
    wxString *ptext=NULL;
    if(text.Len()!=0)
    {
        wxChar *str=new wxChar[text.Len()];
        int count=::ConvertChinese(text.c_str(), str, text.Len(), flag);
        if(count>0)
        {
            ptext=new wxString(str, text.Len());
        }
        delete []str;
    }
    return ptext;
}

void MadEdit::ConvertEncoding(const wxString &newenc, MadConvertEncodingFlag flag)
{
    if(IsReadOnly() || !IsTextFile())
        return;

    wxString lowerenc=newenc.Lower();
    if(lowerenc == m_Encoding->GetName().Lower())
    {
        switch(flag)
        {
        case cefTC2SC:
        case cefSC2TC:
        case cefJK2TC:
        case cefJK2SC:
        case cefC2JK:
            ConvertChinese(flag);
            break;
        default: break;
        }
        return;
    }

    if(m_Lines->m_Size == 0)
    {
        SetEncoding(newenc);
        return;
    }

    wxFileOffset caretpos=m_CaretPos.pos;
    int toprow=m_TopRow;

    bool ignoreBOM=true;
    if(lowerenc.Len()>3 && lowerenc.Left(3)==wxT("utf"))
    {
        ignoreBOM=false;
    }

    wxString text, *ptext=NULL;
    GetText(text, ignoreBOM);

    if(flag != cefNone)
    {
        MadConvertEncodingFlag cefs[]=
            { cefSC2TC, cefTC2SC, cefJK2TC, cefJK2SC, cefC2JK };
        MadConvertChineseFlag ccfs[]=
            { ccfSimp2Trad, ccfTrad2Simp, ccfKanji2Trad, ccfKanji2Simp, ccfChinese2Kanji };
        for(int i=0; i<sizeof(cefs)/sizeof(cefs[0]); ++i)
        {
            if(flag==cefs[i])
            {
                ptext=ConvertTextToNewString(text, ccfs[i]);
                break;
            }
        }
    }

    m_LoadingFile=true; // don't reformat
    SetEncoding(newenc);
    m_LoadingFile=false;

    if(ptext)
    {
        SetText(*ptext);
        delete ptext;
    }
    else
    {
        SetText(text);
    }

    RestorePosition(caretpos, toprow);
}

void MadEdit::ConvertChinese(MadConvertEncodingFlag flag)
{
    if(IsReadOnly() || !IsTextFile() || m_Lines->m_Size==0)
        return;

    wxFileOffset caretpos=m_CaretPos.pos;
    int toprow=m_TopRow;

    wxString text, *ptext=NULL;
    GetText(text, false);

    if(flag != cefNone)
    {
        MadConvertEncodingFlag cefs[]=
            { cefSC2TC, cefTC2SC, cefJK2TC, cefJK2SC, cefC2JK };
        MadConvertChineseFlag ccfs[]=
            { ccfSimp2Trad, ccfTrad2Simp, ccfKanji2Trad, ccfKanji2Simp, ccfChinese2Kanji };
        for(int i=0; i<sizeof(cefs)/sizeof(cefs[0]); ++i)
        {
            if(flag==cefs[i])
            {
                ptext=ConvertTextToNewString(text, ccfs[i]);
                break;
            }
        }
    }

    if(ptext!=NULL)
    {
        SetText(*ptext);
        delete ptext;
    }

    RestorePosition(caretpos, toprow);
}

void MadEdit::ConvertNewLineType(MadNewLineType type)
{
    if(IsReadOnly() || !IsTextFile())
        return;

    if(m_Lines->m_LineCount<2)
    {
        m_NewLineType=m_InsertNewLineType=type;
        DoStatusChanged();
        return;
    }

    MadBlock newline_blk(m_Lines->m_MemData, -1, 0);
    ucs4_t newline[2]={ 0x0D, 0x0A };
    switch(type)
    {
    case nltDOS:
        UCStoBlock(newline, 2, newline_blk);
        break;
    case nltMAC:
        UCStoBlock(newline, 1, newline_blk);
        break;
    case nltUNIX:
        UCStoBlock(newline+1, 1, newline_blk);
        break;
    default: break;
    }

    wxByte newlinedata[16];
    size_t newlinesize=newline_blk.m_Size;
    m_Lines->m_MemData->Get(newline_blk.m_Pos, newlinedata, newlinesize);

    vector<wxByte> buffervector;
    wxByte *buf=NULL;

    MadBlock blk(m_Lines->m_MemData, m_Lines->m_MemData->m_Size, 0);

    MadLineIterator lit=m_Lines->m_LineList.begin();
    int lineid=0;
    int count=int(m_Lines->m_LineCount-1);

    wxFileOffset newCaretPos=m_CaretPos.pos;

    do
    {
        size_t size=lit->m_Size-lit->m_NewLineSize;

        if(size>0)
        {
            if(buffervector.size()<size)
            {
                buffervector.resize(size);
                buf=&buffervector[0];
            }

            // add line text
            lit->Get(0, buf, size);
            m_Lines->m_MemData->Put(buf, size);
            blk.m_Size+=size;
        }

        if(lineid==count)   // last line
            break;

        // add newline char
        m_Lines->m_MemData->Put(newlinedata, newlinesize);
        blk.m_Size+=newlinesize;

        if(lineid<m_CaretPos.lineid)
        {
            newCaretPos= newCaretPos - lit->m_NewLineSize + newlinesize;
        }
        else if(lineid==m_CaretPos.lineid)
        {
            wxFileOffset len = lit->m_Size - lit->m_NewLineSize;
            if(m_CaretPos.linepos > len)
            {
                len = m_CaretPos.linepos - len;
                if(len>=newlinesize)
                {
                    wxFileOffset newlen= newlinesize - 1;
                    newCaretPos -= (len-newlen);
                }
            }
        }

        ++lit;
    }
    while(++lineid<=count);

    MadOverwriteUndoData *oudata = new MadOverwriteUndoData();

    oudata->m_Pos = 0;
    oudata->m_DelSize = m_Lines->m_Size;

    oudata->m_InsSize = blk.m_Size;
    oudata->m_InsData.push_back(blk);

    DeleteInsertData(oudata->m_Pos, oudata->m_DelSize, &oudata->m_DelData,
                                    oudata->m_InsSize, &oudata->m_InsData);

    MadUndo *undo=m_UndoBuffer->Add();
    undo->m_CaretPosBefore=m_CaretPos.pos;
    undo->m_CaretPosAfter=newCaretPos;
    undo->m_Undos.push_back(oudata);

    m_CaretPos.pos=newCaretPos;

    if(m_Selection)
    {
        m_Selection=false;
        m_RepaintSelection=true;
    }
    m_RepaintAll=true;
    Refresh(false);

    ReformatAll();

    m_NewLineType=m_InsertNewLineType=type;
    m_Modified=true;

    DoSelectionChanged();
    DoStatusChanged();
}

void MadEdit::GetSelHexString(wxString &ws, bool withSpace)
{
    if(!m_Selection) return;

    MadLineIterator lit=m_SelectionBegin->iter;
    wxFileOffset linepos=m_SelectionBegin->linepos;
    wxFileOffset pos=m_SelectionBegin->pos;

    do
    {
        int b=lit->Get(linepos);

        ws<< wxChar(ToHex(b>>4));
        ws<< wxChar(ToHex(b&0xf));

        if(withSpace)
        {
            ws<< wxChar(' ');
        }

        if(++linepos == lit->m_Size)
        {
            ++lit;
            linepos=0;
        }
    }
    while(++pos < m_SelectionEnd->pos);

}

void MadEdit::CopyAsHexString(bool withSpace)
{
    if(!m_Selection) return;

    wxString ws;
    GetSelHexString(ws, withSpace);
    PutTextToClipboard(ws);
}


void MadEdit::IncreaseDecreaseIndent(bool incIndent)
{
    if(IsReadOnly() || m_EditMode==emHexMode)
        return;

    bool oldModified=m_Modified;

    MadBlock blk(m_Lines->m_MemData, m_Lines->m_MemData->m_Size, 0);

    MadLineIterator lit;
    size_t count;
    bool SelEndAtBOL=false;
    wxFileOffset linestartpos;
    if(m_Selection && m_SelectionBegin->lineid!=m_SelectionEnd->lineid)
    {
        lit=m_SelectionBegin->iter;
        if(m_SelectionEnd->linepos == 0) // selend at begin of line
        {
            count = m_SelectionEnd->lineid - m_SelectionBegin->lineid;
            SelEndAtBOL=true;
        }
        else
        {
            count = m_SelectionEnd->lineid - m_SelectionBegin->lineid +1;
        }

        // save first line pos
        linestartpos=lit->m_RowIndices.front().m_Start;
        m_SelectionPos1.pos = m_SelectionBegin->pos - m_SelectionBegin->linepos + linestartpos;
    }
    else
    {
        lit=m_CaretPos.iter;
        count=1;
        SelEndAtBOL=true;

        // save first line pos
        linestartpos=lit->m_RowIndices.front().m_Start;
        m_SelectionPos1.pos = m_CaretPos.pos - m_CaretPos.linepos + linestartpos;
    }

    vector<wxByte> buffervector;
    buffervector.resize(2048);
    wxByte *buf=&buffervector[0];

    vector <ucs4_t> spaces;
    MadUCQueue ucqueue;
    MadLines::NextUCharFuncPtr NextUChar=m_Lines->NextUChar;
    wxFileOffset delsize=0;
    for(;;)  // for each line
    {
        --count;
        m_Lines->InitNextUChar(lit, linestartpos);

        delsize += (lit->m_Size-linestartpos);
        ucs4_t uc=0x0D;
        wxFileOffset nonspacepos=linestartpos;

        linestartpos=0;
        spaces.clear();
        ucqueue.clear();

        // get spaces at begin of line
        while((m_Lines->*NextUChar)(ucqueue))
        {
            uc=ucqueue.back().first;
            if(uc==0x20 || uc==0x09)
            {
                spaces.push_back(uc);
                nonspacepos+=ucqueue.back().second;
            }
            else
            {
                break;
            }
        }

        // writeback the indent-spaces and rest content of line
        if(spaces.size()!=0 || (uc!=0x0D && uc!=0x0A)) // this line is not a empty line
        {
            if(incIndent)
                IncreaseIndentSpaces(spaces);
            else
                DecreaseIndentSpaces(spaces);

            if(spaces.size()!=0)
                UCStoBlock(&spaces[0], spaces.size(), blk);
        }

        size_t size = lit->m_Size - nonspacepos;
        if(count==0 && !SelEndAtBOL)
        {
            // exclude NewLine chars
            size -= lit->m_NewLineSize;
            delsize -= lit->m_NewLineSize;
        }

        if(size>0)
        {
            if(buffervector.size()<size)
            {
                buffervector.resize(size);
                buf=&buffervector[0];
            }

            lit->Get(nonspacepos, buf, size);
            m_Lines->m_MemData->Put(buf, size);
            blk.m_Size+=size;
        }

        if(count==0)
            break;

        ++lit;
    }

    if(blk.m_Size==0 && delsize==0)
        return;

    MadOverwriteUndoData *oudata = new MadOverwriteUndoData();

    oudata->m_Pos = m_SelectionPos1.pos;
    oudata->m_DelSize = delsize;

    oudata->m_InsSize = blk.m_Size;
    oudata->m_InsData.push_back(blk);

    lit = DeleteInsertData(oudata->m_Pos, oudata->m_DelSize, &oudata->m_DelData,
                                          oudata->m_InsSize, &oudata->m_InsData);

    MadUndo *undo=m_UndoBuffer->Add();
    undo->m_CaretPosBefore=m_CaretPos.pos;
    undo->m_CaretPosAfter=oudata->m_Pos;
    undo->m_Undos.push_back(oudata);

    count = m_Lines->Reformat(lit, lit);

    m_CaretPos.pos=oudata->m_Pos;
    UpdateCaretByPos(m_CaretPos, m_ActiveRowUChars, m_ActiveRowWidths, m_CaretRowUCharPos);

    m_Selection=true;
    m_SelectionPos2.pos=m_SelectionPos1.pos+blk.m_Size;
    UpdateSelectionPos();
    m_SelectionBegin=&m_SelectionPos1;
    m_SelectionEnd=&m_SelectionPos2;

    AppearCaret();
    UpdateScrollBarPos();
    m_LastCaretXPos = m_CaretPos.xpos;

    m_RepaintAll = true;
    Refresh(false);

    bool sc=(oldModified==false);
    m_Modified=true;

    DoSelectionChanged();
    if(sc) DoStatusChanged();
}

void MadEdit::CommentUncomment(bool comment)
{
    if(IsReadOnly() || m_EditMode==emHexMode || m_Syntax->m_LineComment.empty())
        return;

    bool oldModified=m_Modified;

    MadBlock blk(m_Lines->m_MemData, m_Lines->m_MemData->m_Size, 0);

    MadLineIterator lit;
    size_t count;
    bool SelEndAtBOL=false;
    wxFileOffset linestartpos;
    if(m_Selection && m_SelectionBegin->lineid!=m_SelectionEnd->lineid)
    {
        lit=m_SelectionBegin->iter;
        if(m_SelectionEnd->linepos == 0) // selend at begin of line
        {
            count = m_SelectionEnd->lineid - m_SelectionBegin->lineid;
            SelEndAtBOL=true;
        }
        else
        {
            count = m_SelectionEnd->lineid - m_SelectionBegin->lineid +1;
        }

        // save first line pos
        linestartpos=lit->m_RowIndices.front().m_Start;
        m_SelectionPos1.pos = m_SelectionBegin->pos - m_SelectionBegin->linepos + linestartpos;
    }
    else
    {
        lit=m_CaretPos.iter;
        count=1;
        SelEndAtBOL=true;

        // save first line pos
        linestartpos=lit->m_RowIndices.front().m_Start;
        m_SelectionPos1.pos = m_CaretPos.pos - m_CaretPos.linepos + linestartpos;
    }

    wxString &str=m_Syntax->m_LineComment.front();
    vector <ucs4_t> commentstr;
    const size_t commentlen=str.size();
    for(size_t i=0;i<commentlen;i++)
    {
        commentstr.push_back(str[i]);
    }

    vector<wxByte> buffervector;
    buffervector.resize(2048);
    wxByte *buf=&buffervector[0];

    vector <ucs4_t> spaces;
    MadUCQueue ucqueue;
    MadLines::NextUCharFuncPtr NextUChar=m_Lines->NextUChar;
    wxFileOffset delsize=0;
    for(;;)  // for each line
    {
        --count;
        m_Lines->InitNextUChar(lit, linestartpos);

        delsize += (lit->m_Size-linestartpos);
        ucs4_t uc=0x0D;
        wxFileOffset nonspacepos=linestartpos;

        linestartpos=0;
        spaces.clear();
        ucqueue.clear();

        // get spaces at begin of line
        while((m_Lines->*NextUChar)(ucqueue))
        {
            uc=ucqueue.back().first;
            if(uc==0x20 || uc==0x09)
            {
                spaces.push_back(uc);
                nonspacepos+=ucqueue.back().second;
            }
            else
            {
                break;
            }
        }

        // comment/uncomment the line
        if(uc!=0x0D && uc!=0x0A) // this line is not a empty line
        {
            // get data from the line
            vector<ucs4_t> cmt;
            size_t cmtsize=0;
            do
            {
                cmt.push_back(ucqueue.back().first);
                cmtsize+=ucqueue.back().second;
            }
            while(cmt.size()<commentlen && (m_Lines->*NextUChar)(ucqueue));

            if(comment)
            {
                if(cmt!=commentstr)
                {
                    // insert the comment string
                    spaces.insert(spaces.end(), commentstr.begin(), commentstr.end());
                }
            }
            else // uncomment
            {
                if(cmt==commentstr)
                {
                    nonspacepos+=cmtsize;
                }
            }
        }

        if(spaces.size()!=0)
            UCStoBlock(&spaces[0], spaces.size(), blk);

        size_t size = lit->m_Size - nonspacepos;
        if(count==0 && !SelEndAtBOL)
        {
            // exclude NewLine chars
            size -= lit->m_NewLineSize;
            delsize -= lit->m_NewLineSize;
        }

        if(size>0)
        {
            if(buffervector.size()<size)
            {
                buffervector.resize(size);
                buf=&buffervector[0];
            }

            lit->Get(nonspacepos, buf, size);
            m_Lines->m_MemData->Put(buf, size);
            blk.m_Size+=size;
        }

        if(count==0)
            break;

        ++lit;
    }

    if(blk.m_Size==0 && delsize==0)
        return;

    MadOverwriteUndoData *oudata = new MadOverwriteUndoData();

    oudata->m_Pos = m_SelectionPos1.pos;
    oudata->m_DelSize = delsize;

    oudata->m_InsSize = blk.m_Size;
    oudata->m_InsData.push_back(blk);

    lit = DeleteInsertData(oudata->m_Pos, oudata->m_DelSize, &oudata->m_DelData,
                                          oudata->m_InsSize, &oudata->m_InsData);

    MadUndo *undo=m_UndoBuffer->Add();
    undo->m_CaretPosBefore=m_CaretPos.pos;
    undo->m_CaretPosAfter=oudata->m_Pos;
    undo->m_Undos.push_back(oudata);

    count = m_Lines->Reformat(lit, lit);

    m_CaretPos.pos=oudata->m_Pos;
    UpdateCaretByPos(m_CaretPos, m_ActiveRowUChars, m_ActiveRowWidths, m_CaretRowUCharPos);

    m_Selection=true;
    m_SelectionPos2.pos=m_SelectionPos1.pos+blk.m_Size;
    UpdateSelectionPos();
    m_SelectionBegin=&m_SelectionPos1;
    m_SelectionEnd=&m_SelectionPos2;

    AppearCaret();
    UpdateScrollBarPos();
    m_LastCaretXPos = m_CaretPos.xpos;

    m_RepaintAll = true;
    Refresh(false);

    bool sc=(oldModified==false);
    m_Modified=true;

    DoSelectionChanged();
    if(sc) DoStatusChanged();
}


void MadEdit::ToUpperCase()
{
    if(IsReadOnly() || !m_Selection)
        return;

    wxString text;
    GetSelText(text);
    bool modified=false;

    size_t i=0, count=text.Len();
    while(i<count)
    {
        int c=text[i];
        int nc=towupper(c);
        if(nc != c)
        {
            text.SetChar(i, nc);
            modified=true;
        }
        ++i;
    }

    if(modified)
    {
        vector<ucs4_t> ucs;
        TranslateText(text.c_str(), text.Len(), &ucs, true);

        if(m_EditMode==emColumnMode)
        {
            int colcount = m_SelectionEnd->rowid - m_SelectionBegin->rowid + 1;
            InsertColumnString(&ucs[0], ucs.size(), colcount, false, true);
        }
        else
        {
            InsertString(&ucs[0], ucs.size(), false, false, true);
        }
    }
}

void MadEdit::ToLowerCase()
{
    if(IsReadOnly() || !m_Selection)
        return;

    wxString text;
    GetSelText(text);
    bool modified=false;

    size_t i=0, count=text.Len();
    while(i<count)
    {
        int c=text[i];
        int nc=towlower(c);
        if(nc != c)
        {
            text.SetChar(i, nc);
            modified=true;
        }
        ++i;
    }

    if(modified)
    {
        vector<ucs4_t> ucs;
        TranslateText(text.c_str(), text.Len(), &ucs, true);

        if(m_EditMode==emColumnMode)
        {
            int colcount = m_SelectionEnd->rowid - m_SelectionBegin->rowid + 1;
            InsertColumnString(&ucs[0], ucs.size(), colcount, false, true);
        }
        else
        {
            InsertString(&ucs[0], ucs.size(), false, false, true);
        }
    }
}

void MadEdit::InvertCase()
{
    if(IsReadOnly() || !m_Selection)
        return;

    wxString text;
    GetSelText(text);
    bool modified=false;

    size_t i=0, count=text.Len();
    while(i<count)
    {
        int c=text[i];
        int nc=c;
        if(iswlower(c))
        {
            nc=towupper(c);
        }
        else if(iswupper(c))
        {
            nc=towlower(c);
        }

        if(nc != c)
        {
            text.SetChar(i, nc);
            modified=true;
        }
        ++i;
    }

    if(modified)
    {
        vector<ucs4_t> ucs;
        TranslateText(text.c_str(), text.Len(), &ucs, true);

        if(m_EditMode==emColumnMode)
        {
            int colcount = m_SelectionEnd->rowid - m_SelectionBegin->rowid + 1;
            InsertColumnString(&ucs[0], ucs.size(), colcount, false, true);
        }
        else
        {
            InsertString(&ucs[0], ucs.size(), false, false, true);
        }
    }
}

//==============================================================================
// data are from http://zh.wikipedia.org/wiki/%E5%85%A8%E5%BD%A2
ucs2_t ASCII_Halfwidth_Fullwidth_Table[]=
{
    //halfwidth, fullwidth,
    //ASCII char
    0x0020, 0x3000,
    0x0021, 0xFF01,
    0x0022, 0xFF02,
    0x0023, 0xFF03,
    0x0024, 0xFF04,
    0x0025, 0xFF05,
    0x0026, 0xFF06,
    0x0027, 0xFF07,
    0x0028, 0xFF08,
    0x0029, 0xFF09,
    0x002A, 0xFF0A,
    0x002B, 0xFF0B,
    0x002C, 0xFF0C,
    0x002D, 0xFF0D,
    0x002E, 0xFF0E,
    0x002F, 0xFF0F,
    0x0030, 0xFF10,
    0x0031, 0xFF11,
    0x0032, 0xFF12,
    0x0033, 0xFF13,
    0x0034, 0xFF14,
    0x0035, 0xFF15,
    0x0036, 0xFF16,
    0x0037, 0xFF17,
    0x0038, 0xFF18,
    0x0039, 0xFF19,
    0x003A, 0xFF1A,
    0x003B, 0xFF1B,
    0x003C, 0xFF1C,
    0x003D, 0xFF1D,
    0x003E, 0xFF1E,
    0x003F, 0xFF1F,
    0x0040, 0xFF20,
    0x0041, 0xFF21,
    0x0042, 0xFF22,
    0x0043, 0xFF23,
    0x0044, 0xFF24,
    0x0045, 0xFF25,
    0x0046, 0xFF26,
    0x0047, 0xFF27,
    0x0048, 0xFF28,
    0x0049, 0xFF29,
    0x004A, 0xFF2A,
    0x004B, 0xFF2B,
    0x004C, 0xFF2C,
    0x004D, 0xFF2D,
    0x004E, 0xFF2E,
    0x004F, 0xFF2F,
    0x0050, 0xFF30,
    0x0051, 0xFF31,
    0x0052, 0xFF32,
    0x0053, 0xFF33,
    0x0054, 0xFF34,
    0x0055, 0xFF35,
    0x0056, 0xFF36,
    0x0057, 0xFF37,
    0x0058, 0xFF38,
    0x0059, 0xFF39,
    0x005A, 0xFF3A,
    0x005B, 0xFF3B,
    0x005C, 0xFF3C,
    0x005D, 0xFF3D,
    0x005E, 0xFF3E,
    0x005F, 0xFF3F,
    0x0060, 0xFF40,
    0x0061, 0xFF41,
    0x0062, 0xFF42,
    0x0063, 0xFF43,
    0x0064, 0xFF44,
    0x0065, 0xFF45,
    0x0066, 0xFF46,
    0x0067, 0xFF47,
    0x0068, 0xFF48,
    0x0069, 0xFF49,
    0x006A, 0xFF4A,
    0x006B, 0xFF4B,
    0x006C, 0xFF4C,
    0x006D, 0xFF4D,
    0x006E, 0xFF4E,
    0x006F, 0xFF4F,
    0x0070, 0xFF50,
    0x0071, 0xFF51,
    0x0072, 0xFF52,
    0x0073, 0xFF53,
    0x0074, 0xFF54,
    0x0075, 0xFF55,
    0x0076, 0xFF56,
    0x0077, 0xFF57,
    0x0078, 0xFF58,
    0x0079, 0xFF59,
    0x007A, 0xFF5A,
    0x007B, 0xFF5B,
    0x007C, 0xFF5C,
    0x007D, 0xFF5D,
    0x007E, 0xFF5E,
    0
};
ucs2_t Japanese_Halfwidth_Fullwidth_Table[]=
{
    //halfwidth, fullwidth,
    //Japanese char
    0xFF61, 0x3002,
    0xFF62, 0x300C,
    0xFF63, 0x300D,
    0xFF64, 0x3001,
    0xFF65, 0x30FB,
    0xFF66, 0x30F2,
    0xFF67, 0x30A1,
    0xFF68, 0x30A3,
    0xFF69, 0x30A5,
    0xFF6A, 0x30A7,
    0xFF6B, 0x30A9,
    0xFF6C, 0x30E3,
    0xFF6D, 0x30E5,
    0xFF6E, 0x30E7,
    0xFF6F, 0x30C3,
    0xFF70, 0x30FC,
    0xFF71, 0x30A2,
    0xFF72, 0x30A4,
    0xFF73, 0x30A6,
    0xFF74, 0x30A8,
    0xFF75, 0x30AA,
    0xFF76, 0x30AB,
    0xFF77, 0x30AD,
    0xFF78, 0x30AF,
    0xFF79, 0x30B1,
    0xFF7A, 0x30B3,
    0xFF7B, 0x30B5,
    0xFF7C, 0x30B7,
    0xFF7D, 0x30B9,
    0xFF7E, 0x30BB,
    0xFF7F, 0x30BD,
    0xFF80, 0x30BF,
    0xFF81, 0x30C1,
    0xFF82, 0x30C4,
    0xFF83, 0x30C6,
    0xFF84, 0x30C8,
    0xFF85, 0x30CA,
    0xFF86, 0x30CB,
    0xFF87, 0x30CC,
    0xFF88, 0x30CD,
    0xFF89, 0x30CE,
    0xFF8A, 0x30CF,
    0xFF8B, 0x30D2,
    0xFF8C, 0x30D5,
    0xFF8D, 0x30D8,
    0xFF8E, 0x30DB,
    0xFF8F, 0x30DE,
    0xFF90, 0x30DF,
    0xFF91, 0x30E0,
    0xFF92, 0x30E1,
    0xFF93, 0x30E2,
    0xFF94, 0x30E4,
    0xFF95, 0x30E6,
    0xFF96, 0x30E8,
    0xFF97, 0x30E9,
    0xFF98, 0x30EA,
    0xFF99, 0x30EB,
    0xFF9A, 0x30EC,
    0xFF9B, 0x30ED,
    0xFF9C, 0x30EF,
    0xFF9D, 0x30F3,
    0xFF9E, 0x309B,
    0xFF9F, 0x309C,
    0
};
ucs2_t Korean_Halfwidth_Fullwidth_Table[]=
{
    //halfwidth, fullwidth,
    //Korean char
    0xFFA0, 0x3164,
    0xFFA1, 0x3131,
    0xFFA2, 0x3132,
    0xFFA3, 0x3133,
    0xFFA4, 0x3134,
    0xFFA5, 0x3135,
    0xFFA6, 0x3136,
    0xFFA7, 0x3137,
    0xFFA8, 0x3138,
    0xFFA9, 0x3139,
    0xFFAA, 0x313A,
    0xFFAB, 0x313B,
    0xFFAC, 0x313C,
    0xFFAD, 0x313D,
    0xFFAE, 0x313E,
    0xFFAF, 0x313F,
    0xFFB0, 0x3140,
    0xFFB1, 0x3141,
    0xFFB2, 0x3142,
    0xFFB3, 0x3143,
    0xFFB4, 0x3144,
    0xFFB5, 0x3145,
    0xFFB6, 0x3146,
    0xFFB7, 0x3147,
    0xFFB8, 0x3148,
    0xFFB9, 0x3149,
    0xFFBA, 0x314A,
    0xFFBB, 0x314B,
    0xFFBC, 0x314C,
    0xFFBD, 0x314D,
    0xFFBE, 0x314E,
    0xFFC2, 0x314F,
    0xFFC3, 0x3150,
    0xFFC4, 0x3151,
    0xFFC5, 0x3152,
    0xFFC6, 0x3153,
    0xFFC7, 0x3154,
    0xFFCA, 0x3155,
    0xFFCB, 0x3156,
    0xFFCC, 0x3157,
    0xFFCD, 0x3158,
    0xFFCE, 0x3159,
    0xFFCF, 0x315A,
    0xFFD2, 0x315B,
    0xFFD3, 0x315C,
    0xFFD4, 0x315D,
    0xFFD5, 0x315E,
    0xFFD6, 0x315F,
    0xFFD7, 0x3160,
    0xFFDA, 0x3161,
    0xFFDB, 0x3162,
    0xFFDC, 0x3163,
    0
};
ucs2_t Other_Halfwidth_Fullwidth_Table[]=
{
    //halfwidth, fullwidth,
    //other1
    0x2985, 0xFF5F,
    0x2986, 0xFF60,
    0x00A2, 0xFFE0,
    0x00A3, 0xFFE1,
    0x00AC, 0xFFE2,
    0x00AF, 0xFFE3,
    0x00A6, 0xFFE4,
    0x00A5, 0xFFE5,
    0x20A9, 0xFFE6,
    //other2
    0xFFE8, 0x2502,
    0xFFE9, 0x2190,
    0xFFEA, 0x2191,
    0xFFEB, 0x2192,
    0xFFEC, 0x2193,
    0xFFED, 0x25A0,
    0xFFEE, 0x25CB,
    0
};

void InitHalfwidthTable(ucs2_t *Halfwidth_Table, ucs2_t *hftable)
{
    do
    {
        Halfwidth_Table[*hftable] = *(hftable+1);
        hftable+=2;
    }
    while(*hftable != 0);
}
void InitFullwidthTable(ucs2_t *Fullwidth_Table, ucs2_t *hftable)
{
    do
    {
        Fullwidth_Table[*(hftable+1)] = *hftable;
        hftable+=2;
    }
    while(*hftable != 0);
}

ucs2_t *GetHalfwidthTable(bool ascii, bool japanese, bool korean, bool other)
{
    static ucs2_t *Halfwidth_Table=NULL; // halfwidth-char to fullwidth-char table

    if(Halfwidth_Table==NULL)
    {
        Halfwidth_Table=new ucs2_t[65536];
    }
    memset(Halfwidth_Table, 0, sizeof(ucs2_t)*65536);

    if(ascii)    InitHalfwidthTable(Halfwidth_Table, ASCII_Halfwidth_Fullwidth_Table);
    if(japanese) InitHalfwidthTable(Halfwidth_Table, Japanese_Halfwidth_Fullwidth_Table);
    if(korean)   InitHalfwidthTable(Halfwidth_Table, Korean_Halfwidth_Fullwidth_Table);
    if(other)    InitHalfwidthTable(Halfwidth_Table, Other_Halfwidth_Fullwidth_Table);

    return Halfwidth_Table;
}

ucs2_t *GetFullwidthTable(bool ascii, bool japanese, bool korean, bool other)
{
    static ucs2_t *Fullwidth_Table=NULL; // fullwidth-char to halfwidth-char table

    if(Fullwidth_Table==NULL)
    {
        Fullwidth_Table=new ucs2_t[65536];
    }
    memset(Fullwidth_Table, 0, sizeof(ucs2_t)*65536);

    if(ascii)    InitFullwidthTable(Fullwidth_Table, ASCII_Halfwidth_Fullwidth_Table);
    if(japanese) InitFullwidthTable(Fullwidth_Table, Japanese_Halfwidth_Fullwidth_Table);
    if(korean)   InitFullwidthTable(Fullwidth_Table, Korean_Halfwidth_Fullwidth_Table);
    if(other)    InitFullwidthTable(Fullwidth_Table, Other_Halfwidth_Fullwidth_Table);

    return Fullwidth_Table;
}

inline ucs4_t tohalfwidth(ucs4_t uc, ucs2_t *full)
{
    if(uc<=0xFFFF)
    {
        ucs2_t hw=full[uc];
        if(hw!=0) return hw;
    }
    return uc;
}
inline ucs4_t tofullwidth(ucs4_t uc, ucs2_t *half)
{
    if(uc<=0xFFFF)
    {
        ucs2_t fw=half[uc];
        if(fw!=0) return fw;
    }
    return uc;
}

void MadEdit::ToHalfWidth(bool ascii, bool japanese, bool korean, bool other)
{
    if(IsReadOnly() || !m_Selection)
        return;

    wxString text;
    GetSelText(text);
    bool modified=false;
    ucs2_t *full_table=GetFullwidthTable(ascii, japanese, korean, other);

    size_t i=0, count=text.Len();
    while(i<count)
    {
        ucs4_t c=text[i];
        ucs4_t nc=tohalfwidth(c, full_table);
        if(nc != c)
        {
            text.SetChar(i, nc);
            modified=true;
        }
        ++i;
    }

    if(modified)
    {
        vector<ucs4_t> ucs;
        TranslateText(text.c_str(), text.Len(), &ucs, true);

        if(m_EditMode==emColumnMode)
        {
            int colcount = m_SelectionEnd->rowid - m_SelectionBegin->rowid + 1;
            InsertColumnString(&ucs[0], ucs.size(), colcount, false, true);
        }
        else
        {
            InsertString(&ucs[0], ucs.size(), false, false, true);
        }
    }
}

void MadEdit::ToFullWidth(bool ascii, bool japanese, bool korean, bool other)
{
    if(IsReadOnly() || !m_Selection)
        return;

    wxString text;
    GetSelText(text);
    bool modified=false;
    ucs2_t *half_table = GetHalfwidthTable(ascii, japanese, korean, other);

    size_t i=0, count=text.Len();
    while(i<count)
    {
        ucs4_t c=text[i];
        ucs4_t nc=tofullwidth(c, half_table);
        if(nc != c)
        {
            text.SetChar(i, nc);
            modified=true;
        }
        ++i;
    }

    if(modified)
    {
        vector<ucs4_t> ucs;
        TranslateText(text.c_str(), text.Len(), &ucs, true);

        if(m_EditMode==emColumnMode)
        {
            int colcount = m_SelectionEnd->rowid - m_SelectionBegin->rowid + 1;
            InsertColumnString(&ucs[0], ucs.size(), colcount, false, true);
        }
        else
        {
            InsertString(&ucs[0], ucs.size(), false, false, true);
        }
    }
}

//==============================================================================
// Unicode-4.1 Blocks (from: http://www.unicode.org/Public/UNIDATA/Blocks.txt)

struct UnicodeBlock
{
    ucs4_t       begin;
    ucs4_t       end;
    bool         is_alphabet;
    bool         is_fullwidth;
    const wxChar *description;
};

// do not use wxGetTranslation() now
#undef _
#define _(s)    wxT(s)
const UnicodeBlock UnicodeBlocks[]=
{
    { 0x0000, 0x001F, false, false, _("Control Characters") },
    { 0x0020, 0x007F, true, false, _("Basic Latin") },
    { 0x0080, 0x00FF, true, false, _("Latin-1 Supplement") },
    { 0x0100, 0x017F, true, false, _("Latin Extended-A") },
    { 0x0180, 0x024F, true, false, _("Latin Extended-B") },
    { 0x0250, 0x02AF, true, false, _("IPA Extensions") },
    { 0x02B0, 0x02FF, true, false, _("Spacing Modifier Letters") },
    { 0x0300, 0x036F, true, false, _("Combining Diacritical Marks") },
    { 0x0370, 0x03FF, true, false, _("Greek and Coptic") },
    { 0x0400, 0x04FF, true, false, _("Cyrillic") },
    { 0x0500, 0x052F, true, false, _("Cyrillic Supplement") },
    { 0x0530, 0x058F, true, false, _("Armenian") },
    { 0x0590, 0x05FF, true, false, _("Hebrew") },
    { 0x0600, 0x06FF, true, false, _("Arabic") },
    { 0x0700, 0x074F, true, false, _("Syriac") },
    { 0x0750, 0x077F, true, false, _("Arabic Supplement") },
    { 0x0780, 0x07BF, true, false, _("Thaana") },
    { 0x0900, 0x097F, true, false, _("Devanagari") },
    { 0x0980, 0x09FF, true, false, _("Bengali") },
    { 0x0A00, 0x0A7F, true, false, _("Gurmukhi") },
    { 0x0A80, 0x0AFF, true, false, _("Gujarati") },
    { 0x0B00, 0x0B7F, true, false, _("Oriya") },
    { 0x0B80, 0x0BFF, true, false, _("Tamil") },
    { 0x0C00, 0x0C7F, true, false, _("Telugu") },
    { 0x0C80, 0x0CFF, true, false, _("Kannada") },
    { 0x0D00, 0x0D7F, true, false, _("Malayalam") },
    { 0x0D80, 0x0DFF, true, false, _("Sinhala") },
    { 0x0E00, 0x0E7F, true, false, _("Thai") },
    { 0x0E80, 0x0EFF, true, false, _("Lao") },
    { 0x0F00, 0x0FFF, true, false, _("Tibetan") },
    { 0x1000, 0x109F, true, false, _("Myanmar") },
    { 0x10A0, 0x10FF, true, false, _("Georgian") },
    { 0x1100, 0x11FF, false, true, _("Hangul Jamo") },
    { 0x1200, 0x137F, true, false, _("Ethiopic") },
    { 0x1380, 0x139F, true, false, _("Ethiopic Supplement") },
    { 0x13A0, 0x13FF, true, false, _("Cherokee") },
    { 0x1400, 0x167F, true, false, _("Unified Canadian Aboriginal Syllabics") },
    { 0x1680, 0x169F, true, false, _("Ogham") },
    { 0x16A0, 0x16FF, true, false, _("Runic") },
    { 0x1700, 0x171F, true, false, _("Tagalog") },
    { 0x1720, 0x173F, true, false, _("Hanunoo") },
    { 0x1740, 0x175F, true, false, _("Buhid") },
    { 0x1760, 0x177F, true, false, _("Tagbanwa") },
    { 0x1780, 0x17FF, true, false, _("Khmer") },
    { 0x1800, 0x18AF, true, false, _("Mongolian") },
    { 0x1900, 0x194F, true, false, _("Limbu") },
    { 0x1950, 0x197F, true, false, _("Tai Le") },
    { 0x1980, 0x19DF, true, false, _("New Tai Lue") },
    { 0x19E0, 0x19FF, true, false, _("Khmer Symbols") },
    { 0x1A00, 0x1A1F, true, false, _("Buginese") },
    { 0x1D00, 0x1D7F, true, false, _("Phonetic Extensions") },
    { 0x1D80, 0x1DBF, true, false, _("Phonetic Extensions Supplement") },
    { 0x1DC0, 0x1DFF, true, false, _("Combining Diacritical Marks Supplement") },
    { 0x1E00, 0x1EFF, true, false, _("Latin Extended Additional") },
    { 0x1F00, 0x1FFF, true, false, _("Greek Extended") },
    { 0x2000, 0x206F, true, false, _("General Punctuation") },
    { 0x2070, 0x209F, true, false, _("Superscripts and Subscripts") },
    { 0x20A0, 0x20CF, true, false, _("Currency Symbols") },
    { 0x20D0, 0x20FF, true, false, _("Combining Diacritical Marks for Symbols") },
    { 0x2100, 0x214F, true, false, _("Letterlike Symbols") },
    { 0x2150, 0x218F, true, false, _("Number Forms") },
    { 0x2190, 0x21FF, true, false, _("Arrows") },
    { 0x2200, 0x22FF, true, false, _("Mathematical Operators") },
    { 0x2300, 0x23FF, true, false, _("Miscellaneous Technical") },
    { 0x2400, 0x243F, true, false, _("Control Pictures") },
    { 0x2440, 0x245F, true, false, _("Optical Character Recognition") },
    { 0x2460, 0x24FF, true, false, _("Enclosed Alphanumerics") },
    { 0x2500, 0x257F, true, false, _("Box Drawing") },
    { 0x2580, 0x259F, true, false, _("Block Elements") },
    { 0x25A0, 0x25FF, true, false, _("Geometric Shapes") },
    { 0x2600, 0x26FF, true, false, _("Miscellaneous Symbols") },
    { 0x2700, 0x27BF, true, false, _("Dingbats") },
    { 0x27C0, 0x27EF, true, false, _("Miscellaneous Mathematical Symbols-A") },
    { 0x27F0, 0x27FF, true, false, _("Supplemental Arrows-A") },
    { 0x2800, 0x28FF, true, false, _("Braille Patterns") },
    { 0x2900, 0x297F, true, false, _("Supplemental Arrows-B") },
    { 0x2980, 0x29FF, true, false, _("Miscellaneous Mathematical Symbols-B") },
    { 0x2A00, 0x2AFF, true, false, _("Supplemental Mathematical Operators") },
    { 0x2B00, 0x2BFF, true, false, _("Miscellaneous Symbols and Arrows") },
    { 0x2C00, 0x2C5F, true, false, _("Glagolitic") },
    { 0x2C80, 0x2CFF, true, false, _("Coptic") },
    { 0x2D00, 0x2D2F, true, false, _("Georgian Supplement") },
    { 0x2D30, 0x2D7F, true, false, _("Tifinagh") },
    { 0x2D80, 0x2DDF, true, false, _("Ethiopic Extended") },
    { 0x2E00, 0x2E7F, true, false, _("Supplemental Punctuation") },
    { 0x2E80, 0x2EFF, false, true, _("CJK Radicals Supplement") },
    { 0x2F00, 0x2FDF, false, true, _("Kangxi Radicals") },
    { 0x2FF0, 0x2FFF, false, true, _("Ideographic Description Characters") },
    { 0x3000, 0x303F, false, true, _("CJK Symbols and Punctuation") },
    { 0x3040, 0x309F, false, true, _("Hiragana") },
    { 0x30A0, 0x30FF, false, true, _("Katakana") },
    { 0x3100, 0x312F, false, true, _("Bopomofo") },
    { 0x3130, 0x318F, false, true, _("Hangul Compatibility Jamo") },
    { 0x3190, 0x319F, false, true, _("Kanbun") },
    { 0x31A0, 0x31BF, false, true, _("Bopomofo Extended") },
    { 0x31C0, 0x31EF, false, true, _("CJK Strokes") },
    { 0x31F0, 0x31FF, false, true, _("Katakana Phonetic Extensions") },
    { 0x3200, 0x32FF, false, true, _("Enclosed CJK Letters and Months") },
    { 0x3300, 0x33FF, false, true, _("CJK Compatibility") },
    { 0x3400, 0x4DBF, false, true, _("CJK Unified Ideographs Extension A") },
    { 0x4DC0, 0x4DFF, false, true, _("Yijing Hexagram Symbols") },
    { 0x4E00, 0x9FFF, false, true, _("CJK Unified Ideographs") },
    { 0xA000, 0xA48F, false, true, _("Yi Syllables") },
    { 0xA490, 0xA4CF, false, true, _("Yi Radicals") },
    { 0xA700, 0xA71F, true, false, _("Modifier Tone Letters") },
    { 0xA800, 0xA82F, true, false, _("Syloti Nagri") },
    { 0xAC00, 0xD7AF, false, true, _("Hangul Syllables") },
    { 0xD800, 0xDB7F, true, false, _("High Surrogates") },
    { 0xDB80, 0xDBFF, true, false, _("High Private Use Surrogates") },
    { 0xDC00, 0xDFFF, true, false, _("Low Surrogates") },
    { 0xE000, 0xF8FF, false, true, _("Private Use Area") },
    { 0xF900, 0xFAFF, false, true, _("CJK Compatibility Ideographs") },
    { 0xFB00, 0xFB4F, true, false, _("Alphabetic Presentation Forms") },
    { 0xFB50, 0xFDFF, true, false, _("Arabic Presentation Forms-A") },
    { 0xFE00, 0xFE0F, true, false, _("Variation Selectors") },
    { 0xFE10, 0xFE1F, true, false, _("Vertical Forms") },
    { 0xFE20, 0xFE2F, true, false, _("Combining Half Marks") },
    { 0xFE30, 0xFE4F, false, true, _("CJK Compatibility Forms") },
    { 0xFE50, 0xFE6F, true, false, _("Small Form Variants") },
    { 0xFE70, 0xFEFF, true, false, _("Arabic Presentation Forms-B") },
    { 0xFF00, 0xFFEF, true, false, _("Halfwidth and Fullwidth Forms") },
    { 0xFFF0, 0xFFFF, true, false, _("Specials") },
    { 0x10000, 0x1007F, true, false, _("Linear B Syllabary") },
    { 0x10080, 0x100FF, true, false, _("Linear B Ideograms") },
    { 0x10100, 0x1013F, true, false, _("Aegean Numbers") },
    { 0x10140, 0x1018F, true, false, _("Ancient Greek Numbers") },
    { 0x10300, 0x1032F, true, false, _("Old Italic") },
    { 0x10330, 0x1034F, true, false, _("Gothic") },
    { 0x10380, 0x1039F, true, false, _("Ugaritic") },
    { 0x103A0, 0x103DF, true, false, _("Old Persian") },
    { 0x10400, 0x1044F, true, false, _("Deseret") },
    { 0x10450, 0x1047F, true, false, _("Shavian") },
    { 0x10480, 0x104AF, true, false, _("Osmanya") },
    { 0x10800, 0x1083F, true, false, _("Cypriot Syllabary") },
    { 0x10A00, 0x10A5F, true, false, _("Kharoshthi") },
    { 0x1D000, 0x1D0FF, true, false, _("Byzantine Musical Symbols") },
    { 0x1D100, 0x1D1FF, true, false, _("Musical Symbols") },
    { 0x1D200, 0x1D24F, true, false, _("Ancient Greek Musical Notation") },
    { 0x1D300, 0x1D35F, true, false, _("Tai Xuan Jing Symbols") },
    { 0x1D400, 0x1D7FF, true, false, _("Mathematical Alphanumeric Symbols") },
    { 0x20000, 0x2A6DF, false, true, _("CJK Unified Ideographs Extension B") },
    { 0x2F800, 0x2FA1F, false, true, _("CJK Compatibility Ideographs Supplement") },
    { 0xE0000, 0xE007F, true, false, _("Tags") },
    { 0xE0100, 0xE01EF, true, false, _("Variation Selectors Supplement") },
    { 0xF0000, 0xFFFFF, false, true, _("Supplementary Private Use Area-A") },
    { 0x100000, 0x10FFFF, false, true, _("Supplementary Private Use Area-B") }
};
// restore the definition of _(s)
#undef _
#define _(s)    wxGetTranslation(_T(s))

const int UnicodeBlocksCount = sizeof(UnicodeBlocks) / sizeof(UnicodeBlock) ;

int FindBlockIndex(ucs4_t uc)
{
    if(uc<0 || uc>0x10FFFF) return UnicodeBlocksCount; // not in the ranges
    if(uc<=0x1F) return 0;
    if(uc<=0x7F) return 1;

    static const UnicodeBlock *pblock=UnicodeBlocks;
    static int index=0;

    if(uc >= pblock->begin)
    {
        if(uc <= pblock->end) return index;

        do
        {
            ++index;
            ++pblock;
            wxASSERT(index < UnicodeBlocksCount);
        }while(uc > pblock->end);

        if(uc >= pblock->begin) return index;
    }
    else //if(uc < pblock->begin)
    {
        do
        {
            --index;
            --pblock;
            wxASSERT(index >= 0);
        }while(uc < pblock->begin);

        if(uc <= pblock->end) return index;
    }

    return UnicodeBlocksCount; // not in the ranges
}

inline bool IsDelimiter(ucs4_t uc) // include space char
{
    return ((uc<=0x20 && uc>=0) || uc==0x3000);
}

wxString PrefixString(int i)
{
    int count=1;
    while(i>=10)
    {
        i/=10;
        ++count;
    }
    if(count>=8) return wxEmptyString;
    return wxString(wxT('_'), 8-count);
}

void MadEdit::WordCount(bool selection, int &wordCount, int &charCount, int &spaceCount,
                        int &halfWidthCount, int &fullWidthCount, int &lineCount,
                        wxArrayString *detail)
{
    wordCount=charCount=spaceCount=halfWidthCount=fullWidthCount=0;

    vector<int> counts;
    counts.resize(UnicodeBlocksCount+1, 0); // all counts=0

    MadLineIterator lit;
    wxFileOffset linepos, nowpos, endpos;
    if(selection && m_Selection)
    {
        lineCount=m_SelectionEnd->lineid - m_SelectionBegin->lineid +1;
        lit=m_SelectionBegin->iter;
        linepos=m_SelectionBegin->linepos;
        nowpos=m_SelectionBegin->pos;
        endpos=m_SelectionEnd->pos;
    }
    else
    {
        lineCount=(int)m_Lines->m_LineCount;
        lit=m_Lines->m_LineList.begin();
        linepos=0;
        nowpos=0;
        endpos=m_Lines->m_Size;
    }

    // begin counting
    MadLines::NextUCharFuncPtr NextUChar=m_Lines->NextUChar;
    MadUCQueue ucqueue;
    m_Lines->InitNextUChar(lit, linepos);
    int idx=0, previdx=-1, count=0;
    ucs2_t *half=GetHalfwidthTable(true, true, true, true);
    ucs2_t *full=GetFullwidthTable(true, true, true, true);

    while(nowpos < endpos)
    {
        if(++count >= 1024)
        {
            ucqueue.clear();
            count=0;
        }

        if(!(m_Lines->*NextUChar)(ucqueue))
        {
            ++lit;
            m_Lines->InitNextUChar(lit, 0);
            (m_Lines->*NextUChar)(ucqueue);
        }
        MadUCPair &ucp=ucqueue.back();
        nowpos+=ucp.second;
        ucs4_t uc=ucp.first;

        idx=FindBlockIndex(uc);
        counts[idx]++;

        if(IsDelimiter(uc))
        {
            if(uc==0x09 || uc==0x20 || uc==0x3000)
            {
                ++spaceCount;
            }
            idx=-1;
        }
        else if(idx<UnicodeBlocksCount)
        {
            if(previdx>=0 && previdx<UnicodeBlocksCount && UnicodeBlocks[previdx].is_alphabet && UnicodeBlocks[idx].is_alphabet)
            {
                ++charCount;
            }
            else
            {
                ++wordCount;
                ++charCount;

                bool tested=false;
                if(uc<=0xFFFF)
                {
                    if(half[uc]!=0)
                    {
                        ++halfWidthCount;
                        tested=true;
                    }
                    else if(full[uc]!=0)
                    {
                        ++fullWidthCount;
                        tested=true;
                    }
                }
                if(!tested)
                {
                    if(UnicodeBlocks[idx].is_fullwidth)
                        ++fullWidthCount;
                    else
                        ++halfWidthCount;
                }
            }
        }

        previdx=idx;
    }

    if(detail!=NULL)
    {
        for(idx=0;idx<UnicodeBlocksCount;++idx)
        {
            if(counts[idx]>0)
            {
                detail->Add(wxString::Format(wxT("%s %d    U+%04X - U+%04X: %s"), PrefixString(counts[idx]).c_str(), counts[idx],
                    UnicodeBlocks[idx].begin, UnicodeBlocks[idx].end, wxGetTranslation(UnicodeBlocks[idx].description)));
            }
        }
    }
    if(counts[UnicodeBlocksCount]>0)
    {
        detail->Add(wxString::Format(wxT("%s %d    ? - ? %s"), PrefixString(counts[idx]).c_str(), counts[idx], _("Invalid Unicode Characters")));
    }
}

void MadEdit::TrimTrailingSpaces()
{
    if(IsReadOnly() || m_EditMode==emHexMode)
        return;

    // use Regular Expressions to trim all trailing spaces
    ReplaceTextAll(wxT("[ \t]+(\r|\n|$)"), wxT("$1"), true, true, false);
}


//==============================================================================
struct SortLineData
{
    static bool s_casesensitive;
    static bool s_numeric;
    static MadLines *s_lines;
    static MadLines::NextUCharFuncPtr s_NextUChar;

    MadLineIterator lit;
    int lineid;
    vector<ucs4_t> ucdata;
    // for numeric sorting. int_begin=-1 indicates invalid number
    int int_begin, int_len, frac_begin, frac_len;
    bool negative;

    SortLineData(const MadLineIterator& l, int id);
    bool Equal(const SortLineData *data);
};
bool SortLineData::s_casesensitive=false;
bool SortLineData::s_numeric=false;
MadLines *SortLineData::s_lines=NULL;
MadLines::NextUCharFuncPtr SortLineData::s_NextUChar=NULL;

SortLineData::SortLineData(const MadLineIterator& l, int id)
    : lit(l), lineid(id), ucdata(), int_begin(-1), frac_begin(-1), negative(false)
{
    enum { NUM_SIGN, NUM_INT, NUM_FRAC ,NUM_END};
    int numstep=NUM_SIGN;
    int num_idx=0;

    static MadUCQueue ucq;
    ucq.clear();

    s_lines->InitNextUChar(lit, lit->m_RowIndices[0].m_Start);
    ucs4_t uc;
    while((s_lines->*s_NextUChar)(ucq)) // get line content
    {
        if( (uc=ucq.back().first)==0x0D || uc==0x0A)
        {
            ucq.pop_back();
            break;
        }

        if(!s_casesensitive) // ignore case
        {
            if(uc<0x10000 && uc>=0)
            {
                uc = towlower(wchar_t(uc));
            }
        }

        //save every character
        ucdata.push_back(uc);

        if(s_numeric && numstep!=NUM_END)
        {
            if(numstep==NUM_SIGN)
            {
                switch(uc)
                {
                case ' ':
                case '\t':
                    break;
                case '-':
                    negative=true;
                    ++numstep;
                    break;
                case '+':
                    ++numstep;
                    break;
                case '.':
                    numstep=NUM_FRAC;
                    break;
                default:
                    if(uc>='0' && uc<='9')
                    {
                        int_begin=num_idx;
                        int_len=1;
                        ++numstep;
                    }
                    else
                    {
                        numstep=NUM_END; // invalid number format
                    }
                    break;
                }
            }
            else if(numstep==NUM_INT)
            {
                switch(uc)
                {
                case '.':
                    ++numstep;
                    break;
                default:
                    if(uc>='0' && uc<='9')
                    {
                        if(int_begin==-1)
                        {
                            int_begin=num_idx;
                            int_len=1;
                        }
                        else
                        {
                            ++int_len;
                        }
                    }
                    else
                    {
                        numstep=NUM_END; // invalid number format
                    }
                    break;
                }
            }
            else //if(numstep==NUM_FRAC)
            {
                if(uc>='0' && uc<='9')
                {
                    if(int_begin==-1)
                    {
                        int_begin=0;
                        int_len=0;
                    }
                    if(frac_begin==-1)
                    {
                        frac_begin=num_idx;
                        frac_len=1;
                    }
                    else
                    {
                        ++frac_len;
                    }
                }
                else
                {
                    numstep=NUM_END; // invalid number format
                }
            }
            ++num_idx;
        }
    }

    if(s_numeric) //post processing
    {
        if(int_begin>=0 && int_len>0) // trim leading '0'
        {
            do
            {
                if(ucq[int_begin].first!='0')
                {
                    break;
                }
                ++int_begin;
            }
            while(--int_len > 0);
        }
        if(frac_begin>=0) // trim trailing '0'
        {
            do
            {
                if(ucq[frac_begin+frac_len-1].first!='0')
                {
                    break;
                }
            }
            while(--frac_len > 0);
            if(frac_len==0)
            {
                frac_begin=-1;
            }
        }
    }
}

bool SortLineData::Equal(const SortLineData *data)
{
    size_t count = ucdata.size();
    const vector<ucs4_t> &ucdata2 = data->ucdata;
    if(count != ucdata2.size()) return false;
    if(count == 0) return true;

    size_t i=0;
    do
    {
        if(ucdata[i] != ucdata2[i]) return false;
    }
    while(++i < count);
    return true;
}


// it's LessThan Comparable. For std::sort().
struct SortLineComp
{
    static bool s_numeric;
    SortLineData *data;

    SortLineComp() :data(NULL) {}
    SortLineComp(SortLineData *d) :data(d) {}

    bool operator<(const SortLineComp& it) const
    {
        if(s_numeric) // compare number
        {
            if(data->int_begin>=0)
            {
                if(it.data->int_begin < 0) return true;

                if(data->negative && !it.data->negative) return true;
                if(!data->negative && it.data->negative) return false;

                const SortLineData *data1, *data2;
                if(data->negative)
                {
                    data1 = it.data;
                    data2 = this->data;
                }
                else
                {
                    data1 = this->data;
                    data2 = it.data;
                }

                // compare integer
                if(data1->int_len < data2->int_len) return true;
                if(data1->int_len > data2->int_len) return false;

                //int_len == it.int_len
                const vector<ucs4_t> &it1ucdata = data1->ucdata;
                const vector<ucs4_t> &it2ucdata = data2->ucdata;
                if(data1->int_len > 0)
                {
                    const int it1beg=data1->int_begin;
                    const int it2beg=data2->int_begin;
                    int i=0;
                    do
                    {
                        const ucs4_t uc1 = it1ucdata[it1beg+i];
                        const ucs4_t uc2 = it2ucdata[it2beg+i];
                        if(uc1 < uc2) return true;
                        if(uc1 > uc2) return false;
                    }
                    while(++i < data1->int_len);
                }

                // compare fraction
                const int it1beg=data1->frac_begin;
                const int it2beg=data2->frac_begin;
                if(it2beg < 0) return false;
                if(it1beg < 0) return true;

                const int it1len=data1->frac_len;
                const int it2len=data2->frac_len;
                int i=0;
                do
                {
                    const ucs4_t uc1 = it1ucdata[it1beg+i];
                    const ucs4_t uc2 = it2ucdata[it2beg+i];
                    if(uc1 < uc2) return true;
                    if(uc1 > uc2) return false;
                    ++i;
                }
                while(i<it1len && i<it2len);

                if(it1len < it2len) return true;
            }
        }
        else // compare string
        {
            const vector<ucs4_t> &ucdata1 = data->ucdata;
            const vector<ucs4_t> &ucdata2 = it.data->ucdata;
            const size_t len1 = ucdata1.size();
            const size_t len2 = ucdata2.size();

            if(len1==0) return true;
            if(len2==0) return false;

            size_t i = 0;
            do
            {
                if(ucdata1[i] < ucdata2[i]) return true;
                if(ucdata1[i] > ucdata2[i]) return false;
                ++i;
            }
            while(i<len1 && i<len2);

            if(len1 < len2) return true;
        }
        return false;
    }
};
bool SortLineComp::s_numeric=false;

void MadEdit::SortLines(MadSortFlags flags, int beginline, int endline)
{
    if(IsReadOnly() || m_EditMode==emHexMode)
        return;

    int maxline=int(m_Lines->m_LineCount) - 1;
    MadLineIterator lit = m_Lines->m_LineList.end();
    --lit;
    if(lit->m_Size == 0) --maxline; // the last line is empty

    if(beginline<0) // sort all lines
    {
        beginline=0;
        endline=maxline;
    }
    else
    {
        if(beginline>maxline) beginline=maxline;
        if(endline>maxline) endline=maxline;
    }
    if(endline-beginline <= 0) return; // it's unneeded to sort

    bool oldModified = m_Modified;

    bool bDescending = (flags&sfDescending)!=0;
    bool bRemoveDup = (flags&sfRemoveDuplicate)!=0;
    SortLineData::s_casesensitive = (flags&sfCaseSensitive)!=0;
    SortLineData::s_numeric = (flags&sfNumericSort)!=0;
    SortLineComp::s_numeric = SortLineData::s_numeric;

    SortLineData::s_lines=m_Lines;
    SortLineData::s_NextUChar=m_Lines->NextUChar;

    std::list<SortLineData*> datalist;
    std::vector<SortLineComp> lines;

    wxFileOffset pos=0, delsize=0, lastNewLineSize;

    lit = m_Lines->m_LineList.begin();
    int i=0;
    while(i<beginline) // ignore preceding lines
    {
        pos += lit->m_Size;
        ++lit;
        ++i;
    }

    if(i==0) // ignore BOM
    {
        pos = lit->m_RowIndices[0].m_Start;
        delsize = -pos;
    }

    for(;;) // prepare for data & calc delsize
    {
        SortLineData *d = new SortLineData(lit, i);
        datalist.push_back(d);
        lines.push_back(SortLineComp(d));
        delsize += lit->m_Size;
        if(++i > endline)
        {
            lastNewLineSize = lit->m_NewLineSize;
            delsize -= lastNewLineSize; // ignore newline char of last line
            break;
        }

        ++lit;
    }

    // sort lines
    std::stable_sort(lines.begin(), lines.end());

    // put the sorted lines to MemData
    MadBlock blk(m_Lines->m_MemData, m_Lines->m_MemData->m_Size, 0);

    vector<wxByte> buffervector;
    wxByte *buf=NULL;

    std::vector<SortLineComp>::iterator slit = lines.begin();
    std::vector<SortLineComp>::iterator slitend = lines.end();
    std::vector<SortLineComp>::reverse_iterator slrit = lines.rbegin();
    std::vector<SortLineComp>::reverse_iterator slritend = lines.rend();
    SortLineData *dupdata=NULL;
    do
    {
        SortLineData *data=NULL;
        if(bDescending)
        {
            data = (*slrit).data;
        }
        else
        {
            data = slit->data;
        }
        ++slrit;
        ++slit;

        if(bRemoveDup)
        {
            if(dupdata!=NULL && dupdata->Equal(data))
            {
                dupdata = data;
                data = NULL;
                if(slit == slitend)
                {
                    delsize += lastNewLineSize; // delete last newner char
                }
            }
            else
            {
                dupdata = data;
            }
        }

        if(data != NULL)
        {
            lit = data->lit;
            wxFileOffset spos = lit->m_RowIndices[0].m_Start;
            size_t size= lit->m_Size - spos;

            if(slit == slitend) // ignore newline char of last line
            {
                size -= lit->m_NewLineSize;
            }

            if(size>0)
            {
                if(buffervector.size()<size)
                {
                    buffervector.resize(size);
                    buf=&buffervector[0];
                }
                lit->Get(spos, buf, size);
                m_Lines->m_MemData->Put(buf, size); // put line text (include newline char)
                blk.m_Size+=size;
            }

            if(lit->m_NewLineSize == 0 && slit != slitend) //append a newline char
            {
                ucs4_t newline[2]={ 0x0D, 0x0A };
                switch(m_InsertNewLineType)
                {
                case nltDOS:
#ifdef __WXMSW__
                case nltDefault:
#endif
                    UCStoBlock(newline, 2, blk);
                    break;
                case nltMAC:
                    UCStoBlock(newline, 1, blk);
                    break;
                case nltUNIX:
#ifndef __WXMSW__
                case nltDefault:
#endif
                    UCStoBlock(newline+1, 1, blk);
                    break;
                }
            }
        }

    }
    while(slit != slitend);

    // free all data in datalist
    std::list<SortLineData*>::iterator dit=datalist.begin();
    do
    {
        delete (*dit);
    }
    while(++dit != datalist.end());

    //
    MadOverwriteUndoData *oudata = new MadOverwriteUndoData();
    oudata->m_Pos = pos;
    oudata->m_DelSize = delsize;
    oudata->m_InsSize = blk.m_Size;
    oudata->m_InsData.push_back(blk);

    lit = DeleteInsertData( oudata->m_Pos,
                            oudata->m_DelSize, &oudata->m_DelData,
                            oudata->m_InsSize, &oudata->m_InsData);

    MadUndo *undo = m_UndoBuffer->Add();

    if(m_CaretPos.pos > m_Lines->m_Size) m_CaretPos.pos = m_Lines->m_Size;
    undo->m_CaretPosBefore=m_CaretPos.pos;
    undo->m_CaretPosAfter=m_CaretPos.pos;

    undo->m_Undos.push_back(oudata);

    bool sc= (oldModified==false);
    m_Modified = true;
    m_Selection = false;
    m_RepaintAll = true;
    Refresh(false);

    if(IsTextFile())
    {
        m_Lines->Reformat(lit, lit);

        //m_CaretPos.pos = undo->m_CaretPosAfter;
        UpdateCaretByPos(m_CaretPos, m_ActiveRowUChars, m_ActiveRowWidths, m_CaretRowUCharPos);

        AppearCaret();
        UpdateScrollBarPos();

        if(m_EditMode == emHexMode)
        {
            if(!m_CaretAtHexArea)
            {
                UpdateTextAreaXPos();
                m_LastTextAreaXPos = m_TextAreaXPos;
            }
        }
    }
    else
    {
        //m_CaretPos.pos = undo->m_CaretPosAfter;
        m_CaretPos.linepos = m_CaretPos.pos;

        AppearCaret();
        UpdateScrollBarPos();

        if(!m_CaretAtHexArea)
        {
            UpdateTextAreaXPos();
            m_LastTextAreaXPos = m_TextAreaXPos;
        }
    }

    m_LastCaretXPos = m_CaretPos.xpos;

    DoSelectionChanged();
    if(sc) DoStatusChanged();
}

void MadEdit::ConvertWordWrapToNewLine()
{
    if(IsReadOnly() || GetEditMode()==emHexMode || m_Lines->m_LineCount==m_Lines->m_RowCount)
        return;

    wxFileOffset begpos = 0, endpos = GetFileSize();
    if(IsSelected())
    {
        begpos = m_SelectionBegin->pos;
        endpos = m_SelectionEnd->pos;
    }
    wxFileOffset pos = begpos;
    MadLineIterator lit;
    int rowid;
    GetLineByPos(lit, pos, rowid); // get first line we want to process

    vector<wxFileOffset> del_pos;
    do
    {
        if(lit->RowCount() > 1) // there are wrapped-lines
        {
            MadRowIndexIterator rit = lit->m_RowIndices.begin();
            ++rit;
            MadRowIndexIterator ritend = lit->m_RowIndices.end();
            --ritend;
            wxFileOffset p;
            while(rit!=ritend && (p=pos+rit->m_Start) <= endpos)
            {
                if(p >= begpos) del_pos.push_back(p);
                ++rit;
            }
        }
        pos += lit->m_Size;
        ++lit;
    }
    while(pos < endpos);

    if(del_pos.size()==0) return; // there is no wrapped-line

    MadBlock blk(m_Lines->m_MemData, -1, 0);
    ucs4_t newline[2]={ 0x0D, 0x0A };
    switch(m_InsertNewLineType)
    {
    case nltDOS:
#ifdef __WXMSW__
    case nltDefault:
#endif
        UCStoBlock(newline, 2, blk);
        break;
    case nltMAC:
        UCStoBlock(newline, 1, blk);
        break;
    case nltUNIX:
#ifndef __WXMSW__
    case nltDefault:
#endif
        UCStoBlock(newline+1, 1, blk);
        break;
    }

    vector<wxByte> newlinedata;
    newlinedata.resize(blk.m_Size);
    wxByte *buf = &newlinedata[0];
    blk.Get(0, buf, blk.m_Size);

    vector<wxByte*> ins_data;
    vector<wxFileOffset> ins_len;
    size_t count = del_pos.size();
    do
    {
        ins_data.push_back(buf);
        ins_len.push_back(blk.m_Size);
    }
    while(--count > 0);

    wxFileOffset size=del_pos.back() - del_pos.front();
    if((size <= 2*1024*1024) || (del_pos.size()>=40 && size<= 10*1024*1024))
    {
        OverwriteDataSingle(del_pos, del_pos, NULL, &ins_data, ins_len);
    }
    else
    {
        OverwriteDataMultiple(del_pos, del_pos, NULL, &ins_data, ins_len);
    }
}

void MadEdit::ConvertNewLineToWordWrap()
{
    if(IsReadOnly() || GetEditMode()==emHexMode || !IsSelected())
        return;

    wxFileOffset begpos = m_SelectionBegin->pos, endpos = m_SelectionEnd->pos;
    wxFileOffset pos = begpos, pos1;
    MadLineIterator lit, lit1;
    int rowid;
    GetLineByPos(lit, pos, rowid); // get first line we want to process

    lit1 = lit;
    if(++lit1 == m_Lines->m_LineList.end()) return;
    pos1 = pos+lit->m_Size;

    vector<wxFileOffset> del_bpos, del_epos;
    while(pos1 <= endpos && lit1 != m_Lines->m_LineList.end())
    {
        if(lit->m_Size!=lit->m_NewLineSize && lit1->m_Size!=lit1->m_NewLineSize)
        {
            del_bpos.push_back(pos1-lit->m_NewLineSize);
            del_epos.push_back(pos1);
        }

        pos = pos1;
        pos1 += lit1->m_Size;
        ++lit;
        ++lit1;
    }

    if(del_bpos.size()==0) return;

    vector<wxByte*> ins_data;
    vector<wxFileOffset> ins_len;
    size_t count = del_bpos.size();
    do
    {
        ins_data.push_back(0);
        ins_len.push_back(0);
    }
    while(--count > 0);

    wxFileOffset size=del_epos.back() - del_bpos.front();
    if((size <= 2*1024*1024) || (del_bpos.size()>=40 && size<= 10*1024*1024))
    {
        OverwriteDataSingle(del_bpos, del_epos, NULL, &ins_data, ins_len);
    }
    else
    {
        OverwriteDataMultiple(del_bpos, del_epos, NULL, &ins_data, ins_len);
    }
}

void MadEdit::ConvertSpaceToTab()
{
    if(IsReadOnly() || GetEditMode()==emHexMode || !IsSelected())
        return;

    vector < ucs4_t > newtext;
    bool modified = false;

    size_t firstrow = m_SelectionBegin->rowid;
    size_t subrowid = m_SelectionBegin->subrowid;
    MadLineIterator lit = m_SelectionBegin->iter;
    wxFileOffset pos = m_SelectionBegin->pos - m_SelectionBegin->linepos;

    const size_t lastrow = m_SelectionEnd->rowid;
    const int RowCount = int(lastrow - firstrow + 1);

    MadUCQueue ucqueue;
    MadLines::NextUCharFuncPtr NextUChar=m_Lines->NextUChar;
    for(;;)
    {
        int rowwidth = lit->m_RowIndices[subrowid].m_Width;
        int nowxpos = 0;
        int xpos1, xpos2;

        if(GetEditMode() == emColumnMode)
        {
            xpos1 = m_SelLeftXPos;
            xpos2 = m_SelRightXPos;
        }
        else
        {
            if(RowCount == 1) // one row only
            {
                xpos1 = m_SelectionBegin->xpos;
                xpos2 = m_SelectionEnd->xpos;
            }
            else
            {
                if(firstrow == m_SelectionBegin->rowid) // first row
                {
                    xpos1 = m_SelectionBegin->xpos;
                    xpos2 = rowwidth;
                }
                else if(firstrow == lastrow) // last line
                {
                    xpos1 = 0;
                    xpos2 = m_SelectionEnd->xpos;
                }
                else // middle line
                {
                    xpos1 = 0;
                    xpos2 = rowwidth;
                }
            }
        }

        if(xpos1 < rowwidth)
        {
            int space_count = 0;
            wxFileOffset rowpos = lit->m_RowIndices[subrowid].m_Start;
            wxFileOffset rowendpos = lit->m_RowIndices[subrowid + 1].m_Start;

            m_Lines->InitNextUChar(lit, rowpos);
            do
            {
                int uc = 0x0D;
                if((m_Lines->*NextUChar)(ucqueue))
                    uc = ucqueue.back().first;

                if(uc == 0x0D || uc == 0x0A)    // EOL
                {
                    break;
                }

                int ucwidth = GetUCharWidth(uc);
                if(uc == 0x09)
                {
                    int tabwidth = m_TabColumns * GetUCharWidth(0x20);
                    ucwidth = rowwidth - nowxpos;
                    tabwidth -= (nowxpos % tabwidth);
                    if(tabwidth < ucwidth)
                        ucwidth = tabwidth;
                }
                nowxpos += ucwidth;

                int uchw = ucwidth >> 1;
                if(xpos1 > uchw)
                {
                    rowpos += ucqueue.back().second;
                    xpos1 -= ucwidth;
                    xpos2 -= ucwidth;
                }
                else
                {
                    xpos1 = 0;
                    if(xpos2 > uchw)
                    {
                        if(uc == 0x20)
                        {
                            ++space_count;
                            int w = nowxpos % (m_TabColumns * ucwidth);
                            if(w < ucwidth)
                            {
                                newtext.push_back(0x09);
                                modified = true;
                                space_count = 0;
                                nowxpos -= w;
                                ucwidth -= w;
                            }
                        }
                        else if(uc == 0x09)
                        {
                            newtext.push_back(0x09);
                            if(space_count > 0)
                            {
                                modified = true;
                                space_count = 0;
                            }
                        }
                        else
                        {
                            if(space_count > 0)
                            {
                                do
                                {
                                    newtext.push_back(0x20);
                                }
                                while(--space_count > 0);
                            }
                            newtext.push_back(uc);
                        }

                        xpos2 -= ucwidth;
                    }
                    else
                        xpos2 = 0;
                }

            }
            while(xpos2 > 0 && rowpos < rowendpos);

            if(space_count > 0)
            {
                do
                {
                    newtext.push_back(0x20);
                }
                while(--space_count > 0);
            }
        }

        // add newline
        if(GetEditMode() == emColumnMode)
        {
#ifdef __WXMSW__
            newtext.push_back(0x0D);
#endif
            newtext.push_back(0x0A);
        }
        else
        {
            if((firstrow != lastrow) && (subrowid+1 == lit->RowCount()))
            {
                switch(m_Lines->GetNewLine(lit))
                {
                case 0x0D:
                    newtext.push_back(0x0D);
                    break;
                case 0x0A:
                    newtext.push_back(0x0A);
                    break;
                case 0x0D+0x0A:
                    newtext.push_back(0x0D);
                    newtext.push_back(0x0A);
                    break;
                }
            }
        }

        if(firstrow == lastrow)
            break;

        ++firstrow;
        ucqueue.clear();

        // to next row, line
        if(++subrowid == lit->RowCount())
        {
            pos += lit->m_Size;
            ++lit;
            subrowid = 0;
        }
    }

    if(modified)
    {
        if(GetEditMode()==emColumnMode)
            InsertColumnString(&newtext[0], newtext.size(), RowCount, false, true);
        else
            InsertString(&newtext[0], newtext.size(), false, false, true);
    }
}

void MadEdit::ConvertTabToSpace()
{
    if(IsReadOnly() || GetEditMode()==emHexMode || !IsSelected())
        return;

    vector < ucs4_t > newtext;
    bool modified = false;

    size_t firstrow = m_SelectionBegin->rowid;
    size_t subrowid = m_SelectionBegin->subrowid;
    MadLineIterator lit = m_SelectionBegin->iter;
    wxFileOffset pos = m_SelectionBegin->pos - m_SelectionBegin->linepos;

    const size_t lastrow = m_SelectionEnd->rowid;
    const int RowCount = int(lastrow - firstrow + 1);

    MadUCQueue ucqueue;
    MadLines::NextUCharFuncPtr NextUChar=m_Lines->NextUChar;
    for(;;)
    {
        int rowwidth = lit->m_RowIndices[subrowid].m_Width;
        int nowxpos = 0;
        int xpos1, xpos2;

        if(GetEditMode() == emColumnMode)
        {
            xpos1 = m_SelLeftXPos;
            xpos2 = m_SelRightXPos;
        }
        else
        {
            if(RowCount == 1) // one row only
            {
                xpos1 = m_SelectionBegin->xpos;
                xpos2 = m_SelectionEnd->xpos;
            }
            else
            {
                if(firstrow == m_SelectionBegin->rowid) // first row
                {
                    xpos1 = m_SelectionBegin->xpos;
                    xpos2 = rowwidth;
                }
                else if(firstrow == lastrow) // last line
                {
                    xpos1 = 0;
                    xpos2 = m_SelectionEnd->xpos;
                }
                else // middle line
                {
                    xpos1 = 0;
                    xpos2 = rowwidth;
                }
            }
        }

        if(xpos1 < rowwidth)
        {
            wxFileOffset rowpos = lit->m_RowIndices[subrowid].m_Start;
            wxFileOffset rowendpos = lit->m_RowIndices[subrowid + 1].m_Start;

            m_Lines->InitNextUChar(lit, rowpos);
            do
            {
                int uc = 0x0D;
                if((m_Lines->*NextUChar)(ucqueue))
                    uc = ucqueue.back().first;

                if(uc == 0x0D || uc == 0x0A)    // EOL
                {
                    break;
                }

                int ucwidth = GetUCharWidth(uc);
                if(uc == 0x09)
                {
                    int tabwidth = m_TabColumns * GetUCharWidth(0x20);
                    ucwidth = rowwidth - nowxpos;
                    tabwidth -= (nowxpos % tabwidth);
                    if(tabwidth < ucwidth)
                        ucwidth = tabwidth;
                }
                nowxpos += ucwidth;

                int uchw = ucwidth >> 1;
                if(xpos1 > uchw)
                {
                    rowpos += ucqueue.back().second;
                    xpos1 -= ucwidth;
                    xpos2 -= ucwidth;
                }
                else
                {
                    xpos1 = 0;
                    if(xpos2 > uchw)
                    {
                        if(uc == 0x09)
                        {
                            int sw = GetUCharWidth(0x20);
                            int space_count = ucwidth / sw;
                            if(space_count*sw < ucwidth) ++space_count;
                            ucwidth = space_count * sw;

                            do
                            {
                                newtext.push_back(0x20);
                            }
                            while(--space_count > 0);
                            modified = true;
                        }
                        else
                        {
                            newtext.push_back(uc);
                        }

                        xpos2 -= ucwidth;
                    }
                    else
                        xpos2 = 0;
                }
            }
            while(xpos2 > 0 && rowpos < rowendpos);
        }

        // add newline
        if(GetEditMode() == emColumnMode)
        {
#ifdef __WXMSW__
            newtext.push_back(0x0D);
#endif
            newtext.push_back(0x0A);
        }
        else
        {
            if((firstrow != lastrow) && (subrowid+1 == lit->RowCount()))
            {
                switch(m_Lines->GetNewLine(lit))
                {
                case 0x0D:
                    newtext.push_back(0x0D);
                    break;
                case 0x0A:
                    newtext.push_back(0x0A);
                    break;
                case 0x0D+0x0A:
                    newtext.push_back(0x0D);
                    newtext.push_back(0x0A);
                    break;
                }
            }
        }

        if(firstrow == lastrow)
            break;

        ++firstrow;
        ucqueue.clear();

        // to next row, line
        if(++subrowid == lit->RowCount())
        {
            pos += lit->m_Size;
            ++lit;
            subrowid = 0;
        }
    }

    if(modified)
    {
        if(GetEditMode()==emColumnMode)
            InsertColumnString(&newtext[0], newtext.size(), RowCount, false, true);
        else
            InsertString(&newtext[0], newtext.size(), false, false, true);
    }
}
