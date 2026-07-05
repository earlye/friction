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

#ifndef SVGLABEL_H
#define SVGLABEL_H

#include <QString>

// Parsed form of an `inkscape:label` following the convention
// `{display-name}?kind={animation|flipbook|follow|pivot}[&controller={name}][&page=N]`.
// See issue "use-labels-instead-of-desc-yaml" for the full grammar. The
// first '?' always starts the query string; there is no escaping for a
// literal '?' in a display name.
struct SvgLabelQuery {
    QString baseName;
    QString kind;
    QString controller;
    bool hasController = false;
    int page = 0;
    bool hasPage = false;
};

SvgLabelQuery parseSvgLabel(const QString& label);

#endif // SVGLABEL_H
