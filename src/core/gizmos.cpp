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

#include "gizmos.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QtMath>

using namespace Friction::Core;

Gizmos::Shapes Gizmos::buildShapes(const Config &config,
                                   bool showRotate,
                                   bool showPosition,
                                   bool showScale,
                                   bool showShear,
                                   qreal scale,
                                   const QPointF &origin)
{
    Shapes result;

    const qreal axisWidthWorld = config.axisWidthPx * scale;
    const qreal axisHeightWorld = config.axisHeightPx * scale;
    const qreal axisGapYWorld = config.axisYOffsetPx * scale;
    const qreal axisGapXWorld = config.axisXOffsetPx * scale;
    const qreal xLineLengthWorld = config.xLineLengthPx * scale;
    const qreal xLineStrokeWorld = config.xLineStrokePx * scale;
    const qreal yLineLengthWorld = config.yLineLengthPx * scale;
    const qreal yLineStrokeWorld = config.yLineStrokePx * scale;

    const qreal anchorOffset = 2.0 * scale;
    result.rotateHandleAnchor = origin + QPointF(anchorOffset, -anchorOffset);

    const qreal baseRotateRadiusWorld = config.rotateRadiusPx * scale;
    result.rotateHandleRadius = baseRotateRadiusWorld;

    const qreal rotateHandleSweepDeg = config.rotateSweepDeg;
    const qreal rotateHandleStartOffsetDeg = config.rotateBaseOffsetDeg;
    const qreal rotateHandleAngleDeg = 0.0; // keep gizmo orientation screen-aligned

    const qreal strokeWorld = config.rotateStrokePx * scale;
    const qreal sweepDegAbs = std::abs(rotateHandleSweepDeg);
    auto normalizeAngle = [](qreal degrees) {
        degrees = std::fmod(degrees, 360.0);
        if (degrees < 0.0) { degrees += 360.0; }
        return degrees;
    };
    const qreal startAngleSk = normalizeAngle(rotateHandleStartOffsetDeg + rotateHandleAngleDeg);
    const qreal direction = (rotateHandleSweepDeg >= 0.0) ? 1.0 : -1.0;

    if (showRotate && sweepDegAbs > std::numeric_limits<qreal>::epsilon()) {
        const int segments = std::max(8, static_cast<int>(std::ceil(sweepDegAbs / 6.0)));
        auto angleToPoint = [&](qreal angleDeg, qreal radius) {
            const qreal angleRad = qDegreesToRadians(angleDeg);
            return QPointF(result.rotateHandleAnchor.x() + radius * std::cos(angleRad),
                           result.rotateHandleAnchor.y() + radius * std::sin(angleRad));
        };
        auto buildArcPolygon = [&](qreal halfThickness, QVector<QPointF> &storage) {
            storage.clear();
            const qreal outerRadius = result.rotateHandleRadius + halfThickness;
            if (outerRadius <= 0.0) { return; }
            const qreal innerRadius = std::max(result.rotateHandleRadius - halfThickness, 0.0);
            storage.reserve((segments + 1) * 2);
            for (int i = 0; i <= segments; ++i) {
                const qreal angle = normalizeAngle(startAngleSk + direction * (sweepDegAbs * i) / segments);
                storage.append(angleToPoint(angle, outerRadius));
            }
            if (innerRadius > std::numeric_limits<qreal>::epsilon()) {
                for (int i = 0; i <= segments; ++i) {
                    const qreal angle = normalizeAngle(startAngleSk + direction * sweepDegAbs - direction * (sweepDegAbs * i) / segments);
                    storage.append(angleToPoint(angle, innerRadius));
                }
            } else {
                storage.append(result.rotateHandleAnchor);
            }
        };

        buildArcPolygon(strokeWorld * 0.5, result.rotateHandlePolygon);
        const qreal hitHalfThickness = (config.rotateHitWidthPx * scale) * 0.5;
        buildArcPolygon(hitHalfThickness, result.rotateHandleHitPolygon);
    }

    result.axisYGeom.center = origin + QPointF(0.0, -axisGapYWorld);
    result.axisYGeom.size = QSizeF(axisWidthWorld, axisHeightWorld);
    result.axisYGeom.angleDeg = 0.0;
    result.axisYGeom.visible = showPosition;
    result.axisYGeom.usePolygon = true;
    result.axisYGeom.polygonPoints = {
        origin + QPointF(0.0, - 10.0 * scale),
        origin + QPointF(-2.0 * scale, - 11.0 * scale),
        origin + QPointF(-2.0 * scale, - 55.0 * scale),
        origin + QPointF(-6.0 * scale, - 56.5 * scale),
        origin + QPointF(0.0, - 70.0 * scale),
        origin + QPointF(6.0 * scale, - 56.5 * scale),
        origin + QPointF(2.0 * scale, - 55.0 * scale),
        origin + QPointF(2.0 * scale, - 11.0 * scale)
    };

    result.axisXGeom.center = origin + QPointF(axisGapXWorld, 0.0);
    result.axisXGeom.size = QSizeF(axisHeightWorld, axisWidthWorld);
    result.axisXGeom.angleDeg = 0.0;
    result.axisXGeom.visible = showPosition;
    result.axisXGeom.usePolygon = true;
    result.axisXGeom.polygonPoints = {
        origin + QPointF(10.0 * scale, 0.0),
        origin + QPointF(11.0 * scale, -2.0 * scale),
        origin + QPointF(55.0 * scale, -2.0 * scale),
        origin + QPointF(56.5 * scale, -6.0 * scale),
        origin + QPointF(70.0 * scale, 0.0),
        origin + QPointF(56.5 * scale, 6.0 * scale),
        origin + QPointF(55.0 * scale, 2.0 * scale),
        origin + QPointF(11.0 * scale, 2.0 * scale)
    };

    result.axisUniformGeom.center = origin + QPointF(axisGapXWorld, 0.0);
    result.axisUniformGeom.size = QSizeF(axisHeightWorld, axisWidthWorld);
    result.axisUniformGeom.visible = showPosition;
    result.axisUniformGeom.usePolygon = true;
    result.axisUniformGeom.polygonPoints = {
        origin + QPointF((config.axisUniformOffsetPx + config.axisUniformChamferPx) * scale, - config.axisUniformOffsetPx * scale),
        origin + QPointF(config.axisUniformOffsetPx * scale, - (config.axisUniformOffsetPx + config.axisUniformChamferPx) * scale),
        origin + QPointF(config.axisUniformOffsetPx * scale, - (config.axisUniformWidthPx - config.axisUniformChamferPx) * scale),
        origin + QPointF((config.axisUniformOffsetPx + config.axisUniformChamferPx) * scale, - config.axisUniformWidthPx * scale),
        origin + QPointF((config.axisUniformWidthPx - config.axisUniformChamferPx) * scale, - config.axisUniformWidthPx * scale),
        origin + QPointF(config.axisUniformWidthPx * scale, - (config.axisUniformWidthPx - config.axisUniformChamferPx) * scale),
        origin + QPointF(config.axisUniformWidthPx * scale, - (config.axisUniformOffsetPx + config.axisUniformChamferPx) * scale),
        origin + QPointF((config.axisUniformWidthPx - config.axisUniformChamferPx) * scale, - config.axisUniformOffsetPx * scale)
    };

    result.xLineGeom.start = origin;
    result.xLineGeom.end = origin + QPointF(xLineLengthWorld, 0.0);
    result.xLineGeom.strokeWidth = xLineStrokeWorld;

    result.yLineGeom.start = origin;
    result.yLineGeom.end = origin + QPointF(0.0, yLineLengthWorld);
    result.yLineGeom.strokeWidth = yLineStrokeWorld;

    const qreal rotateOffsetWorld = result.axisYGeom.size.width() * 0.5;
    result.rotateHandleRadius = baseRotateRadiusWorld + rotateOffsetWorld;

    const QPointF offset(0.0, -result.rotateHandleRadius);
    result.rotateHandlePos = origin + offset;

    const qreal scaleSizeWorld = config.scaleSizePx * scale;
    const qreal scaleHalf = scaleSizeWorld * 0.5;
    const qreal scaleGapWorld = config.scaleGapPx * scale;
    const qreal axisYTop = result.axisYGeom.center.y() - (result.axisYGeom.size.height() * 0.5);
    const qreal axisXRight = result.axisXGeom.center.x() + (result.axisXGeom.size.width() * 0.5);
    const qreal scaleCenterY = axisYTop - scaleGapWorld - scaleHalf;
    const qreal scaleCenterX = axisXRight + scaleGapWorld + scaleHalf;

    result.scaleYGeom.center = QPointF(result.axisYGeom.center.x(), scaleCenterY);
    result.scaleYGeom.halfExtent = scaleHalf;
    result.scaleYGeom.visible = showScale;
    result.scaleYGeom.usePolygon = false;

    result.scaleXGeom.center = QPointF(scaleCenterX, result.axisXGeom.center.y());
    result.scaleXGeom.halfExtent = scaleHalf;
    result.scaleXGeom.visible = showScale;
    result.scaleXGeom.usePolygon = false;

    result.scaleUniformGeom.center = QPointF(scaleCenterX, scaleCenterY);
    result.scaleUniformGeom.halfExtent = scaleHalf;
    result.scaleUniformGeom.visible = showScale;
    result.scaleUniformGeom.usePolygon = true;
    result.scaleUniformGeom.polygonPoints = {
        QPointF(result.scaleUniformGeom.center.x() - scaleHalf * 2, result.scaleUniformGeom.center.y() - scaleHalf),
        QPointF(result.scaleUniformGeom.center.x() + scaleHalf, result.scaleUniformGeom.center.y() - scaleHalf),
        QPointF(result.scaleUniformGeom.center.x() + scaleHalf, result.scaleUniformGeom.center.y() + scaleHalf * 2),
        QPointF(result.scaleUniformGeom.center.x() - scaleHalf, result.scaleUniformGeom.center.y() + scaleHalf * 2),
        QPointF(result.scaleUniformGeom.center.x() - scaleHalf, result.scaleUniformGeom.center.y() + scaleHalf),
        QPointF(result.scaleUniformGeom.center.x() - scaleHalf * 2, result.scaleUniformGeom.center.y() + scaleHalf)
    };

    const qreal shearRadiusWorld = config.shearRadiusPx * scale;

    const QPointF scaleYCenter = result.scaleYGeom.center;
    const QPointF scaleXCenter = result.scaleXGeom.center;
    const QPointF scaleUniformCenter = result.scaleUniformGeom.center;

    result.shearXGeom.center = QPointF((scaleYCenter.x() + scaleUniformCenter.x()) * 0.5,
                                       scaleCenterY);
    result.shearXGeom.radius = shearRadiusWorld;
    result.shearXGeom.visible = showShear;
    result.shearXGeom.usePolygon = true;
    {
        const qreal halfWidth = shearRadiusWorld * 1.5;
        const qreal topHeight = shearRadiusWorld * 0.8;
        const QPointF c = result.shearXGeom.center;
        result.shearXGeom.polygonPoints = {
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth * 1.5, c.y()),
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth, c.y() - topHeight),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth, c.y() - topHeight),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth * 1.5, c.y()),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth, c.y() + topHeight),
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth, c.y() + topHeight)
        };
    }

    result.shearYGeom.center = QPointF(scaleCenterX,
                                       (scaleXCenter.y() + scaleUniformCenter.y()) * 0.5);
    result.shearYGeom.radius = shearRadiusWorld;
    result.shearYGeom.visible = showShear;
    result.shearYGeom.usePolygon = true;
    {
        const qreal halfHeight = shearRadiusWorld * 1.5;
        const qreal topWidth = shearRadiusWorld * 0.8;
        const QPointF c = result.shearYGeom.center;
        result.shearYGeom.polygonPoints = {
            QPointF(c.x(), c.y() + scaleHalf * 0.5 - halfHeight * 1.5),
            QPointF(c.x() + topWidth, c.y() + scaleHalf * 0.5 - halfHeight),
            QPointF(c.x() + topWidth, c.y() + scaleHalf * 0.5 + halfHeight),
            QPointF(c.x(), c.y() + scaleHalf * 0.5 + halfHeight * 1.5),
            QPointF(c.x() - topWidth, c.y() + scaleHalf * 0.5 + halfHeight),
            QPointF(c.x() - topWidth, c.y() + scaleHalf * 0.5 - halfHeight)
        };
    }

    return result;
}
