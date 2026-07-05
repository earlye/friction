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

#ifndef SVGFLIPBOOKTRACK_H
#define SVGFLIPBOOKTRACK_H

#include "Animators/staticcomplexanimator.h"
#include <QLoggingCategory>
#include <QMap>

Q_DECLARE_LOGGING_CATEGORY(lcSvgFlipbookTrack)

class ContainerBox;
class BoundingBox;
class IntAnimator;

class CORE_EXPORT SvgFlipbookTrack : public StaticComplexAnimator {
    Q_OBJECT
    e_OBJECT
public:
    explicit SvgFlipbookTrack(const QString& ownerElementId);

    void setPageMap(const QMap<int, QString>& pageMap);
    // Sets already-resolved page targets directly, bypassing name-based
    // re-resolution in resolveTargets(). For the inkscape:label
    // convention, a page's target is definitionally the direct child it
    // was scanned from - re-resolving it by name (as setPageMap()'s
    // locator-string mechanism does, needed since a YAML `map:` value
    // can point anywhere in the tree) risks aliasing to an unrelated box
    // that happens to share the same display name, e.g. the flipbook's
    // own container.
    void setResolvedPagesDirect(const QMap<int, BoundingBox*>& pages);
    void setOwnerBox(ContainerBox* ownerBox);
    bool isOrphaned() const { return mOrphaned; }
    void setOrphaned(bool orphaned);

    void resolveTargets(ContainerBox* svgRoot);
    void syncToTargets();
    int currentPageIndex() const;

    void writeTrack(eWriteStream& dst) const;
    void readTrack(eReadStream& src);

    void prp_drawTimelineControls(
            QPainter* p, qreal pixelsPerFrame,
            const FrameRange& absFrameRange, int rowHeight) override;

    void prp_setupTreeViewMenu(PropertyMenu* menu) override;

signals:
    void deleteRequested();
    void pageChanged();

private:
    bool mOrphaned = true;
    bool mDirectResolve = false;
    QMap<int, QString> mPageMap;
    QMap<int, BoundingBox*> mResolvedPages;
    qsptr<IntAnimator> mIndex;
    ContainerBox* mOwnerBox = nullptr;
};

#endif // SVGFLIPBOOKTRACK_H
