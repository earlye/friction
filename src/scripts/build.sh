#!/bin/bash
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

# This is just a run_docker.sh wrapper for GitHub Actions CI

set -e -x

CWD=`pwd`
MKJOBS=${MKJOBS:-4}
COMMIT=${COMMIT:-`git rev-parse --short=8 HEAD`}
BRANCH=${BRANCH:-`git rev-parse --abbrev-ref HEAD`}
GHA_RUN_NUMBER=${GHA_RUN_NUMBER:-0}
BUILD_ORIGIN=${BUILD_ORIGIN:-ci}
HEAD_REPO_URL=${HEAD_REPO_URL:-""}
REL=${REL:-0}
DOWNLOAD_SDK=${DOWNLOAD_SDK:-1}
SDK_VERSION=${SDK_VERSION:-"1.0.0"}
SDK_REV=${SDK_REV:-"r11"}

SDK_VERSION=${SDK_VERSION} SDK_REV=${SDK_REV} DOWNLOAD_SDK=${DOWNLOAD_SDK} HEAD_REPO_URL=${HEAD_REPO_URL} LOCAL_BUILD=0 MKJOBS=${MKJOBS} REL=${REL} BRANCH=${BRANCH} COMMIT=${COMMIT} CUSTOM=${CUSTOM} GHA_RUN_NUMBER=${GHA_RUN_NUMBER} BUILD_ORIGIN=${BUILD_ORIGIN} ./src/scripts/run_docker.sh
