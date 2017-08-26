#  -*- mode: cmake -*-

#
# Build TPL:  ALQUIMIA 
#   

# --- add the alquimia version to the autogenerated include file
include(${SuperBuild_SOURCE_DIR}/TPLVersions.cmake)

# --- Define all the directories and common external project flags
define_external_project_args(ALQUIMIA
                             TARGET alquimia)

# Alquimia needs PFlotran or CrunchTope
list(APPEND ALQUIMIA_PACKAGE_DEPENDS ${PFLOTRAN_BUILD_TARGET})
list(APPEND ALQUIMIA_PACKAGE_DEPENDS ${CRUNCHTOPE_BUILD_TARGET})

# add version version to the autogenerated tpl_versions.h file
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
  PREFIX ALQUIMIA
  VERSION ${ALQUIMIA_VERSION_MAJOR} ${ALQUIMIA_VERSION_MINOR} ${ALQUIMIA_VERSION_PATCH})
  
# Alquimia and Amanzi disagree about how to find PETSc, so we override 
# Alquimia's method here using a patch command to turn $ENV{var} -> ${var}.
set(ALQUIMIA_PATCH_COMMAND sh ${SuperBuild_TEMPLATE_FILES_DIR}/alquimia-patch-step.sh)

# --- Define the arguments passed to CMake.
set(ALQUIMIA_CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:FILEPATH=${TPL_INSTALL_PREFIX}"
                        "-DBUILD_SHARED_LIBS:BOOL=OFF"
                        "-DPETSC_DIR=${PETSC_DIR}"
                        "-DPETSC_ARCH=."
                        "-DXSDK_WITH_PFLOTRAN:BOOL=${ENABLE_PFLOTRAN}" 
                        "-DTPL_PFLOTRAN_LIBRARIES:FILEPATH=${PFLOTRAN_DIR}/src/pflotran/libpflotranchem.a" 
                        "-DTPL_PFLOTRAN_INCLUDE_DIRS:FILEPATH=${PFLOTRAN_INCLUDE_DIRS}"
 			"-DXSDK_WITH_CRUNCHFLOW:BOOL=${ENABLE_CRUNCHTOPE}"
 			"-DTPL_CRUNCHFLOW_LIBRARIES:FILEPATH=${TPL_INSTALL_PREFIX}/lib/libcrunchchem.a"
 			"-DTPL_CRUNCHFLOW_INCLUDE_DIRS:FILEPATH=${TPL_INSTALL_PREFIX}/lib")

# --- Add external project build and tie to the ALQUIMIA build target
ExternalProject_Add(${ALQUIMIA_BUILD_TARGET}
                    DEPENDS   ${ALQUIMIA_PACKAGE_DEPENDS}             # Package dependency target
                    TMP_DIR   ${ALQUIMIA_tmp_dir}                     # Temporary files directory
                    STAMP_DIR ${ALQUIMIA_stamp_dir}                   # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR ${TPL_DOWNLOAD_DIR}              # Download directory
                    URL          ${ALQUIMIA_URL}                      # URL may be a web site OR a local file
                    URL_MD5      ${ALQUIMIA_MD5_SUM}                  # md5sum of the archive file
                    PATCH_COMMAND ${ALQUIMIA_PATCH_COMMAND}       # Mods to source
                    # -- Configure
                    SOURCE_DIR       ${ALQUIMIA_source_dir}               # Source directory
                    CMAKE_ARGS       ${ALQUIMIA_CMAKE_ARGS}         # CMAKE_CACHE_ARGS or CMAKE_ARGS => CMake configure
                                     ${Amanzi_CMAKE_C_COMPILER_ARGS}  # Ensure uniform build
                                     -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                                     -DCMAKE_Fortran_COMPILER:FILEPATH=${CMAKE_Fortran_COMPILER}
                    # -- Build
                    BINARY_DIR        ${ALQUIMIA_build_dir}           # Build directory 
                    BUILD_COMMAND     $(MAKE)
                    # -- Install
                    INSTALL_DIR      ${TPL_INSTALL_PREFIX}        # Install directory
            		    INSTALL_COMMAND  $(MAKE) install
                    # -- Output control
                    # -- Output control
                    ${ALQUIMIA_logging_args})

include(BuildLibraryName)
build_library_name(alquimia_c ALQUIMIA_C_LIB APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)
build_library_name(alquimia_cutils ALQUIMIA_CUTILS_LIB APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)
build_library_name(alquimia_fortran ALQUIMIA_F_LIB APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)

