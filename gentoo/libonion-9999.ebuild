# Copyright 1999-2012 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: libonion Exp $

EAPI=4

inherit git-2 cmake-utils multilib

DESCRIPTION="HTTP server library in C designed to be lightweight"
HOMEPAGE="http://www.coralbits.com/libonion/"

EGIT_REPO_URI="git://github.com/davidmoreno/onion.git"

LICENSE="LGPL-3,GPL-2,AGPL-3"
SLOT="0"
KEYWORDS=""
IUSE="test cxx gnutls threads pam png xml systemd"

RDEPEND="
    net-misc/curl
    gnutls? ( net-libs/gnutls )
    pam? ( sys-libs/pam )
    png? ( || ( media-libs/libpng x11-libs/cairo ) )
    xml? ( dev-libs/libxml2 )
    systemd? ( sys-apps/systemd )
    "
DEPEND="${RDEPEND}"

src_unpack() {
    git-2_src_unpack
}

src_prepare() {
    epatch "${FILESDIR}"/cmake_lists.patch
    epatch "${FILESDIR}"/new-libpng.patch
}

src_configure() {
    local mycmakeargs=(
        $(cmake-utils_use test    ONION_USE_TESTS)
        $(cmake-utils_use cxx     ONION_USE_BINDINGS_CPP)
        $(cmake-utils_use gnutls  ONION_USE_SSL)
        $(cmake-utils_use threads ONION_USE_PTHREADS)
        $(cmake-utils_use pam     ONION_USE_PAM)
        $(cmake-utils_use png     ONION_USE_PNG)
        $(cmake-utils_use xml     ONION_USE_XML2)
        $(cmake-utils_use systemd ONION_USE_SYSTEMD)
    )
    cmake-utils_src_configure
}

src_test() {
    cmake-utils_src_test
}

src_install() {
    cmake-utils_src_install
}
