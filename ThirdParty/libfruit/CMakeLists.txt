project(libfruit)

include(ProjectDefaults)
include(ExternalProject)
include(ExternalCMakeArgs)

ExternalProject_Add(
	libfruit

	PREFIX ${PROJECT_DEPS_BUILD_PREFIX}
	GIT_REPOSITORY "https://github.com/google/fruit.git"
	GIT_TAG master
	GIT_PROGRESS TRUE
#	URL "https://github.com/google/fruit/archive/v3.4.0.tar.gz"
#	URL_HASH SHA256=0f3793ee5e437437c3d6360a037866429a7f1975451fd60d740f9d2023e92034
#	DOWNLOAD_NAME "fruit-v3.4.0.tar.gz"
	DOWNLOAD_DIR ${PROJECT_DEPS_TARBALL_DIR}
	DOWNLOAD_NO_PROGRESS 1

	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_INT_DIST_DIR}
		${CMAKE_ARGS_INIT}
		-DFRUIT_TESTS_USE_PRECOMPILED_HEADERS=OFF
		-DCMAKE_BUILD_TYPE=Release
		-DFRUIT_USES_BOOST=OFF
		-DBUILD_SHARED_LIBS=OFF
		-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
)

if(SPV_ENABLE_STATIC)
	export_static_library(fruit)
endif()