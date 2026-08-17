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

set -e -x

CWD=`pwd`
BRANCH=${BRANCH:-`git rev-parse --abbrev-ref HEAD`}
COMMIT=${COMMIT:-`git rev-parse --short=8 HEAD`}
GHA_RUN_NUMBER=${GHA_RUN_NUMBER:-0}
BUILD_ORIGIN=${BUILD_ORIGIN:-local}
OSX=11.0
CPU=`arch`

if [ "${CPU}" = "i386" ]; then
    CPU=x86_64
fi

SDK=${SDK:-"${CWD}/sdk/${CPU}"}
BUILD_DIR=${BUILD_DIR:-"${CWD}/build-release-${CPU}"}

export PATH="${SDK}/bin:/usr/bin:/bin:/usr/sbin:/sbin"
export PKG_CONFIG_PATH="${SDK}/lib/pkgconfig"

whereis python
python --version

whereis ninja
ninja --version

whereis cmake
cmake --version

whereis pkg-config
pkg-config --version

clang --version

if [ -d "${BUILD_DIR}" ]; then
    rm -rf ${BUILD_DIR}
fi
mkdir ${BUILD_DIR} && cd ${BUILD_DIR}

cmake -G Ninja \
-DCMAKE_OSX_DEPLOYMENT_TARGET=${OSX} \
-DMAC_DEPLOY=ON \
-DGIT_COMMIT=${COMMIT} \
-DGIT_BRANCH=${BRANCH} \
-DGHA_RUN_NUMBER=${GHA_RUN_NUMBER} \
-DBUILD_ORIGIN=${BUILD_ORIGIN} \
-DBUILD_SKIA=OFF \
-DSKIA_STATIC=ON \
-DSKIA_LIB_PATH=${SDK}/lib \
-DCMAKE_BUILD_TYPE=Release \
-DQSCINTILLA_INCLUDE_DIRS=${SDK}/include \
-DQSCINTILLA_LIBRARIES_DIRS=${SDK}/lib \
${CWD}

VERSION=`cat version.txt`

cmake --build .

mv src/app/friction.app src/app/Friction.app
macdeployqt src/app/Friction.app

rm -f src/app/Friction.app/Contents/Frameworks/{libQt5MultimediaWidgets.5.dylib,libQt5Svg.5.dylib}
rm -rf src/app/Friction.app/Contents/PlugIns/{bearer,iconengines,imageformats,mediaservice,printsupport,styles}

# disable offline docs for now
#if [ -f "${CWD}/docs/offline/index.html" ]; then
#    cp -a ${CWD}/docs/offline src/app/Friction.app/Contents/Resources/docs
#fi

# Sign with a persistent self-signed identity so `just enable-gha-dmg`
# can register a one-time Gatekeeper allow-rule for it. This is not an
# Apple Developer ID and isn't notarized, so it only helps machines
# that have explicitly trusted this identity, not the general public.
if [ -n "${MACOS_CODESIGN_P12_BASE64:-}" ]; then
    MACOS_CODESIGN_IDENTITY="${MACOS_CODESIGN_IDENTITY:-Friction CI Code Signing}"
    KEYCHAIN="${CWD}/ci-codesign.keychain-db"
    KEYCHAIN_PASS=`openssl rand -base64 24`
    ORIGINAL_KEYCHAINS=`security list-keychains -d user | sed 's/^ *"//; s/" *$//'`

    security create-keychain -p "${KEYCHAIN_PASS}" "${KEYCHAIN}"
    security set-keychain-settings "${KEYCHAIN}"
    security unlock-keychain -p "${KEYCHAIN_PASS}" "${KEYCHAIN}"
    security list-keychains -d user -s "${KEYCHAIN}" ${ORIGINAL_KEYCHAINS}

    echo "${MACOS_CODESIGN_P12_BASE64}" | base64 --decode > "${CWD}/ci-codesign.p12"
    security import "${CWD}/ci-codesign.p12" -k "${KEYCHAIN}" -P "${MACOS_CODESIGN_P12_PASSWORD}" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "${KEYCHAIN_PASS}" "${KEYCHAIN}"
    rm -f "${CWD}/ci-codesign.p12"

    codesign --force --deep --timestamp=none --sign "${MACOS_CODESIGN_IDENTITY}" src/app/Friction.app
    codesign --verify --deep --strict --verbose=2 src/app/Friction.app

    security list-keychains -d user -s ${ORIGINAL_KEYCHAINS}
    security delete-keychain "${KEYCHAIN}"
else
    echo "MACOS_CODESIGN_P12_BASE64 not set — skipping code signing (app will be unsigned)."
fi

mkdir dmg
mv src/app/Friction.app dmg/
(cd dmg ; ln -sf /Applications Applications)

# disable offline docs for now
#if [ -f "${CWD}/docs/offline/index.html" ]; then
#    (cd dmg ; ln -sf Friction.app/Contents/Resources/docs/index.html Documentation.html)
#fi

ARCH_LABEL="${CPU}"
if [ "${CPU}" = "arm64" ]; then
    ARCH_LABEL="arm"
fi
ARCH_VERSION="${VERSION/+/+${ARCH_LABEL}.}"

# https://github.com/actions/runner-images/issues/7522
max_tries=10
i=0
until hdiutil create -volname "Friction" -srcfolder dmg -ov -format ULMO Friction-${ARCH_VERSION}.dmg
do
    if [ $i -eq $max_tries ]; then
        echo 'Error: hdiutil did not succeed even after 10 tries.'
        exit 1
    fi
    i=$((i+1))
done
