/*
    Access to Garmin PCX5 files.

    Copyright (C) 2002 Robert Lipe, robertlipe@usa.net

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA
*/


#include "defs.h"
#include <ctype.h>

static FILE *file_in;
static FILE *file_out;
static void *mkshort_handle;
static char *deficon = "Waypoint";

#define MYNAME "PCX"

static
arglist_t pcx_args[] = {
	{"deficon", &deficon, "Default icon name"},
	{0, 0, 0}
};

static void
rd_init(const char *fname, const char *args)
{
	file_in = fopen(fname, "r");
	if (file_in == NULL) {
		fatal(MYNAME ": Cannot open %s for reading\n", fname);
	}
}

static void
rd_deinit(void)
{
	fclose(file_in);
}

static void
wr_init(const char *fname, const char *args)
{
	file_out = fopen(fname, "w");
	mkshort_handle = mkshort_new_handle();

	if (file_out == NULL) {
		fatal(MYNAME ": Cannot open %s for writing\n", fname);
	}
}

static void
wr_deinit(void)
{
	fclose(file_out);
	mkshort_del_handle(mkshort_handle);
}

static void
data_read(void)
{
	char name[9], desc[90];
	double lat,lon;
	char latdir, londir;
	long alt; 
	char alttype;
	char date[10];
	char time[9];
	waypoint *wpt_tmp;
	char ibuf[100];

	for(;fgets(ibuf, sizeof(ibuf), file_in);) {
		switch (ibuf[0]) {
			case 'W': 
			sscanf(ibuf, "W  %s %c%lf %c%lf %s %s %ld %90[^\n']", 
				name, &latdir, &lat, &londir, &lon, 
				date, time, &alt, desc);
		wpt_tmp = xcalloc(sizeof(*wpt_tmp), 1);
		wpt_tmp->position.altitude.altitude_meters = alt;
		wpt_tmp->shortname = xstrdup(name);
		wpt_tmp->description = xstrdup(desc);

		if (latdir == 'S') lat = -lat;
		if (londir == 'W') lon = -lon;
		wpt_tmp->position.longitude.degrees = lon/100.0;
		wpt_tmp->position.latitude.degrees = lat/100.0;
		waypt_add(wpt_tmp);
		}
	}
}

static void
gpsutil_disp(const waypoint *wpt)
{
	double lon,lat;
	signed int ilon, ilat;
	int icon_token = 0;
	char tbuf[1024];
	char *tp = tbuf;
	time_t tm = wpt->creation_time;

	ilon = (signed int)wpt->position.longitude.degrees;
	ilat = (signed int)wpt->position.latitude.degrees;
	lon = (ilon * 100.0) + (wpt->position.longitude.degrees - ilon) * 60.0;
	lat = (ilat * 100.0) + (wpt->position.latitude.degrees - ilat) * 60.0;

	if (tm == 0) 
		tm = time(NULL);
	strftime(tbuf, sizeof(tbuf), "%d-%b-%y %I:%M:%S", localtime(&tm));
	while (*tp) {
		*tp = toupper(*tp);
		tp++;
	}

	icon_token = mps_find_icon_number_from_desc(deficon);
	if (get_cache_icon(wpt)) {
		icon_token = mps_find_icon_number_from_desc(get_cache_icon(wpt));
	}


	fprintf(file_out, "W  %-6.6s %c%08.5f %c%011.5f %s %5d %-40.40s %5e  %d\n",
                global_opts.synthesize_shortnames ?
                        mkshort(mkshort_handle, wpt->description) : 
			wpt->shortname,
		lat < 0.0 ? 'S' : 'N',
		fabs(lat),
		lon < 0.0 ? 'W' : 'E',
		fabs(lon),
		tbuf, 
		-9999,
		wpt->description,
		0.0,
		icon_token);
}

static void
data_write(void)
{
fprintf(file_out,
"H  SOFTWARE NAME & VERSION\n"
"I  PCX5 2.09\n"
"\n"
"H  R DATUM                IDX DA            DF            DX            DY            DZ\n"
"M  G WGS 84               121 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00 +0.000000e+00\n"
"\n"
"H  COORDINATE SYSTEM\n"
"U  LAT LON DM\n"
"\n"
"H  IDNT   LATITUDE    LONGITUDE    DATE      TIME     ALT   DESCRIPTION                              PROXIMITY     SYMBOL ;waypts\n");
	setshort_length(mkshort_handle, 6);
	waypt_disp_all(gpsutil_disp);
}


ff_vecs_t pcx_vecs = {
	rd_init,
	wr_init,
	rd_deinit,
	wr_deinit,
	data_read,
	data_write,
	pcx_args
};
