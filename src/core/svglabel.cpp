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

#include "svglabel.h"

#include <QStringList>

SvgLabelQuery parseSvgLabel(const QString& label) {
    SvgLabelQuery result;
    const int qIdx = label.indexOf('?');
    if (qIdx < 0) {
        result.baseName = label;
        return result;
    }
    result.baseName = label.left(qIdx);
    const QString queryPart = label.mid(qIdx + 1);
    const QStringList pairs = queryPart.split('&', Qt::SkipEmptyParts);
    for (const QString& pair : pairs) {
        const int eqIdx = pair.indexOf('=');
        const QString key = eqIdx >= 0 ? pair.left(eqIdx) : pair;
        const QString value = eqIdx >= 0 ? pair.mid(eqIdx + 1) : QString();
        if (key == QStringLiteral("kind")) {
            result.kind = value;
        } else if (key == QStringLiteral("controller")) {
            result.controller = value;
            result.hasController = true;
        } else if (key == QStringLiteral("page")) {
            bool ok = false;
            const int page = value.toInt(&ok);
            if (ok) {
                result.page = page;
                result.hasPage = true;
            }
        }
    }
    return result;
}
