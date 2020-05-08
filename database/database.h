#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QWidget>
#include "chess/game.h"
#include "chess/pgn_reader.h"
#include "model/search_pattern.h"

class Database
{

public:

    virtual ~Database() = 0;

    virtual void setParentWidget(QWidget *parentWidget) = 0;
    virtual void open(QString &filename) = 0;
    virtual void close() = 0;
    virtual int createNew(QString &filename) = 0;
    virtual void search(SearchPattern &pattern) = 0;

    virtual int getRowCount() = 0;
    virtual chess::Game* getGameAt(int idx) = 0;
    virtual chess::PgnHeader getRowInfo(int idx) = 0;
    virtual int countGames() = 0;

};

#endif // DATABASE_H
