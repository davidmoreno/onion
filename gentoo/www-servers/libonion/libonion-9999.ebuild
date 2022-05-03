# Copyright 1999-2022 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: libonion Exp $

EAPI="7"

inherit cmake-multilib cmake-utils git-r3

DESCRIPTION="HTTP server library in C designed to be lightweight"
HOMEPAGE="http://www.coralbits.com/libonion/"

EGIT_REPO_URI="https://github.com/davidmoreno/onion.git"
EGIT_BRANCH=master

LICENSE="LGPL-3,GPL-2,AGPL-3"
SLOT="0"
KEYWORDS=""
IUSE="cxx gnutls threads pam png xml systemd redis examples test"

RDEPEND="
    net-misc/curl
    gnutls? ( net-libs/gnutls )
    pam? ( sys-libs/pam )
    png? ( || ( media-libs/libpng x11-libs/cairo ) )
    xml? ( dev-libs/libxml2 )
    systemd? ( sys-apps/systemd )
    redis? ( dev-libs/hiredis )
    test? ( || ( net-analyzer/netcat net-analyzer/netcat6 ) )
"
DEPEND="${RDEPEND}"

src_configure() {
    local mycmakeargs=(
        -DONION_USE_SSL=$(usex gnutls)
        -DONION_USE_PAM=$(usex pam)
        -DONION_USE_PTHREADS=$(usex threads)
        -DONION_USE_PNG=$(usex png)
        -DONION_USE_XML2=$(usex xml)
        -DONION_USE_SYSTEMD=$(usex systemd)
        -DONION_USE_BINDINGS_CPP=$(usex cxx)
        -DONION_USE_REDIS=$(usex redis)
        -DONION_EXAMPLES=$(usex examples)
        -DONION_USE_TESTS=$(usex test)
    )
    cmake-multilib_src_configure
}

src_test() {
    cmake-multilib_src_test
}

src_install() {
    cmake-multilib_src_install
}
