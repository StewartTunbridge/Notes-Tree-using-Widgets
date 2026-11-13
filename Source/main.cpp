////////////////////////////////////////////////////////////////////////////////////////////////////
//
// NOTES TREE - Tree of Notes
// ==========================
//
//  7 Jan 2026  Genesis

#define Revision_ "1.6"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include "../Widgets/Widgets.hpp"
#include "../Widgets/WidgetsTree.hpp"
#include "../Widgets/WidgetsEdit.hpp"
#include "../Widgets/ColourSelect.hpp"
#include "../Widgets/MessageBox.hpp"
#include "../Widgets/MessageResponseBox.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SUPPORT
//
////////////////////////////////////////////////////////////////////////////////////////////////////

extern bool TextCallBackEdit (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size);
extern void RenderDrawLine (int Colour, int x1, int y1, int x2, int y2, int Width);
extern void RenderFillRect (_Rect Rect, int Colour);

_Bitmap *BitmapResource;
_Point BitmapResourceSize;
int BitmapIndex = 10;

//bool RenderBitmap (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Point Dest, int ColTransparent, byte Alpha = 0xFF);   // same size
int DrawTextBitmap (_Window *Window, int Index, int x, int y, int FontHeight);

int x_, y_;

void RenderDrawLine (int Col, int x, int y)
  {
    if (Col < 0)
      {
        x_ = x;
        y_ = y;
      }
    else
      {
        RenderDrawLine (Col, x_, y_, x_ + x, y_ + y, 2);
        x_ += x;
        y_ += y;
      }
  }

bool TextCallBackEditSpecial (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    byte a;
    //
    a = **Text;
    if (!TextCallBackEdit (Window, Text, Font, Posn, Size))
      if (Font && a && (a <= 4))   // Draw Image
        {
          if (Posn)
            {
              DrawTextBitmap (Window, 7 + a - 1, Posn->x - 4, Posn->y, Font->YAdvance);
              Posn->x += 8;
            }
          if (Size)
            {
              Size->x += 8;
              Size->y = Max (Size->y, Font->YAdvance);
            }
          (*Text)++;
          return true;
        }
    return false;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TreeViewSpecial
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class _TreeViewSpecial: public _TreeView
  {
    public:
      _Edit *EditGet (void);
      _TreeViewSpecial (_Container *Parent, _Rect Rect_, _Action Action_ = NULL);
  };

_TreeViewSpecial::_TreeViewSpecial (_Container *Parent, _Rect Rect_, _Action Action_)
  : _TreeView (Parent, Rect_, Action_ )
  {
  }

_Edit *_TreeViewSpecial::EditGet (void)
  {
    return Edit;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN FORM - DEFINED
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class _FormMain: public _Form
  {
    public:
      _Container *Toolbar;
      //_Button *bCut, *bCopy, *bPaste, *bRename, *bDelete;
      _ButtonArrow *bUp, *bDown;
      _ButtonArrow *bLeft, *bRight;
      _Button *bAdd, *bAddChild, *bRename, *bRemove;
      _Button *bMarks [4];
      _Edit *eTarget;
      _MenuPopup *mTarget;
      _ButtonArrow *bNext;
      _Label *lChanged;
      _TreeViewSpecial *tCategories;
      _MenuPopup *mCategories;
      _Splitter *Splitter;
      _EditMultiline *eContent;
      _MenuPopup *mContent;
      //
      _FormMain (char *Title);
      ~_FormMain ();
  };

_FormMain *fMain;


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN FORM - ACTIONS
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bool Quit = false;
_TreeViewNode *NodeOld = NULL;
int ChangeTick = 0;

void ActionNodeSelected (_TreeView *Tree)
  {
    if (NodeOld)
      {
        if (NodeOld->Data)
          free (NodeOld->Data);
        NodeOld->Data = NULL;
        StrAssignCopy ((char **) &NodeOld->Data, fMain->eContent->TextGet ());
      }
    if (Tree->Selected != NodeOld)
      fMain->eContent->TextSet ((char *) Tree->Selected->Data);
    NodeOld = Tree->Selected;
  }

void ActionShift (_ButtonArrow *Button)
  {
    bool Res;
    //
    if (Button->Direction == dUp)
      Res = fMain->tCategories->Selected->NodeMoveUp ();
    else if (Button->Direction == dDown)
      Res = fMain->tCategories->Selected->NodeMoveDown ();
    else if (Button->Direction == dLeft)
      Res = fMain->tCategories->Selected->NodeMoveLeft ();
    else // if (Button->Direction == dRight)
      Res = fMain->tCategories->Selected->NodeMoveRight ();
    if (Res)
      ChangeTick = ClockMS ();
    else
      Beep ();
    fMain->tCategories->Invalidate ();
  }

void ActionAdd (_Button *Button)
  {
    if (fMain->tCategories->Selected)
      {
        fMain->tCategories->Selected = fMain->tCategories->Selected->NodeAdd (false, "Title");
        ActionNodeSelected (fMain->tCategories);
        fMain->tCategories->Invalidate ();
        ChangeTick = ClockMS ();
      }
  }

void ActionAddChild (_Container *Button)
  {
    if (fMain->tCategories->Selected)
      {
        fMain->tCategories->Selected->Expanded = true;
        fMain->tCategories->Selected = fMain->tCategories->Selected->NodeAdd (true, "Sub Title");
        ActionNodeSelected (fMain->tCategories);
        fMain->tCategories->Invalidate ();
        ChangeTick = ClockMS ();
      }
  }

void ActionRename (_Container *Button)
  {
    _Edit *Edit;
    //
    if (fMain->tCategories->Selected)
      {
        fMain->tCategories->NodeEdit ();
        Edit = fMain->tCategories->EditGet ();
        Edit->TextCallBack = TextCallBackEditSpecial;
      }
  }

void ActionNodeEditWrite (_TreeView *TreeView, char *Text, bool Abort)
  {
    if (!Abort)
      {
        StrAssignCopy (&TreeView->Selected->Text, Text);
        ChangeTick = ClockMS ();
      }
  }

void ActionRemove_ (int Result, void *Parameter)
  {
    if (Result == 0)   // Yes
      {
        delete fMain->tCategories->Selected;
        fMain->tCategories->Selected = fMain->tCategories->Nodes->Children;
        NodeOld = NULL;
        ActionNodeSelected (fMain->tCategories);
        ChangeTick = ClockMS ();
      }
    fMain->tCategories->Invalidate ();
  }

void ActionRemove (_Container *Button)
  {
    if (fMain->tCategories->Selected)
      {
        MessageResponseBox ({400, 64}, "Delete this Category (and sub Categories) ?", "Yes\tNo", ActionRemove_);
        fMessageResponseBox->Container->Colour = ColourAdjust (cRed, 170);
        //fMessageResponseBox->Container->ColourText = cWhite;
      }
  }

void ActionFindFocus (void)
  {
    char *Text, *s;
    int *Sel;
    //
    Text = fMain->eContent->TextGet ();
    Sel = &(fMain->eContent->Selected [0]);
    if (Sel [0] == Sel [1])
      fMain->eContent->SelectWord ();
    if (Sel [0] < Sel [1])
      s = StrMake (&Text [Sel [0]], Sel [1] - Sel [0]);
    else
      s = StrMake (&Text [Sel [1]], Sel [0] - Sel [1]);
    fMain->eTarget->TextSet (s);
    free (s);
    fMain->eTarget->Clipboard (coSelectAll);
    fMain->eTarget->FocusSet ();
  }

void CategoryChange (_TreeViewNode *Node)
  {
    fMain->tCategories->Selected = Node;
    while (Node->Parent)
      {
        Node = Node->Parent;
        Node->Expanded = true;
      }
    // %%%% fMain->tCategories->ScrollToSelected ();
    fMain->tCategories->Invalidate ();
    ActionNodeSelected (fMain->tCategories);
  }

void ActionFindNext (_Container *Button)
  {
    char *Target;
    char *Text;
    int tl;
    _TreeViewNode *page;
    int pos;
    char *p;
    int Loops;
    //
    // Setup
    Target = fMain->eTarget->TextGet ();
    tl = StrLen (Target);
    page = fMain->tCategories->Selected;
    Text = fMain->eContent->TextGet ();
    // Perform Search
    Loops = 0;
    pos = fMain->eContent->Selected [0];
    while (true)
      {
        p = StrPos (Text + pos, Target);
        if (p)   // Found
          {
            if (page != fMain->tCategories->Selected)   // Move to the right page
              {
                CategoryChange (page);
                fMain->tCategories->ScrollToSelected ();
              }
            // Move to the right text
            fMain->eContent->MoveToIndex (p - Text + tl);
            fMain->eContent->Selected [1] = p - Text;
            fMain->eContent->Selected [0] = p - Text + tl;
            fMain->eContent->ScrollToCursor ();
            fMain->eContent->Invalidate ();
            fMain->eContent->FocusSet ();
            break;
          }
        else   // not in this page
          {
            page = page->NodeNext (NULL, false);   // Go to next page
            if (page == NULL)   // End of Tree?
              {
                page = fMain->tCategories->Nodes;   // Start of tree
                if (++Loops >= 2)
                  {
                    Beep ();
                    break;
                  }
              }
            // Grab the New Page
            Text = (char *) page->Data;
            pos = 0;
            // check the Category
            if (StrPos (page->Text, Target))   // found in Category
              {
                CategoryChange (page);
                fMain->tCategories->ScrollToSelected ();
                fMain->tCategories->FocusSet ();
                break;
              }
          }
      }
  }

void ActionChange (_Container *EditMulti)
  {
    ChangeTick = ClockMS ();
  }

void ActionPopupCategories (_MenuPopup *Popup)
  {
    switch (Popup->Selected)
      {
        case 0: ActionShift (fMain->bUp); break;
        case 1: ActionShift (fMain->bDown); break;
        case 2: ActionAdd (NULL); break;
        case 3: ActionAddChild (NULL); break;
        case 4: ActionRename (NULL); break;
        case 5: ActionRemove (NULL); break;
      }
  }

const char *MenuPopupEditText = "\e3 Cut\t\e4 Copy\t\e5 Paste\t_\e6 Undo\t\e7 Tick\t\e8 Cross\t\e9 Dot\t\ea WTF";

char const Symbols [] = {'\1', '\2', '\3', '\4'};

void MarkCategory (int Sym)
  {
    char *Category, c;
    int l;
    //
    l = StrLen (fMain->tCategories->Selected->Text);
    Category = (char *) malloc (l + 1 + 1);
    strcpy (Category, fMain->tCategories->Selected->Text);
    c = *Category;   // remember what was there
    while (*Category > '\0' && *Category <= 4)   // remove old tag(s)
      StrDelete (Category, 1);
    if (c != Symbols [Sym])   // setting a different tag
      StrInsert (Category, Symbols [Sym]);
    StrAssign (&fMain->tCategories->Selected->Text, Category);
    fMain->tCategories->Invalidate ();
    ChangeTick = ClockMS ();
  }

void EditInsertSymbol (int Sym)
  {
    _Edit *Edit;
    //
    //Edit = dynamic_cast <_Edit *> (fMain->Focused);
    if (fMain->Focused == fMain->eTarget || fMain->Focused == fMain->eContent)
      {
        Edit = (_Edit *) (fMain->Focused);
        Edit->Insert (Edit->Selected [0], 1, (char *) &Symbols [Sym]);
        Edit->MoveToIndex (Edit->Selected [0] + 1);
        Edit->Invalidate ();
      }
  }

void ActionPopupEdit (_MenuPopup *Popup)
  {
    _Edit *Edit;
    //
    Edit = (_Edit *) Popup->DataContainer;
    if (Popup->Selected >= 0)
      switch (Popup->Selected)
        {
          case 0: Edit->Clipboard (coCut); break;
          case 1: Edit->Clipboard (coCopy); break;
          case 2: Edit->Clipboard (coPaste); break;
          case 3: Edit->Undo (); break;
          default: EditInsertSymbol (Popup->Selected - 4);
        }
  }

void ActionMark (_Button *Button)
  {
    if (fMain->Focused == fMain->tCategories)
      MarkCategory (Button->DataInt);
    else
      EditInsertSymbol (/*(_Edit *) fMain->eContent,*/ Button->DataInt);
  }

bool FormEventPreview (_Form *Form, _Event *Event)
  {
    bool Res;
    //
    Res = false;
    if (Event->Type == etKeyDown)
      {
        Res = true;
        if (Event->Key == esc && (Form->Focused == fMain->tCategories || Form->Focused == fMain->eTarget))
          fMain->eContent->FocusSet ();
        else if (Event->Key == esc && Form->Focused == fMain->eContent)
          fMain->tCategories->FocusSet ();
        else if (Event->Key == Cntrl ('Q'))
          Quit = true;
        else if (Event->Key == Cntrl ('F'))
          ActionFindFocus ();
        else if ((Event->Key == KeyEnter && Form->Focused == fMain->eTarget) || Event->Key == Cntrl ('N'))
          ActionFindNext (NULL);
        else
          Res = false;
      }
    return Res;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SAVE / LOAD
//
////////////////////////////////////////////////////////////////////////////////////////////////////

char *Filename = "NotesTree.dat";

bool DataSave (void)
  {
    _TreeViewNode *Node;
    int f;
    char *Line, *l;
    int Len;
    char *c;
    //
    if (ChangeTick == 0)
      return true;
    FileBackup (Filename);
    DebugAdd ("Saving Data");  // ####
    ActionNodeSelected (fMain->tCategories);
    f = FileOpen (Filename, foWrite);
    if (f >= 0)
      {
        Node = fMain->tCategories->Nodes->Children;
        while (Node)
          {
            Line = NULL;
            while (true)
              {
                l = Line;
                Len = 0;
                Len += StrCat (&l, Node->Text);
                Len += StrCat (&l, '\t');
                Len += NumToStr (&l, Node->Depth ());
                if (Node == fMain->tCategories->Selected)
                  Len += StrCat (&l, '*');
                if (Node->Data)
                  {
                    Len += StrCat (&l, '\t');
                    c = (char *) Node->Data;
                    while (*c)
                      Len += NumToStrBase (&l, (byte) (*c++), 2 | DigitsZeros, 16);
                  }
                if (Line == NULL)
                  Line = (char *) malloc (Len + 1);
                else
                  break;
              }
            *l = 0;
            FileWriteLine (f, Line);
            if (Line)
              free (Line);
            if (Node->Children)
              Node = Node->Children;
            else if (Node->Next)
              Node = Node->Next;
            else
              while (true)
                {
                  Node = Node->Parent;
                  if (Node == NULL)
                    break;
                  if (Node->Next)
                    {
                      Node = Node->Next;
                      break;
                    }
                }
          }
        FileClose (f);
        ChangeTick = 0;
        return true;
      }
    return false;
  }

void DataLoad ()
  {
    _TreeViewNode *Node;
    int f;
    int fs;
    char *Data;
    char *d1, *d2, *d3;
    char *Name;
    int Depth, DepthPrev;
    char *Notes;
    int ns;
    bool Sel;
    _TreeViewNode *NodeStart;
    //
    NodeStart = NULL;
    f = FileOpen (Filename, foRead);
    if (f >= 0)
      {
        fs = FileSize (f);
        Data = (char *) malloc (fs + 1);
        FileRead (f, (byte *) Data, fs);
        Data [fs] = 0;
        d1 = Data;
        Node = fMain->tCategories->Nodes;
        DepthPrev = 0;
        while (true)
          {
            Sel = false;
            d2 = StrPos (d1, '\n');
            if (d2 == NULL)
              break;
            *d2 = 0;   // terminate line
            d2++;   // step past lf
            Name = StrGetField (&d1);
            Depth = StrGetNum (&d1);
            if (*d1 == '*')   // Selected Category
              {
                d1++;
                Sel = true;
              }
            Notes = NULL;
            if (*d1)
              {
                d1++;   // step past tab
                d3 = d1;   // 2 pass: 1) Get Size. 2) Get Data
                ns = StrGetHHBlock (&d3, (byte *) Notes, IntMax);
                Notes = (char *) malloc (ns + 1);
                ns = StrGetHHBlock (&d1, (byte *) Notes, ns);
                Notes [ns] = 0;
              }
            while (Depth < DepthPrev)
              {
                Node = Node->Parent;
                DepthPrev--;
              }
            Node = Node->NodeAdd (Depth > DepthPrev, Name, Notes);
            free (Name);
            if (Sel)
              NodeStart = Node;
            d1 = d2;
            DepthPrev = Depth;
          }
        free (Data);
        FileClose (f);
      }
    if (fMain->tCategories->Nodes->Children == NULL)   // nothing loaded
      fMain->tCategories->Selected = fMain->tCategories->Nodes->NodeAdd (true, "My Notes");
    else
      {
        fMain->tCategories->Selected = fMain->tCategories->Nodes->Children;   // first category
        if (NodeStart)
          CategoryChange (NodeStart);
      }
    ActionNodeSelected (fMain->tCategories);
    ChangeTick = 0;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN FORM - CREATE / DESTROY
//
////////////////////////////////////////////////////////////////////////////////////////////////////

_FormMain::_FormMain (char *Title): _Form (Title, {0, 0, 550, 350}, waResizable | waAlwaysOnTop)
  {
    const int bw = 54;   // button width
    const int bh = 36;   // button height
    const int tw = 160;   // Tree Width
    int x, y, bh_;
    int x2;
    int i;
    char MarkText [4], *m;
    //
    bh_ = bh / 2;
    Container->FontSet (NULL); //("Ubuntu-R.ttf", 14);"ARI.ttf"
    Container->Bitmap = BitmapLoad (Window, "NotesTree.bmp");
    BitmapResource = Container->Bitmap;
    BitmapGetSize (BitmapResource, &BitmapResourceSize.x, &BitmapResourceSize.y);
    y = 0;
    //
    Toolbar = new _Container (Container, {0, y, 0, bh}, NULL);
    x = 0;
    bLeft = new _ButtonArrow (Toolbar, {x, bh_ / 2, bh_, bh_}, NULL, (_Action) ActionShift, dLeft);
    x += bh_;
    bUp = new _ButtonArrow (Toolbar, {x, 0, bh_, bh_}, NULL, (_Action) ActionShift, dUp);
    bDown = new _ButtonArrow (Toolbar, {x, bh_, bh_, 0}, NULL, (_Action) ActionShift, dDown);
    x += bh_;
    bRight = new _ButtonArrow (Toolbar, {x, bh_ / 2, bh_, bh_}, NULL, (_Action) ActionShift, dRight);
    x += bh_ + 4;
    bAdd = new _Button (Toolbar, {x, 0, bw, 0}, "\e0\nAdd", (_Action) ActionAdd); x += bw;
    bAddChild = new _Button (Toolbar, {x, 0, bw, 0}, "\e0\nAdd Sub", (_Action) ActionAddChild); x += bw;
    bRename = new _Button (Toolbar, {x, 0, bw, 0}, "\e2\nRename", ActionRename); x += bw;
    bRemove = new _Button (Toolbar, {x, 0, bw, 0}, "\e1\nRemove", ActionRemove); x += bw;
    x += 4;
    strcpy (MarkText, "\e#");
    m = StrPos (MarkText, '#');
    for (i = 0; i < SIZEARRAY (bMarks); i++)
      {
        *m = IntToHexDigit (7 + i);
        bMarks [i] = new _Button (Toolbar, {x + (i&1 ? bh_ : 0), i&2 ? bh_ : 0, bh_, bh_}, MarkText, (_Action) ActionMark);
        bMarks [i]->DataInt = i;
      }
    x += bh + 4;
    x2 = 0;
    lChanged = new _Label (Toolbar, {x2 - bh_, 0, 0}, "*", aCenter);
    lChanged->FontSet (NULL, bh * 2 / 3);
    lChanged->VisibleSet (false);
    x2 -= bh_;
    bNext = new _ButtonArrow (Toolbar, {x2 - bh_, 4, bh_, 0}, NULL, ActionFindNext, dRight);
    x2 -= bh_;
    eTarget = new _Edit (Toolbar, {x, 4, x2, -4}, "Search");
    eTarget->TextCallBack = TextCallBackEditSpecial;
    mTarget = new _MenuPopup (eTarget, {0}, MenuPopupEditText, (_Action) ActionPopupEdit);
    mTarget->DataContainer = eTarget;
    x = 0;
    y += bh;
    //
    tCategories = new _TreeViewSpecial (Container, {x, y, tw, 0}, (_Action) ActionNodeSelected);
    tCategories->Colour = ColourAdjust (cBlue, 195);
    tCategories->ColourText = cBlue;
    tCategories->ActionNodeEditWrite = ActionNodeEditWrite;
    tCategories->TextCallBack = TextCallBackEditSpecial;
    mCategories = new _MenuPopup (tCategories, {0}, "Move Up\t_MoveDown\t\e0 Add Category\t\e0 Add Sub Category\t\e2 Rename\t\e1 Remove", (_Action) ActionPopupCategories);
    x += tw;
    //
    Splitter = new _Splitter (Container, {x, y, 8, 0});
    x += 8;
    //
    eContent = new _EditMultiline (Container, {x, y, 0, 0}, NULL, (_Action) ActionChange);
    eContent->Colour = ColourAdjust (cYellow, 195); // (cYellow, 195); //
    eContent->ColourText = ColourAdjust (cYellow, 20); //cGreenDark;
    eContent->TextCallBack = TextCallBackEditSpecial;
    mContent = new _MenuPopup (eContent, {0}, MenuPopupEditText, (_Action) ActionPopupEdit);
    mContent->DataContainer = eContent;
    //
    tCategories->FocusSet ();
    EventPreview = FormEventPreview;
  }

_FormMain::~_FormMain ()
  {
    ActionNodeSelected (fMain->tCategories);
    DataSave ();
  }

int main_ (int argc, char *argv [])
  {
    char Title [128], *t;
    _Bitmap *Icon;
    //
    if (AlreadyRunning ("NotesTree"))
      return -1;
    ButtonStyleDefault = bsFlat; //; //bsSimple
    t = Title;
    StrCat (&t, "Stewy's Notes Tree [");
    StrCat (&t, Revision_);
    StrCat (&t, ']');
    *t = 0;
    fMain = new _FormMain (Title);
    Icon = BitmapLoad (fMain->Window, "NotesTreeIcon.bmp");
    WindowSetIcon (fMain->Window, Icon);
    BitmapDestroy (Icon);
    DataLoad ();
    while (!FormsUpdate () && !Quit)
      {
        usleep (10000);
        if (ChangeTick && (ClockMS () - ChangeTick > 10000))   // auto save 10 seconds after changes
          DataSave ();
        fMain->lChanged->VisibleSet (ChangeTick);
      }
    while (FormList)
      delete FormList;
    return 0;
  }
