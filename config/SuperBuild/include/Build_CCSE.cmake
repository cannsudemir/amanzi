#  -*- mode: cmake -*-

#
# Build TPL:  CCSE 
#  

# --- Define all the directories and common external project flags
define_external_project_args(CCSE
                             TARGET ccse)

# add CCSE version to the autogenerated tpl_versions.h file
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
  PREFIX CCSE
  VERSION ${CCSE_VERSION_MAJOR} ${CCSE_VERSION_MINOR} ${CCSE_VERSION_PATCH})


# --- Define the CMake configure parameters
# Note:
#      CMAKE_CACHE_ARGS requires -DVAR:<TYPE>=VALUE syntax
#      CMAKE_ARGS -DVAR=VALUE OK
# NO WHITESPACE between -D and VAR. Parser blows up otherwise.
#

#convert BOOLSs to INTs, as reqd
set(ENABLE_OpenMP_INT 0)
if (ENABLE_OpenMP)
  set(ENABLE_OpenMP_INT 1)
endif (ENABLE_OpenMP)
message(STATUS "Build CCSE with space dimension ${CCSE_BL_SPACEDIM}")

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)

set(CCSE_CMAKE_CACHE_ARGS
                       -DENABLE_Config_Report:BOOL=TRUE
                       -DENABLE_MPI:INT=1
                       -DENABLE_OpenMP:INT=${ENABLE_OpenMP_INT}
                       -DENABLE_TESTS:BOOL=FALSE
                       -DBL_PRECISION:STRING=DOUBLE
                       -DBL_SPACEDIM:INT=${CCSE_BL_SPACEDIM}
                       -DBL_USE_PARTICLES:INT=0
                       -DCMAKE_INSTALL_PREFIX:PATH=${TPL_INSTALL_PREFIX}
                       -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                       -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
                       -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                       -DCMAKE_C_FLAGS_${BUILD_TYPE_UPPER}:STRING=${Amanzi_COMMON_CFLAGS}
                       -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                       -DCMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER}:STRING=${Amanzi_COMMON_CXXFLAGS}
                       -DCMAKE_CXX_FLAGS_RELEASE:STRING=${Amanzi_COMMON_CXXFLAGS}
                       -DCMAKE_Fortran_COMPILER:FILEPATH=${CMAKE_Fortran_COMPILER}
                       -DCMAKE_Fortran_FLAGS_${BUILD_TYPE_UPPER}:STRING=${Amanzi_COMMON_FCFLAGS}
                       -DMPI_CXX_COMPILER:FILEPATH=${MPI_CXX_COMPILER}
                       -DMPI_C_COMPILER:FILEPATH=${MPI_C_COMPILER}
                       -DMPI_Fortran_COMPILER:FILEPATH=${MPI_Fortran_COMPILER}
                       -DVERBOSE:BOOL=ON)


# --- Set the name of the patch
set(CCSE_patch_file ccse-1.3.4-dependency.patch ccse-1.3.4-tools-compilers.patch
                    ccse-1.3.4-tools-plot1d.patch)
# --- Configure the bash patch script
set(CCSE_sh_patch ${CCSE_prefix_dir}/ccse-patch-step.sh)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/ccse-patch-step.sh.in
               ${CCSE_sh_patch}
               @ONLY)
# --- Configure the CMake patch step
set(CCSE_cmake_patch ${CCSE_prefix_dir}/ccse-patch-step.cmake)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/ccse-patch-step.cmake.in
               ${CCSE_cmake_patch}
               @ONLY)
# --- Set the patch command
set(CCSE_PATCH_COMMAND ${CMAKE_COMMAND} -P ${CCSE_cmake_patch})     

  
# --- Add external project build and tie to the CCSE build target
ExternalProject_Add(${CCSE_BUILD_TARGET}
                    DEPENDS   ${CCSE_PACKAGE_DEPENDS}             # Package dependency target
                    TMP_DIR   ${CCSE_tmp_dir}                     # Temporary files directory
                    STAMP_DIR ${CCSE_stamp_dir}                   # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR ${TPL_DOWNLOAD_DIR}              # Download directory
                    URL          ${CCSE_URL}                      # URL may be a web site OR a local file
                    URL_MD5      ${CCSE_MD5_SUM}                  # md5sum of the archive file
		    PATCH_COMMAND ${CCSE_PATCH_COMMAND}                    
                    # -- Configure
                    SOURCE_DIR       ${CCSE_source_dir}           # Source directory
		    CMAKE_CACHE_ARGS ${AMANZI_CMAKE_CACHE_ARGS}   # Ensure uniform build
		                     ${CCSE_CMAKE_CACHE_ARGS}     
                    # -- Build
                    BINARY_DIR       ${CCSE_build_dir}            # Build directory 
                    BUILD_COMMAND    $(MAKE)                      # $(MAKE) enables parallel builds through make
                    BUILD_IN_SOURCE  ${CCSE_BUILD_IN_SOURCE}      # Flag for in source builds
                    # -- Install
                    INSTALL_DIR      ${TPL_INSTALL_PREFIX}        # Install directory
                    # -- Output control
                    ${CCSE_logging_args}) 

# --- This custom command builds fsnapshot.so, which is a Python module used 
# --- to extract Amanzi-S plot data. It executes after the CCSE library is 
# --- built, builds the module, and copies it into place.
if (ENABLE_CCSE_TOOLS)

  message(STATUS "CCSE: Unwrapping MPI compilers to build shared libraries for python tools")
  if ( CMAKE_C_COMPILER MATCHES "mpi" )
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -show
      OUTPUT_VARIABLE  COMPILE_CMDLINE OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE   COMPILE_CMDLINE ERROR_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE  COMPILER_RETURN
      )
    # Extract the name of the compiler
    if ( COMPILER_RETURN EQUAL 0)
       string(REPLACE " " ";" COMPILE_CMDLINE_LIST ${COMPILE_CMDLINE})
       list(GET COMPILE_CMDLINE_LIST 0 RAW_C_COMPILER)
    else()
       message (FATAL_ERROR "CCSE: Unable to determine the compiler command")
    endif()
  else()
   set(RAW_CC_COMPILER ${CMAKE_C_COMPILER})
  endif()
  message (STATUS "CCSE: RAW_C_COMPILER       = ${RAW_C_COMPILER}")


  if ( CMAKE_CXX_COMPILER MATCHES "mpi")
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -show
      OUTPUT_VARIABLE  COMPILE_CMDLINE OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE   COMPILE_CMDLINE ERROR_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE  COMPILER_RETURN
      )
    # Extract the name of the compiler
    if ( COMPILER_RETURN EQUAL 0)
       string(REPLACE " " ";" COMPILE_CMDLINE_LIST ${COMPILE_CMDLINE})
       list(GET COMPILE_CMDLINE_LIST 0 RAW_CXX_COMPILER)
    else()
       message (FATAL_ERROR "CCSE: Unable to determine the compiler command")
    endif()
  else()
   set(RAW_CXX_COMPILER ${CMAKE_CXX_COMPILER})
  endif()
  message (STATUS "CCSE: RAW_CXX_COMPILER     = ${RAW_CXX_COMPILER}")

  if ( CMAKE_Fortran_COMPILER MATCHES "mpi" )
    execute_process(
      COMMAND ${CMAKE_Fortran_COMPILER} -show
      OUTPUT_VARIABLE  COMPILE_CMDLINE OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE   COMPILE_CMDLINE ERROR_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE  COMPILER_RETURN
      )
    # Extract the name of the compiler
    if ( COMPILER_RETURN EQUAL 0)
       string(REPLACE " " ";" COMPILE_CMDLINE_LIST ${COMPILE_CMDLINE})
       list(GET COMPILE_CMDLINE_LIST 0 RAW_Fortran_COMPILER)
    else()
       message (FATAL_ERROR "CCSE: Unable to determine the compiler command")
    endif()
  else()
   set(RAW_Fortran_COMPILER ${CMAKE_Fortran_COMPILER})
  endif()
  message (STATUS "CCSE: RAW_Fortran_COMPILER = ${RAW_Fortran_COMPILER}")

  add_custom_command(TARGET ${CCSE_BUILD_TARGET}
                     POST_BUILD
                     COMMAND $(MAKE) BOXLIB_HOME=${CCSE_source_dir} BOXLIB_f2py_f90=${RAW_Fortran_COMPILER} CC=${RAW_C_COMPILER} CXX=${RAW_CXX_COMPILER} 3>&2 2>&1 > ${CCSE_stamp_dir}/CCSE-tools-build.log
                     COMMAND ${CMAKE_COMMAND} -E copy fsnapshot.*so ${TPL_INSTALL_PREFIX}/lib/fsnapshot.so
                     DEPENDS ${CCSE_BUILD_TARGET}
                     WORKING_DIRECTORY ${CCSE_source_dir}/Tools/Py_util)

  # --- This guy right here builds AMRDeriveTecplot, an executable program for 
  # --- producing Tecplot/ASCII output from CCSE's native AMR output.
  # --- Like the fsnapshot.so command above, it executes after the CCSE library 
  # --- is built, builds the module, and copies it into place.
  #if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  #  # We need to link against libquadmath on Linux, it seems.
  #  set(AMRDERIVETECPLOT_ARGS "LDFLAGS=\"-lquadmath\"")
  #endif()
  #if (APPLE)
  #  set(AMRDERIVETECPLOT_ARGS "LDFLAGS=\"-lgfortran\"")
  #endif()
  #add_custom_command(TARGET ${CCSE_BUILD_TARGET}
  #                   POST_BUILD
  #                   COMMAND $(MAKE) BOXLIB_HOME=${CCSE_source_dir} ${AMRDERIVETECPLOT_ARGS}
  #                   COMMAND cp AmrDeriveTecplot*.ex ${TPL_INSTALL_PREFIX}/bin
  #                   DEPENDS ${CCSE_BUILD_TARGET}
  #                   WORKING_DIRECTORY ${CCSE_source_dir}/Tools/C_util/AmrDeriveTecplot)
endif()
