// File generated by cmake from cmake/config.h.cmake.

#ifndef _EXV_CONF_H_
#define _EXV_CONF_H_

// Define to 1 if you want to use libcurl in httpIO.
/* #undef EXV_USE_CURL */

// Define if you require webready support.
/* #undef EXV_ENABLE_WEBREADY */

// Define if you have the <libintl.h> header file.
/* #undef EXV_HAVE_LIBINTL_H */

// Define if you want translation of program messages to the user's native language
/* #undef EXV_ENABLE_NLS */

// Define if you want BMFF support.
#define EXV_ENABLE_BMFF

// Define if you have the strerror_r function.
/* #undef EXV_HAVE_STRERROR_R */

// Define if the strerror_r function returns char*.
/* #undef EXV_STRERROR_R_CHAR_P */

// Define to enable the Windows unicode path support.
/* #undef EXV_UNICODE_PATH */

/* Define to `const' or to empty, depending on the second argument of `iconv'. */
/* #undef ICONV_ACCEPTS_CONST_INPUT */
#if defined(ICONV_ACCEPTS_CONST_INPUT) || defined(__NetBSD__)
#define EXV_ICONV_CONST const
#else
#define EXV_ICONV_CONST
#endif

// Define if you have the mmap function.
/* #undef EXV_HAVE_MMAP */

// Define if you have the munmap function.
/* #undef EXV_HAVE_MUNMAP */

/* Define if you have the <libproc.h> header file. */
/* #undef EXV_HAVE_LIBPROC_H */

/* Define if you have the <unistd.h> header file. */
/* #undef EXV_HAVE_UNISTD_H */

// Define if you have the <sys/mman.h> header file.
/* #undef EXV_HAVE_SYS_MMAN_H */

// Define if you have are using the zlib library.
#define EXV_HAVE_LIBZ

/* Define if you have (Exiv2/xmpsdk) Adobe XMP Toolkit. */
#define EXV_HAVE_XMP_TOOLKIT

/* Define to the full name of this package. */
#define EXV_PACKAGE_NAME "exiv2"

/* Define to the full name and version of this package. */
#define EXV_PACKAGE_STRING "exiv2 1.0.0.9"

/* Define to the version of this package. */
#define EXV_PACKAGE_VERSION "1.0.0.9"

#define EXIV2_MAJOR_VERSION (1)
#define EXIV2_MINOR_VERSION (0)
#define EXIV2_PATCH_VERSION (0)
#define EXIV2_TWEAK_VERSION (9)

// Definition to enable translation of Nikon lens names.
#define EXV_HAVE_LENSDATA

// Define if you have the iconv function.
/* #undef EXV_HAVE_ICONV */

// Definition to enable conversion of UCS2 encoded Windows tags to UTF-8.
#define EXV_HAVE_PRINTUCS2

#endif /* !_EXV_CONF_H_ */
