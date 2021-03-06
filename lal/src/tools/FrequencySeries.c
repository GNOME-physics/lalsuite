/*
 * Copyright (C) 2007  Kipp Cannon
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <complex.h>
#include <math.h>
#include <string.h>
#include <lal/Date.h>
#include <lal/LALDatatypes.h>
#include <lal/LALStdlib.h>
#include <lal/FrequencySeries.h>
#include <lal/Sequence.h>
#include <lal/Units.h>
#include <lal/XLALError.h>

#define DATATYPE REAL4
#include "FrequencySeries_source.c"
#undef DATATYPE

#define DATATYPE REAL8
#include "FrequencySeries_source.c"
#undef DATATYPE

#define DATATYPE COMPLEX8
#include "FrequencySeries_source.c"
#include "FrequencySeriesComplex_source.c"
#undef DATATYPE

#define DATATYPE COMPLEX16
#include "FrequencySeries_source.c"
#include "FrequencySeriesComplex_source.c"
#undef DATATYPE

#define DATATYPE INT2
#include "FrequencySeries_source.c"
#undef DATATYPE

#define DATATYPE UINT2
#include "FrequencySeries_source.c"
#undef DATATYPE

#define DATATYPE INT4
#include "FrequencySeries_source.c"
#undef DATATYPE

#define DATATYPE UINT4
#include "FrequencySeries_source.c"
#undef DATATYPE

#define DATATYPE INT8
#include "FrequencySeries_source.c"
#undef DATATYPE

#define DATATYPE UINT8
#include "FrequencySeries_source.c"
#undef DATATYPE
