#!/bin/bash

# install zetasql compiled header files and libs

set -eE

pushd "$(dirname "$0")"
pushd "$(git rev-parse --show-toplevel)"

VERSION=${TAG:-$(git rev-parse --short HEAD)}
export ROOT=$(pwd)
export ZETASQL_LIB_NAME="libzetasql-$VERSION"
export PREFIX="$ROOT/${ZETASQL_LIB_NAME}"

rm -rf tmp-lib libzetasql.mri "${PREFIX}"
mkdir -p tmp-lib
mkdir -p "${PREFIX}"
mkdir -p "${PREFIX}"/lib

install_lib() {
    local file
    file=$1
    local libname
    libname=lib$(echo "$file" | tr '/' '_' | sed -e 's/lib//')

    if [[ "$OSTYPE" == "linux-gnu"* ]]
    then
        INSTALL_BIN="install"
    else
        INSTALL_BIN="ginstall"
    fi
    ${INSTALL_BIN} -D "$file" "$ROOT/tmp-lib/$libname"
}

install_gen_include_file() {
    local file
    file=$1
    local outfile
    outfile=$(echo "$file" | sed -e 's/^.*proto\///')

    if [[ "$OSTYPE" == "linux-gnu"* ]]
    then
        INSTALL_BIN="install"
    else
        INSTALL_BIN="ginstall"
    fi
    ${INSTALL_BIN} -D "$file" "$PREFIX/include/$outfile"
}

install_external_lib() {
    local file
    file=$1
    local libname
    libname=$(basename "$file")
    if [[ "$OSTYPE" == "linux-gnu"* ]]
    then
        INSTALL_BIN="install"
    else
        INSTALL_BIN="ginstall"
    fi
    ${INSTALL_BIN} -D "$file" "$PREFIX/lib/$libname"
}

export -f install_gen_include_file
export -f install_lib
export -f install_external_lib


if [[ "$OSTYPE" == "linux-gnu"* ]]
then
    INSTALL_BIN="install"
else
    INSTALL_BIN="ginstall"
fi

pushd bazel-bin/
find zetasql -type f -iname '*.a' -exec bash -c 'install_lib $0' {} \;

# external lib headers
pushd "$(realpath .)/../../../../../external/com_googlesource_code_re2"
find re2 -iname "*.h" -exec ${INSTALL_BIN} -D {} "$PREFIX"/include/{} \;
popd

pushd "$(realpath .)/../../../../../external/com_googleapis_googleapis"
find google -iname "*.h" -exec ${INSTALL_BIN} -D {} "$PREFIX"/include/{} \;
popd

pushd "$(realpath .)/../../../../../external/com_google_file_based_test_driver"
find file_based_test_driver -iname "*.h" -exec ${INSTALL_BIN} -D {} "$PREFIX"/include/{} \;
popd

# external lib
pushd external

find icu -type f -iname '*.a' -exec bash -c 'install_external_lib $0' {} \;
find com_googlesource_code_re2 -type f -iname '*.a' -exec bash -c 'install_external_lib $0' {} \;
find com_googleapis_googleapis -type f -iname '*.a' -exec bash -c 'install_external_lib $0' {} \;
find com_google_file_based_test_driver -type f -iname '*.a' -exec bash -c 'install_external_lib $0' {} \;

popd

# zetasql generated files: protobuf & template generated files
find zetasql -type f -iname "*.h" -exec ${INSTALL_BIN} -D {} "$PREFIX"/include/{} \;
find zetasql -iregex ".*/_virtual_includes/.*\.h\$" -exec bash -c 'install_gen_include_file $0' {} \;
popd # bazel-bin

# header files from source
find zetasql -type f -iname "*.h" -exec ${INSTALL_BIN} -D {} "$PREFIX"/include/{} \;


if [[ "$OSTYPE" == "linux-gnu"* ]]
then
    echo 'create libzetasql.a' >> libzetasql.mri
    find tmp-lib/ -iname "*.a" -type f -exec bash -c 'echo "addlib $0" >> libzetasql.mri' {} \;
    echo "save" >> libzetasql.mri
    echo "end" >> libzetasql.mri

    ar -M <libzetasql.mri
    ranlib libzetasql.a
    mv libzetasql.a "$PREFIX/lib/"
else
    libtool -static -o libzetasql.a tmp-lib/*.a
    mv libzetasql.a "$PREFIX/lib"
fi

if [[ $OSTYPE = 'darwin'* ]]; then
    tar czf "${ZETASQL_LIB_NAME}-darwin-$(uname -m).tar.gz" "${ZETASQL_LIB_NAME}"/
else
    tar czf "${ZETASQL_LIB_NAME}-$OSTYPE-$(uname -m).tar.gz" "${ZETASQL_LIB_NAME}"/
fi

popd
popd
