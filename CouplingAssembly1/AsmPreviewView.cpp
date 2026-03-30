#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "CouplingAssembly1.h"
#endif

#include "CouplingAssembly1Doc.h"
#include "AsmPreviewView.h"
#include "GostTables.h"
#include "SpiderProfile2D.h"

#include <cmath>
#include <functional>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
constexpr double kPi = 3.14159265358979323846;

static bool SpiderTryOuterCornerFillet(
	double px,
	double py,
	double sx,
	double sy,
	double ex,
	double ey,
	double r,
	double* oT1x,
	double* oT1y,
	double* oT2x,
	double* oT2y,
	double* oMx,
	double* oMy)
{
	if (r < 0.025)
		return false;
	const double d1x = sx - px;
	const double d1y = sy - py;
	const double d2x = ex - sx;
	const double d2y = ey - sy;
	const double l1 = std::hypot(d1x, d1y);
	const double l2 = std::hypot(d2x, d2y);
	if (l1 < 1e-4 || l2 < 1e-4)
		return false;
	const double u1x = d1x / l1;
	const double u1y = d1y / l1;
	const double u2x = d2x / l2;
	const double u2y = d2y / l2;
	const double cphi = u1x * u2x + u1y * u2y;
	const double phi = std::acos((std::max)(-1.0, (std::min)(1.0, cphi)));
	if (phi < 1e-3 || phi > kPi - 1e-3)
		return false;
	double t = r / std::tan(phi * 0.5);
	t = (std::min)(t, l1 * 0.42);
	t = (std::min)(t, l2 * 0.42);
	*oT1x = sx - u1x * t;
	*oT1y = sy - u1y * t;
	*oT2x = sx + u2x * t;
	*oT2y = sy + u2y * t;
	double bx = -u1x + u2x;
	double by = -u1y + u2y;
	const double bl = std::hypot(bx, by);
	if (bl < 1e-8)
		return false;
	bx /= bl;
	by /= bl;
	if (bx * sx + by * sy > 0.0)
	{
		bx = -bx;
		by = -by;
	}
	const double dist = r / std::sin(phi * 0.5);
	*oMx = sx + bx * dist;
	*oMy = sy + by * dist;
	return true;
}

static void AppendCircleArcInteriorSamples(
	const std::function<void(double, double)>& vtx,
	double ax,
	double ay,
	double mx,
	double my,
	double bx,
	double by,
	int nInterior)
{
	if (nInterior < 1)
		return;
	const double x1 = ax;
	const double y1 = ay;
	const double x2 = mx;
	const double y2 = my;
	const double x3 = bx;
	const double y3 = by;
	const double d =
		2.0 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
	if (std::fabs(d) < 1e-12)
		return;
	const double ux =
		((x1 * x1 + y1 * y1) * (y2 - y3) + (x2 * x2 + y2 * y2) * (y3 - y1) + (x3 * x3 + y3 * y3) * (y1 - y2)) / d;
	const double uy =
		((x1 * x1 + y1 * y1) * (x3 - x2) + (x2 * x2 + y2 * y2) * (x1 - x3) + (x3 * x3 + y3 * y3) * (x2 - x1)) / d;
	const double R = std::hypot(x1 - ux, y1 - uy);
	if (R < 1e-8)
		return;
	const double tA = std::atan2(y1 - uy, x1 - ux);
	const double tM = std::atan2(y2 - uy, x2 - ux);
	const double tB = std::atan2(y3 - uy, x3 - ux);
	auto normD = [](double a, double b) { return std::atan2(std::sin(b - a), std::cos(b - a)); };
	double dAB = normD(tA, tB);
	double dAM = normD(tA, tM);
	const bool mOnShort =
		(dAB > 0.0 && dAM > 0.0 && dAM < dAB - 1e-6) || (dAB < 0.0 && dAM < 0.0 && dAM > dAB + 1e-6);
	if (!mOnShort)
		dAB = (dAB > 0.0) ? dAB - 2.0 * kPi : dAB + 2.0 * kPi;
	const double tEnd = tA + dAB;
	for (int i = 1; i <= nInterior; ++i)
	{
		const double u = static_cast<double>(i) / static_cast<double>(nInterior + 1);
		const double tt = tA + u * (tEnd - tA);
		vtx(ux + R * std::cos(tt), uy + R * std::sin(tt));
	}
}

void BuildSpiderProfilePolygonPoints(
	std::vector<POINT>& pts,
	int cx,
	int cy,
	int n,
	double Ro,
	double Ri,
	double filletR,
	double legWidth,
	int arcSegmentsPerCap)
{
	pts.clear();
	if (n < 3 || Ro <= Ri + 1.0 || arcSegmentsPerCap < 2)
		return;

	if (n == 4 || n == 6)
	{
		std::vector<std::pair<double, double>> contour;
		SpiderProfile2D::AppendClosedContourMm(contour, n, Ro, Ri, legWidth, arcSegmentsPerCap);
		if (contour.size() < 3)
			return;
		pts.reserve(contour.size());
		for (const auto& pr : contour)
		{
			pts.push_back(
				{ cx + static_cast<LONG>(std::lround(pr.first)), cy + static_cast<LONG>(std::lround(pr.second)) });
		}
		return;
	}

	const double riDraw =
		(filletR > 0.02) ? (std::max)(Ro * 0.22, Ri - (std::min)(filletR * 1.55, Ri * 0.28)) : Ri;
	double capHalfDeg = (180.0 / kPi) * std::asin((std::min)(0.995, legWidth / (2.0 * Ro)));
	capHalfDeg = (std::max)(6.0, (std::min)(capHalfDeg, 180.0 / n - 1.0));
	const bool fourRayChord90 = (n == 4);
	if (fourRayChord90)
		capHalfDeg = 45.0;
	const double rCornerMin = (std::min)(0.10, (std::max)(0.025, Ro * 0.0045));
	const double rCorner = (std::max)(rCornerMin, (std::min)(filletR, Ro * 0.15));
	const int filletSeg = (std::max)(2, arcSegmentsPerCap / 3);

	const double toRad = kPi / 180.0;

	auto add = [&](double x, double y) {
		pts.push_back(
			{ cx + static_cast<LONG>(std::lround(x)), cy + static_cast<LONG>(std::lround(y)) });
	};

	for (int k = 0; k < n; ++k)
	{
		const double midDeg = -90.0 + k * (360.0 / n);
		const double a1 = midDeg - capHalfDeg;
		const double a2 = midDeg + capHalfDeg;
		const double inDeg = -90.0 + (k + 0.5) * (360.0 / n);
		const double prevInDeg =
			(k == 0) ? (-90.0 + (n - 0.5) * (360.0 / n)) : (-90.0 + (k - 0.5) * (360.0 / n));

		const double xP = riDraw * std::cos(prevInDeg * toRad);
		const double yP = riDraw * std::sin(prevInDeg * toRad);
		const double xS = Ro * std::cos(a1 * toRad);
		const double yS = Ro * std::sin(a1 * toRad);
		const double xE = Ro * std::cos(a2 * toRad);
		const double yE = Ro * std::sin(a2 * toRad);
		const double xI = riDraw * std::cos(inDeg * toRad);
		const double yI = riDraw * std::sin(inDeg * toRad);

		if (fourRayChord90)
		{
			double t1sx, t1sy, t2sx, t2sy, msx, msy;
			double t3ex, t3ey, t4ex, t4ey, mex, mey;
			const bool okS = SpiderTryOuterCornerFillet(
				xP,
				yP,
				xS,
				yS,
				xE,
				yE,
				rCorner,
				&t1sx,
				&t1sy,
				&t2sx,
				&t2sy,
				&msx,
				&msy);
			const bool okE = SpiderTryOuterCornerFillet(
				xS,
				yS,
				xE,
				yE,
				xI,
				yI,
				rCorner,
				&t3ex,
				&t3ey,
				&t4ex,
				&t4ey,
				&mex,
				&mey);
			if (k == 0)
				add(xP, yP);
			if (okS)
			{
				add(t1sx, t1sy);
				AppendCircleArcInteriorSamples(add, t1sx, t1sy, msx, msy, t2sx, t2sy, filletSeg);
				add(t2sx, t2sy);
			}
			else
				add(xS, yS);
			if (okE)
			{
				add(t3ex, t3ey);
				AppendCircleArcInteriorSamples(add, t3ex, t3ey, mex, mey, t4ex, t4ey, filletSeg);
				add(t4ex, t4ey);
			}
			else
				add(xE, yE);
			add(xI, yI);
		}
		else
		{
			if (k == 0)
				add(xP, yP);
			add(xS, yS);
			for (int i = 1; i < arcSegmentsPerCap; ++i)
			{
				const double ang = a1 + (a2 - a1) * (static_cast<double>(i) / arcSegmentsPerCap);
				add(Ro * std::cos(ang * toRad), Ro * std::sin(ang * toRad));
			}
			add(xE, yE);
			add(xI, yI);
		}
	}
}

void DrawSpiderStar(CDC* pDC, int cx, int cy, int Ro, int Ri, int nRays, double filletR, double legWidthMm, double outerDiameterMm)
{
	if (nRays < 3 || Ro <= Ri + 2 || outerDiameterMm <= 1.0)
		return;

	const double legW = Ro * (legWidthMm / outerDiameterMm);
	const double filletPx = Ro * (filletR / outerDiameterMm);

	std::vector<POINT> pts;
	BuildSpiderProfilePolygonPoints(pts, cx, cy, nRays, static_cast<double>(Ro), static_cast<double>(Ri), filletPx, legW, 10);
	if (pts.size() < 3)
		return;

	std::vector<POINT> closed = pts;
	closed.push_back(pts[0]);
	pDC->Polyline(closed.data(), static_cast<int>(closed.size()));
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
		CPen* op = pDC->SelectObject(&pen);
		DrawSpiderStar(pDC, cx, cy, Ro, Ri, s.rays, s.filletRadius, s.legWidth, s.outerDiameter);
		pDC->SelectObject(op);
	}
}

void DrawHalfSide(CDC* pDC, const CRect& area, const HalfCouplingParams& h)
{
	const int hgt = area.Height();
	(void)h;
	CPen pen(PS_SOLID, 2, RGB(60, 60, 60));
	CPen* op = pDC->SelectObject(&pen);
	pDC->Rectangle(area.left + 2, area.top + hgt / 5, area.right - 2, area.bottom - hgt / 5);
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

	const int Ro = (std::min)(mid.Width(), mid.Height()) / 2 - 2;
	const int Ri = (s.outerDiameter > 1.0) ? (std::max)(Ro / 4, static_cast<int>(Ro * (s.innerDiameter / s.outerDiameter))) : Ro / 3;
	CPen pen2(PS_SOLID, 2, RGB(40, 40, 120));
	CPen* op2 = pDC->SelectObject(&pen2);
	DrawSpiderStar(
		pDC,
		mid.CenterPoint().x,
		mid.CenterPoint().y,
		Ro,
		Ri,
		s.rays,
		s.filletRadius,
		s.legWidth,
		s.outerDiameter);
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

		line1 = L"Сборка (ГОСТ 14084-76, табл. 21.3.1)";
		line2.Format(L"Момент (заданный): %.2f Н·м", params.torque);
		line3.Format(L"Ряд для таблицы: %.1f Н·м", tRow);
		line4.Format(
			L"Исполнение ГОСТ: %d — %s. Вариант задания: %d",
			params.execution,
			(params.execution == 1) ? L"2 губки на полумуфте, звезда 4 луча" : L"3 губки, звезда 6 лучей",
			params.courseVariant);
		line5.Format(L"Валы 1 и 2 (две полумуфты, одно исполнение ГОСТ): %.2f / %.2f мм", params.shaftDiameter1, params.shaftDiameter2);
		line6.Format(
			L"L=%.2f мм, D₁=%.2f мм, n_max=%.0f об/мин",
			params.assemblyLengthL,
			params.envelopeD1,
			params.maxSpeedRpm);
		line7.Format(L"m=%.3f кг, b₁=%.2f мм", params.massKg, params.widthB1);

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
		line1.Format(
			L"Полумуфта 1 (вал 1, d вала в сборке: %.1f мм)",
			pDoc->GetAssemblyParams().shaftDiameter1);
		line2.Format(L"d (отверстие): %.2f мм", h.boreDiameter);
		line3.Format(L"D (наружный): %.2f мм", h.outerDiameter);
		line4.Format(L"L₁=%.2f мм, l=%.2f мм (длины по табл.)", h.lengthTotalL1, h.lengthHubL);
		line5.Format(L"d₁=%.2f мм, b=%.2f, d+t₁=%.2f мм", h.hubOuterD1, h.keywayWidthB, h.keywayDt1);
		line6.Format(L"B=%.2f, B₁=%.2f мм, губки: %d", h.faceSlotB, h.faceSlotB1, h.lugCount);
		line7.Format(L"l₂=%.2f, l₃=%.2f мм, r=%.2f мм", h.lengthL2, h.lengthL3, h.filletR);
		DrawHalfSide(pDC, drawArea, h);
		break;
	}

	case NodeHalfCoupling2:
	{
		const HalfCouplingParams& h = pDoc->GetHalfCoupling2Params();
		line1.Format(
			L"Полумуфта 2 (вал 2, d вала в сборке: %.1f мм)",
			pDoc->GetAssemblyParams().shaftDiameter2);
		line2.Format(L"d (отверстие): %.2f мм", h.boreDiameter);
		line3.Format(L"D (наружный): %.2f мм", h.outerDiameter);
		line4.Format(L"L₁=%.2f мм, l=%.2f мм (длины по табл.)", h.lengthTotalL1, h.lengthHubL);
		line5.Format(L"d₁=%.2f мм, b=%.2f, d+t₁=%.2f мм", h.hubOuterD1, h.keywayWidthB, h.keywayDt1);
		line6.Format(L"B=%.2f, B₁=%.2f мм, губки: %d", h.faceSlotB, h.faceSlotB1, h.lugCount);
		line7.Format(L"l₂=%.2f, l₃=%.2f мм, r=%.2f мм", h.lengthL2, h.lengthL3, h.filletR);
		DrawHalfSide(pDC, drawArea, h);
		break;
	}

	case NodeSpider:
	{
		const SpiderParams& s = pDoc->GetSpiderParams();
		const AssemblyParams& ap = pDoc->GetAssemblyParams();
		line1 = L"Звёздочка";
		line2.Format(L"D (наружн.): %.2f мм", s.outerDiameter);
		line3.Format(L"d (внутр.): %.2f мм", s.innerDiameter);
		line4.Format(L"H (толщина): %.2f мм", s.thickness);
		line5.Format(L"B (ширина луча): %.2f мм", s.legWidth);
		line6.Format(L"Число лучей: %d, r луча и впадин: %.2f мм", s.rays, s.filletRadius);
		line7.Format(L"m=%.3f кг (по табл. звезды)", s.massKg);
		line8.Format(
			L"По сборке: исп. %d → %s (кнопка «Из ГОСТ» задаёт лучи и размеры).",
			ap.execution,
			(ap.execution == 1) ? L"4 луча" : L"6 лучей");
		DrawSpiderPreview(pDC, drawArea, s);
		break;
	}

	default:
		line1 = L"Неизвестный элемент";
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
		L"Схема в приложении; 3D в КОМПАСе — команда «Построить» (нужен SDK, COUPLING_USE_KOMPAS_SDK=1).");
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
