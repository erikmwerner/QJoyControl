#ifndef POWERTOOLS_H
#define POWERTOOLS_H

#if !defined(__cplusplus)
#define C_API extern
#else
#define C_API extern "C"
#endif

#include <QString>

/* Power Tools
 * A few tools to control power management features in MacOS that
 * can cause problems with scientific applications. Designed for use with Qt.
 *
 * To use:
 * HEADERS += powertools.h
 * OBJECTIVE_SOURCES += powertools.mm
 * LIBS += -framework Foundation
 *
 * App Nap (timer coalescing, introduced in 10.9)
 * Disable App Nap if continued processing is needed
 * when the application is in the background
 * API from: https://developer.apple.com/documentation/foundation/nsprocessinfo
 * Based on the thread at: https://stackoverflow.com/questions/30686488/qapplication-is-lazy-or-making-other-threads-lazy-in-the-app
 *
 * User Sleep
 * Prevent the computer or display and computer from sleeping after
 * spending too long without user input
 * API from: https://developer.apple.com/documentation/iokit/iopmlib_h?language=objc
 *
 * License: MIT
 * Copyright 2019 Erik Werner
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
*/


C_API void disableAppNap(const QString reason_string = "Disable App Nap");
C_API void enableAppNap();
C_API bool preventSleep(const QString assert_type = "PreventUserIdleDisplaySleep",
                        const QString reason_string = "Program is running");
C_API bool allowSleep();

#endif // POWERTOOLS_H
