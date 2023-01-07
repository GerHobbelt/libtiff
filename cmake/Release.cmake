# Release support
#
# Copyright © 2021 Roger Leigh <rleigh@codelibre.net>
# Written by Roger Leigh <rleigh@codelibre.net>
#
# Permission to use, copy, modify, distribute, and sell this software and
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
#
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
#
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.


# Update version information in root files only when configure.ac has changed.

# Note: Single command "cmake --build . --target tiff_release"
#       does not work correctly, because version information is taken from CMakeCache.txt,
#       because AutotoolsVersion.cmake is not executed then. 

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
	${CMAKE_CURRENT_SOURCE_DIR}/RELEASE-DATE
	COMMAND ${CMAKE_COMMAND}
	"-DSOURCE_DIR:PATH=${PROJECT_SOURCE_DIR}"
	"-DLIBTIFF_VERSION=${PROJECT_VERSION}"
	"-DLIBTIFF_RELEASE_DATE=${LIBTIFF_RELEASE_DATE}"
	"-DLIBTIFF_MAJOR_VERSION=${LIBTIFF_MAJOR_VERSION}"
	"-DLIBTIFF_MINOR_VERSION=${LIBTIFF_MINOR_VERSION}"
	"-DLIBTIFF_MICRO_VERSION=${LIBTIFF_MICRO_VERSION}"
	"-DLIBTIFF_BASIC_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
	"-DLIBTIFF_BASIC_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
	-P "${CMAKE_CURRENT_LIST_DIR}/ReleaseScript.cmake"
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configure.ac
    COMMENT "== Update release version information from configure.ac: ${PROJECT_NAME} ${PROJECT_VERSION} ${LIBTIFF_RELEASE_DATE}")

add_custom_target(tiff_release ALL 
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
	${CMAKE_CURRENT_SOURCE_DIR}/RELEASE-DATE
)

