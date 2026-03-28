#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "CouplingAssembly1.h"
#endif

#include "CouplingAssembly1Doc.h"
#include "AsmPreviewView.h"
#include "GostTables.h"

#include <cmath>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
constexpr double kPi = 3.14159265358979323846;

void DrawSpiderStar(CDC* pDC, int cx, int cy, int Ro, int Ri, int nRays)
{
	if (nRays < 3 || Ro <= Ri + 2)
		return;

	std::vector<POINT> pts;
	pts.resize(static_cast<size_t>(nRays * 2));
	for (int i = 0; i < nRays * 2; ++i)
	{
		const double ang = -kPi / 2 + (kPi * i) / nRays;
		const int R = (i % 2 == 0) ? Ro : Ri;
		pts[static_cast<size_t>(i)].x = cx + static_cast<int>(R * std::cos(ang));
		pts[static_cast<size_t>(i)].y = cy + static_cast<int>(R * std::sin(ang));
	}
	pDC->Polygon(pts.data(), nRays * 2);
}

void DrawSpiderPreview(CDC* pDC, const CRect& area, const SpiderParams& s)
{
	const int cx = area.CenterPoint().x;
	const int cy = area.CenterPoint().y;
	const int span = (std::min)(area.Width(), area.Height()) / 2 - 8;
	if (span < 20)
		return;

	int Ro = span;
	if (s.outerDiameter > 1.0)
	{
		const double ri = s.innerDiameter / s.outerDiameter;
		const int Ri = (std::max)(static_cast<int>(Ro * ri), Ro / 4);
		CPen pen(PS_SOLID, 2, RGB(40, 40, 120));
		CBrush br(RGB(220, 225, 245));
		CPen* op = pDC->SelectObject(&pen);
		CBrush* ob = pDC->SelectObject(&br);
		DrawSpiderStar(pDC, cx, cy, Ro, Ri, s.rays);
		pDC->SelectObject(ob);
		pDC->SelectObject(op);
	}
}

void DrawHalfSide(CDC* pDC, const CRect& area, const HalfCouplingParams& h)
{
	const int w = area.Width();
	const int hgt = area.Height();
	const int hubW = (std::max)(w / 5, 18);
	const int jawW = w - hubW - 4;
	CPen pen(PS_SOLID, 2, RGB(60, 60, 60));
	CPen* op = pDC->SelectObject(&pen);
	pDC->Rectangle(area.left, area.top + hgt / 4, area.left + hubW, area.bottom - hgt / 4);
	pDC->Rectangle(area.left + hubW + 2, area.top + hgt / 6, area.right - 2, area.bottom - hgt / 6);
	pDC->MoveTo(area.left + hubW, area.top + hgt / 4);
	pDC->LineTo(area.left + hubW + 2, area.top + hgt / 6);
	pDC->MoveTo(area.left + hubW, area.bottom - hgt / 4);
	pDC->LineTo(area.left + hubW + 2, area.bottom - hgt / 6);
	pDC->SelectObject(op);
}

void DrawAssemblyPreview(CDC* pDC, const CRect& area, const HalfCouplingParams& h1, const HalfCouplingParams& h2, const SpiderParams& s)
{
	const int cx = area.CenterPoint().x;
	const int cy = area.CenterPoint().y;
	const int halfW = (std::min)(area.Width() / 3 - 4, 70);
	const int H = (std::min)(area.Height() - 20, 100);

	CRect left(cx - halfW - halfW / 2 - 6, cy - H / 2, cx - halfW / 2 - 6, cy + H / 2);
	CRect right(cx + halfW / 2 + 6, cy - H / 2, cx + halfW + halfW / 2 + 6, cy + H / 2);
	DrawHalfSide(pDC, left, h1);
	DrawHalfSide(pDC, right, h2);

	CRect mid(cx - halfW / 2 - 2, cy - H / 3, cx + halfW / 2 + 2, cy + H / 3);
	CPen pen(PS_SOLID, 1, RGB(100, 100, 160));
	CPen* op = pDC->SelectObject(&pen);
	CBrush br(RGB(230, 230, 250));
	CBrush* ob = pDC->SelectObject(&br);
	pDC->Rectangle(&mid);
	pDC->SelectObject(ob);
	pDC->SelectObject(op);

	const int Ro = (std::min)(mid.Width(), mid.Height()) / 2 - 2;
	const int Ri = (s.outerDiameter > 1.0) ? (std::max)(Ro / 4, static_cast<int>(Ro * (s.innerDiameter / s.outerDiameter))) : Ro / 3;
	CPen pen2(PS_SOLID, 2, RGB(40, 40, 120));
	CPen* op2 = pDC->SelectObject(&pen2);
	DrawSpiderStar(pDC, mid.CenterPoint().x, mid.CenterPoint().y, Ro, Ri, s.rays);
	pDC->SelectObject(op2);
}

}

IMPLEMENT_DYNCREATE(CAsmPreviewView, CView)

BEGIN_MESSAGE_MAP(CAsmPreviewView, CView)
END_MESSAGE_MAP()

CAsmPreviewView::CAsmPreviewView() noexcept
{
}

CAsmPreviewView::~CAsmPreviewView()
{
}

BOOL CAsmPreviewView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void CAsmPreviewView::OnDraw(CDC* pDC)
{
	CCouplingAssembly1Doc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	pDC->SetBkMode(TRANSPARENT);

	CRect client;
	GetClientRect(&client);

	const int splitX = (std::min)(280, client.Width() / 2 + 40);
	CRect drawArea(client.left + 8, client.top + 8, client.left + splitX - 8, client.bottom - 8);
	CRect textArea(client.left + splitX + 4, client.top + 12, client.right - 8, client.bottom - 8);

	CPen framePen(PS_DOT, 1, RGB(200, 200, 200));
	CPen* oldPen = pDC->SelectObject(&framePen);
	pDC->Rectangle(drawArea);
	pDC->SelectObject(oldPen);

	CString line1, line2, line3, line4, line5, line6, line7, line8;
	line7.Empty();
	line8.Empty();

	switch (pDoc->GetSelectedNode())
	{
	case NodeAssembly:
	{
		const AssemblyParams& params = pDoc->GetAssemblyParams();
		const double tRow = GostTables::SnapTorqueToSeries(params.torque);

		line1 = L"\u0421\u0431\u043e\u0440\u043a\u0430 (\u0413\u041e\u0421\u0422 14084-76, \u0442\u0430\u0431\u043b. 21.3.1)";
		line2.Format(L"\u041c\u043e\u043c\u0435\u043d\u0442 (\u0437\u0430\u0434\u0430\u043d\u043d\u044b\u0439): %.2f \u041d\u00b7\u043c", params.torque);
		line3.Format(L"\u0420\u044f\u0434 \u0434\u043b\u044f \u0442\u0430\u0431\u043b\u0438\u0446\u044b: %.1f \u041d\u00b7\u043c", tRow);
		line4.Format(L"\u0418\u0441\u043f\u043e\u043b\u043d\u0435\u043d\u0438\u0435: %d  (1 \u2014 4 \u043b\u0443\u0447\u0430; 2 \u2014 6 \u043b\u0443\u0447\u0435\u0439)", params.execution);
		line5.Format(L"\u0412\u0430\u043b 1 / 2: %.2f / %.2f \u043c\u043c", params.shaftDiameter1, params.shaftDiameter2);
		line6.Format(
			L"L=%.2f \u043c\u043c, D\u2081=%.2f \u043c\u043c, n_max=%.0f \u043e\u0431/\u043c\u0438\u043d",
			params.assemblyLengthL,
			params.envelopeD1,
			params.maxSpeedRpm);
		line7.Format(L"m=%.3f \u043a\u0433, b\u2081=%.2f \u043c\u043c", params.massKg, params.widthB1);

		DrawAssemblyPreview(
			pDC,
			drawArea,
			pDoc->GetHalfCoupling1Params(),
			pDoc->GetHalfCoupling2Params(),
			pDoc->GetSpiderParams());
		break;
	}

	case NodeHalfCoupling1:
	{
		const HalfCouplingParams& h = pDoc->GetHalfCoupling1Params();
		line1 = L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 1";
		line2.Format(L"d (\u043e\u0442\u0432\u0435\u0440\u0441\u0442\u0438\u0435): %.2f \u043c\u043c", h.boreDiameter);
		line3.Format(L"D (\u043d\u0430\u0440\u0443\u0436\u043d\u044b\u0439): %.2f \u043c\u043c", h.outerDiameter);
		line4.Format(L"L\u2081=%.2f \u043c\u043c, l=%.2f \u043c\u043c (\u0434\u043b\u0438\u043d\u044b \u043f\u043e \u0442\u0430\u0431\u043b.)", h.lengthTotalL1, h.lengthHubL);
		line5.Format(L"d\u2081=%.2f \u043c\u043c, b=%.2f, d+t\u2081=%.2f \u043c\u043c", h.hubOuterD1, h.keywayWidthB, h.keywayDt1);
		line6.Format(L"B=%.2f, B\u2081=%.2f \u043c\u043c, \u0433\u0443\u0431\u043a\u0438: %d", h.faceSlotB, h.faceSlotB1, h.lugCount);
		line7.Format(L"l\u2082=%.2f, l\u2083=%.2f \u043c\u043c, r=%.2f \u043c\u043c", h.lengthL2, h.lengthL3, h.filletR);
		DrawHalfSide(pDC, drawArea, h);
		break;
	}

	case NodeHalfCoupling2:
	{
		const HalfCouplingParams& h = pDoc->GetHalfCoupling2Params();
		line1 = L"\u041f\u043e\u043b\u0443\u043c\u0443\u0444\u0442\u0430 2";
		line2.Format(L"d (\u043e\u0442\u0432\u0435\u0440\u0441\u0442\u0438\u0435): %.2f \u043c\u043c", h.boreDiameter);
		line3.Format(L"D (\u043d\u0430\u0440\u0443\u0436\u043d\u044b\u0439): %.2f \u043c\u043c", h.outerDiameter);
		line4.Format(L"L\u2081=%.2f \u043c\u043c, l=%.2f \u043c\u043c (\u0434\u043b\u0438\u043d\u044b \u043f\u043e \u0442\u0430\u0431\u043b.)", h.lengthTotalL1, h.lengthHubL);
		line5.Format(L"d\u2081=%.2f \u043c\u043c, b=%.2f, d+t\u2081=%.2f \u043c\u043c", h.hubOuterD1, h.keywayWidthB, h.keywayDt1);
		line6.Format(L"B=%.2f, B\u2081=%.2f \u043c\u043c, \u0433\u0443\u0431\u043a\u0438: %d", h.faceSlotB, h.faceSlotB1, h.lugCount);
		line7.Format(L"l\u2082=%.2f, l\u2083=%.2f \u043c\u043c, r=%.2f \u043c\u043c", h.lengthL2, h.lengthL3, h.filletR);
		DrawHalfSide(pDC, drawArea, h);
		break;
	}

	case NodeSpider:
	{
		const SpiderParams& s = pDoc->GetSpiderParams();
		line1 = L"\u0417\u0432\u0435\u0437\u0434\u043e\u0447\u043a\u0430";
		line2.Format(L"D (\u043d\u0430\u0440\u0443\u0436\u043d.): %.2f \u043c\u043c", s.outerDiameter);
		line3.Format(L"d (\u0432\u043d\u0443\u0442\u0440.): %.2f \u043c\u043c", s.innerDiameter);
		line4.Format(L"H (\u0442\u043e\u043b\u0449\u0438\u043d\u0430): %.2f \u043c\u043c", s.thickness);
		line5.Format(L"B (\u0448\u0438\u0440\u0438\u043d\u0430 \u043b\u0443\u0447\u0430): %.2f \u043c\u043c", s.legWidth);
		line6.Format(L"\u0427\u0438\u0441\u043b\u043e \u043b\u0443\u0447\u0435\u0439: %d, r: %.2f \u043c\u043c", s.rays, s.filletRadius);
		line7.Format(L"m=%.3f \u043a\u0433 (\u043f\u043e \u0442\u0430\u0431\u043b. \u0437\u0432\u0435\u0437\u0434\u044b)", s.massKg);
		DrawSpiderPreview(pDC, drawArea, s);
		break;
	}

	default:
		line1 = L"\u041d\u0435\u0438\u0437\u0432\u0435\u0441\u0442\u043d\u044b\u0439 \u044d\u043b\u0435\u043c\u0435\u043d\u0442";
		line2 = L"";
		line3 = L"";
		line4 = L"";
		line5 = L"";
		line6 = L"";
		line7.Empty();
		line8.Empty();
		break;
	}

	int ty = textArea.top;
	const int step = 20;
	pDC->TextOutW(textArea.left, ty, line1);
	ty += step;
	pDC->TextOutW(textArea.left, ty, line2);
	ty += step;
	pDC->TextOutW(textArea.left, ty, line3);
	ty += step;
	pDC->TextOutW(textArea.left, ty, line4);
	ty += step;
	pDC->TextOutW(textArea.left, ty, line5);
	ty += step;
	pDC->TextOutW(textArea.left, ty, line6);
	ty += step;
	if (!line7.IsEmpty())
	{
		pDC->TextOutW(textArea.left, ty, line7);
		ty += step;
	}
	if (!line8.IsEmpty())
		pDC->TextOutW(textArea.left, ty, line8);

	pDC->TextOutW(
		drawArea.left,
		drawArea.bottom - 18,
		L"\u0421\u0445\u0435\u043c\u0430 \u0432 \u043f\u0440\u0438\u043b\u043e\u0436\u0435\u043d\u0438\u0438; 3D \u0432 \u041a\u041e\u041c\u041f\u0410\u0421\u0435 "
		L"\u2014 \u043a\u043e\u043c\u0430\u043d\u0434\u0430 \u00ab\u041f\u043e\u0441\u0442\u0440\u043e\u0438\u0442\u044c\u00bb (\u043d\u0443\u0436\u0435\u043d SDK, COUPLING_USE_KOMPAS_SDK=1).");
}

#ifdef _DEBUG
void CAsmPreviewView::AssertValid() const
{
	CView::AssertValid();
}

void CAsmPreviewView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CCouplingAssembly1Doc* CAsmPreviewView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCouplingAssembly1Doc)));
	return (CCouplingAssembly1Doc*)m_pDocument;
}
#endif
