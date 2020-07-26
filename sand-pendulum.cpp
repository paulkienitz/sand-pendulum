// The purpose of this is to mimic the patterns made by a desktop toy of the
// entertainment/meditation type, consisting of a pendulum with slightly
// unequal lengths on the two axes tracing patterns in sand.  Overlapping
// shrinking ellipses add up to make a pattern of nesting parallelograms.
// Paul Kienitz, 2002, public domain.

#include <owl/owlpch.h>
#include <owl/gadgetwi.h>
#include <owl/controlg.h>
#include <owl/textgadg.h>
#include <owl/slider.h>
#include <owl/edit.h>
#include <owl/validate.h>
#include <owl/button.h>
#include <owl/printer.h>
#pragma hdrstop
#include <math.h>
#include <stdio.h>

#define APPNAME         "Sand Pendulum"
#define BACKGROUND      TColor(230, 230, 230)
//#define RINGCOLOR       TColor(200, 200, 200)

#define SLIDERHEIGHT    200
#define SLIDERRANGE     2000     // reduce roundoff glitchies
#define PERIODY_EXTRA   1.02     // initial period Y / period X
#define LOSSPERPERIODX  0.125    // initial drag on speed
//#define MAXRATIO        4.0
//#define MAXDRAG         0.5

#define PERIODX         500.0    // simulation ticks -- eventually user settable
#define MAXPOINTS       50000
#define POINTSPERTIMER  250      // workaround for OS lameness!
#define POINTSPERSECOND 100000   // eventually user settable

#define TICK            100      // timer ID
#define RATIOSLIDERID   101
#define DRAGSLIDERID    102
#define RATIOTEXTID     103
#define DRAGTEXTID      104
#define PRINTID         105

#define log2(d)         (log(d) / M_LN2)

class TSandFrame;

class TFractionEdit : public TEdit
{
public:
    TFractionEdit(TGadgetWindow* parent, int id, int width, int height, int chars)
        : TEdit(parent, id, "", 0, 0, width, height, chars, false)
        {Parent = parent; Attr.Style &= ~WS_BORDER;}

protected:
    TGadgetWindow* Parent;
    void SetupWindow();
    // XXX  add validator that enforces numeric input within given range
};

class TSandCanvas : public TWindow
{
//friend class TSandPrintout;
public:
    TSandCanvas(TSandFrame* parent);
    void Increment(int count);
    TPoint PointToPixel(double x, double y)
        {return TPoint(int(x * radius + pCenter.x), int(y * radius + pCenter.y));}
    TPoint PointToPrinterPixel(double x, double y)
        {return TPoint(int(x * printerRadius + pPrinterCenter.x), int(y * printerRadius + pPrinterCenter.y));}
    void MovePendulum();
    void PaintIncrement(TDC& dc, int index, bool paper);

    double periodX, ratioXY, drag;

protected:
    TSandFrame* Parent;
    TDC* updc;
    double historyX[MAXPOINTS], historyY[MAXPOINTS];
    int historySize;
    TPoint pCenter, pPrinterCenter;
    int radius, printerRadius;
    volatile bool busy, late, death;

    void SetupWindow();
    void CleanupWindow();
    void Paint(TDC& dc, bool erase, TRect& rect);
    void EvTimer(uint timerID);
    void EvLButtonDown(uint modKeys, TPoint& point);
    bool EvEraseBkgnd(HDC) {return true;}
    void EvSize(uint sizeType, TSize& size);
    void EvVScroll(uint scrollCode, uint thumbPos, HWND hWndCtl);
    void EvRatioChange();
    void EvDragChange();
    void EvPrint();

    DECLARE_RESPONSE_TABLE(TSandCanvas);
};

/*class TSandPrintout : public TPrintout
{
public:
    TSandPrintout(TSandCanvas* source, TPrinter* boss) : TPrintout(APPNAME)
            {Source = source; Boss = boss;}
    void GetDialogInfo(int& minPage, int& maxPage,
                             int& selFromPage, int& selToPage)
            {minPage = maxPage = 0; selFromPage = selToPage = 0;}
    void SetPrintParams(TPrintDC* dc, TSize pageSize);
    void PrintPage(int page, TRect& rect, unsigned flags);

    TSandCanvas* Source;
    TPrinter*    Boss;
};   */

class TSandFrame : public TDecoratedFrame
{
public:
    TSandFrame(TSandCanvas* client);

    TGadgetWindow* Sliders;
    TVSlider* RatioSlider;
    TVSlider* DragSlider;
    TEdit* RatioVal;
    TEdit* DragVal;
    TButton* Printer;

protected:
    void SetupWindow();
};

class TSandSliders : public TGadgetWindow
{
public:
    TSandSliders(TWindow* parent, TTileDirection direction, TFont* font)
        : TGadgetWindow(parent, direction, font) {}
    void EvVScroll(uint scrollCode, uint thumbPos, HWND hWndCtl);
    void EvRatioChange();
    void EvDragChange();
    void EvPrint();
    // XXX  add border drawing

    DECLARE_RESPONSE_TABLE(TSandSliders);
};

class TSandPendulumApp : public TApplication
{
public:
    TSandPendulumApp(const char* name) : TApplication(name) {}
protected:
    void InitMainWindow() {SetMainWindow((TFrameWindow*) new TSandFrame(new TSandCanvas(NULL)));}
};

bool IsMessageQueued(TWindow* wind, int message);

// -------------------------------------------------------------------------


int OwlMain(int, char*[])
{
    TSandPendulumApp app("Sand Pendulum");
    app.nCmdShow = SW_SHOWMAXIMIZED;
    return app.Run();
}


void TFractionEdit::SetupWindow()
{
    TEdit::SetupWindow();
    SetExStyle(Attr.ExStyle | WS_EX_CLIENTEDGE);        // Why doesn't this work??
    SetWindowFont(Parent->GetFont(), false);
}


DEFINE_RESPONSE_TABLE1(TSandSliders, TGadgetWindow)
    EV_WM_VSCROLL,
    EV_EN_CHANGE(RATIOTEXTID, EvRatioChange),
    EV_EN_CHANGE(DRAGTEXTID, EvDragChange),
    EV_COMMAND(PRINTID, EvPrint),
END_RESPONSE_TABLE;

void TSandSliders::EvVScroll(uint, uint, HWND)
        {TYPESAFE_DOWNCAST(Parent, TSandFrame)->GetClientWindow()->ForwardMessage();}
void TSandSliders::EvRatioChange()
        {TYPESAFE_DOWNCAST(Parent, TSandFrame)->GetClientWindow()->ForwardMessage();}
void TSandSliders::EvDragChange()
        {TYPESAFE_DOWNCAST(Parent, TSandFrame)->GetClientWindow()->ForwardMessage();}
void TSandSliders::EvPrint()
        {TYPESAFE_DOWNCAST(Parent, TSandFrame)->GetClientWindow()->ForwardMessage();}


TSandFrame::TSandFrame(TSandCanvas* client)
         : TDecoratedFrame(NULL, APPNAME "  -  click on starting point", client, false)
{
    TMargins marge(TMargins::Pixels, 1, 1, 4, 4);
    Sliders = new TSandSliders(this, TGadgetWindow::Vertical, new TGadgetWindowFont(8));
    Sliders->SetMargins(marge);
    RatioVal = new TFractionEdit(Sliders, RATIOTEXTID, 50, 18, 10);
    DragVal = new TFractionEdit(Sliders, DRAGTEXTID, 50, 18, 10);
    RatioSlider = new TVSlider(Sliders, RATIOSLIDERID, 0, 0, 50, SLIDERHEIGHT);
    DragSlider = new TVSlider(Sliders, DRAGSLIDERID, 0, 0, 50, SLIDERHEIGHT);
    Printer = new TButton(Sliders, PRINTID, "Print", 0, 0, 50, 21);
    Sliders->Insert(*new TTextGadget(-1, TGadget::None, TTextGadget::Center, 3, "Ratio"));
    Sliders->Insert(*new TControlGadget(*RatioSlider, TGadget::None));
    Sliders->Insert(*new TControlGadget(*RatioVal, TGadget::None));
    Sliders->Insert(*new TSeparatorGadget(50));
    Sliders->Insert(*new TTextGadget(-1, TGadget::None, TTextGadget::Center, 3, "Drag"));
    Sliders->Insert(*new TControlGadget(*DragSlider, TGadget::None));
    Sliders->Insert(*new TControlGadget(*DragVal, TGadget::None));
    Sliders->Insert(*new TSeparatorGadget(50));
    Sliders->Insert(*new TControlGadget(*Printer, TGadget::None));
    Insert(*Sliders, Left);
}

void TSandFrame::SetupWindow()
{
    char buf[32];
    TDecoratedFrame::SetupWindow();
    RatioSlider->SetRange(0, SLIDERRANGE);
    RatioSlider->SetRuler(SLIDERRANGE / 8, false);
//  RatioSlider->SetPosition(SLIDERRANGE - int(log2(PERIODY_EXTRA) * (double) SLIDERRANGE / 2.0));
    RatioSlider->SetPosition(SLIDERRANGE - int((double) SLIDERRANGE * atan(PERIODY_EXTRA - 1.0) / 1.55));
    sprintf(buf, "%.4f", PERIODY_EXTRA);
    RatioVal->SetText(buf);
    DragSlider->SetRange(0, SLIDERRANGE);
    DragSlider->SetRuler(SLIDERRANGE / 10, false);
    DragSlider->SetPosition(SLIDERRANGE - int(LOSSPERPERIODX * (double) SLIDERRANGE / 0.5));
    sprintf(buf, "%6.5f", LOSSPERPERIODX);
    DragVal->SetText(buf);
    Printer->SetWindowFont(Sliders->GetFont(), false);
}


DEFINE_RESPONSE_TABLE1(TSandCanvas, TWindow)
    EV_WM_VSCROLL,
    EV_EN_CHANGE(RATIOTEXTID, EvRatioChange),
    EV_EN_CHANGE(DRAGTEXTID, EvDragChange),
    EV_COMMAND(PRINTID, EvPrint),
    EV_WM_TIMER,
    EV_WM_SIZE,
    EV_WM_ERASEBKGND,
    EV_WM_LBUTTONDOWN,
END_RESPONSE_TABLE;

TSandCanvas::TSandCanvas(TSandFrame* parent) : TWindow(parent, NULL)
{
    historySize = 0;
    busy = late = death = false;
    periodX = PERIODX;
    ratioXY = PERIODY_EXTRA;
    drag = LOSSPERPERIODX;
}

void TSandCanvas::SetupWindow()
{
    TWindow::SetupWindow();
    Parent = TYPESAFE_DOWNCAST(TWindow::Parent, TSandFrame);
    updc = new TClientDC(HWindow);      // creating and destroying regularly is too slow
}

void TSandCanvas::CleanupWindow()
{
    KillTimer(TICK);
    death = true;
    delete updc;
    TWindow::CleanupWindow();
}

void TSandCanvas::EvTimer(uint TimerID)
{
    if (TimerID == TICK)
        if (busy)
            late = true;
        else
            Increment(POINTSPERTIMER);
}

void TSandCanvas::EvVScroll(uint scrollCode, uint thumbPos, HWND hWndCtl)
{
    char buf[32];
    TVSlider* who;
    int page;
    if (hWndCtl == Parent->RatioSlider->HWindow)
    {
        who = Parent->RatioSlider;
        page = SLIDERRANGE / 16;
    }
    else if (hWndCtl == Parent->DragSlider->HWindow)
    {
        who = Parent->DragSlider;
        page = SLIDERRANGE / 10;
    }
    else
        return;
    // They move the thumb, but they won't tell you where it moved to!
    // So you're forced to set your own distance even if default is fine.
    switch (scrollCode)
    {
        case SB_PAGEDOWN:  thumbPos = who->GetPosition() + page; break;
        case SB_PAGEUP:    thumbPos = who->GetPosition() - page; break;
        case SB_LINEDOWN:  thumbPos = who->GetPosition() + 5;    break;
        case SB_LINEUP:    thumbPos = who->GetPosition() - 5;    break;
        case SB_BOTTOM:    thumbPos = SLIDERRANGE;               break;
        case SB_TOP:       thumbPos = 0;                         break;
        case SB_ENDSCROLL: return;
    }
    if ((int) thumbPos < 0)
        thumbPos = 0;
    if (thumbPos > SLIDERRANGE)
        thumbPos = SLIDERRANGE;
    if (scrollCode != SB_THUMBTRACK)
        who->SetPosition(thumbPos);     // I can't fucking believe I have to do this.
    if (who == Parent->RatioSlider)
    {
//      ratioXY = pow(2.0, (double) (SLIDERRANGE - thumbPos) * 2.0 / (double) SLIDERRANGE);
        ratioXY = tan((double) (SLIDERRANGE - thumbPos) * 1.55 / (double) SLIDERRANGE) + 1.0;
        sprintf(buf, "%.4f", ratioXY);
        Parent->RatioVal->SetText(buf);
    }
    else
    {
        drag = (double) (SLIDERRANGE - thumbPos) * 0.5 / (double) SLIDERRANGE;
        sprintf(buf, "%6.5f", drag);
        Parent->DragVal->SetText(buf);
    }
}

void TSandCanvas::EvRatioChange()
{
    if (GetFocus() != Parent->RatioVal->HWindow)
        return;
    char buf[32];
    char* end;
    Parent->RatioVal->GetText(buf, sizeof(buf));
    double v = strtod(buf, &end);
    if ((*end && !isspace(*end)) || v < 1.0 || v > 50.0)
    {
        MessageBeep(-1);
        return;
    }
    ratioXY = v;
//  Parent->RatioSlider->SetPosition(SLIDERRANGE - int(log2(ratioXY) * (double) SLIDERRANGE / 2.0));
    Parent->RatioSlider->SetPosition(SLIDERRANGE - int((double) SLIDERRANGE * atan(ratioXY - 1.0) / 1.55));
}

void TSandCanvas::EvDragChange()
{
    if (GetFocus() != Parent->DragVal->HWindow)
        return;
    char buf[32];
    char* end;
    Parent->DragVal->GetText(buf, sizeof(buf));
    double v = strtod(buf, &end);
    if ((*end && !isspace(*end)) || v < 0.0 || v > 0.5)
    {
        MessageBeep(-1);
        return;
    }
    drag = v;
    Parent->DragSlider->SetPosition(SLIDERRANGE - int(drag * (double) SLIDERRANGE / 0.5));
}

void TSandCanvas::EvSize(uint sizeType, TSize& size)
{
    TWindow::EvSize(sizeType, size);
    TRect rc = GetClientRect();
    pCenter = TPoint(rc.left + rc.Width() / 2, rc.top + rc.Height() / 2);
    radius = min(rc.Width() / 2, rc.Height() / 2);
    Invalidate();
}

void TSandCanvas::EvLButtonDown(uint modKeys, TPoint& point)
{
    TWindow::EvLButtonDown(modKeys, point);
    if (!GetClientRect().Contains(point))
        return;
    historyX[0] = double(point.x - pCenter.x) / radius;
    historyY[0] = double(point.y - pCenter.y) / radius;
    historySize = 1;
    Parent->SetCaption(APPNAME);
    Invalidate();
    death = late = busy = false;
    SetTimer(TICK, POINTSPERTIMER * 1000 / POINTSPERSECOND);
}

void TSandCanvas::EvPrint()
{
    TPrinter printer;
/*  TSandPrintout spo(this, &printer);
    try {
        printer.Print(this, spo, true);
    } catch (xmsg& e) {
//      printer.ReportError(this, spo);
        int r = ::GetLastError();
        char foo[256];
        wsprintf(foo, "Printer error number is %d; OS error %d; exception message:\r\n%s",
                    printer.GetError(), r, e.why().c_str());
        MessageBox(foo, "Dammit", MB_OK);
    }     */
    HCURSOR hOrigCursor = ::SetCursor(::LoadCursor(0, IDC_WAIT));
    printer.Setup(this);
    TPrintDC pdc(printer.GetData()->GetDriverName(), printer.GetData()->GetDeviceName(),
                     printer.GetData()->GetOutputName(), printer.GetData()->GetDevMode());
    // Gaah, drawing directly to the printer DC is ultra-slow for some reason.
    try
    {
//      TSize pagesize = printer.GetPageSize();
        TSize pagesize(pdc.GetDeviceCaps(HORZRES), pdc.GetDeviceCaps(VERTRES));
#ifdef IMAGE_BUFFER
        TBitmap imagebuffer(pagesize.cx, pagesize.cy, 1, 1, NULL);
        TMemoryDC mdc(imagebuffer);
#endif
        pdc.SetMapMode(MM_TEXT);
        pPrinterCenter = TPoint(pagesize.cx / 2, pagesize.cy / 2);
        printerRadius = min(pagesize.cx / 2, pagesize.cy / 2) * 5 / 6;
#ifdef IMAGE_BUFFER
        mdc.SelectObject(TPen(TColor::Black, 3, PS_SOLID));
//      mdc.MoveTo(PointToPrinterPixel(historyX[0], historyY[0]));
        for (int i = 1; i < historySize; i++)
//          mdc.LineTo(PointToPrinterPixel(historyX[i], historyY[i]));
            PaintIncrement(mdc, i, true);
::MessageBeep(-1);
#endif
        pdc.StartDoc(APPNAME, printer.GetData()->GetOutputName());
        pdc.StartPage();
#ifdef IMAGE_BUFFER
        if (!pdc.BitBlt(TRect(TPoint(0, 0), pagesize), mdc, TPoint(0, 0), SRCCOPY))
        {
            char foo[128];
            wsprintf(foo, "Fuck god damn.  Error %d.", ::GetLastError());
            MessageBox(foo);
        }
#else
        pdc.SelectObject(TPen(TColor::Black, /*3*/ 1, PS_SOLID));
//      pdc.MoveTo(PointToPrinterPixel(historyX[0], historyY[0]));
        for (int i = 1; i < historySize; i++)
//          pdc.LineTo(PointToPrinterPixel(historyX[i], historyY[i]));
            PaintIncrement(pdc, i, true);
#endif
        pdc.EndPage();
        pdc.EndDoc();
    }
    catch (xmsg& e)
    {
        int r = ::GetLastError();
        char foo[256];
        wsprintf(foo, "Printer error number is %d; OS error %d; exception message:\r\n%s",
                    printer.GetError(), r, e.why().c_str());
        MessageBox(foo, "Dammit", MB_OK);
    }
    ::SetCursor(hOrigCursor);
}

void TSandCanvas::Paint(TDC& dc, bool, TRect&)
{
    MSG msg;
    dc.FillRect(GetClientRect(), BACKGROUND);
#ifdef RINGCOLOR
    dc.SelectObject(TPen(RINGCOLOR));
    dc.SelectStockObject(NULL_BRUSH);
    dc.Ellipse(pCenter.x - radius, pCenter.y - radius, pCenter.x + radius, pCenter.y + radius);
#endif
    for (int i = 1; i < historySize; i++)
    {
        PaintIncrement(dc, i, false);
        if (i % POINTSPERTIMER == 0 && ::PeekMessage(&msg, HWindow, WM_SIZE, WM_SIZE, PM_NOREMOVE))
            break;      // XXX  Gaah, this peek doesn't work.  Do I have to use a system-level hook?
    }
}

void TSandCanvas::Increment(int count)
{
    do {
        late = false;
        if (historySize >= MAXPOINTS)
        {
            KillTimer(TICK);
            Parent->SetCaption(APPNAME "   (done)");
        }
        else
        {
            busy = true;
            MovePendulum();     // increments historySize
            PaintIncrement(*updc, historySize - 1, false);
            if ((historySize % 1000) == 0)
            {
                char buf[256];
                sprintf(buf, APPNAME "    step %d", historySize);
                Parent->SetCaption(buf);
            }
            GetApplication()->PumpWaitingMessages();    // in case timer has already ticked
            busy = false;
        }
    } while ((late || --count >= 0) && !death);
}

void TSandCanvas::PaintIncrement(TDC& dc, int index, bool paper)    // our exotic painting effect
{
    if (index <= 0 || index > MAXPOINTS)
        return;
    TPoint start, stop;
    if (paper)
    {
        start = PointToPrinterPixel(historyX[index - 1], historyY[index - 1]);
        stop = PointToPrinterPixel(historyX[index], historyY[index]);
        dc.SelectStockObject(WHITE_PEN);
    }
    else
    {
        start = PointToPixel(historyX[index - 1], historyY[index - 1]);
        stop = PointToPixel(historyX[index], historyY[index]);
        dc.SelectObject(TPen(BACKGROUND));
    }
    if (fabs(historyX[index] - historyX[index - 1]) > fabs(historyY[index] - historyY[index - 1]))
    {       // roughly horizontal line
        if (historyY[index] >= 0) {
            dc.MoveTo(start.x, start.y - 1);
            dc.LineTo(stop.x, stop.y - 1);
        } else {
            dc.MoveTo(start.x, start.y + 1);
            dc.LineTo(stop.x, stop.y + 1);
        }
    }
    else    // roughly vertical line
    {
        if (historyX[index] >= 0) {
            dc.MoveTo(start.x - 1, start.y);
            dc.LineTo(stop.x - 1, stop.y);
        } else {
            dc.MoveTo(start.x + 1, start.y);
            dc.LineTo(stop.x + 1, stop.y);
        }
    }
    dc.SelectStockObject(BLACK_PEN);
    dc.MoveTo(start);
    dc.LineTo(stop);
}

void TSandCanvas::MovePendulum()        // the mathematical rule of movement
{
    // the basic rule is that, along each axis, motion is accelerated by an
    // amount proportional to distance from center, of a magnitude correct to
    // produce that axis's desired period, and then drag is subtracted.
    double xf = periodX / (4.0 * M_PI * M_PI);
    double yf = periodX * ratioXY / (4.0 * M_PI * M_PI);
    double cx = historyX[historySize - 1];
    double cy = historyY[historySize - 1];
    double ox = historySize > 1 ? historyX[historySize - 2] : cx;
    double oy = historySize > 1 ? historyY[historySize - 2] : cy;
    double ax = (cx - ox) - cx / (xf * xf);
    double ay = (cy - oy) - cy / (yf * yf);
    historyX[historySize] = cx + ax * (1.0 - drag / periodX);
    historyY[historySize] = cy + ay * (1.0 - drag / periodX);
    historySize++;
}


/*
void TSandPrintout::SetPrintParams(TPrintDC* dc, TSize pageSize)
{
    TPrintout::SetPrintParams(dc, pageSize);
    Source->pPrinterCenter = TPoint(pageSize.cx / 2, pageSize.cy / 2);
    Source->printerRadius = min(pageSize.cx / 2, pageSize.cy / 2);
}

void TSandPrintout::PrintPage(int, TRect&, unsigned)
{
    TPrintDC *pdc = GetPrintDC();
    for (int i = 1; i < Source->historySize; i++)
    {
        Source->PaintIncrement(*pdc, i, true);
        if (Boss->GetUserAbort())
            break;
    }
}
*/

