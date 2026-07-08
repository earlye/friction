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
#include <cmath>

// MovablePoint::drawOnAbsPosSk's precision bug lived inside SkCanvas::drawCircle
// itself: it builds a world-space bounding rect (cx-r, cy-r, cx+r, cy+r) in
// SkScalar (float32) *before* applying the current matrix. These helpers
// reproduce that exact float32 arithmetic without needing Skia, the same way
// tst_gizmos.cpp's narrowToFloat32() modeled SkScalar narrowing — so this test
// can characterize the bug and the fix without linking the whole frictioncore
// (Skia, FFmpeg, ...) static archive for one dependency-free computation.
//
// Caveat: unlike tst_gizmos.cpp (which compiles and calls the real
// Gizmos::buildShapes), the helpers below are standalone reimplementations
// of the arithmetic, not the actual functions in movablepoint.cpp — this
// test proves the fix's approach is numerically sound in isolation, it does
// not exercise movablepoint.cpp's projectToScreen/drawOnAbsPosSk/drawHovered
// directly, so it would not catch a regression that reintroduced the bug
// there while leaving this file untouched.

// Old (buggy) approach: scale the radius in world space (radius * invScale,
// tied to how extreme the zoom is) and let it be added/subtracted straight
// from the absolute, possibly huge, world position.
struct WorldSpaceBounds { float left, right, top, bottom; };

static WorldSpaceBounds worldSpaceCircleBounds(float absPosX, float absPosY,
                                               float radius, float invScale)
{
    const float scaledRadius = radius * invScale;
    return { absPosX - scaledRadius, absPosX + scaledRadius,
             absPosY - scaledRadius, absPosY + scaledRadius };
}

// New (fixed) approach: derive a scale factor that does not depend on the
// absolute position at all (just invScale times the matrix's own scale, a
// pure multiplication of two well-conditioned numbers), then apply the
// resulting radius to an already-small, already-projected screen position.
static WorldSpaceBounds screenSpaceCircleBounds(float screenPosX, float screenPosY,
                                                float radius, float invScale,
                                                float matrixScale)
{
    const float pixelScale = invScale * matrixScale;
    const float scaledRadius = radius * pixelScale;
    return { screenPosX - scaledRadius, screenPosX + scaledRadius,
             screenPosY - scaledRadius, screenPosY + scaledRadius };
}

class TstMovablePointPrecision : public QObject
{
    Q_OBJECT

private slots:
    // Characterizes the bug: at extreme zoom-in, the tiny scaled radius gets
    // added to a huge absolute world coordinate in float32, and how much of
    // it survives depends on that coordinate's own magnitude — different for
    // the x and y axes whenever absPos.x and absPos.y differ in magnitude,
    // producing an asymmetric (oval, not circular) bounding rect.
    void worldSpaceBoundsCollapseAsymmetricallyAtExtremeZoom();

    // The fix: since the radius no longer derives from the absolute position
    // at all, the bounding rect stays symmetric (a real circle) regardless of
    // how extreme the underlying world coordinates or zoom level were.
    void screenSpaceBoundsStaySymmetricAtExtremeZoom();
};

void TstMovablePointPrecision::worldSpaceBoundsCollapseAsymmetricallyAtExtremeZoom()
{
    // A pivot far from the origin on x but close to it on y - realistic for a
    // gizmo/point positioned away from visible artwork near the scene origin,
    // as described in the reproduction (issue-...-movablepoint-circle-precision-loss.md).
    const float absPosX = 1.0e8f;
    const float absPosY = 3.0f;
    const float radius = 5.0f;
    const float extremeInvScale = 1.0e-6f;

    const auto bounds = worldSpaceCircleBounds(absPosX, absPosY, radius, extremeInvScale);

    const float xExtent = bounds.right - bounds.left;
    const float yExtent = bounds.bottom - bounds.top;

    // The x extent (large absPos) loses the radius entirely...
    QCOMPARE(xExtent, 0.0f);
    // ...while the y extent (small absPos) keeps it, close to the true 2*scaledRadius.
    QVERIFY(yExtent > 0.0f);
    // Same scaledRadius, wildly different surviving extents: an oval, not a circle.
    QVERIFY(std::abs(xExtent - yExtent) > 1.0e-6f);
}

void TstMovablePointPrecision::screenSpaceBoundsStaySymmetricAtExtremeZoom()
{
    // Same extreme invScale as above, but the matrix's own scale is its exact
    // reciprocal (as it is in practice: invScale is derived from that same
    // scale), and the position being bounded is a plausible on-screen point
    // rather than the raw absolute world coordinate.
    const float extremeInvScale = 1.0e-6f;
    const float matrixScale = 1.0f / extremeInvScale;
    const float radius = 5.0f;
    const float screenPosX = 512.0f;
    const float screenPosY = 384.0f;

    const auto bounds = screenSpaceCircleBounds(screenPosX, screenPosY, radius,
                                                extremeInvScale, matrixScale);

    const float xExtent = bounds.right - bounds.left;
    const float yExtent = bounds.bottom - bounds.top;

    QVERIFY(xExtent > 0.0f);
    QCOMPARE(xExtent, yExtent);
    QCOMPARE(xExtent, 2.0f * radius);
}

QTEST_APPLESS_MAIN(TstMovablePointPrecision)

#include "tst_movablepoint_precision.moc"
