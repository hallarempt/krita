/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "kis_debug.h"

#include "config-debug.h"

#if HAVE_BACKTRACE
#include <execinfo.h>
#ifdef __GNUC__
#define HAVE_BACKTRACE_DEMANGLE
#include <cxxabi.h>
#endif
#endif

#include <string>


#if HAVE_BACKTRACE
static QString maybeDemangledName(char *name)
{
#ifdef HAVE_BACKTRACE_DEMANGLE
    const int len = strlen(name);
    QByteArray in = QByteArray::fromRawData(name, len);
    const int mangledNameStart = in.indexOf("(_");
    if (mangledNameStart >= 0) {
        const int mangledNameEnd = in.indexOf('+', mangledNameStart + 2);
        if (mangledNameEnd >= 0) {
            int status;
            // if we forget about this line and the one that undoes its effect we don't change the
            // internal data of the QByteArray::fromRawData() ;)
            name[mangledNameEnd] = 0;
            char *demangled = abi::__cxa_demangle(name + mangledNameStart + 1, 0, 0, &status);
            name[mangledNameEnd] = '+';
            if (demangled) {
                QString ret = QString::fromLatin1(name, mangledNameStart + 1) +
                              QString::fromLatin1(demangled) +
                              QString::fromLatin1(name + mangledNameEnd, len - mangledNameEnd);
                free(demangled);
                return ret;
            }
        }
    }
#endif
    return QString::fromLatin1(name);
}
#endif

QString kisBacktrace()
{
    QString s;
#if HAVE_BACKTRACE
    void *trace[256];
    int n = backtrace(trace, 256);
    if (!n) {
        return s;
    }
    char **strings = backtrace_symbols(trace, n);

    s = QLatin1String("[\n");

    for (int i = 0; i < n; ++i)
        s += QString::number(i) + QLatin1String(": ") +
             maybeDemangledName(strings[i]) + QLatin1Char('\n');
    s += QLatin1String("]\n");
    if (strings) {
        free(strings);
    }
#endif
    return s;
}

Q_LOGGING_CATEGORY(_30009, "krita.lib.resources")
Q_LOGGING_CATEGORY(_41000, "krita.general")
Q_LOGGING_CATEGORY(_41001, "krita.core")
Q_LOGGING_CATEGORY(_41002, "krita.registry")
Q_LOGGING_CATEGORY(_41003, "krita.tools")
Q_LOGGING_CATEGORY(_41004, "krita.tiles")
Q_LOGGING_CATEGORY(_41005, "krita.filters")
Q_LOGGING_CATEGORY(_41006, "krita.plugins")
Q_LOGGING_CATEGORY(_41007, "krita.ui")
Q_LOGGING_CATEGORY(_41008, "krita.file")
Q_LOGGING_CATEGORY(_41009, "krita.math")
Q_LOGGING_CATEGORY(_41010, "krita.render")
Q_LOGGING_CATEGORY(_41011, "krita.scripting")
Q_LOGGING_CATEGORY(_41012, "krita.input")
Q_LOGGING_CATEGORY(_41013, "krita.action")
Q_LOGGING_CATEGORY(_41014, "krita.tabletlog")



const char* __methodName(const char *_prettyFunction)
{
    std::string prettyFunction(_prettyFunction);

    size_t colons = prettyFunction.find("::");
    size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
    size_t end = prettyFunction.rfind("(") - begin;

    return std::string(prettyFunction.substr(begin,end) + "()").c_str();
}
