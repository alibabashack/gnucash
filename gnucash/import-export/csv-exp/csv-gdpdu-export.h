/*******************************************************************\
 * csv-gdpdu-export.h -- Export data for German tax authority       *
 *                                                                  *
 * Copyright (C) 2021 Alexander Graf                                *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
\********************************************************************/
/** @file csv-gdpdu-export.h
    @brief GDPdU data set export for German tax authority
    @author Copyright (c) 2021 Alexander Graf
*/

#ifndef CSV_GDPDU_EXPORT
#define CSV_GDPDU_EXPORT

#include "assistant-csv-export.h"

/** The csv_gdpdu_export() will let the user export a set of csv files
 * containing the book content accompanied by a structural description in xml
 * together known as a GDPdU data set which may be requested by the German
 * tax authority during a tax audit.
 */
void csv_gdpdu_export (CsvExportInfo *info);

#endif

