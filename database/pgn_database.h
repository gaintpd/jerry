#ifndef PGNDATABASE_H
#define PGNDATABASE_H

#include "database.h"
#include "model/search_pattern.h"
#include <QFile>
#include <QThread>
#include <QProgressDialog>

class PgnDatabase : public Database
{

public:

    PgnDatabase();
    ~PgnDatabase();

    void setParentWidget(QWidget *parentWidget);
    void open(QString &filename);
    void close();
    int createNew(QString &filename);
    int appendCurrentGame(chess::Game &game);

    int getRowCount();
    chess::Game* getGameAt(int idx);
    chess::PgnHeader getRowInfo(int idx);
    int countGames();
    bool isOpen();
    void search(SearchPattern &pattern);
    void resetSearch();
    void setLastSelectedIndex(int idx);
    int getLastSelectedIndex();

private:
    QVector<qint64> allOffsets;
    QVector<qint64> searchedOffsets;
    QWidget *parentWidget;
    chess::PgnReader reader;
    QString filename;
    QHash<qint64, chess::PgnHeader> headerCache;
    int cacheSize;
    bool isUtf8;
    bool currentlyOpen;
    int lastSelectedIndex;
    bool pgnHeaderMatches(QFile &file, SearchPattern &pattern, qint64 offset);
    bool pgnHeaderMatches(SearchPattern &pattern, chess::PgnHeader &header);
    QVector<qint64> scanPgn(QString &filename, bool is_utf8);


};

#endif // PGNDATABASE_H
