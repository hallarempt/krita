SET(PREFIX_ext_vc "${EXTPREFIX}" )
ExternalProject_Add( ext_vc
	DOWNLOAD_DIR ${EXTERNALS_DOWNLOAD_DIR}
	URL http://files.kde.org/krita/build/dependencies/Vc-0.7.5.tar.gz
        URL_MD5 59854e2381b78d602b4a00e459e3d333

	INSTALL_DIR ${PREFIX_ext_vc}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PREFIX_ext_vc} -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=${GLOBAL_BUILD_TYPE} ${GLOBAL_PROFILE}

	UPDATE_COMMAND ""
	ALWAYS 0
)

