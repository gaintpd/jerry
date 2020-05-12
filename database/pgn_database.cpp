#include "chess/pgn_reader.h"
#include "pgn_database.h"
#include <QDebug>
#include <QFile>
#include <QProgressDialog>
#include <QTextCodec>
#include "chess/pgn_printer.h"
#include "chess/node_pool.h"
#include <QProgressDialog>
#include <iostream>
#include <QCoreApplication>

PgnDatabase::PgnDatabase()
{
    this->parentWidget = nullptr;
    this->filename = "";
    //this->cacheSize = 50;
    this->isUtf8 = true;
    this->currentlyOpen = false;
    this->lastSelectedIndex = 0;
}

PgnDatabase::~PgnDatabase() {

}

bool PgnDatabase::isOpen() {
    return this->currentlyOpen;
}

int PgnDatabase::createNew(QString &filename) {

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return -1;
    } else {
        QTextStream out(&file);
        out << "\n";
        file.close();

        this->filename = filename;
        this->allOffsets.clear();
        this->searchedOffsets.clear();
        this->currentlyOpen = true;
        return 0;
    }
}


void PgnDatabase::setLastSelectedIndex(int idx) {
    if(idx > 0 && idx < this->searchedOffsets.size()) {
        this->lastSelectedIndex = idx;
    }
}

int PgnDatabase::getLastSelectedIndex() {
    return this->lastSelectedIndex;
}

int PgnDatabase::appendCurrentGame(chess::Game &game) {

    QFile file(this->filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return -1;
    } else {
        QTextStream out(&file);
        out << "\n";
        qint64 gamePos = out.pos();
        chess::PgnPrinter printer;
        QStringList pgn = printer.printGame(&game);
        for (int i = 0; i < pgn.size(); ++i) {
            out << pgn.at(i) << '\n';
        }
        file.close();
        this->allOffsets.append(gamePos);
        this->searchedOffsets.clear();
        this->searchedOffsets = this->allOffsets;
        this->lastSelectedIndex = this->countGames() - 1;
        return 0;
    }


}

void PgnDatabase::setParentWidget(QWidget *parentWidget) {
    this->parentWidget = parentWidget;
}

QVector<qint64> PgnDatabase::scanPgn(QString &filename, bool is_utf8) {

    /*
    auto start = std::chrono::steady_clock::now();
    QVector<qint64> temp = this->reader.scanPgn(filename, is_utf8);
    auto stop = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = (stop - start);
    std::chrono::milliseconds i_millis = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    std::cout << "reading w/o even proc.:" << i_millis.count() <<  "ms" << std::endl;


    start = std::chrono::steady_clock::now();
    */

    QVector<qint64> offsets;
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return offsets;

    qint64 fileSize = file.size();

    QProgressDialog dlgProgress("Processing Data...", "Abort Operation", 0, fileSize, this->parentWidget);
    dlgProgress.setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    dlgProgress.setWindowTitle("Processing");
    dlgProgress.setWindowModality(Qt::WindowModal);
    dlgProgress.setMinimumDuration(400);
    dlgProgress.show();

    bool inComment = false;

    qint64 game_pos = -1;

    QByteArray byteLine;
    QString line("");
    qint64 last_pos = file.pos();

    int i= 0;
    while(!file.atEnd()) {
        i++;
        byteLine = file.readLine();
        if(!is_utf8) {
            line = QString::fromLatin1(byteLine);
        } else {
            line = QString::fromUtf8(byteLine);
        }

        // skip comments
        if(line.startsWith("%")) {
            byteLine = file.readLine();
            continue;
        }

        if(!inComment && line.startsWith("[")) {

            if(game_pos == -1) {
                game_pos = last_pos;
            }
            last_pos = file.pos();
            continue;
        }
        if((!inComment && line.contains("{"))
                || (inComment && line.contains("}"))) {
            inComment = line.lastIndexOf("{") > line.lastIndexOf("}");
        }

        if(game_pos != -1) {
            offsets.append(game_pos);
            game_pos = -1;
        }

        last_pos = file.pos();

        if(offsets.length() % 2500 == 0) { // 1000 games per second
            dlgProgress.setValue(last_pos);
            if(dlgProgress.wasCanceled()) {
                break;
            }
            QCoreApplication::processEvents();
        }

        byteLine = file.readLine();
    }
    // for the last game
    if(game_pos != -1) {
        offsets.append(game_pos);
        game_pos = -1;
    }

    /*
    stop = std::chrono::steady_clock::now();
    diff = (stop - start);
    i_millis = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
    std::cout << "reading w/ even proc..:" << i_millis.count() <<  "ms" << std::endl;
    */
    return offsets;
}


void PgnDatabase::open(QString &filename) {

    bool isUtf8 = this->reader.isUtf8(filename);
    this->searchedOffsets = this->scanPgn(filename, isUtf8);
    this->filename = filename;
    this->currentlyOpen = true;
    this->lastSelectedIndex = -1;
}


void PgnDatabase::close() {
    this->allOffsets.clear();
    this->searchedOffsets.clear();
    this->filename = "";
    this->currentlyOpen = false;
    this->lastSelectedIndex = -1;
}

int PgnDatabase::getRowCount() {
    return this->searchedOffsets.size();
}

chess::Game* PgnDatabase::getGameAt(int idx) {
    chess::Game *g = new chess::Game();
    this->lastSelectedIndex = idx;

    bool is_utf8 = this->reader.isUtf8(this->filename);

    QFile file(this->filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        g;
    }
    QTextStream in(&file);
    QTextCodec *codec;
    if(is_utf8) {
        codec = QTextCodec::codecForName("UTF-8");
    } else {
        codec = QTextCodec::codecForName("ISO 8859-1");
    }
    in.setCodec(codec);
    int res = this->reader.readGame(in, this->searchedOffsets.at(idx), g);
    if(res == chess::GENERAL_ERROR) {
        chess::GameNode *root = g->getRootNode();
        if(root != nullptr) {
            chess::NodePool::deleteNode(root);
        }
        delete g;
        return new chess::Game();
    } else {

        return g;
    }
}

bool PgnDatabase::pgnHeaderMatches(QFile &file, SearchPattern &pattern, qint64 offset) {


    bool foundHeader = false;
    bool continueSearch = true;

    QByteArray byteLine;
    QString line;
    file.seek(offset);

    QString pattern_year_min = QString::number(pattern.year_min);
    QString pattern_year_max = QString::number(pattern.year_max);

    QString elo_min = QString::number(pattern.elo_min);
    QString elo_max = QString::number(pattern.elo_max);

    int whiteElo = -1;
    int blackElo = -1;

    QString whiteName = "";
    QString blackName = "";

    while(!file.atEnd() && continueSearch) {
        byteLine = file.readLine();
        line = QString::fromUtf8(byteLine);
        if(line.startsWith("%") || line.isEmpty()) {
            byteLine = file.readLine();
            continue;
        }

        QRegularExpressionMatch match_t = chess::TAG_REGEX.match(line);

        if(match_t.hasMatch()) {

            foundHeader = true;

            QString tag = match_t.captured(1);
            QString value = match_t.captured(2);

            if(!pattern.event.isEmpty() && tag == "Event") {
                if(!value.contains(pattern.event, Qt::CaseInsensitive)) {
                        return false;
                }
            }
            if(!pattern.site.isEmpty() && tag == "Site") {
                if(!value.contains(pattern.site, Qt::CaseInsensitive)) {
                    return false;
                }
            }
            if(pattern.checkYear && tag == "Date") {
                QString year_i = value.left(4);
                if(year_i < pattern_year_min) {
                    return false;
                }
                if(year_i > pattern_year_max) {
                    return false;
                }
            }
            if(pattern.checkEco && tag == "ECO") {
                if(value < pattern.ecoStart || value > pattern.ecoStop) {
                    return false;
                }
            }
            if(tag == "WhiteElo") {
                whiteElo = value.toInt();
            }
            if(tag == "blackElo") {
                blackElo = value.toInt();
            }
            if(tag == "White") {
                whiteName = value;
            }
            if(tag == "Black") {
                blackName = value;
            }
            if(pattern.result != chess::RES_ANY) {
                if(pattern.result == chess::RES_BLACK_WINS && !value.contains("0-1")) {
                    return false;
                } else if(pattern.result == chess::RES_DRAW && !value.contains("1/2-1/2")) {
                    return false;
                } else if(pattern.result == chess::RES_UNDEF && !value.contains("*")) {
                    return false;
                } else if(pattern.result == chess::RES_WHITE_WINS && !value.contains("1-0")) {
                    return false;
                }
            }
        } else {
            if(foundHeader) {
                if(pattern.ignoreNameColor) {
                    if(!pattern.whiteName.isEmpty()) {
                        // must match either black or white
                        if(!whiteName.contains(pattern.whiteName, Qt::CaseInsensitive)
                           && !blackName.contains(pattern.whiteName, Qt::CaseInsensitive)) {
                            return false;
                        }
                    }
                    if(!pattern.blackName.isEmpty()) {
                        // must match either black or white
                        if(!whiteName.contains(pattern.blackName, Qt::CaseInsensitive)
                           && !blackName.contains(pattern.blackName, Qt::CaseInsensitive)) {
                            return false;
                        }
                    }
                } else {
                    // if whiteName is not empty, then it must match
                    if(!pattern.whiteName.isEmpty() &&
                            !whiteName.contains(pattern.whiteName, Qt::CaseInsensitive)) {
                        return false;
                    }
                    if(!pattern.blackName.isEmpty() &&
                            !blackName.contains(pattern.blackName, Qt::CaseInsensitive)) {
                        return false;
                    }
                }
                if(pattern.checkElo != SEARCH_IGNORE_ELO) {
                    // only if we could find elo information for both players
                    if(whiteElo > 0 && blackElo > 0) {
                        if(pattern.checkElo == SEARCH_AVERAGE_ELO) {
                            int avg = (whiteElo + blackElo) / 2;
                            if(pattern.elo_min > avg || pattern.elo_max < avg) {
                                return false;
                            }
                        } else if(pattern.checkElo == SEARCH_ONE_ELO) {
                            if((pattern.elo_min > whiteElo || pattern.elo_max < whiteElo) &&
                                    (pattern.elo_min > blackElo || pattern.elo_max < blackElo)) {
                                return false;
                            }
                        } else if(pattern.checkElo == SEARCH_BOTH_ELO) {
                            if(pattern.elo_min > whiteElo || pattern.elo_max < whiteElo ||
                                pattern.elo_min > blackElo || pattern.elo_max < blackElo) {
                                return false;
                            }
                        }
                    }
                }
                return true;
            }
        }
    }
    return false;
}



bool PgnDatabase::pgnHeaderMatches1(QTextStream &openStream, SearchPattern &pattern, qint64 offset) {

    bool foundHeader = false;
    bool continueSearch = true;

    openStream.seek(offset);
    QString line = openStream.readLine();
    while(!openStream.atEnd() && continueSearch) {
        line = openStream.readLine();
        if(line.startsWith("%") || line.isEmpty()) {
            line = openStream.readLine();
            continue;
        }

        QRegularExpressionMatch match_t = chess::TAG_REGEX.match(line);

        if(match_t.hasMatch()) {

            foundHeader = true;

            QString tag = match_t.captured(1);
            QString value = match_t.captured(2);

            /*
            if(tag == "Event") {
                if(!value.contains(pattern.event, Qt::CaseInsensitive)) {
                    return false;
                }
            }
            if(tag == "Site") {
                if(!value.contains(pattern.site, Qt::CaseInsensitive)) {
                    return false;
                }
            }*/
            if(tag == "Date") {
                // todo: compare date
            }
            if(tag == "Round") {
                // todo
            }
            if(tag == "White") {
                if(!value.contains(pattern.whiteName, Qt::CaseInsensitive)) {
                    return false;
                } else {
                    return true;
                }
            }
            /*
            if(tag == "Black") {
                if(!value.contains(pattern.blackName, Qt::CaseInsensitive)) {
                    return false;
                }
            }*/
            if(tag == "Result") {
                // todo
            }
            if(tag == "ECO") {
                // todo
            }
        } else {
            if(foundHeader) {

                return true;
                //continueSearch = false;
                //break;
            }
        }
    }
    return false;
}


void PgnDatabase::search(SearchPattern &pattern) {

    QFile file(filename);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::invalid_argument("unable to open file w/ supplied filename");
    }

    this->searchedOffsets.clear();

    QProgressDialog progress(this->parentWidget->tr("searching..."), this->parentWidget->tr("Cancel"), 0, this->countGames(), this->parentWidget);
    progress.setMinimumDuration(400);
    progress.setWindowModality(Qt::WindowModal);
    //progress.setCancelButton(0);
    //progress.show();

    quint64 stepCounter = 0;
    //qDebug() << pattern.whiteName;

    for(int i=0;i<this->allOffsets.size();i++) {

        if(progress.wasCanceled()) {
            break;
        }

        if(stepCounter %50 == 0) {
            progress.setValue(i);
            stepCounter = 0;
        }
        stepCounter += 1;

        qint64 offset_i = this->allOffsets.at(i);
        if(this->pgnHeaderMatches(file, pattern, offset_i)) {
            this->searchedOffsets.append(offset_i);
        }
    }
    file.close();
}


/*
void chess::PgnDatabase::search(SearchPattern &pattern) {

    QFile file(filename);

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::invalid_argument("unable to open file w/ supplied filename");
    }
    QTextStream in(&file);
    QTextCodec *codec;
    if(isUtf8) {
        codec = QTextCodec::codecForName("UTF-8");
    } else {
        codec = QTextCodec::codecForName("ISO 8859-1");
    }
    in.setCodec(codec);

    this->searchedOffsets.clear();

    QProgressDialog progress(this->parentWidget->tr("searching..."), this->parentWidget->tr("Cancel"), 0, this->countGames(), this->parentWidget);
    progress.setMinimumDuration(400);
    progress.setWindowModality(Qt::WindowModal);
    progress.setCancelButton(0);
    progress.show();

    quint64 stepCounter = 0;
    qDebug() << pattern.whiteName;

    for(int i=0;i<this->allOffsets.size();i++) {


        if(stepCounter %50 == 0) {
            progress.setValue(i);
            stepCounter = 0;
        }
        stepCounter += 1;

        qint64 offset_i = this->allOffsets.at(i);
        if(this->pgnHeaderMatches(in, pattern, offset_i)) {
            this->searchedOffsets.append(offset_i);
        }
    }
    file.close();
}*/


/*
chess::PgnHeader chess::PgnDatabase::getRowInfo(int idx) {

    if(this->headerCache.contains(idx)) {
        qDebug() << "in cache";
        chess::PgnHeader header = headerCache.value(idx);
        return header;
    } else {
        int start = std::max(idx - 10, 0);
        int stop = std::min(this->offsets.size(), idx + 30);
        QVector<qint64> cacheOffsets;
        for(int i=start; i<=stop;i++) {
            qint64 oi = this->offsets.at(i);
            cacheOffsets.append(oi);
        }
        const char* utf8 = "UTF-8";
        chess::PgnHeader h;
        if(idx >= this->offsets.size()) {
            return h;
        } else {

            QVector<PgnHeaderOffset> cacheHeaderOffsets = this->reader.readMultipleHeadersFromPgnAround(this->filename, cacheOffsets, utf8);
            for(int i=0;i<cacheHeaderOffsets.size();i++) {
                chess::PgnHeader ci = cacheHeaderOffsets.at(i).header;
                qint64 ii = cacheHeaderOffsets.at(i).offset;
                this->headerCache.insert(ii, ci);
                if(ii == this->offsets.at(idx)) {
                    h = cacheHeaderOffsets.at(i).header;
                }
            }
            return h;

            //qint64 offset = this->offsets.at(idx);
            //chess::PgnHeader h_idx = this->reader.readSingleHeaderFromPgnAt(this->filename, offset, utf8);
            //return h_idx;
        }
    }
}
*/

chess::PgnHeader PgnDatabase::getRowInfo(int idx) {

        chess::PgnHeader h;
        if(idx >= this->searchedOffsets.size()) {
            return h;
        } else {

            qint64 offset = this->searchedOffsets.at(idx);
            chess::PgnHeader h_idx = this->reader.readSingleHeaderFromPgnAt(this->filename, offset, this->isUtf8);
            return h_idx;
        }
}

void PgnDatabase::resetSearch() {
    this->searchedOffsets.clear();
    this->searchedOffsets = this->allOffsets;
    this->lastSelectedIndex = -1;
}


int PgnDatabase::countGames() {
    //qDebug() << "pgn database: count games";

    return this->allOffsets.size();
}
