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
IUSE=""

RDEPEND="sys-libs/zlib"
DEPEND="${RDEPEND}"

src_unpack() {
	git-2_src_unpack
}

src_configure() {
	local mycmakeargs=(
		-DINSTALL_LIB=/usr/$(get_libdir)
		$(cmake-utils_use_build test TESTS)
	)
	cmake-utils_src_configure
}

src_install() {
	cmake-utils_src_install
}
