#
# Build TPL:  PHREEQC
#   
# --- Define all the directories and common external project flags
define_external_project_args(PHREEQC
                             TARGET phreeqc)

# Add version version to the autogenerated tpl_versions.h file
include(${SuperBuild_SOURCE_DIR}/TPLVersions.cmake)
amanzi_tpl_version_write(FILENAME ${TPL_VERSIONS_INCLUDE_FILE}
  PREFIX PHREEQC
  VERSION ${PHREEQC_VERSION_MAJOR} ${PHREEQC_VERSION_MINOR} ${PHREEQC_VERSION_PATCH})
  
# --- Patch the original code
set(PHREEQC_patch_file phreeqc-clang.patch)
set(PHREEQC_sh_patch ${PHREEQC_prefix_dir}/phreeqc-patch-step.sh)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/phreeqc-patch-step.sh.in
               ${PHREEQC_sh_patch}
               @ONLY)
# configure the CMake patch step
set(PHREEQC_cmake_patch ${PHREEQC_prefix_dir}/phreeqc-patch-step.cmake)
configure_file(${SuperBuild_TEMPLATE_FILES_DIR}/phreeqc-patch-step.cmake.in
               ${PHREEQC_cmake_patch}
               @ONLY)
# set the patch command
set(PHREEQC_PATCH_COMMAND ${CMAKE_COMMAND} -P ${PHREEQC_cmake_patch})

# --- Define the arguments passed to CMake.
set(PHREEQC_CMAKE_ARGS 
      "-DCMAKE_INSTALL_PREFIX:FILEPATH=${TPL_INSTALL_PREFIX}"
      "-DCMAKE_INSTALL_LIBDIR:FILEPATH=${TPL_INSTALL_PREFIX}/lib"
      "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}")

# --- Add external project build and tie to the PHREEQC build target
ExternalProject_Add(${PHREEQC_BUILD_TARGET}
                    DEPENDS   ${PHREEQC_PACKAGE_DEPENDS}       # Package dependency target
                    TMP_DIR   ${PHREEQC_tmp_dir}               # Temporary files directory
                    STAMP_DIR ${PHREEQC_stamp_dir}             # Timestamp and log directory
                    # -- Download and URL definitions
                    DOWNLOAD_DIR  ${TPL_DOWNLOAD_DIR}          # Download directory
                    URL           ${PHREEQC_URL}               # URL may be a web site OR a local file
                    URL_MD5       ${PHREEQC_MD5_SUM}           # md5sum of the archive file
                    DOWNLOAD_NAME ${PHREEQC_SAVEAS_FILE}       # file name to store (if not end of URL)
                    # -- Patch 
                    PATCH_COMMAND ${PHREEQC_PATCH_COMMAND}     # Mods to source
                    # -- Configure
                    SOURCE_DIR    ${PHREEQC_source_dir}        # Source directory
                    CMAKE_ARGS    ${PHREEQC_CMAKE_ARGS}        # CMAKE_CACHE_ARGS or CMAKE_ARGS => CMake configure
                                  -DCMAKE_C_FLAGS:STRING=${Amanzi_COMMON_CFLAGS}  # Ensure uniform build
                                  -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
                                  -DCMAKE_Fortran_COMPILER:FILEPATH=${CMAKE_Fortran_COMPILER}
                    # -- Build
                    BINARY_DIR      ${PHREEQC_build_dir}       # Build directory 
                    BUILD_COMMAND   $(MAKE)
                    # -- Install
                    INSTALL_DIR     ${TPL_INSTALL_PREFIX}      # Install directory
                    INSTALL_COMMAND $(MAKE) install
                    # -- Output control
                    ${PHREEQC_logging_args})

include(BuildLibraryName)
build_library_name(phreeqc PHREEQC_LIB APPEND_PATH ${TPL_INSTALL_PREFIX}/lib)

# --- set cache (global) variables
global_set(PHREEQC_LIBRARY "${PHREEQC_LIB}")
global_set(PHREEQC_DIR ${TPL_INSTALL_PREFIX})