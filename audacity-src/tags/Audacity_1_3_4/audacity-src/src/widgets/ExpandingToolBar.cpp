/**********************************************************************

  Audacity: A Digital Audio Editor

  ExpandingToolBar.cpp

  Dominic Mazzoni

*******************************************************************//**

\class ExpandingToolBar
\brief A smart ToolBar class that has a "MainPanel" which is always
  displayed, and an "ExtraPanel" that can be hidden to save space.

  Can be docked into a ToolBarArea or floated in an ToolBarFrame;

  If auto-expanding is off, behavior is very simple: clicking the
  toggle button expands, clicking it again collapses.

  If auto-expanding is on, behavior is a little more complicated.
  When the mouse movers over a toolbar and it is collapsed, it gets
  auto-expanded, and it gets auto-collapsed as soon as the mouse
  leaves.  However, if they manually toggle it collapsed
  while it was auto-expanded, it will stay collapsed until you move
  the mouse completely away and then back again later.  If you
  manually expand it, it will stay manually expanded until you
  manually collapse it.

*//****************************************************************//**

\class ExpandingToolBarEvtHandler
\brief A custom event handler for ExpandingToolBar.

*//****************************************************************//**

\class ToolBarGrabber
\brief Draws the grabber for an ExpandingToolBar.

*//****************************************************************//**

\class ToolBarDialog
\brief A dialog based container for ExpandingToolBars providing modal
based operations.

*//****************************************************************//**

\class ToolBarFrame
\brief A miniframe based container for ExpandingToolBars providing
modeless presentation.

*//****************************************************************//**

\class ToolBarArea
\brief An alterantive to ToolBarFrame which can contain an
ExpandingToolBar.  ToolBarArea is used for a 'docked' ToolBar, 
ToolBarFrame for a floating one.

*//****************************************************************//**

\class ToolBarArrangement
\brief Small class that holds some layout information for an 
ExpandingToolBar.

*//*******************************************************************/



#include "../Theme.h"

#include <wx/dcmemory.h>
#include <wx/dragimag.h>
#include <wx/arrimpl.cpp>
#include <wx/dialog.h>

#include "ExpandingToolBar.h"
#include "AButton.h"
#include "../AllThemeResources.h"
#include "../Experimental.h"

const int kToggleButtonHeight = 8;
const int kTimerInterval = 50; // every 50 ms -> ~20 updates per second   
const wxRect kDummyRect = wxRect(-9999, -9999, 0, 0);

enum {
   kToggleButtonID = 5000,
   kTimerID
};

WX_DEFINE_OBJARRAY(wxArrayRect);

class ToolBarArrangement
{
public:
   ExpandingToolBarArray    childArray;
   wxArrayRect              rectArray;
   wxArrayInt               rowArray;
};

//
// ExpandingToolBar
//

BEGIN_EVENT_TABLE(ExpandingToolBar, wxPanel)
   EVT_SIZE(ExpandingToolBar::OnSize)
   EVT_TIMER(kTimerID, ExpandingToolBar::OnTimer)
   EVT_BUTTON(kToggleButtonID, ExpandingToolBar::OnToggle)
END_EVENT_TABLE()

IMPLEMENT_CLASS(ExpandingToolBar, wxPanel)

//static
int ExpandingToolBar::msNoAutoExpandStack = 0;

ExpandingToolBar::ExpandingToolBar(wxWindow* parent,
                                   wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size):
   wxPanel(parent, id, pos, size),
   mIsAutoExpanded(false),
   mIsManualExpanded(false),
   mIsExpanded(false),
   mAutoExpand(true),
   mFirstTime(true),
   mFrameParent(NULL),
   mDialogParent(NULL),
   mAreaParent(NULL),
   mSavedArrangement(NULL),
   mDragImage(NULL),
   mTopLevelParent(NULL)
{
   mMainPanel = new wxPanel(this, -1,
                            wxDefaultPosition, wxSize(1, 1));
   mExtraPanel = new wxPanel(this, -1,
                             wxDefaultPosition, wxSize(1, 1));

   mGrabber = NULL;

   ToolBarArea *toolBarParent =
      dynamic_cast<ToolBarArea *>(GetParent());
   if (toolBarParent)
      mGrabber = new ToolBarGrabber(this, -1, this);

   /// \todo check whether this is a memory leak (and check similar code)
   wxImage hbar = theTheme.Image(bmpToolBarToggle);
   wxColour magicColor = wxColour(0, 255, 255);
   ImageArray fourStates = ImageRoll::SplitV(hbar, magicColor);

   mToggleButton = new AButton(this, kToggleButtonID,
                               wxDefaultPosition, wxDefaultSize,
                               ImageRoll(ImageRoll::HorizontalRoll,
                                         fourStates[0], magicColor),
                               ImageRoll(ImageRoll::HorizontalRoll,
                                         fourStates[1], magicColor),
                               ImageRoll(ImageRoll::HorizontalRoll,
                                         fourStates[2], magicColor),
                               ImageRoll(ImageRoll::HorizontalRoll,
                                         fourStates[3], magicColor),
                               true);
   mToggleButton->UseDisabledAsDownHiliteImage(true);

   SetAutoLayout(true);
   mTimer.SetOwner(this, kTimerID);
}

ExpandingToolBar::~ExpandingToolBar()
{
}

void ExpandingToolBar::OnSize(wxSizeEvent &evt)
{
   if (mFrameParent || mDialogParent || mAreaParent)
      return;

   // At the time of construction, it wasn't "safe" to tell
   // our parent that we've just joined the window, so we check
   // for it during our first OnSize event.

   if (!mFrameParent) {   
      ToolBarFrame *toolBarParent =
         dynamic_cast<ToolBarFrame *>(GetParent());
      if (toolBarParent) {
         // We were placed into a floating window
         mFrameParent = toolBarParent;
         toolBarParent->SetChild(this);
      }
   }

   if (!mDialogParent) {   
      ToolBarDialog *toolBarParent =
         dynamic_cast<ToolBarDialog *>(GetParent());
      if (toolBarParent) {
         // We were placed into a dialog
         mDialogParent = toolBarParent;
         toolBarParent->SetChild(this);
      }
   }

   if (!mAreaParent) {
      ToolBarArea *toolBarParent =
         dynamic_cast<ToolBarArea *>(GetParent());
      if (toolBarParent) {
         // We were placed into an area full of other toolbars
         mAreaParent = toolBarParent;
         toolBarParent->AddChild(this);
      }
   }
}

void ExpandingToolBar::OnToggle(wxCommandEvent &evt)
{
   if (mIsExpanded)
      Collapse();
   else
      Expand();
}

void ExpandingToolBar::Expand()
{
   // We set both mIsManualExpanded and mIsAutoExpanded to true;
   // that way if the user manually collapses the toolbar we set
   // mIsManualExpanded to false but keep mIsAutoExpanded to true
   // to prevent it from being auto-expanded again until the user
   // actually moves the mouse completely away and back again later.

   mToggleButton->PushDown();
   mIsManualExpanded = true;
   mIsAutoExpanded = true;
   Fit();
}

void ExpandingToolBar::Collapse(bool now /* = false */)
{
   // After being manually collapsed, we set mIsAutoExpanded back to
   // true, which prevents it from being immediately auto-expanded
   // again until after the mouse actually moves away and then
   // back again later.

   mToggleButton->PopUp();
   mIsManualExpanded = false;
   mIsAutoExpanded = false;
   Fit();
   mIsAutoExpanded = true;

   if (now) {
      mCurrentDrawerSize = mTargetDrawerSize;

      MoveDrawer(wxSize(0, 0));
   }
}

void ExpandingToolBar::TryAutoExpand()
{
   if (mAutoExpand && msNoAutoExpandStack==0 &&
       mIsManualExpanded == false && mIsAutoExpanded == false) {
      mToggleButton->PushDown();
      mIsAutoExpanded = true;
      Fit();
   }
}

void ExpandingToolBar::TryAutoCollapse()
{
#ifdef EXPERIMENTAL_ROLL_UP_DIALOG
   if (mIsAutoExpanded == true && mIsManualExpanded == false) {
      mToggleButton->PopUp();
      mIsAutoExpanded = false;
      Fit();
   }
#endif
}

class ExpandingToolBarEvtHandler : public wxEvtHandler
{
 public:
   ExpandingToolBarEvtHandler(ExpandingToolBar *toolbar,
                              wxEvtHandler *inheritedEvtHandler)
   {
      mToolBar = toolbar;
      mInheritedEvtHandler = inheritedEvtHandler;
   }

   virtual bool ProcessEvent(wxEvent& evt)
   {
      if (mToolBar->IsCursorInWindow())
         mToolBar->TryAutoExpand();
      else
         mToolBar->TryAutoExpand();

      return mInheritedEvtHandler->ProcessEvent(evt);
   }

protected:
   ExpandingToolBar *mToolBar;
   wxEvtHandler *mInheritedEvtHandler;

   DECLARE_NO_COPY_CLASS(ExpandingToolBarEvtHandler);
};

void ExpandingToolBar::RecursivelyPushEventHandlers(wxWindow *win)
{
   if (!mWindowHash[win]) {
      ExpandingToolBarEvtHandler *evtHandler =
         new ExpandingToolBarEvtHandler(this, win->GetEventHandler());
      win->PushEventHandler(evtHandler);
      mWindowHash[win] = 1;
   }
 
   wxWindowList children = win->GetChildren();

   typedef wxWindowList::compatibility_iterator Node;
   for(Node node = children.GetFirst(); node; node = node->GetNext()) {
      wxWindow *child = node->GetData();
      RecursivelyPushEventHandlers(child);
   }
}

bool ExpandingToolBar::Layout()
{
   mMainSize = mMainPanel->GetBestFittingSize();
   mExtraSize = mExtraPanel->GetBestFittingSize();
   mButtonSize = wxSize(wxMax(mMainSize.x, mExtraSize.x),
                        kToggleButtonHeight);

   int left = 0;

   if (mGrabber) {
      mGrabberSize = mGrabber->GetMinSize();
      left += mGrabberSize.x;
   }
   else
      mGrabberSize = wxSize(0, 0);

   mMainPanel->SetSize(left, 0, mMainSize.x, mMainSize.y);
   mToggleButton->SetSize(left, mMainSize.y, mButtonSize.x, mButtonSize.y);
   mExtraPanel->SetSize(left, mMainSize.y + mButtonSize.y,
                        mExtraSize.x, mExtraSize.y);

   if (mGrabber)
      mGrabber->SetSize(0, 0, left, mMainSize.y + mButtonSize.y);

   // Add event handlers to all children
   //RecursivelyPushEventHandlers(this);

   return true;
}

void ExpandingToolBar::Fit()
{
#ifdef EXPERIMENTAL_ROLL_UP_DIALOG
   mIsExpanded = (mIsAutoExpanded || mIsManualExpanded);
#else
   mIsExpanded = true;// JKC - Wedge it open at all times.
#endif

   int width = mButtonSize.x + mGrabberSize.x;

   wxSize baseWindowSize = wxSize(width,
                                  mMainSize.y + mButtonSize.y);

   mTargetDrawerSize = wxSize(mButtonSize.x, 0);

   if (mIsExpanded)
      mTargetDrawerSize.y += mExtraSize.y;

   mCurrentDrawerSize.x = mTargetDrawerSize.x;

   // The first time, we always update the size.  Otherwise, we set
   // a target size, and the actual size changes during a timer
   // event.

   if (mFirstTime) {
      mFirstTime = false;
      mCurrentDrawerSize = wxSize(mExtraSize.x, 0);
      mCurrentTotalSize = baseWindowSize;

      SetSizeHints(mCurrentTotalSize, mCurrentTotalSize);
      SetSize(mCurrentTotalSize);
   }

   // wxTimers seem to be a little unreliable - sometimes they stop for
   // no good reason, so this "primes" it every now and then...
   mTimer.Stop();
   mTimer.Start(kTimerInterval);
}

bool ExpandingToolBar::IsCursorInWindow()
{
   wxPoint globalMouse = ::wxGetMousePosition();
   wxPoint localMouse = ScreenToClient(globalMouse);

   bool result = (localMouse.x >= 0 && localMouse.y >= 0 &&
                  localMouse.x < mCurrentTotalSize.x &&
                  localMouse.y < mCurrentTotalSize.y);

   // The grabber doesn't count!
   if (mGrabber && mGrabber->GetRect().Inside(localMouse))
      result = false;

   return result;
}

void ExpandingToolBar::ReparentExtraPanel()
{
   // This is how we make sure the extra panel, which slides out
   // like a drawer, appears on top of everything else in the window...

   wxPoint pos;
   pos.x = mGrabberSize.x;
   pos.y = mMainSize.y + mButtonSize.y;
   wxWindow *frame = this;
   while(!frame->IsTopLevel()) {
      pos += frame->GetPosition();
      frame = frame->GetParent();
   }

   mExtraPanel->Reparent(frame);
   mExtraPanel->SetPosition(pos);
}

void ExpandingToolBar::MoveDrawer(wxSize prevSize)
{
   mCurrentTotalSize = wxSize(mButtonSize.x,
                              mMainSize.y +
                              mButtonSize.y +
                              mCurrentDrawerSize.y);

   if (mFrameParent) {
      // If we're in a tool window

      SetSizeHints(mCurrentTotalSize, mCurrentTotalSize);
      SetSize(mCurrentTotalSize);
      
      GetParent()->Fit();
   }

   if (mDialogParent) {
      // If we're in a dialog

      SetSizeHints(mCurrentTotalSize, mCurrentTotalSize);
      SetSize(mCurrentTotalSize);
      
      GetParent()->Fit();
   }

   if (mAreaParent) {
      // If we're in a tool area

      if (mCurrentDrawerSize.y > 0 && prevSize.y == 0) {
         ReparentExtraPanel();
         mExtraPanel->Show();
      }

      mExtraPanel->SetSizeHints(mCurrentDrawerSize, mCurrentDrawerSize);
      mExtraPanel->SetSize(mCurrentDrawerSize);

      if (mCurrentDrawerSize.y == 0)
         mExtraPanel->Hide();
   }
}

void ExpandingToolBar::OnTimer(wxTimerEvent &evt)
{
   if (mAutoExpand && msNoAutoExpandStack==0 &&
       IsCursorInWindow())
      TryAutoExpand();
   else if (!IsCursorInWindow())
      TryAutoCollapse();

   if (mCurrentDrawerSize == mTargetDrawerSize)
      return;

   // This accelerates the current size towards the target size;
   // it's a neat way for the window to roll open, but in such a
   // way that it 

   wxSize prevSize = mCurrentDrawerSize;
   mCurrentDrawerSize = (mCurrentDrawerSize*2 + mTargetDrawerSize) / 3;
   if (abs((mCurrentDrawerSize-mTargetDrawerSize).x)<2 &&
       abs((mCurrentDrawerSize-mTargetDrawerSize).y)<2)
      mCurrentDrawerSize = mTargetDrawerSize;

   MoveDrawer(prevSize);
}

wxBitmap ExpandingToolBar::GetToolbarBitmap()
{
   wxSize size = GetClientSize();
   wxBitmap bitmap(size.x, size.y);
   wxClientDC winDC(this);
   wxMemoryDC memDC;
   memDC.SelectObject(bitmap);
   memDC.Blit(0, 0, size.x, size.y,
              &winDC, 0, 0);
   return bitmap;
}

void ExpandingToolBar::StartMoving()
{
   if (!mAreaParent)
      return;

   int j;

   mAreaParent->CollapseAll(true);

   mTimer.Stop();

   // This gives time for wx to finish redrawing the window that way.
   // HACK: why do we need to do it so many times???
   for(j=0; j<500; j++)
      wxYield();

   wxBitmap toolbarBitmap = GetToolbarBitmap();

   msNoAutoExpandStack++;
   mSavedArrangement = mAreaParent->SaveArrangement();
   mAreaParent->RemoveChild(this);

   mAreaParent->Refresh(true);

   mTopLevelParent = this;
   while(!mTopLevelParent->IsTopLevel())
      mTopLevelParent = mTopLevelParent->GetParent();

   wxPoint hotSpot = ScreenToClient(wxGetMousePosition());

   hotSpot -= (ClientToScreen(wxPoint(0, 0)) -
               mAreaParent->ClientToScreen(wxPoint(0, 0)));

   mDropTargets = mAreaParent->GetDropTargets();
   mDropTarget = kDummyRect;
   
   wxColour magicColor = wxColour(0, 255, 255);
   wxImage tgtImage = theTheme.Image(bmpToolBarTarget);
   ImageRoll tgtImageRoll = ImageRoll(ImageRoll::VerticalRoll,
                                      tgtImage,
                                      magicColor);
   mTargetPanel = new ImageRollPanel(mAreaParent, -1, tgtImageRoll,
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxTRANSPARENT_WINDOW);
   mTargetPanel->SetLogicalFunction(wxXOR);
   mTargetPanel->SetSize(mDropTarget);

   // This gives time for wx to finish redrawing the window that way.
   // HACK: why do we need to do it several times???
   for(j=0; j<500; j++)
      wxYield();

   mAreaParent->SetCapturedChild(this);

   mDragImage = new wxDragImage(toolbarBitmap);
   mDragImage->BeginDrag(hotSpot, mAreaParent, mTopLevelParent);
   mDragImage->Show();
   mDragImage->Move(ScreenToClient(wxGetMousePosition()));
}

void ExpandingToolBar::UpdateMoving()
{
   if (!mAreaParent || !mSavedArrangement || !mDragImage)
      return;

   wxPoint cursorPos = mAreaParent->ScreenToClient(wxGetMousePosition());
   wxRect prevTarget = mDropTarget;
   int best_dist_sq = 99999;
   int i;

   for(i=0; i<(int)mDropTargets.GetCount(); i++) {
      int x = (mDropTargets[i].x + (mDropTargets[i].width/2))-cursorPos.x;
      int y = (mDropTargets[i].y + (mDropTargets[i].height/2))-cursorPos.y;
      int dist_sq = (x*x) + (y*y);

      if (dist_sq < best_dist_sq) {
         best_dist_sq = dist_sq;
         mDropTarget = mDropTargets[i];
      }         
   }

   if (!mAreaParent->GetRect().Inside(cursorPos))
      mDropTarget = kDummyRect;

   if (mDropTarget != prevTarget) {
      mDragImage->Hide();

      wxRect r = mDropTarget;
      r.Inflate(4, 4);
      mTargetPanel->SetSize(r);

      #if 0
      wxClientDC dc(mAreaParent);
      dc.DestroyClippingRegion();
      dc.SetLogicalFunction(wxINVERT);
      wxRect r = prevTarget;
      r.Inflate(4, 4);
      dc.DrawRectangle(r);
      r = mDropTarget;
      r.Inflate(4, 4);
      dc.DrawRectangle(r);
      #endif
      
      // This gives time for wx to finish redrawing the window that way.
      // HACK: why do we need to do it so many times???
      for(i=0; i<500; i++)
         wxYield();

      mDragImage->Show();
      mDragImage->Move(ScreenToClient(wxGetMousePosition()));
   }
   else
      mDragImage->Move(ScreenToClient(wxGetMousePosition()));
}

void ExpandingToolBar::FinishMoving()
{
   if (!mAreaParent || !mSavedArrangement)
      return;

   delete mTargetPanel;

   mAreaParent->SetCapturedChild(NULL);

   mDragImage->Hide();
   mDragImage->EndDrag();

   msNoAutoExpandStack--;

   if (mDropTarget == kDummyRect) {
      mAreaParent->RestoreArrangement(mSavedArrangement);
      mSavedArrangement = NULL;
   }
   else {
      delete mSavedArrangement;
      mSavedArrangement = NULL;

      mAreaParent->MoveChild(this, mDropTarget);
   }

   // Keep all drawers closed until the user moves specifically to a
   // different window
   mAreaParent->CollapseAll();

   mTopLevelParent->Refresh(true);

   mTimer.Start(kTimerInterval);
}

//
// ToolBarGrabber
//

BEGIN_EVENT_TABLE(ToolBarGrabber, wxPanel)
   EVT_PAINT(ToolBarGrabber::OnPaint)
   EVT_SIZE(ToolBarGrabber::OnSize)
   EVT_MOUSE_EVENTS(ToolBarGrabber::OnMouse)
END_EVENT_TABLE()
   
IMPLEMENT_CLASS(ToolBarGrabber, wxPanel)

ToolBarGrabber::ToolBarGrabber(wxWindow *parent,
                               wxWindowID id,
                               ExpandingToolBar *ownerToolbar,
                               const wxPoint& pos,
                               const wxSize& size):
   wxPanel(parent, id, pos, size),
   mOwnerToolBar(ownerToolbar)
{
   wxImage grabberImages = theTheme.Image(bmpToolBarGrabber);
   wxColour magicColor = wxColour(0, 255, 255);
   ImageArray images = ImageRoll::SplitH(grabberImages, magicColor);

   mImageRoll[0] = ImageRoll(ImageRoll::VerticalRoll,
                             images[0],
                             magicColor);
   mImageRoll[1] = ImageRoll(ImageRoll::VerticalRoll,
                             images[1],
                             magicColor);

   SetSizeHints(mImageRoll[0].GetMinSize(),
                mImageRoll[1].GetMaxSize());

   mState = 0;
}

void ToolBarGrabber::OnMouse(wxMouseEvent &event)
{
   int prevState = mState;

   // Handle hilighting the image if the mouse is over it

   if (event.Entering())
      mState = 1;
   else if (event.Leaving())
      mState = 0;
   else {
      wxSize clientSize = GetClientSize();

      if (event.m_x >= 0 && event.m_y >= 0 &&
          event.m_x < clientSize.x && event.m_y < clientSize.y)
         mState = 1;
      else
         mState = 0;
   }

   if (event.ButtonDown())
      mOwnerToolBar->StartMoving();

   if (mState != prevState)
      Refresh(false);
}

void ToolBarGrabber::OnPaint(wxPaintEvent &evt)
{
   wxPaintDC dc(this);

   mImageRoll[mState].Draw(dc, GetClientRect());
}

void ToolBarGrabber::OnSize(wxSizeEvent &evt)
{
   Refresh(false);
}

//
// ToolBarDialog
//

BEGIN_EVENT_TABLE(ToolBarDialog, wxDialog)
END_EVENT_TABLE()

IMPLEMENT_CLASS(ToolBarDialog, wxDialog)

ToolBarDialog::ToolBarDialog(wxWindow* parent,
                           wxWindowID id,
                           const wxString& name,
                           const wxPoint& pos):
   wxDialog(parent, id, name, pos, wxSize(1, 1), 
// Workaround for bug in __WXMSW__.  No close box on a wxDialog unless wxSYSTEM_MENU is used.
#ifdef __WXMSW__
      wxSYSTEM_MENU |
#endif
      wxCAPTION|wxCLOSE_BOX),      
   mChild(NULL)
{
}

ToolBarDialog::~ToolBarDialog()
{
}

void ToolBarDialog::SetChild(ExpandingToolBar *child)
{
   mChild = child;
   if (mChild && mChild->GetParent() != this)
      mChild->Reparent(this);

   Fit();
}

void ToolBarDialog::Fit()
{
   if (mChild) {
      wxSize childSize = mChild->GetBestFittingSize();

      // Take into account the difference between the content
      // size and the frame size
      wxSize curContentSize = GetClientSize();
      wxSize curFrameSize = GetSize();
      wxSize newFrameSize = childSize + (curFrameSize - curContentSize);

      SetSizeHints(newFrameSize, newFrameSize);
      SetSize(newFrameSize);
   }
}

//
// ToolBarFrame
//

BEGIN_EVENT_TABLE(ToolBarFrame, wxMiniFrame)
END_EVENT_TABLE()

IMPLEMENT_CLASS(ToolBarFrame, wxMiniFrame)

ToolBarFrame::ToolBarFrame(wxWindow* parent,
                           wxWindowID id,
                           const wxString& name,
                           const wxPoint& pos):
   wxMiniFrame(parent, id, name, pos, wxSize(1, 1),
// Workaround for bug in __WXMSW__.  No close box on a miniframe unless wxSYSTEM_MENU is used.
#ifdef __WXMSW__
      wxSYSTEM_MENU |
#endif
      wxCAPTION|wxCLOSE_BOX),
   mChild(NULL)
{
}

ToolBarFrame::~ToolBarFrame()
{
}

void ToolBarFrame::SetChild(ExpandingToolBar *child)
{
   mChild = child;
   if (mChild && mChild->GetParent() != this)
      mChild->Reparent(this);

   Fit();
}

void ToolBarFrame::Fit()
{
   if (mChild) {
      wxSize childSize = mChild->GetBestFittingSize();

      // Take into account the difference between the content
      // size and the frame size
      wxSize curContentSize = GetClientSize();
      wxSize curFrameSize = GetSize();
      wxSize newFrameSize = childSize + (curFrameSize - curContentSize);

      SetSizeHints(newFrameSize, newFrameSize);
      SetSize(newFrameSize);
   }
}

//
// ToolBarArea
//

BEGIN_EVENT_TABLE(ToolBarArea, wxPanel)
   EVT_SIZE(ToolBarArea::OnSize)
   EVT_MOUSE_EVENTS(ToolBarArea::OnMouse)
END_EVENT_TABLE()

IMPLEMENT_CLASS(ToolBarArea, wxPanel)

ToolBarArea::ToolBarArea(wxWindow* parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size):
   wxPanel(parent, id, pos, size),
   mInOnSize(false),
   mCapturedChild(NULL)
{
   
}

ToolBarArea::~ToolBarArea()
{
}

void ToolBarArea::ContractRow(int rowIndex)
{
   // Contract all of the toolbars in a given row to their
   // minimum size.  This is an intermediate step in layout.

   int i;
   int x = 0;

   for(i=0; i<(int)mChildArray.GetCount(); i++)
      if (mRowArray[i] == rowIndex) {
         wxPoint childPos = mChildArray[i]->GetPosition();
         wxSize childMin = mChildArray[i]->GetMinSize();

         mChildArray[i]->SetSize(x, childPos.y,
                                 childMin.x, childMin.y);
         x += childMin.x;
      }
}

bool ToolBarArea::ExpandRow(int rowIndex)
{
   // Expand all of the toolbars in a given row so that the
   // whole width is filled, if possible.  This is the last
   // step after laying out as many toolbars as possible in
   // that row.  Returns false if it's not possible to fit
   // all of these toolbars in one row anymore.

   wxSize area = GetClientSize();
   int i, j, x;
   int minWidth = 0;
   int leftoverSpace = 0;
   int expandableCount = 0;
   int toolbarCount = 0;
   
   for(i=0; i<(int)mChildArray.GetCount(); i++)
      if (mRowArray[i] == rowIndex) {
         ExpandingToolBar *child = mChildArray[i];
         wxPoint childPos = child->GetPosition();
         wxSize childMin = child->GetMinSize();
         wxSize childMax = child->GetMaxSize();

         minWidth += childMin.x;

         toolbarCount++;
         if (childMax.x > childMin.x)
            expandableCount++;
      }

   leftoverSpace = area.x - minWidth;

   if (leftoverSpace <= 0) {
      if (toolbarCount > 1)
         return false; // not possible to fit all in one row
      else
         return true; // there's only one, so it doesn't matter
   }

   j = 0;
   x = 0;
   for(i=0; i<(int)mChildArray.GetCount(); i++)
      if (mRowArray[i] == rowIndex) {
         ExpandingToolBar *child = mChildArray[i];
         wxPoint childPos = child->GetPosition();
         wxSize childMin = child->GetMinSize();
         wxSize childMax = child->GetMaxSize();

         int width = childMin.x;

         if (childMax.x > childMin.x)
            width +=
               (leftoverSpace * (j+1) / expandableCount) -
               (leftoverSpace * (j) / expandableCount);

         mChildArray[i]->SetSize(x, childPos.y,
                                 width, childMin.y);
         x += width;
         j++;
      }

   return true; // success
}

void ToolBarArea::LayoutOne(int childIndex)
{
   wxSize area = GetClientSize();
   ExpandingToolBar *child = mChildArray[childIndex];
   wxSize childMin = child->GetMinSize();
   wxSize childMax = child->GetMaxSize();

   if (childIndex == 0) {
      mRowArray[childIndex] = 0;
      mChildArray[childIndex]->SetSize(0, 0, childMin.x, childMin.y);
      ExpandRow(0);

      wxPoint p = mChildArray[childIndex]->GetPosition();
      wxSize s = mChildArray[childIndex]->GetSize();

      #if 0
      printf("ToolBar %d moved to row %d at (%d, %d), size (%d x %d)\n",
             childIndex, mRowArray[childIndex],
             p.x, p.y, s.x, s.y);
      #endif

      mLastLayoutSize = area;

      return;
   }

   int prevRow = mRowArray[childIndex-1];
   ContractRow(prevRow);
   wxPoint prevPos = mChildArray[childIndex-1]->GetPosition();
   wxSize prevSize = mChildArray[childIndex-1]->GetSize();
   
   int prevX = prevPos.x + prevSize.x;
   int availableWidth = area.x - prevX;

   if (childMin.x <= availableWidth) {
      // It fits into the same row
      mRowArray[childIndex] = prevRow;
      mChildArray[childIndex]->SetSize(prevX, prevPos.y,
                                       childMin.x, childMin.y);
      ExpandRow(prevRow);
   }
   else {
      // Go to the next row
      ExpandRow(prevRow);
      mRowArray[childIndex] = prevRow + 1;

      int i;
      int maxRowHeight = 0;
      for(i=0; i<childIndex; i++)
         if (mRowArray[i] == prevRow &&
             mChildArray[i]->GetSize().y > maxRowHeight)
            maxRowHeight = mChildArray[i]->GetSize().y;

      mChildArray[childIndex]->SetSize(0, prevPos.y + maxRowHeight,
                                       childMin.x, childMin.y);
      ExpandRow(prevRow+1);
   }

   // Save the size of the window the last time we moved one of the
   // toolbars around.  If the user does a minor resize, we try to
   // preserve the layout.  If the user does a major resize, we're
   // allowed to redo the layout.
   mLastLayoutSize = area;

   wxPoint p = mChildArray[childIndex]->GetPosition();
   wxSize s = mChildArray[childIndex]->GetSize();

   #if 0
   printf("ToolBar %d moved to row %d at (%d, %d), size (%d x %d)\n",
          childIndex, mRowArray[childIndex],
          p.x, p.y, s.x, s.y);
   #endif
}

bool ToolBarArea::Layout()
{
   // Redo the layout from scratch, preserving only the order of
   // the children

   int i;

   for(i=0; i<(int)mChildArray.GetCount(); i++)
      mRowArray[i] = -1;

   for(i=0; i<(int)mChildArray.GetCount(); i++)
      LayoutOne(i);

   Refresh(true);

   return true;
}

void ToolBarArea::AdjustLayout()
{
   // Try to modify the layout as little as possible - but if that's
   // impossible, redo the layout as necessary.

   int row = -1;
   int i, j;

   for(i=0; i<(int)mChildArray.GetCount(); i++) {
      if (mRowArray[i] > row) {
         row = mRowArray[i];
         bool success = ExpandRow(row);
         if (!success) {
            // Re-layout all toolbars from this row on
            for(j=i; j<(int)mChildArray.GetCount(); j++)
               LayoutOne(j);
            return;
         }
      }
   }   
}

void ToolBarArea::Fit()
{
   Fit(true, true);
}

void ToolBarArea::Fit(bool horizontal, bool vertical)
{
   wxSize clientSize = GetClientSize();
   wxSize minSize;
   wxSize maxSize;
   wxSize actualSize;
   int i;

   minSize.x = 0;
   minSize.y = 0;
   maxSize.x = 9999;
   maxSize.y = 0;
   for(i=0; i<(int)mChildArray.GetCount(); i++) {
      wxPoint childPos = mChildArray[i]->GetPosition();
      wxSize childSize = mChildArray[i]->GetSize();

      if (childPos.x + childSize.x > actualSize.x) {
         actualSize.x = childPos.x + childSize.x;
      }

      if (childSize.x > minSize.x) {
         minSize.x = childSize.x;
      }

      if (childPos.y + childSize.y > maxSize.y) {
         maxSize.y = childPos.y + childSize.y;
         minSize.y = maxSize.y;
         actualSize.y = maxSize.y;
      }
   }

   if (!horizontal && actualSize.x < clientSize.x)
      actualSize.x = clientSize.x;
   if (!vertical && actualSize.y < clientSize.y)
      actualSize.y = clientSize.y;

   if (minSize != mMinSize ||
       maxSize != mMaxSize) {
      mMinSize = minSize;
      mMaxSize = maxSize;
      SetSizeHints(mMinSize, mMaxSize);
   }
   if (actualSize != mActualSize) {
      mActualSize = actualSize;
      SetSize(mActualSize);
   }
}

void ToolBarArea::OnSize(wxSizeEvent &evt)
{
   if (mInOnSize)
      return;

   mInOnSize = true;

   wxSize currentSize = GetClientSize();

   if (abs(currentSize.x - mLastLayoutSize.x >= 100)) {
      // If they resize by more than 100 pixels (horizontally),
      // we totally redo the layout, preserving the order of the
      // toolbars but not the exact position.
      Layout();
   }
   else {
      // If it was a minor resize, we try to preserve the positions of
      // the toolbars.  If this is impossible, we still redo the layout,
      // of course.
      AdjustLayout();
   }

   Fit(false, true);

   mInOnSize = false;
}

void ToolBarArea::OnMouse(wxMouseEvent &evt)
{
   if (mCapturedChild) {
      if (evt.ButtonUp())
         mCapturedChild->FinishMoving();
      else if (evt.Moving() || evt.Dragging())
         mCapturedChild->UpdateMoving();
   }
   else {
      evt.Skip();
   }
}

void ToolBarArea::CollapseAll(bool now)
{
   int i;
   
   for(i=0; i<(int)mChildArray.GetCount(); i++)
      mChildArray[i]->Collapse(now);
}

void ToolBarArea::AddChild(ExpandingToolBar *child)
{
   mChildArray.Add(child);
   mRowArray.Add(-1); // unknown row
   LayoutOne(mChildArray.GetCount()-1);
   Fit(false, true);
}

void ToolBarArea::RemoveChild(ExpandingToolBar *child)
{
   int i, j;

   for(i=0; i<(int)mChildArray.GetCount(); i++) {
      if (mChildArray[i] == child) {
         child->Hide();

         mChildArray.RemoveAt(i);
         mRowArray.RemoveAt(i);

         for(j=i; j<(int)mChildArray.GetCount(); j++)
            mRowArray[j] = -1;

         for(j=i; j<(int)mChildArray.GetCount(); j++)
            LayoutOne(j);

         Fit(false, true);
      }
   }
}

ToolBarArrangement *ToolBarArea::SaveArrangement()
{
   ToolBarArrangement *arrangement = new ToolBarArrangement();
   int i;

   arrangement->childArray = mChildArray;
   arrangement->rowArray = mRowArray;

   for(i=0; i<(int)mChildArray.GetCount(); i++)
      arrangement->rectArray.Add(mChildArray[i]->GetRect());

   return arrangement;
}

void ToolBarArea::RestoreArrangement(ToolBarArrangement *arrangement)
{
   int i;

   mChildArray = arrangement->childArray;
   mRowArray = arrangement->rowArray;

   for(i=0; i<(int)mChildArray.GetCount(); i++) {
      mChildArray[i]->SetSize(arrangement->rectArray[i]);
      mChildArray[i]->Show();
   }

   Fit(false, true);

   delete arrangement;
}

wxArrayRect ToolBarArea::GetDropTargets()
{
   mDropTargets.Clear();
   mDropTargetIndices.Clear();
   mDropTargetRows.Clear();

   int numChildren = (int)mChildArray.GetCount();
   int i;
   int row = -1;

   if (numChildren == 0)
      return mDropTargets;

   for(i=0; i<numChildren; i++) {
      int childRow = mRowArray[i];
      wxRect childRect = mChildArray[i]->GetRect();

      if (childRow != row) {
         // Add a target before this child (at beginning of row only)
         row = childRow;
         mDropTargetIndices.Add(i);
         mDropTargetRows.Add(row);
         mDropTargets.Add(wxRect(childRect.x, childRect.y,
                                 0, childRect.height));
      }

      // Add a target after this child (always)
      mDropTargetIndices.Add(i+1);
      mDropTargetRows.Add(row);
      mDropTargets.Add(wxRect(childRect.x+childRect.width, childRect.y,
                              0, childRect.height));
   }

   return mDropTargets;
}

void ToolBarArea::MoveChild(ExpandingToolBar *toolBar, wxRect dropTarget)
{
   int i, j;

   for(i=0; i<(int)mDropTargets.GetCount(); i++) {
      if (dropTarget == mDropTargets[i]) {
         int newIndex = mDropTargetIndices[i];
         int newRow = mDropTargetRows[i];

         mChildArray.Insert(toolBar, newIndex);
         mRowArray.Insert(newRow, newIndex);

         for(j=newIndex+1; j<(int)mChildArray.GetCount(); j++)
            mRowArray[j] = -1;

         ContractRow(newRow);

         mChildArray[newIndex]->Show();

         for(j=newIndex; j<(int)mChildArray.GetCount(); j++)
            LayoutOne(j);

         Fit(false, true);

         return;
      }
   }
}                                             

void ToolBarArea::SetCapturedChild(ExpandingToolBar *child)
{
   mCapturedChild = child;
}

// Indentation settings for Vim and Emacs.
// Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// 
