TARGET   = sqlite3tools
TEMPLATE = lib
DESTDIR  = $$PWD/../bin

INCLUDEPATH += $$PWD/../sqlite3

LIBS = -L$$DESTDIR -lsqlite3

HEADERS += sqlite3.h

SOURCES += showdb.c
