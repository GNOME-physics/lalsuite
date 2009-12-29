/*
*  Copyright (C) 2007 Duncan Brown, Jolien Creighton
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with with program; see the file COPYING. If not, write to the
*  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*  MA  02111-1307  USA
*/

#include <config.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lalapps.h>
#include <lal/LALMalloc.h>
#include <lal/LALStatusMacros.h>
#include <lal/LALVCSInfo.h>
#include <LALAppsVCSInfo.h>

#ifdef HAVE_LIBLALFRAME
#include <lal/LALFrameVCSInfo.h>
#endif

#ifdef HAVE_LIBLALMETAIO
#include <lal/LALMetaIOVCSInfo.h>
#endif

#ifdef HAVE_LIBLALBURST
#include <lal/LALBurstVCSInfo.h>
#endif

#ifdef HAVE_LIBLALSTOCHASTIC
#include <lal/LALStochasticVCSInfo.h>
#endif

#ifdef HAVE_LIBLALPULSAR
#include <lal/LALPulsarVCSInfo.h>
#endif

#ifdef HAVE_LIBLALXML
#include <lal/LALXMLVCSInfo.h>
#endif

#define FAILMSG( stat, func, file, line, id )                                  \
  do {                                                                         \
    if ( lalDebugLevel & LALERROR )                                            \
    {                                                                          \
      LALPrintError( "Error[0]: file %s, line %d, %s\n"                        \
          "\tLAL_CALL: Function call `%s' failed.\n", file, line, id, func );  \
    }                                                                          \
    if ( vrbflg )                                                              \
    {                                                                          \
      fprintf(stderr,"Level 0: %s\n\tFunction call `%s' failed.\n"             \
          "\tfile %s, line %d\n", id, func, file, line );                      \
      REPORTSTATUS( stat );                                                    \
    }                                                                          \
  } while( 0 )

const LALStatus blank_status;
extern int lalDebugLevel;
int vrbflg = 0;

lal_errhandler_t lal_errhandler = LAL_ERR_DFLT;

int LAL_ERR_EXIT(
    LALStatus  *stat,
    const char *func,
    const char *file,
    const int   line,
    volatile const char *id
    )
{
  if ( stat->statusCode )
  {
    FAILMSG( stat, func, file, line, id );
    exit( 1 );
  }
  return stat->statusCode;
}

int LAL_ERR_ABRT(
    LALStatus  *stat,
    const char *func,
    const char *file,
    const int   line,
    volatile const char *id
    )
{
  if ( stat->statusCode )
  {
    FAILMSG( stat, func, file, line, id );
    abort();
  }
  return 0;
}

int LAL_ERR_RTRN(
    LALStatus  *stat,
    const char *func,
    const char *file,
    const int   line,
    volatile const char *id
    )
{
  if ( stat->statusCode )
  {
    FAILMSG( stat, func, file, line, id );
  }
  return stat->statusCode;
}

int clear_status( LALStatus *stat )
{
  if ( ! stat )
    return 1;
  while ( stat->statusPtr )
  {
    LALStatus *next = stat->statusPtr->statusPtr;
    LALFree( stat->statusPtr );
    stat->statusPtr = next;
  }
  memset( stat, 0, sizeof( *stat ) );
  return 0;
}

int set_debug_level( const char *s )
{
  unsigned level = 0;
  if ( ! s )
  {
    if ( ! ( s = getenv( "LAL_DEBUG_LEVEL" ) ) )
      return lalDebugLevel = 0;
  }

  /* skip whitespace */
  while ( isspace( *s ) )
    ++s;

  /* a value is set */
  if ( isdigit( *s ) )
    return lalDebugLevel = atoi( s );

  /* construct the debug level */
  if ( strstr( s, "NDEBUG" ) )
    level |= LALNDEBUG;
  if ( strstr( s, "ERROR" ) )
    level |= LALERROR;
  if ( strstr( s, "WARNING" ) )
    level |= LALWARNING;
  if ( strstr( s, "INFO" ) )
    level |= LALINFO;
  if ( strstr( s, "TRACE" ) )
    level |= LALTRACE;
  if ( strstr( s, "MEMINFO" ) )
    level |= LALMEMINFO;
  if ( strstr( s, "MEMDBG" ) )
    level |= LALMEMDBG;
  if ( strstr( s, "MSGLVL1" ) )
    level |= LALMSGLVL1;
  if ( strstr( s, "MSGLVL2" ) )
    level |= LALMSGLVL2;
  if ( strstr( s, "MSGLVL3" ) )
    level |= LALMSGLVL3;
  if ( strstr( s, "MEMTRACE" ) )
    level |= LALMEMTRACE;
  if ( strstr( s, "ALLDBG" ) )
    level |= LALALLDBG;

  return lalDebugLevel = level;
}


/** Function that assembles a default VCS info/version string from LAL and LALapps
 *  Also checks LAL header<>library version consistency and returns NULL on error.
 *
 * The VCS version string is allocated here and must be freed by caller.
 */
char *
XLALGetVersionString( int level )
{
  const char *fn = __func__;
  char lal_info[1024];
#ifdef HAVE_LIBLALFRAME
  char lalframe_info[1024];
#endif
#ifdef HAVE_LIBLALMETAIO
  char lalmetaio_info[1024];
#endif
#ifdef HAVE_LIBLALBURST
  char lalburst_info[1024];
#endif
#ifdef HAVE_LIBLALSTOCHASTIC
  char lalstochastic_info[1024];
#endif
#ifdef HAVE_LIBLALPULSAR
  char lalpulsar_info[1024];
#endif
#ifdef HAVE_LIBLALXML
  char lalxml_info[1024];
#endif
  char lalapps_info[1024];
  char *ret;
  const char delim[] = ":";
  char *tree_status;

  /* check version consistency between LAL headers <> library */
  if ( XLALVCSInfoCompare(&lalHeaderVCSInfo, &lalLibraryVCSInfo) )
    {
      XLALPrintError("%s: FATAL: version mismatch between LAL headers (%s) and LAL library (%s)\n",
                     fn, lalHeaderVCSInfo.vcsId, lalLibraryVCSInfo.vcsId );
      XLALPrintError("This indicates a compilation problem: make sure you setup is consistent and recompile this code.\n");
      XLAL_ERROR_NULL (fn, XLAL_EERR );
    }

  switch(level)
  {
    case 0:
      /* get lal info */
      tree_status = strdup(lalLibraryVCSInfo.vcsStatus);
      snprintf(lal_info, sizeof(lal_info),
          "%%%% LAL: %s (%s %s)\n", lalLibraryVCSInfo.version, \
          strsep(&tree_status, delim), lalLibraryVCSInfo.vcsId);

#ifdef HAVE_LIBLALFRAME
      /* get lalframe info */
      tree_status = strdup(lalFrameVCSInfo.vcsStatus);
      snprintf(lalframe_info, sizeof(lalframe_info),
          "%%%% LALFrame: %s (%s %s)\n", lalFrameVCSInfo.version, \
          strsep(&tree_status, delim), lalFrameVCSInfo.vcsId);
#endif

#ifdef HAVE_LIBLALMETAIO
      /* get lalmetaio info */
      tree_status = strdup(lalMetaIOVCSInfo.vcsStatus);
      snprintf(lalmetaio_info, sizeof(lalmetaio_info),
          "%%%% LALMetaIO: %s (%s %s)\n", lalMetaIOVCSInfo.version, \
          strsep(&tree_status, delim), lalMetaIOVCSInfo.vcsId);
#endif

#ifdef HAVE_LIBLALBURST
      /* get lalburst info */
      tree_status = strdup(lalBurstVCSInfo.vcsStatus);
      snprintf(lalburst_info, sizeof(lalburst_info),
          "%%%% LALBurst: %s (%s %s)\n", lalBurstVCSInfo.version, \
          strsep(&tree_status, delim), lalBurstVCSInfo.vcsId);
#endif

#ifdef HAVE_LIBLALSTOCHASTIC
      /* get lalstochastic info */
      tree_status = strdup(lalStochasticVCSInfo.vcsStatus);
      snprintf(lalstochastic_info, sizeof(lalstochastic_info),
          "%%%% LALStochastic: %s (%s %s)\n", lalStochasticVCSInfo.version, \
          strsep(&tree_status, delim), lalStochasticVCSInfo.vcsId);
#endif

#ifdef HAVE_LIBLALPULSAR
      /* get lalpulsar info */
      tree_status = strdup(lalPulsarVCSInfo.vcsStatus);
      snprintf(lalpulsar_info, sizeof(lalpulsar_info),
          "%%%% LALPulsar: %s (%s %s)\n", lalPulsarVCSInfo.version, \
          strsep(&tree_status, delim), lalPulsarVCSInfo.vcsId);
#endif

#ifdef HAVE_LIBLALXML
      /* get lalxml info */
      tree_status = strdup(lalXMLVCSInfo.vcsStatus);
      snprintf(lalxml_info, sizeof(lalxml_info),
          "%%%% LALXML: %s (%s %s)\n", lalXMLVCSInfo.version, \
          strsep(&tree_status, delim), lalXMLVCSInfo.vcsId);
#endif

      /* get lalapps info */
      tree_status = strdup(lalAppsVCSInfo.vcsStatus);
      snprintf(lalapps_info, sizeof(lalapps_info),
          "%%%% LALApps: %s (%s %s)", lalAppsVCSInfo.version, \
          strsep(&tree_status, delim), lalAppsVCSInfo.vcsId);

      break;

    default:
      /* get lal info */
      snprintf( lal_info, sizeof(lal_info),
          "%%%% LAL-Version: %s\n"
          "%%%% LAL-Id: %s\n"
          "%%%% LAL-Date: %s\n"
          "%%%% LAL-Branch: %s\n"
          "%%%% LAL-Tag: %s\n"
          "%%%% LAL-Status: %s\n"
          "%%%% LAL-Configure Date: %s\n"
          "%%%% LAL-Configure Arguments: %s\n",
          lalLibraryVCSInfo.version,
          lalLibraryVCSInfo.vcsId,
          lalLibraryVCSInfo.vcsDate,
          lalLibraryVCSInfo.vcsBranch,
          lalLibraryVCSInfo.vcsTag,
          lalLibraryVCSInfo.vcsStatus,
          LAL_CONFIGURE_DATE ,
          LAL_CONFIGURE_ARGS );

#ifdef HAVE_LIBLALFRAME
      /* get lalframe info */
      snprintf( lalframe_info, sizeof(lalframe_info),
          "%%%% LALFrame-Version: %s\n"
          "%%%% LALFrame-Id: %s\n"
          "%%%% LALFrame-Date: %s\n"
          "%%%% LALFrame-Branch: %s\n"
          "%%%% LALFrame-Tag: %s\n"
          "%%%% LALFrame-Status: %s\n"
          "%%%% LALFrame-Configure Date: %s\n"
          "%%%% LALApps-Configure Arguments: %s\n",
          lalFrameVCSInfo.version,
          lalFrameVCSInfo.vcsId,
          lalFrameVCSInfo.vcsDate,
          lalFrameVCSInfo.vcsBranch,
          lalFrameVCSInfo.vcsTag,
          lalFrameVCSInfo.vcsStatus,
          LALFRAME_CONFIGURE_DATE ,
          LALFRAME_CONFIGURE_ARGS );
#endif

#ifdef HAVE_LIBLALMETAIO
      /* get lalmetaio info */
      snprintf( lalmetaio_info, sizeof(lalmetaio_info),
          "%%%% LALMetaIO-Version: %s\n"
          "%%%% LALMetaIO-Id: %s\n"
          "%%%% LALMetaIO-Date: %s\n"
          "%%%% LALMetaIO-Branch: %s\n"
          "%%%% LALMetaIO-Tag: %s\n"
          "%%%% LALMetaIO-Status: %s\n"
          "%%%% LALMetaIO-Configure Date: %s\n"
          "%%%% LALMetaIO-Configure Arguments: %s\n",
          lalMetaIOVCSInfo.version,
          lalMetaIOVCSInfo.vcsId,
          lalMetaIOVCSInfo.vcsDate,
          lalMetaIOVCSInfo.vcsBranch,
          lalMetaIOVCSInfo.vcsTag,
          lalMetaIOVCSInfo.vcsStatus,
          LALMETAIO_CONFIGURE_DATE ,
          LALMETAIO_CONFIGURE_ARGS );
#endif

#ifdef HAVE_LIBLALBURST
      /* get lalburst info */
      snprintf( lalburst_info, sizeof(lalburst_info),
          "%%%% LALBurst-Version: %s\n"
          "%%%% LALBurst-Id: %s\n"
          "%%%% LALBurst-Date: %s\n"
          "%%%% LALBurst-Branch: %s\n"
          "%%%% LALBurst-Tag: %s\n"
          "%%%% LALBurst-Status: %s\n"
          "%%%% LALBurst-Configure Date: %s\n"
          "%%%% LALBurst-Configure Arguments: %s\n",
          lalBurstVCSInfo.version,
          lalBurstVCSInfo.vcsId,
          lalBurstVCSInfo.vcsDate,
          lalBurstVCSInfo.vcsBranch,
          lalBurstVCSInfo.vcsTag,
          lalBurstVCSInfo.vcsStatus,
          LALBURST_CONFIGURE_DATE ,
          LALBURST_CONFIGURE_ARGS );
#endif

#ifdef HAVE_LIBLALSTOCHASTIC
      /* get lalstochastic info */
      snprintf( lalstochastic_info, sizeof(lalstochastic_info),
          "%%%% LALStochastic-Version: %s\n"
          "%%%% LALStochastic-Id: %s\n"
          "%%%% LALStochastic-Date: %s\n"
          "%%%% LALStochastic-Branch: %s\n"
          "%%%% LALStochastic-Tag: %s\n"
          "%%%% LALStochastic-Status: %s\n"
          "%%%% LALStochastic-Configure Date: %s\n"
          "%%%% LALStochastic-Configure Arguments: %s\n",
          lalStochasticVCSInfo.version,
          lalStochasticVCSInfo.vcsId,
          lalStochasticVCSInfo.vcsDate,
          lalStochasticVCSInfo.vcsBranch,
          lalStochasticVCSInfo.vcsTag,
          lalStochasticVCSInfo.vcsStatus,
          LALSTOCHASTIC_CONFIGURE_DATE ,
          LALSTOCHASTIC_CONFIGURE_ARGS );
#endif

#ifdef HAVE_LIBLALPULSAR
      /* get lalpulsar info */
      snprintf( lalpulsar_info, sizeof(lalpulsar_info),
          "%%%% LALPulsar-Version: %s\n"
          "%%%% LALPulsar-Id: %s\n"
          "%%%% LALPulsar-Date: %s\n"
          "%%%% LALPulsar-Branch: %s\n"
          "%%%% LALPulsar-Tag: %s\n"
          "%%%% LALPulsar-Status: %s\n"
          "%%%% LALPulsar-Configure Date: %s\n"
          "%%%% LALPulsar-Configure Arguments: %s\n",
          lalPulsarVCSInfo.version,
          lalPulsarVCSInfo.vcsId,
          lalPulsarVCSInfo.vcsDate,
          lalPulsarVCSInfo.vcsBranch,
          lalPulsarVCSInfo.vcsTag,
          lalPulsarVCSInfo.vcsStatus,
          LALPULSAR_CONFIGURE_DATE ,
          LALPULSAR_CONFIGURE_ARGS );
#endif

#ifdef HAVE_LIBLALXML
      /* get lalxml info */
      snprintf( lalxml_info, sizeof(lalxml_info),
          "%%%% LALXML-Version: %s\n"
          "%%%% LALXML-Id: %s\n"
          "%%%% LALXML-Date: %s\n"
          "%%%% LALXML-Branch: %s\n"
          "%%%% LALXML-Tag: %s\n"
          "%%%% LALXML-Status: %s\n"
          "%%%% LALXML-Configure Date: %s\n"
          "%%%% LALXML-Configure Arguments: %s\n",
          lalXMLVCSInfo.version,
          lalXMLVCSInfo.vcsId,
          lalXMLVCSInfo.vcsDate,
          lalXMLVCSInfo.vcsBranch,
          lalXMLVCSInfo.vcsTag,
          lalXMLVCSInfo.vcsStatus,
          LALXML_CONFIGURE_DATE ,
          LALXML_CONFIGURE_ARGS );
#endif

      /* add lalapps info */
      snprintf( lalapps_info, sizeof(lalapps_info),
          "%%%% LALApps-Version: %s\n"
          "%%%% LALApps-Id: %s\n"
          "%%%% LALApps-Date: %s\n"
          "%%%% LALApps-Branch: %s\n"
          "%%%% LALApps-Tag: %s\n"
          "%%%% LALApps-Status: %s\n"
          "%%%% LALApps-Configure Date: %s\n"
          "%%%% LALApps-Configure Arguments: %s\n",
          lalAppsVCSInfo.version,
          lalAppsVCSInfo.vcsId,
          lalAppsVCSInfo.vcsDate,
          lalAppsVCSInfo.vcsBranch,
          lalAppsVCSInfo.vcsTag,
          lalAppsVCSInfo.vcsStatus,
          LALAPPS_CONFIGURE_DATE ,
          LALAPPS_CONFIGURE_ARGS );

      break;
  }

  size_t len = strlen(lal_info) + strlen(lalapps_info) + 1;
#ifdef HAVE_LIBLALFRAME
  len += strlen(lalframe_info);
#endif
#ifdef HAVE_LIBLALMETAIO
  len += strlen(lalmetaio_info);
#endif
#ifdef HAVE_LIBLALBURST
  len += strlen(lalburst_info);
#endif
#ifdef HAVE_LIBLALSTOCHASTIC
  len += strlen(lalstochastic_info);
#endif
#ifdef HAVE_LIBLALPULSAR
  len += strlen(lalpulsar_info);
#endif
#ifdef HAVE_LIBLALXML
  len += strlen(lalxml_info);
#endif
  if ( (ret = XLALMalloc ( len )) == NULL ) {
    XLALPrintError ("%s: Failed to XLALMalloc(%d)\n", fn, len );
    XLAL_ERROR_NULL ( fn, XLAL_ENOMEM );
  }

  strcpy ( ret, lal_info );
#ifdef HAVE_LIBLALFRAME
  strcat ( ret, lalframe_info );
#endif
#ifdef HAVE_LIBLALMETAIO
  strcat ( ret, lalmetaio_info );
#endif
#ifdef HAVE_LIBLALBURST
  strcat ( ret, lalburst_info );
#endif
#ifdef HAVE_LIBLALSTOCHASTIC
  strcat ( ret, lalstochastic_info );
#endif
#ifdef HAVE_LIBLALPULSAR
  strcat ( ret, lalpulsar_info );
#endif
#ifdef HAVE_LIBLALXML
  strcat ( ret, lalxml_info );
#endif
  strcat ( ret, lalapps_info );

  return ( ret );

} /* XLALGetVersionString() */


/** Simply outputs version information to fp.
 *
 * Returns != XLAL_SUCCESS on error (version-mismatch or writing to fp)
 */
int
XLALOutputVersionString ( FILE *fp, int level )
{
  const char *fn = __func__;

  char *VCSInfoString;

  if (!fp ) {
    XLALPrintError ("%s: invalid NULL input 'fp'\n", fn );
    XLAL_ERROR ( fn, XLAL_EINVAL );
  }
  if ( (VCSInfoString = XLALGetVersionString(level)) == NULL ) {
    XLALPrintError("%s: XLALGetVersionString() failed.\n", fn);
    XLAL_ERROR ( fn, XLAL_EFUNC );
  }

  if ( fprintf (fp, "%s\n", VCSInfoString ) < 0 ) {
    XLALPrintError("%s: fprintf failed for given file-pointer 'fp'\n", fn);
    XLALFree ( VCSInfoString);
    XLAL_ERROR ( fn, XLAL_EIO );
  }

  XLALFree ( VCSInfoString);

  return XLAL_SUCCESS;

} /* XLALOutputVersionString() */
