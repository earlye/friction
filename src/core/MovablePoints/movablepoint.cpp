/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
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

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "movablepoint.h"
#include "skia/skqtconversions.h"
#include "pointhelpers.h"
#include "Animators/transformanimator.h"
#include "themesupport.h"
#include "Private/document.h"

#include <cmath>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcMovablePoint, "friction.movablepoint", QtWarningMsg)

namespace {
// Projects absPos through the canvas's current total matrix and reports the
// effective world-to-screen scale at that point, so callers can draw entirely
// in device/screen space afterward instead of asking SkCanvas to build a
// bounding box from absPos +/- a radius in world space. At extreme zoom,
// absPos and the total matrix's scale are both huge/tiny outliers, and any
// value derived by *adding* a small radius to absPos in float32 loses
// precision asymmetrically between the x and y axes (they rarely share the
// same magnitude), stretching circles into ovals. Multiplying invScale by
// the current total-matrix scale carries no such risk, since it's a single
// multiplication rather than an addition of mismatched magnitudes. Same
// pattern as the gizmo fix in issue-019f2ded.
//
// Returns false (skip the draw) if the point projects to a non-finite
// screen position, or if the resulting pixelScale isn't a usable size
// (negative, or non-finite because e.g. Skia's SkMatrix::getMinScale()
// returned -1 for an overflowed/perspective matrix, or because invScale
// itself - derived from the view transform, unclamped - overflowed).
// Only computes coordinates; callers are still responsible for their own
// canvas->save()/resetMatrix()/restore() bracketing.
bool projectToScreen(SkCanvas * const canvas, const SkPoint &absPos,
                     const float invScale, SkPoint * const screenPos,
                     float * const pixelScale) {
    canvas->getTotalMatrix().mapPoints(screenPos, &absPos, 1);
    if(!std::isfinite(screenPos->x()) || !std::isfinite(screenPos->y())) {
        qCWarning(lcMovablePoint) << "projectToScreen: absPos projected to a non-finite screen point, skipping draw"
                                  << "absPos=" << absPos.x() << absPos.y();
        return false;
    }
    const float matrixScale = canvas->getTotalMatrix().getMinScale();
    *pixelScale = invScale*matrixScale;
    if(!std::isfinite(*pixelScale) || *pixelScale < 0.f) {
        qCWarning(lcMovablePoint) << "projectToScreen: computed pixelScale is invalid, skipping draw"
                                  << "invScale=" << invScale << "matrixScale=" << matrixScale;
        return false;
    }
    qCDebug(lcMovablePoint) << "projectToScreen: invScale=" << invScale
                            << "absPos=" << absPos.x() << absPos.y()
                            << "screenPos=" << screenPos->x() << screenPos->y()
                            << "pixelScale=" << *pixelScale;
    return true;
}
}

MovablePoint::MovablePoint(const MovablePointType type) : mType(type) {}

MovablePoint::MovablePoint(BasicTransformAnimator * const trans,
                           const MovablePointType type) :
    MovablePoint(type) {
    setTransform(trans);
}

void MovablePoint::startTransform() {
    mSavedRelPos = getRelativePos();
}

const QPointF &MovablePoint::getSavedRelPos() const {
    return mSavedRelPos;
}

void MovablePoint::drawHovered(SkCanvas * const canvas,
                               const float invScale) {
    const SkPoint absPos = toSkPoint(getAbsolutePos());
    SkPoint screenPos;
    float pixelScale;
    if(!projectToScreen(canvas, absPos, invScale, &screenPos, &pixelScale)) return;

    canvas->save();
    canvas->resetMatrix();

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(1.5f*pixelScale);
    paint.setColor(SK_ColorRED);
    canvas->drawCircle(screenPos, static_cast<float>(mRadius)*pixelScale, paint);

    canvas->restore();
    //pen.setCosmetic(true);
    //p->setPen(pen);
//    drawCosmeticEllipse(p, getAbsolutePos(),
//                        mRadius, mRadius);
}

void MovablePoint::setAbsolutePos(const QPointF &pos) {
    if(mTrans_cv) setRelativePos(mTrans_cv->mapAbsPosToRel(pos));
    else setRelativePos(pos);
}

QPointF MovablePoint::mapRelativeToAbsolute(const QPointF &relPos) const {
    if(mTrans_cv) return mTrans_cv->mapRelPosToAbs(relPos);
    else return relPos;
}

QPointF MovablePoint::mapAbsoluteToRelative(const QPointF &absPos) const {
    if(mTrans_cv) return mTrans_cv->mapAbsPosToRel(absPos);
    else return absPos;
}

QPointF MovablePoint::getAbsolutePos() const {
    return mapRelativeToAbsolute(getRelativePos());
}

void MovablePoint::drawOnAbsPosSk(SkCanvas * const canvas,
                                  const SkPoint &absPos,
                                  const float invScale,
                                  const SkColor &fillColor,
                                  const bool keyOnCurrent)
{
    // Update global pivot used for gizmos with this point's absolute position
    const auto doc = Document::sInstance;
    doc->fPivotPosForGizmos = getAbsolutePos();
    doc->fPivotPosForGizmosValid = true;

    SkPoint screenPos;
    float pixelScale;
    if(!projectToScreen(canvas, absPos, invScale, &screenPos, &pixelScale)) return;

    canvas->save();
    canvas->resetMatrix();

    const float scaledRadius = static_cast<float>(mRadius)*pixelScale;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(fillColor);

    paint.setStyle(SkPaint::kFill_Style);
    canvas->drawCircle(screenPos, scaledRadius, paint);

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(toSkColor(ThemeSupport::getThemeButtonBaseColor()));
    paint.setStrokeWidth(pixelScale);
    canvas->drawCircle(screenPos, scaledRadius, paint);

    if(keyOnCurrent) {
        const float halfRadius = scaledRadius*0.5f;

        paint.setColor(toSkColor(ThemeSupport::getThemeColorRed()));
        paint.setStyle(SkPaint::kFill_Style);
        canvas->drawCircle(screenPos, halfRadius, paint);

        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(0.5f*pixelScale);
        paint.setColor(SK_ColorWHITE);
        canvas->drawCircle(screenPos, halfRadius, paint);
    }

    canvas->restore();
}

void MovablePoint::drawSk(SkCanvas * const canvas, const CanvasMode mode,
                          const float invScale, const bool keyOnCurrent,
                          const bool ctrlPressed) {
    Q_UNUSED(mode)
    Q_UNUSED(keyOnCurrent)
    Q_UNUSED(ctrlPressed)
    const SkColor fillCol = mSelected ?
                SkColorSetRGB(255, 0, 0) :
                SkColorSetRGB(255, 175, 175);
    const SkPoint absPos = toSkPoint(getAbsolutePos());
    drawOnAbsPosSk(canvas, absPos, invScale, fillCol);
}

bool MovablePoint::isVisible(const CanvasMode mode) const {
    return mode == CanvasMode::pointTransform;
}

BasicTransformAnimator *MovablePoint::getTransform() {
    return mTrans_cv;
}

void MovablePoint::setTransform(BasicTransformAnimator * const trans) {
    mTrans_cv = trans;
}

bool MovablePoint::isPointAtAbsPos(const QPointF &absPoint,
                                   const CanvasMode mode,
                                   const qreal invScale) {
    if(isHidden(mode)) return false;
    const QPointF dist = getAbsolutePos() - absPoint;
    return pointToLen(dist) < mRadius*invScale;
}

void MovablePoint::rectPointsSelection(const QRectF &absRect,
                                       const CanvasMode mode,
                                       const PtOp &adder) {
    if(!selectionEnabled()) return;
    if(isHidden(mode)) return;
    if(isContainedInRect(absRect)) adder(this);
}

bool MovablePoint::isContainedInRect(const QRectF &absRect) {
    return absRect.contains(getAbsolutePos());
}

void MovablePoint::rotateRelativeToSavedPivot(const qreal rot) {
    QMatrix mat;
    mat.translate(mPivot.x(), mPivot.y());
    mat.rotate(rot);
    mat.translate(-mPivot.x(), -mPivot.y());
    moveToRel(mat.map(mSavedRelPos));
}

void MovablePoint::scaleRelativeToSavedPivot(const qreal sx,
                                             const qreal sy)
{
    QMatrix mat;
    mat.translate(mPivot.x(), mPivot.y());
    mat.scale(sx, sy);
    mat.translate(-mPivot.x(), -mPivot.y());
    moveToRel(mat.map(mSavedRelPos));
}

void MovablePoint::shearRelativeToSavedPivot(const qreal shearX,
                                             const qreal shearY)
{
    QMatrix mat;
    mat.translate(mPivot.x(), mPivot.y());
    mat.shear(shearX, shearY);
    mat.translate(-mPivot.x(), -mPivot.y());
    moveToRel(mat.map(mSavedRelPos));
}

void MovablePoint::saveTransformPivotAbsPos(const QPointF &absPivot) {
    if(mTrans_cv) mPivot = mTrans_cv->mapAbsPosToRel(absPivot);
    else mPivot = absPivot;
}

void MovablePoint::rotateBy(const qreal rot) {
    QMatrix rotMatrix;
    rotMatrix.translate(-mPivot.x(), -mPivot.y());
    rotMatrix.rotate(rot);
    rotMatrix.translate(mPivot.x(), mPivot.y());
    setRelativePos(rotMatrix.map(mSavedRelPos));
}

void MovablePoint::scale(const qreal scaleXBy,
                         const qreal scaleYBy) {
    QMatrix scaleMatrix;
    scaleMatrix.translate(-mPivot.x(), -mPivot.y());
    scaleMatrix.scale(scaleXBy, scaleYBy);
    scaleMatrix.translate(mPivot.x(), mPivot.y());
    setRelativePos(scaleMatrix.map(mSavedRelPos));
}

void MovablePoint::moveToAbs(const QPointF& absPos) {
    setAbsolutePos(absPos);
}

void MovablePoint::moveByAbs(const QPointF &absTrans) {
    moveToAbs(mapRelativeToAbsolute(mSavedRelPos) + absTrans);
}

void MovablePoint::moveToRel(const QPointF &relPos) {
    moveByRel(relPos - mSavedRelPos);
}

void MovablePoint::moveByRel(const QPointF &relTranslation) {
    setRelativePos(mSavedRelPos + relTranslation);
}

void MovablePoint::scale(const qreal scaleBy) {
    scale(scaleBy, scaleBy);
}

void MovablePoint::setRadius(const qreal radius) {
    mRadius = radius;
}

qreal MovablePoint::getRadius() {
    return mRadius;
}

void MovablePoint::setSelected(const bool selected, const Op &deselect) {
    mSelected = selected;
    mRemoveFromSelection = deselect;
}

void MovablePoint::deselect() {
    if(mRemoveFromSelection) mRemoveFromSelection();
}

bool MovablePoint::isNodePoint() {
    return mType == MovablePointType::TYPE_PATH_POINT;
}

bool MovablePoint::isSmartNodePoint() {
    return mType == MovablePointType::TYPE_SMART_PATH_POINT;
}

bool MovablePoint::isPivotPoint() {
    return mType == MovablePointType::TYPE_PIVOT_POINT;
}

bool MovablePoint::isCtrlPoint() {
    return mType == MovablePointType::TYPE_CTRL_POINT;
}
