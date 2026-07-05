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

#include "svglinkbox.h"

#include "canvas.h"
#include "exceptions.h"
#include "fileshandler.h"
#include "svgimporter.h"
#include "svglabel.h"
#include "Animators/gradient.h"
#include "Animators/transformanimator.h"
#include "Animators/qpointfanimator.h"
#include "ReadWrite/evformat.h"
#include "swt_abstraction.h"
#include "typemenu.h"
#include "XML/xevexporter.h"

#include "Animators/qrealanimator.h"
#include "Animators/complexanimator.h"
#include "matrixdecomposition.h"

#include <QInputDialog>
#include <QLoggingCategory>
#include <yaml-cpp/yaml.h>

Q_LOGGING_CATEGORY(lcSvgPivot, "friction.svgpivot", QtWarningMsg)
Q_LOGGING_CATEGORY(lcSvgFollower, "friction.svgfollower", QtWarningMsg)

SvgFileCacheHandler* svgFileHandlerGetter(const QString& path) {
    return FilesHandler::sInstance->getFileHandler<SvgFileCacheHandler>(path);
}

SvgLinkBox::SvgLinkBox() :
    SvgLinkBoxBase([](const QString& path) {
                       return svgFileHandlerGetter(path);
                   },
                   [this](SvgFileCacheHandler* obj) {
                       return fileHandlerAfterAssigned(obj);
                   },
                   [this](ConnContext& conn, SvgFileCacheHandler* obj) {
                       fileHandlerConnector(conn, obj);
                   }) {
    mType = eBoxType::svgLink;
}

#include "GUI/edialogs.h"
void SvgLinkBox::changeSourceFile() {
    const QString path = eDialogs::openFile(
                "Change Source", getFilePath(),
                "SVG Files (*.svg)");
    if(!path.isEmpty()) setFilePath(path);
}

void SvgLinkBox::updateContent() {
    removeAllContained();
    mGradients.clear();
    const auto obj = fileHandler();
    QString contentName;
    qsptr<BoundingBox> imported;
    if(obj) {
        const auto gradientCreator = [this]() {
            const auto grad = enve::make_shared<Gradient>();
            mGradients << grad;
            return grad.get();
        };
        try {
            imported = ImportSVG::loadSVGFile(obj->path(), gradientCreator);
        } catch(const std::exception& e) {
            gPrintExceptionCritical(e);
        }
        if(imported) contentName = imported->prp_getName();
    }
    // Sync this box's own name before the content is inserted below it —
    // otherwise the content (e.g. "Ruby") would already be a descendant
    // sharing the candidate name, forcing a spurious "Ruby 0" every time.
    syncAutoName(contentName);
    if(imported) addContained(imported);
    resolveElementTracks();
}

void SvgLinkBox::syncAutoName(const QString& contentName) {
    const QString base = contentName.isEmpty() ? QStringLiteral("Empty Link") : contentName;
    const auto parentScene = getParentScene();
    ContainerBox* const nameCtxt = parentScene ?
                static_cast<ContainerBox*>(parentScene) : this;
    mAutoName = nameCtxt->makeNameUniqueForDescendants(base, this);
    if(mMigrateLegacyName) {
        // Files saved before the auto/manual name split never recorded
        // intent — only a name. Treat anything but the untouched default
        // as a deliberate rename so upgrading never silently discards a
        // name the user chose.
        mMigrateLegacyName = false;
        const QString restoredName = prp_getName();
        if(restoredName != QStringLiteral("Empty Link")) mManualName = restoredName;
    }
    if(mManualName.isEmpty()) {
        prp_setName(mAutoName);
    } else if(mManualName != prp_getName()) {
        // The displayed name can drift from mManualName via a non-action
        // rename (e.g. ContainerBox::insertContained's collision fix-up).
        // Absorb it so a stale value doesn't linger for "reset to auto".
        mManualName = prp_getName();
    }
}

void SvgLinkBox::prp_setNameAction(const QString &newName) {
    if(newName == prp_getName()) return;
    const QString oldManualName = mManualName;
    mManualName = newName;
    UndoRedo ur;
    ur.fUndo = [this, oldManualName]() { mManualName = oldManualName; };
    ur.fRedo = [this, newName]() { mManualName = newName; };
    prp_addUndoRedo(ur);
    SvgLinkBoxBase::prp_setNameAction(newName);
}

static void applyFlipbookFollower(SvgFlipbookTrack* controllerTrack,
                                   BoundingBox* follower,
                                   const QMap<int, BoundingBox*>& resolvedPages);

static bool collectPageChildren(ContainerBox* owner,
                                 QMap<int, BoundingBox*>& outPages);

// Kind declared via the legacy `<desc>` YAML block, normalized to the new
// short label-convention vocabulary ("animation", "flipbook") - empty if
// none declared or unrecognized. Lets label-convention code (e.g. a
// `follow` controller lookup) dispatch consistently regardless of which of
// the two coexisting mechanisms actually declared the target's kind.
static QString descYamlKind(const BoundingBox* box) {
    for (const auto& doc : box->getDescYaml()) {
        if (!doc.isYaml) continue;
        try {
            const auto node = YAML::Load(doc.content.toStdString());
            if (!node["kind"]) continue;
            const std::string kind = node["kind"].as<std::string>();
            if (kind == "animation-node") return QStringLiteral("animation");
            if (kind == "flipbook") return QStringLiteral("flipbook");
        } catch (...) {}
    }
    return QString();
}

// Effective kind of `box`, checking the new `?kind=...` label convention
// first and falling back to the legacy `<desc>` YAML mechanism - the two
// coexist, and either may declare a box's kind.
static QString effectiveKindOf(BoundingBox* box) {
    const QString labelKind = parseSvgLabel(
                box->property("svgInkscapeLabelRaw").toString()).kind;
    if (!labelKind.isEmpty()) return labelKind;
    return descYamlKind(box);
}

static void collectAnimationNodes(BoundingBox* box,
                                   QList<BoundingBox*>& result) {
    if (effectiveKindOf(box) == QStringLiteral("animation")) result << box;
    if (const auto container = enve_cast<ContainerBox*>(box)) {
        for (const auto& child : container->getContainedBoxes()) {
            collectAnimationNodes(child, result);
        }
    }
}

static bool subtreeHasKeys(const Property* prop) {
    if (const auto* anim = enve_cast<const Animator*>(prop)) {
        if (anim->anim_hasKeys()) return true;
    }
    if (const auto* complex = enve_cast<const ComplexAnimator*>(prop)) {
        const int n = complex->ca_getNumberOfChildren();
        for (int i = 0; i < n; i++) {
            if (subtreeHasKeys(complex->ca_getChildAt(i))) return true;
        }
    }
    return false;
}

void SvgLinkBox::resolveElementTracks() {
    ContainerBox* svgRoot = nullptr;
    const auto& contained = getContainedBoxes();
    if (!contained.isEmpty()) {
        svgRoot = enve_cast<ContainerBox*>(contained.first());
    }
    QVector<SvgElementTrack*> elementTracksToRemove;
    for (const auto& track : mElementTracks) {
        BoundingBox* targetBox = track->resolveTarget(svgRoot);
        if (targetBox) {
            track->reconcileWithTarget(targetBox);
            track->syncToTarget(targetBox);
        } else if (!subtreeHasKeys(track.get())) {
            elementTracksToRemove << track.get();
        }
    }
    // Only prune once we actually have a resolved SVG tree to check names
    // against — an unloaded/failed svgRoot must not be mistaken for "this
    // track's owner is gone", or every keyless track would be wiped out
    // and would need to regenerate on the next successful load.
    if (svgRoot) {
        for (auto* track : elementTracksToRemove) removeElementTrack(track);
    }
    if (!svgRoot) return;
    QList<BoundingBox*> animationNodes;
    collectAnimationNodes(svgRoot, animationNodes);
    for (BoundingBox* node : animationNodes) {
        const QString name = node->prp_getName();
        const bool exists = std::any_of(
            mElementTracks.begin(), mElementTracks.end(),
            [&name](const qsptr<SvgElementTrack>& t) {
                return t->prp_getName() == name;
            });
        if (!exists) addElementTrack(name);
    }
    QSet<QString> liveFlipbookOwners;
    applyFlipbookDescIfPresent(svgRoot, liveFlipbookOwners);
    applyFlipbookLabelIfPresent(svgRoot, liveFlipbookOwners);
    collectFlipbookDescs(svgRoot, liveFlipbookOwners);
    applyPivotDescIfPresent(svgRoot);
    collectPivotDescs(svgRoot);
    QVector<SvgFlipbookTrack*> flipbookTracksToRemove;
    for (const auto& track : mFlipbookTracks) {
        if (liveFlipbookOwners.contains(track->prp_getName())) continue;
        if (subtreeHasKeys(track.get())) {
            track->setPageMap({});
            track->setOrphaned(true);
        } else {
            flipbookTracksToRemove << track.get();
        }
    }
    for (auto* track : flipbookTracksToRemove) removeFlipbookTrack(track);
    // Clear before resolving targets: syncToTargets() below emits pageChanged(),
    // which re-applies mFlipbookFollowers synchronously — those bindings still
    // point into the SVG subtree updateContent() already tore down, so they
    // must not be dereferenced until collectFlipbookFollowerDescs rebuilds them
    // against the freshly imported tree.
    mFlipbookFollowers.clear();
    for (const auto& track : mFlipbookTracks) {
        if (!liveFlipbookOwners.contains(track->prp_getName())) continue;
        track->resolveTargets(svgRoot);
        track->syncToTargets();
    }
    mFollowers.clear();
    if (svgRoot) collectFollowerDescs(svgRoot, svgRoot);
    if (svgRoot) collectFlipbookFollowerDescs(svgRoot, svgRoot);
    if (svgRoot) collectFollowLabelDescs(svgRoot, svgRoot);
    for (const auto& binding : mFlipbookFollowers) {
        applyFlipbookFollower(binding.controllerTrack, binding.follower, binding.resolvedPages);
    }
}

static BoundingBox* findBoxByName(ContainerBox* container, const QString& name) {
    for (auto* box : container->getContainedBoxes()) {
        if (box->prp_getName() == name) return box;
        if (box->property("svgElementId").toString() == name) return box;
        if (const auto sub = enve_cast<ContainerBox*>(box)) {
            if (auto* found = findBoxByName(sub, name)) return found;
        }
    }
    return nullptr;
}

static void syncFollowerLeaf(QrealAnimator* src, QrealAnimator* dst) {
    dst->setCurrentBaseValue(src->getEffectiveValue());
}

static void syncFollowerTransform(ComplexAnimator* src, ComplexAnimator* dst) {
    QMap<QString, Property*> dstByName;
    for (int i = 0; i < dst->ca_getNumberOfChildren(); i++) {
        auto* p = dst->ca_getChildAt(i);
        dstByName[p->prp_getName()] = p;
    }
    for (int i = 0; i < src->ca_getNumberOfChildren(); i++) {
        auto* sp = src->ca_getChildAt(i);
        auto it = dstByName.find(sp->prp_getName());
        if (it == dstByName.end()) continue;
        auto* tp = it.value();
        if (const auto srcReal = enve_cast<QrealAnimator*>(sp)) {
            if (const auto dstReal = enve_cast<QrealAnimator*>(tp)) {
                syncFollowerLeaf(srcReal, dstReal);
            }
        } else if (const auto srcNested = enve_cast<ComplexAnimator*>(sp)) {
            if (const auto dstNested = enve_cast<ComplexAnimator*>(tp)) {
                syncFollowerTransform(srcNested, dstNested);
            }
        }
    }
}

static void applyFollowerTransform(BoundingBox* controller, BoundingBox* follower) {
    qCDebug(lcSvgFollower) << "applyFollowerTransform controller:" << controller->prp_getName()
                           << "-> follower:" << follower->prp_getName();
    Property* srcTransform = nullptr;
    Property* dstTransform = nullptr;
    for (int i = 0; i < controller->ca_getNumberOfChildren(); i++) {
        auto* p = controller->ca_getChildAt(i);
        if (p->prp_getName() == "transform") { srcTransform = p; break; }
    }
    for (int i = 0; i < follower->ca_getNumberOfChildren(); i++) {
        auto* p = follower->ca_getChildAt(i);
        if (p->prp_getName() == "transform") { dstTransform = p; break; }
    }
    if (!srcTransform || !dstTransform) return;
    const auto srcCA = enve_cast<ComplexAnimator*>(srcTransform);
    const auto dstCA = enve_cast<ComplexAnimator*>(dstTransform);
    if (!srcCA || !dstCA) return;
    syncFollowerTransform(srcCA, dstCA);
}

void SvgLinkBox::collectFollowerDescs(ContainerBox* svgRoot,
                                       ContainerBox* container) {
    for (auto* box : container->getContainedBoxes()) {
        for (const auto& doc : box->getDescYaml()) {
            if (!doc.isYaml) continue;
            try {
                const auto node = YAML::Load(doc.content.toStdString());
                if (!node["kind"] || node["kind"].as<std::string>() != "animation-follower") continue;
                if (!node["controller"]) break;
                const QString controllerName =
                    QString::fromStdString(node["controller"].as<std::string>());
                BoundingBox* controller = findBoxByName(svgRoot, controllerName);
                if (controller) {
                    qCDebug(lcSvgFollower) << "follower" << box->prp_getName()
                                          << "-> controller" << controllerName;
                    mFollowers.append(FollowerBinding{box, controller});
                } else {
                    qCWarning(lcSvgFollower) << "follower" << box->prp_getName()
                                            << "controller not found:" << controllerName;
                }
                break;
            } catch (...) {}
        }
        if (const auto sub = enve_cast<ContainerBox*>(box))
            collectFollowerDescs(svgRoot, sub);
    }
}

static void applyFlipbookFollower(SvgFlipbookTrack* controllerTrack,
                                   BoundingBox* follower,
                                   const QMap<int, BoundingBox*>& resolvedPages) {
    const int idx = controllerTrack->currentPageIndex();
    qCDebug(lcSvgFollower) << "applyFlipbookFollower controller:" << controllerTrack->prp_getName()
                           << "-> follower:" << follower->prp_getName()
                           << "index:" << idx;
    for (auto it = resolvedPages.begin(); it != resolvedPages.end(); ++it) {
        if (BoundingBox* box = it.value()) box->setVisibleFromAnimation(it.key() == idx);
    }
}

void SvgLinkBox::collectFlipbookFollowerDescs(ContainerBox* svgRoot,
                                               ContainerBox* container) {
    for (auto* box : container->getContainedBoxes()) {
        for (const auto& doc : box->getDescYaml()) {
            if (!doc.isYaml) continue;
            try {
                const auto node = YAML::Load(doc.content.toStdString());
                if (!node["kind"] || node["kind"].as<std::string>() != "flipbook-follower") continue;
                if (!node["controller"] || !node["map"]) break;
                const QString controllerName =
                    QString::fromStdString(node["controller"].as<std::string>());
                SvgFlipbookTrack* controllerTrack = nullptr;
                for (const auto& track : mFlipbookTracks) {
                    if (track->prp_getName() == controllerName) { controllerTrack = track.get(); break; }
                }
                if (!controllerTrack) {
                    qCWarning(lcSvgFollower) << "flipbook-follower" << box->prp_getName()
                                            << "controller not found:" << controllerName;
                    break;
                }
                QMap<int, BoundingBox*> resolvedPages;
                for (const auto& entry : node["map"]) {
                    const int page = entry.first.as<int>();
                    if (entry.second.IsNull()) continue;
                    const QString childName =
                        QString::fromStdString(entry.second.as<std::string>());
                    if (childName.isEmpty()) continue;
                    BoundingBox* child = findBoxByName(svgRoot, childName);
                    if (child) {
                        resolvedPages[page] = child;
                    } else {
                        qCWarning(lcSvgFollower) << "flipbook-follower" << box->prp_getName()
                                                << "page" << page << "child not found:" << childName;
                    }
                }
                qCDebug(lcSvgFollower) << "flipbook-follower" << box->prp_getName()
                                      << "-> controller" << controllerName
                                      << "pages:" << resolvedPages.keys();
                mFlipbookFollowers.append(FlipbookFollowerBinding{box, controllerTrack, resolvedPages});
                break;
            } catch (const std::exception& e) {
                qCWarning(lcSvgFollower) << "flipbook-follower: YAML parse failed for box"
                                        << box->prp_getName() << ":" << e.what();
            } catch (...) {
                qCWarning(lcSvgFollower) << "flipbook-follower: unknown exception parsing"
                                            " desc for box" << box->prp_getName();
            }
        }
        if (const auto sub = enve_cast<ContainerBox*>(box))
            collectFlipbookFollowerDescs(svgRoot, sub);
    }
}

// Label-convention follower: `?kind=follow&controller={name}`. Unlike the
// legacy YAML scheme, there is a single follower kind - which of the two
// bindings (transform-mirror vs flipbook-page-mirror) it becomes depends on
// the *controller*'s own effective kind, resolved here. A controller with
// no recognized kind, or itself declared `follow` (chained following), is
// a hard error: not supported, logged rather than silently skipped.
void SvgLinkBox::collectFollowLabelDescs(ContainerBox* svgRoot,
                                          ContainerBox* container) {
    for (auto* box : container->getContainedBoxes()) {
        const auto query = parseSvgLabel(
                    box->property("svgInkscapeLabelRaw").toString());
        if (query.kind == QStringLiteral("follow")) {
            if (!query.hasController || query.controller.isEmpty()) {
                qCWarning(lcSvgFollower) << "follow" << box->prp_getName()
                                        << "missing controller=";
            } else {
                BoundingBox* controllerBox = findBoxByName(svgRoot, query.controller);
                if (!controllerBox) {
                    qCWarning(lcSvgFollower) << "follow" << box->prp_getName()
                                            << "controller not found:" << query.controller;
                } else {
                    const QString controllerKind = effectiveKindOf(controllerBox);
                    if (controllerKind == QStringLiteral("animation")) {
                        qCDebug(lcSvgFollower) << "follow" << box->prp_getName()
                                              << "-> animation controller" << query.controller;
                        mFollowers.append(FollowerBinding{box, controllerBox});
                    } else if (controllerKind == QStringLiteral("flipbook")) {
                        SvgFlipbookTrack* controllerTrack = nullptr;
                        for (const auto& track : mFlipbookTracks) {
                            if (track->prp_getName() == query.controller) {
                                controllerTrack = track.get();
                                break;
                            }
                        }
                        if (!controllerTrack) {
                            qCWarning(lcSvgFollower) << "follow" << box->prp_getName()
                                                    << "controller declares kind=flipbook but"
                                                       " has no resolved flipbook track:"
                                                    << query.controller;
                        } else {
                            // Unlike a flipbook's own pages, a follower's
                            // page children are placeholders whose *name*
                            // is the locator for the real target elsewhere
                            // in the tree (mirroring the legacy `map:`
                            // field's arbitrary-locator semantics) - so
                            // each child is re-resolved by name here,
                            // deliberately, rather than shown directly.
                            QMap<int, BoundingBox*> pageChildren;
                            if (!collectPageChildren(enve_cast<ContainerBox*>(box), pageChildren)) {
                                qCWarning(lcSvgFollower) << "follow" << box->prp_getName()
                                                        << "duplicate page= among its own"
                                                           " children, not following";
                            } else {
                                QMap<int, BoundingBox*> resolvedPages;
                                for (auto it = pageChildren.begin(); it != pageChildren.end(); ++it) {
                                    const QString childName = it.value()->prp_getName();
                                    BoundingBox* child = findBoxByName(svgRoot, childName);
                                    if (child) {
                                        resolvedPages[it.key()] = child;
                                    } else {
                                        qCWarning(lcSvgFollower) << "follow" << box->prp_getName()
                                                                << "page" << it.key()
                                                                << "child not found:" << childName;
                                    }
                                }
                                qCDebug(lcSvgFollower) << "follow" << box->prp_getName()
                                                      << "-> flipbook controller" << query.controller
                                                      << "pages:" << resolvedPages.keys();
                                mFlipbookFollowers.append(
                                    FlipbookFollowerBinding{box, controllerTrack, resolvedPages});
                            }
                        }
                    } else {
                        qCWarning(lcSvgFollower) << "follow" << box->prp_getName()
                                                << "controller" << query.controller
                                                << "has no kind=animation/flipbook (kind="
                                                << controllerKind << "), cannot follow it";
                    }
                }
            }
        }
        if (const auto sub = enve_cast<ContainerBox*>(box))
            collectFollowLabelDescs(svgRoot, sub);
    }
}

void SvgLinkBox::applyFlipbookDescIfPresent(BoundingBox* box,
                                             QSet<QString>& liveOwnerIds) {
    const auto boxAsContainer = enve_cast<ContainerBox*>(box);
    qCDebug(lcSvgFlipbookTrack) << "collectFlipbookDescs: visiting box"
                                << box->prp_getName()
                                << "isContainer:" << (bool)boxAsContainer
                                << "childCount:" << (boxAsContainer ? boxAsContainer->getContainedBoxesCount() : -1)
                                << "descDocs:" << box->getDescYaml().count();
    for (const auto& doc : box->getDescYaml()) {
        qCDebug(lcSvgFlipbookTrack) << "collectFlipbookDescs: box" << box->prp_getName()
                                    << "desc isYaml:" << doc.isYaml
                                    << "content:" << doc.content;
        if (!doc.isYaml) continue;
        const QString ownerId = box->prp_getName();
        try {
            const auto node = YAML::Load(doc.content.toStdString());
            if (!node["kind"] || node["kind"].as<std::string>() != "flipbook") continue;
            // Owner still declares a flipbook desc, even if it fails to
            // parse below: don't let a malformed map cause this track
            // to be mistaken for a renamed-away/stale one.
            liveOwnerIds.insert(ownerId);
            if (!node["map"]) break;
            QMap<int, QString> pageMap;
            for (const auto& entry : node["map"])
                pageMap[entry.first.as<int>()] =
                    QString::fromStdString(entry.second.as<std::string>());
            if (pageMap.isEmpty()) break;
            qCDebug(lcSvgFlipbookTrack) << "collectFlipbookDescs: resolved flipbook desc for"
                                        << ownerId << "pages:" << pageMap;
            SvgFlipbookTrack* existing = nullptr;
            for (const auto& track : mFlipbookTracks) {
                if (track->prp_getName() == ownerId) { existing = track.get(); break; }
            }
            if (!existing) {
                auto track = enve::make_shared<SvgFlipbookTrack>(ownerId);
                wireFlipbookTrack(track);
                existing = track.get();
            }
            existing->setOwnerBox(enve_cast<ContainerBox*>(box));
            existing->setPageMap(pageMap);
            break;
        } catch (const std::exception& e) {
            qCWarning(lcSvgFlipbookTrack) << "collectFlipbookDescs: YAML parse failed for box"
                                          << ownerId << ":" << e.what();
        } catch (...) {
            qCWarning(lcSvgFlipbookTrack) << "collectFlipbookDescs: unknown exception parsing"
                                              " desc for box" << ownerId;
        }
    }
}

// Collects `owner`'s direct children keyed by their own `?page=N` label
// (the label-convention replacement for a `map:` field) - returning the
// child boxes themselves, not a name to re-resolve later: unlike a YAML
// `map:` locator (an arbitrary string that can point anywhere in the
// tree), a label-convention page's target is definitionally the direct
// child it was scanned from. A duplicate page index across siblings is a
// hard error: the whole result is discarded (empty, `false` returned)
// rather than silently picking one - matching the "know when something
// didn't resolve" error posture agreed for this convention. Gaps/
// non-contiguous indices are fine.
static bool collectPageChildren(ContainerBox* owner,
                                 QMap<int, BoundingBox*>& outPages) {
    outPages.clear();
    if (!owner) return true;
    QMap<int, BoundingBox*> pages;
    bool duplicate = false;
    for (auto* child : owner->getContainedBoxes()) {
        const auto query = parseSvgLabel(
                    child->property("svgInkscapeLabelRaw").toString());
        if (query.pageMalformed) {
            qCWarning(lcSvgFlipbookTrack) << "malformed page= on"
                                          << child->prp_getName()
                                          << "of" << owner->prp_getName()
                                          << "- ignoring";
        }
        if (!query.hasPage) continue;
        const auto it = pages.find(query.page);
        if (it != pages.end()) {
            qCWarning(lcSvgFlipbookTrack) << "duplicate page=" << query.page
                                          << "on children" << it.value()->prp_getName()
                                          << "and" << child->prp_getName()
                                          << "of" << owner->prp_getName();
            duplicate = true;
            continue;
        }
        pages[query.page] = child;
    }
    if (duplicate) return false;
    outPages = pages;
    return true;
}

void SvgLinkBox::applyFlipbookLabelIfPresent(BoundingBox* box,
                                              QSet<QString>& liveOwnerIds) {
    const auto query = parseSvgLabel(
                box->property("svgInkscapeLabelRaw").toString());
    if (query.kind != QStringLiteral("flipbook")) return;
    const QString ownerId = box->prp_getName();
    // Owner still declares itself a flipbook even if its page map below
    // turns out empty/invalid - same "don't mistake malformed metadata for
    // a renamed-away track" reasoning as applyFlipbookDescIfPresent.
    liveOwnerIds.insert(ownerId);
    QMap<int, BoundingBox*> pages;
    if (!collectPageChildren(enve_cast<ContainerBox*>(box), pages)) return;
    if (pages.isEmpty()) return;
    SvgFlipbookTrack* existing = nullptr;
    for (const auto& track : mFlipbookTracks) {
        if (track->prp_getName() == ownerId) { existing = track.get(); break; }
    }
    if (!existing) {
        auto track = enve::make_shared<SvgFlipbookTrack>(ownerId);
        wireFlipbookTrack(track);
        existing = track.get();
    }
    existing->setOwnerBox(enve_cast<ContainerBox*>(box));
    existing->setResolvedPagesDirect(pages);
}

void SvgLinkBox::collectFlipbookDescs(ContainerBox* container,
                                       QSet<QString>& liveOwnerIds) {
    for (auto* box : container->getContainedBoxes()) {
        applyFlipbookDescIfPresent(box, liveOwnerIds);
        applyFlipbookLabelIfPresent(box, liveOwnerIds);
        if (const auto sub = enve_cast<ContainerBox*>(box))
            collectFlipbookDescs(sub, liveOwnerIds);
    }
}

void SvgLinkBox::applyPivotDescIfPresent(BoundingBox* box) {
    qCDebug(lcSvgPivot) << "collectPivotDescs: visiting box"
                        << box->prp_getName()
                        << "svgElementId=" << box->property("svgElementId").toString()
                        << "mCenterPivotPlanned=" << box->isCenterPivotPlanned()
                        << "hasSvgPivotPos=" << box->property("svgPivotPos").isValid();
    const QVariant pivotVar = box->property("svgPivotPos");
    if (!pivotVar.isValid()) return;
    const QPointF pivot = pivotVar.toPointF();
    const QString boxName = box->prp_getName();
    const QString svgId = box->property("svgElementId").toString();
    qCDebug(lcSvgPivot) << "collectPivotDescs: found pivot" << pivot
                        << "on box" << boxName << "svgId=" << svgId;
    bool found = false;
    for (const auto& track : mElementTracks) {
        const QString tName = track->prp_getName();
        if (tName == boxName || (!svgId.isEmpty() && tName == svgId)) {
            found = true;
            auto* target = track->resolvedTarget();
            if (!target) {
                qCDebug(lcSvgPivot) << "collectPivotDescs: track" << tName
                                    << "has no resolved target";
                break;
            }
            auto* transformAdv = enve_cast<AdvancedTransformAnimator*>(
                target->getTransformAnimator());
            if (!transformAdv) {
                qCDebug(lcSvgPivot) << "collectPivotDescs: no AdvancedTransformAnimator on"
                                    << tName;
                break;
            }
            qCDebug(lcSvgPivot) << "collectPivotDescs: target=" << target->prp_getName()
                                << "mCenterPivotPlanned=" << target->isCenterPivotPlanned()
                                << "pivot before=" << transformAdv->getPivotAnimator()->getEffectiveValue();
            target->cancelPlannedCenterPivot();
            // The box's position/rotation/scale were decomposed from its SVG
            // matrix assuming pivot (0,0) (see MatrixDecomposition::decompose),
            // so that decomposition already bakes in a translate compensating
            // for any off-origin rotation. Moving the pivot to svgPivotPos
            // without re-deriving position from decomposePivoted would apply
            // that compensation a second time - doubling the box's translation
            // when svgPivotPos matches the SVG's original rotation center, as
            // it typically does for artist-placed pivot markers.
            const QMatrix currentTransform = target->getRelativeTransformAtCurrentFrame();
            const auto values = MatrixDecomposition::decomposePivoted(currentTransform, pivot);
            transformAdv->setValues(values);
            qCDebug(lcSvgPivot) << "collectPivotDescs: set pivot on" << tName
                                << "=" << pivot
                                << "pivot after=" << transformAdv->getPivotAnimator()->getEffectiveValue()
                                << "mCenterPivotPlanned after cancel=" << target->isCenterPivotPlanned();
            break;
        }
    }
    if (!found) {
        qCDebug(lcSvgPivot) << "collectPivotDescs: no element track found for"
                            << boxName << "(svgId=" << svgId << ")";
    }
}

void SvgLinkBox::collectPivotDescs(ContainerBox* container) {
    for (auto* box : container->getContainedBoxes()) {
        applyPivotDescIfPresent(box);
        if (const auto sub = enve_cast<ContainerBox*>(box))
            collectPivotDescs(sub);
    }
}

void SvgLinkBox::addElementTrack(const QString& targetId) {
    auto track = enve::make_shared<SvgElementTrack>(targetId);
    wireTrack(track);
    ContainerBox* svgRoot = nullptr;
    const auto& contained = getContainedBoxes();
    if (!contained.isEmpty()) svgRoot = enve_cast<ContainerBox*>(contained.first());
    BoundingBox* targetBox = track->resolveTarget(svgRoot);
    if (targetBox) {
        track->reconcileWithTarget(targetBox);
        track->initFromTarget(targetBox);
    }
}

void SvgLinkBox::wireTrack(const qsptr<SvgElementTrack>& track) {
    mElementTracks << track;
    track->setParent(this);
    connect(track.get(), &Property::prp_nameChanged,
            this, [this](const QString&) { resolveElementTracks(); });
    connect(track.get(), &SvgElementTrack::deleteRequested,
            this, [this, t = track.get()]() { removeElementTrack(t); });
    connect(track.get(), &SvgElementTrack::captured,
            this, [this](BoundingBox* controller) {
                for (const auto& binding : mFollowers) {
                    if (binding.controller == controller) {
                        applyFollowerTransform(binding.controller, binding.follower);
                    }
                }
            });
    const int swtId = ca_getNumberOfChildren() + mElementTracks.count() - 1;
    SWT_addChildAt(track.get(), swtId);
}

void SvgLinkBox::wireFlipbookTrack(const qsptr<SvgFlipbookTrack>& track) {
    mFlipbookTracks << track;
    track->setParent(this);
    connect(track.get(), &SvgFlipbookTrack::deleteRequested,
            this, [this, t = track.get()]() { removeFlipbookTrack(t); });
    connect(track.get(), &SvgFlipbookTrack::pageChanged,
            this, [this, t = track.get()]() {
                for (const auto& binding : mFlipbookFollowers) {
                    if (binding.controllerTrack == t) {
                        applyFlipbookFollower(binding.controllerTrack, binding.follower,
                                              binding.resolvedPages);
                    }
                }
            });
    const int swtId = ca_getNumberOfChildren()
                      + mElementTracks.count()
                      + mFlipbookTracks.count() - 1;
    SWT_addChildAt(track.get(), swtId);
}

void SvgLinkBox::removeFlipbookTrack(SvgFlipbookTrack* track) {
    for (int i = 0; i < mFlipbookTracks.count(); i++) {
        if (mFlipbookTracks[i].get() == track) {
            SWT_removeChild(track);
            mFlipbookTracks.removeAt(i);
            for (int j = mFlipbookFollowers.count() - 1; j >= 0; j--) {
                if (mFlipbookFollowers[j].controllerTrack == track)
                    mFlipbookFollowers.removeAt(j);
            }
            return;
        }
    }
}

void SvgLinkBox::anim_setAbsFrame(const int frame) {
    SvgLinkBoxBase::anim_setAbsFrame(frame);
    qCDebug(lcSvgElementTrack) << "anim_setAbsFrame" << frame
                               << "tracks:" << mElementTracks.count();
    for (const auto& track : mElementTracks) {
        track->anim_setAbsFrame(frame);
        auto* target = track->resolvedTarget();
        qCDebug(lcSvgElementTrack) << "  track:" << track->prp_getName()
                                   << "resolvedTarget:" << (target ? target->prp_getName() : "(null)");
        if (target) {
            track->syncToTarget(target);
        }
    }
    for (const auto& track : mFlipbookTracks) {
        track->anim_setAbsFrame(frame);
        track->syncToTargets();
    }
    for (const auto& binding : mFlipbookFollowers) {
        applyFlipbookFollower(binding.controllerTrack, binding.follower, binding.resolvedPages);
    }
    for (const auto& binding : mFollowers) {
        qCDebug(lcSvgFollower) << "anim_setAbsFrame" << frame
                               << "follower:" << binding.follower->prp_getName()
                               << "controller:" << binding.controller->prp_getName();
        applyFollowerTransform(binding.controller, binding.follower);
    }
}

void SvgLinkBox::removeElementTrack(SvgElementTrack* track) {
    for (int i = 0; i < mElementTracks.count(); i++) {
        if (mElementTracks[i].get() == track) {
            SWT_removeChild(track);
            mElementTracks.removeAt(i);
            return;
        }
    }
}

void SvgLinkBox::writeBoundingBox(eWriteStream& dst) const {
    dst << (qint32)mElementTracks.count();
    for (const auto& track : mElementTracks) {
        track->writeTrack(dst);
    }
    dst << (qint32)mFlipbookTracks.count();
    for (const auto& track : mFlipbookTracks) {
        track->writeTrack(dst);
    }
    dst << mManualName;
    SvgLinkBoxBase::writeBoundingBox(dst);
}

void SvgLinkBox::readBoundingBox(eReadStream& src) {
    if (src.evFileVersion() >= EvFormat::svgLinkElementTrackData) {
        qint32 count; src >> count;
        for (int i = 0; i < count; i++) {
            auto track = enve::make_shared<SvgElementTrack>("");
            track->readTrack(src);
            wireTrack(track);
        }
    } else if (src.evFileVersion() >= EvFormat::svgLinkElementTracks) {
        qint32 count; src >> count;
        for (int i = 0; i < count; i++) {
            QString id; src >> id;
            addElementTrack(id);
        }
    }
    if (src.evFileVersion() >= EvFormat::svgLinkFlipbookTracks) {
        qint32 count; src >> count;
        for (int i = 0; i < count; i++) {
            auto track = enve::make_shared<SvgFlipbookTrack>("");
            track->readTrack(src);
            wireFlipbookTrack(track);
        }
    }
    if (src.evFileVersion() >= EvFormat::svgLinkManualName) {
        src >> mManualName;
    } else {
        mMigrateLegacyName = true;
    }
    SvgLinkBoxBase::readBoundingBox(src);
}

void SvgLinkBox::SWT_setupAbstraction(SWT_Abstraction* abstraction,
                                       const UpdateFuncs& updateFuncs,
                                       const int visiblePartWidgetId) {
    SvgLinkBoxBase::SWT_setupAbstraction(abstraction, updateFuncs, visiblePartWidgetId);
    for (const auto& track : mElementTracks) {
        auto abs = track->SWT_abstractionForWidget(updateFuncs, visiblePartWidgetId);
        abstraction->addChildAbstraction(abs->ref<SWT_Abstraction>());
    }
    for (const auto& track : mFlipbookTracks) {
        auto abs = track->SWT_abstractionForWidget(updateFuncs, visiblePartWidgetId);
        abstraction->addChildAbstraction(abs->ref<SWT_Abstraction>());
    }
}

void SvgLinkBox::prp_setupTreeViewMenu(PropertyMenu* menu) {
    if (!menu->hasActionsForType<SvgLinkBox>()) {
        menu->addedActionsForType<SvgLinkBox>();

        const auto parentWidget = menu->getParentWidget();
        menu->addPlainAction(QIcon::fromTheme("plus"),
                             QObject::tr("Add Element Track"),
                             [this, parentWidget]() {
                                 bool ok;
                                 const QString id = QInputDialog::getText(
                                             parentWidget,
                                             QObject::tr("Add Element Track"),
                                             QObject::tr("SVG element id:"),
                                             QLineEdit::Normal, "", &ok);
                                 if (ok && !id.isEmpty()) addElementTrack(id);
                             });

        menu->addSeparator();
    }
    SvgLinkBoxBase::prp_setupTreeViewMenu(menu);
}

QDomElement SvgLinkBox::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = SvgLinkBoxBase::prp_writePropertyXEV_impl(exp);
    // Written unconditionally (even when empty) so its mere presence lets
    // prp_readPropertyXEV_impl tell "saved by name-aware code, genuinely
    // auto" apart from "legacy file, name field is just whatever was set".
    result.setAttribute("manualName", mManualName);
    if (!mElementTracks.isEmpty()) {
        auto tracksEle = exp.createElement("ElementTracks");
        for (const auto& track : mElementTracks) {
            auto trackEle = exp.createElement("Track");
            trackEle.setAttribute("targetId", track->prp_getName());
            tracksEle.appendChild(trackEle);
        }
        result.appendChild(tracksEle);
    }
    if (!mFlipbookTracks.isEmpty()) {
        auto tracksEle = exp.createElement("FlipbookTracks");
        for (const auto& track : mFlipbookTracks) {
            auto trackEle = exp.createElement("FlipbookTrack");
            trackEle.setAttribute("ownerId", track->prp_getName());
            tracksEle.appendChild(trackEle);
        }
        result.appendChild(tracksEle);
    }
    return result;
}

void SvgLinkBox::prp_readPropertyXEV_impl(const QDomElement& ele,
                                           const XevImporter& imp) {
    if (ele.hasAttribute("manualName")) {
        mManualName = ele.attribute("manualName");
    } else {
        mMigrateLegacyName = true;
    }
    const auto tracksEle = ele.firstChildElement("ElementTracks");
    if (!tracksEle.isNull()) {
        auto trackEle = tracksEle.firstChildElement("Track");
        while (!trackEle.isNull()) {
            const QString id = trackEle.attribute("targetId");
            if (!id.isEmpty()) addElementTrack(id);
            trackEle = trackEle.nextSiblingElement("Track");
        }
    }
    const auto flipbookTracksEle = ele.firstChildElement("FlipbookTracks");
    if (!flipbookTracksEle.isNull()) {
        auto trackEle = flipbookTracksEle.firstChildElement("FlipbookTrack");
        while (!trackEle.isNull()) {
            const QString id = trackEle.attribute("ownerId");
            if (!id.isEmpty()) {
                auto track = enve::make_shared<SvgFlipbookTrack>(id);
                wireFlipbookTrack(track);
            }
            trackEle = trackEle.nextSiblingElement("FlipbookTrack");
        }
    }
    SvgLinkBoxBase::prp_readPropertyXEV_impl(ele, imp);
}

void SvgLinkBox::fileHandlerConnector(ConnContext &conn, SvgFileCacheHandler *obj) {
    if(obj) {
        conn << connect(obj, &SvgFileCacheHandler::pathChanged,
                        this, &SvgLinkBox::updateContent);
        conn << connect(obj, &SvgFileCacheHandler::reloaded,
                        this, &SvgLinkBox::updateContent);
    }
}

void SvgLinkBox::fileHandlerAfterAssigned(SvgFileCacheHandler *obj) {
    Q_UNUSED(obj)
    updateContent();
}
