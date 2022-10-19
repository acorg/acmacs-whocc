#! /usr/bin/env bash
set -o errexit -o errtrace -o pipefail -o nounset
echo '======================================================================'
date
echo '======================================================================'
cd $(dirname $0)/..
make -C ${ACMACSD_ROOT}/sources/acmacs-chart-2 &&
make -C ${ACMACSD_ROOT}/sources/acmacs-py &&
make -C ${ACMACSD_ROOT}/sources/acmacs-whocc &&
make -C ${ACMACSD_ROOT}/sources/acmacs-whocc-data &&
make -C ${ACMACSD_ROOT}/sources/acmacs-whocc/web/chains-202105 CHAINS_ROOT=/syn/eu/ac/results/chains-202105 &&
mkdir -p /syn/eu/ac/results/chains-202105/lib &&
rsync -av --copy-unsafe-links --exclude cmake --exclude pkgconfig --exclude '.nfs*' ${ACMACSD_ROOT}/lib/ /syn/eu/ac/results/chains-202105/lib &&
rsync -av --copy-unsafe-links --exclude cmake --exclude pkgconfig --exclude '.nfs*' ${ACMACSD_ROOT}/py/ /syn/eu/ac/results/chains-202105/lib &&
rsync -av --copy-unsafe-links ${ACMACSD_ROOT}/share/conf/clades.mapi /syn/eu/ac/results/chains-202105/ &&
find . -type d -exec chmod g+s {} \; &&
find . -perm -u+x -a \! -type d -a \! -type l -exec chmod g+w {} \;
echo
echo
