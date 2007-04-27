/*
 * Copyright (C) 2006 Ole André Vadla Ravnås <oleavr@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef ODCCM_ERRORS_H
#define ODCCM_ERRORS_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum
{
  InvalidArgument,
  NotAvailable,
} OdccmErrors;

GQuark odccm_errors_quark (void);
#define ODCCM_ERRORS odccm_errors_quark ()

G_END_DECLS

#endif /* ODCCM_ERRORS_H */
