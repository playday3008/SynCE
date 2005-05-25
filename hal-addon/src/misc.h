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

#ifndef _HAS_MISC_H
#define _HAS_MISC_H

#include <stdio.h>

#define SHC_INFO(...)  do { if (hal_verbose) printf ("hald-addon-pocketpc [I]  %s:%d in %s(): ", __FILE__, __LINE__, __FUNCTION__); printf (__VA_ARGS__); printf ("\n"); } while(0)
#define SHC_WARN(...)  do { if (hal_verbose) printf ("hald-addon-pocketpc [W] %s:%d in %s(): ", __FILE__, __LINE__, __FUNCTION__); printf (__VA_ARGS__); printf ("\n"); } while(0)
#define SHC_ERROR(...)  do { printf ("hald-addon-pocketpc [E] %s:%d in %s(): ", __FILE__, __LINE__, __FUNCTION__); printf (__VA_ARGS__); printf ("\n"); } while(0)

#endif /* _HAS_MISC_H */
