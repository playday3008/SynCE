/* 
 * Copyright (c) 2005 Andrei Yurkevich <urruru@ru.ru>
 * Copyright (c) 2002 David Eriksson <twogood@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#include <glib.h> 

typedef enum {
  PROPERTY_TYPE_STRING,
  PROPERTY_TYPE_INT,
  PROPERTY_TYPE_BOOL,
} HalPropertyType;

void	hal_set_property	(gchar            *udi,
                             HalPropertyType  type,
							 gchar            *property,
							 gpointer         value);
							
gchar *	hal_check_device	(gchar            *udi);

gchar *	hal_get_sysfs_path	(gchar            *udi);

void    hal_add_capability  (gchar            *udi,
                             gchar            *capability);
