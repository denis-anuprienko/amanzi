# ############################################################################ #
#                                                                              #
# Trilinos Build Configuration File                                            #
#                                                                              #
# Defining the LAPACK/BLAS libraries from LibSCI                               #
#                                                                              #
# ############################################################################ #

set(LIBSCI_BASE_DIR $ENV{LIBSCI_BASE_DIR})
set(COMPILER_TARGET intel)
set(COMPILER_TARGET_VERSION 120)
set(CRAY_CPU_TARGET $ENV{CRAY_CPU_TARGET})
set(LIBSCI_DIR ${LIBSCI_BASE_DIR}/${COMPILER_TARGET}/${COMPILER_TARGET_VERSION}/${CRAY_CPU_TARGET})

set(BLAS_LIBRARIES
    "${LIBSCI_DIR}/lib/libscicpp_${COMPILER_TARGET}.a;${LIBSCI_DIR}/lib/libsci_${COMPILER_TARGET}.a"
    CACHE FILEPATH "CRAY tuned BLAS libraries"
    FORCE)

set(LAPACK_LIBRARIES
    "${LIBSCI_DIR}/lib/libscicpp_${COMPILER_TARGET}.a;${LIBSCI_DIR}/lib/libsci_${COMPILER_TARGET}.a"
    CACHE FILEPATH "CRAY tuned LAPACK libraries"
    FORCE)

message(STATUS "${TPL_LAPACK_LIBRARIES}")
