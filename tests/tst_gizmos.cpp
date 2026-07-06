/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

#include <QtTest>

#include "gizmos.h"

using namespace Friction::Core;

// Narrows a QPointF (double, qreal) to the precision SkPoint/SkScalar actually
// stores (float32) — this is what canvasgizmos.cpp's toSkScalar() calls do when
// building draw geometry. Mirroring that narrowing here, without needing Skia
// itself, is what lets this test observe the precision-loss bug directly.
static QPointF narrowToFloat32(const QPointF &pt)
{
    return QPointF(static_cast<float>(pt.x()), static_cast<float>(pt.y()));
}

static int countDistinctPoints(const QVector<QPointF> &points)
{
    QVector<QPointF> distinct;
    for (const QPointF &pt : points) {
        bool found = false;
        for (const QPointF &seen : distinct) {
            if (seen == pt) { found = true; break; }
        }
        if (!found) { distinct.append(pt); }
    }
    return distinct.size();
}

class TstGizmos : public QObject
{
    Q_OBJECT

private slots:
    // Characterizes the bug this issue is about: building the gizmo's shape in
    // world coordinates (pixelConstant * invScale, added to an absolute pivot)
    // and narrowing to float32 for drawing loses the whole shape at extreme zoom.
    void worldSpaceOffsetsCollapseAtExtremeZoom();

    // The fix: building the same shape directly in already-projected screen
    // coordinates (scale=1, no invScale at all) survives narrowing regardless
    // of how extreme the zoom was, because the shape math never involves it.
    void screenSpaceOffsetsSurviveExtremeZoom();

    void visibilityFlagsControlGeometryVisibility();
};

void TstGizmos::worldSpaceOffsetsCollapseAtExtremeZoom()
{
    const Gizmos::Config config;
    const QPointF pivot(1000.0, 1000.0); // an ordinary, non-extreme scene position
    const qreal extremeInvScale = 1e-8;  // stands in for an extreme zoom-in level

    const auto shapes = Gizmos::buildShapes(config, true, true, true, true,
                                            extremeInvScale, pivot);

    QVERIFY(shapes.axisYGeom.polygonPoints.size() >= 3);
    QCOMPARE(countDistinctPoints(shapes.axisYGeom.polygonPoints), 8);

    QVector<QPointF> narrowed;
    for (const QPointF &pt : shapes.axisYGeom.polygonPoints) {
        narrowed.append(narrowToFloat32(pt));
    }

    // Every offset (a few tens of pixels times a 1e-8 invScale) is far below
    // float32's precision floor relative to a pivot of magnitude 1000 — they
    // all collapse onto the same point once narrowed, exactly like the bug.
    QCOMPARE(countDistinctPoints(narrowed), 1);
}

void TstGizmos::screenSpaceOffsetsSurviveExtremeZoom()
{
    const Gizmos::Config config;
    const QPointF screenPivot(512.0, 384.0); // a plausible projected device-pixel position

    const auto shapes = Gizmos::buildShapes(config, true, true, true, true,
                                            1.0, screenPivot);

    QVERIFY(shapes.axisYGeom.polygonPoints.size() >= 3);
    QCOMPARE(countDistinctPoints(shapes.axisYGeom.polygonPoints), 8);

    QVector<QPointF> narrowed;
    for (const QPointF &pt : shapes.axisYGeom.polygonPoints) {
        narrowed.append(narrowToFloat32(pt));
    }

    // Screen-space geometry never multiplies by invScale at all, so the shape
    // survives narrowing intact no matter how extreme the underlying zoom was.
    QCOMPARE(countDistinctPoints(narrowed), 8);
}

void TstGizmos::visibilityFlagsControlGeometryVisibility()
{
    const Gizmos::Config config;
    const QPointF pivot(0.0, 0.0);

    const auto allHidden = Gizmos::buildShapes(config, false, false, false, false, 1.0, pivot);
    QVERIFY(!allHidden.axisYGeom.visible);
    QVERIFY(!allHidden.scaleYGeom.visible);
    QVERIFY(!allHidden.shearYGeom.visible);
    QVERIFY(allHidden.rotateHandlePolygon.isEmpty());

    const auto allShown = Gizmos::buildShapes(config, true, true, true, true, 1.0, pivot);
    QVERIFY(allShown.axisYGeom.visible);
    QVERIFY(allShown.scaleYGeom.visible);
    QVERIFY(allShown.shearYGeom.visible);
    QVERIFY(!allShown.rotateHandlePolygon.isEmpty());
}

QTEST_APPLESS_MAIN(TstGizmos)

#include "tst_gizmos.moc"
