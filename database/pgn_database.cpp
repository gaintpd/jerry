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
    dlgProgress.setWindowModality(Qt::ApplicationModal);
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
    this->allOffsets = searchedOffsets;
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

bool PgnDatabase::pgnHeaderMatches(SearchPattern &pattern, chess::PgnHeader &header) {

    if(!pattern.event.isEmpty()) {
        if(!header.event.contains(pattern.event, Qt::CaseInsensitive)) {
                return false;
        }
    }

    if(!pattern.site.isEmpty()) {
        if(!header.site.contains(pattern.site, Qt::CaseInsensitive)) {
                return false;
        }
    }

    if(pattern.checkYear) {
        QString year_i = header.date.left(4);
        if(year_i < pattern.year_min) {
            return false;
        }
        if(year_i > pattern.year_max) {
            return false;
        }
    }

    if(pattern.checkEco) {
        if(header.eco < pattern.ecoStart || header.eco > pattern.ecoStop) {
            return false;
        }
    }

    if(pattern.result != chess::RES_ANY) {
        if(pattern.result == chess::RES_BLACK_WINS && !header.result.contains("0-1")) {
            return false;
        } else if(pattern.result == chess::RES_DRAW && !header.result.contains("1/2-1/2")) {
            return false;
        } else if(pattern.result == chess::RES_UNDEF && !header.result.contains("*")) {
            return false;
        } else if(pattern.result == chess::RES_WHITE_WINS && !header.result.contains("1-0")) {
            return false;
        }
    }

    if(pattern.ignoreNameColor) {
        if(!pattern.whiteName.isEmpty()) {
            // must match either black or white
            if(!header.white.contains(pattern.whiteName, Qt::CaseInsensitive)
               && !header.black.contains(pattern.whiteName, Qt::CaseInsensitive)) {
                return false;
            }
        }
        if(!pattern.blackName.isEmpty()) {
            // must match either black or white
            if(!header.white.contains(pattern.blackName, Qt::CaseInsensitive)
               && !header.black.contains(pattern.blackName, Qt::CaseInsensitive)) {
                return false;
            }
        }
    } else {
        // if whiteName is not empty, then it must match
        if(!pattern.whiteName.isEmpty() &&
                !header.white.contains(pattern.whiteName, Qt::CaseInsensitive)) {
            return false;
        }
        if(!pattern.blackName.isEmpty() &&
                !header.black.contains(pattern.blackName, Qt::CaseInsensitive)) {
            return false;
        }
    }

    if(pattern.checkElo != SEARCH_IGNORE_ELO) {
        int whiteElo = header.whiteElo.toInt();
        int blackElo = header.blackElo.toInt();
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



void PgnDatabase::search(SearchPattern &pattern) {

    //qDebug() << "PgnDatabase search ";
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

    //qDebug() << "PgnDatabase search 2";

    this->searchedOffsets.clear();

    //QProgressDialog progress(this->parentWidget->tr("searching..."), this->parentWidget->tr("Cancel"), 0, this->countGames(), this->parentWidget);
    QProgressDialog progress("searching...", "Cancel", 0, this->countGames());
    progress.setMinimumDuration(400);
    progress.setWindowModality(Qt::ApplicationModal);
    //progress.setCancelButton(0);
    progress.show();

    //qDebug() << "PgnDatabase search 3";

    quint64 stepCounter = 0;
    //qDebug() << pattern.whiteName;

    chess::Game *temp = new chess::Game();

    for(int i=0;i<this->allOffsets.size();i++) {
        //qDebug() << "search: "<<i;

        if(progress.wasCanceled()) {
            break;
        }

        if(stepCounter %100 == 0) {
            progress.setValue(i);
            stepCounter = 0;
        }
        stepCounter += 1;

        qint64 offset_i = this->allOffsets.at(i);

        bool headerMatches = false;
        if(pattern.searchGameData) { // search for game data
            //qDebug() << "search game data";
            chess::PgnHeader header_i = this->reader.readSingleHeaderFromPgnAt(in, offset_i);
            headerMatches = this->pgnHeaderMatches(pattern, header_i);
            if(pattern.searchPosition) { // search also for position
                this->reader.readGame(in, offset_i, temp);
                if(temp->matchesPosition(pattern.posHash) && headerMatches) {
                    this->searchedOffsets.append(offset_i);
                }
                chess::NodePool::deleteNode(temp->getRootNode());
                temp->reset();
            } else {
                if(headerMatches) {
                    this->reader.readGame(in, offset_i, temp);
                    chess::PgnPrinter pgnPrinter;
                    qDebug() << pgnPrinter.printGame(temp).join("\n");
                    chess::NodePool::deleteNode(temp->getRootNode());
                    temp->reset();
                    this->searchedOffsets.append(offset_i);
                }
            }
        } else {  // just check search position
            //qDebug() << "search position only";
            if(pattern.searchPosition) {
                this->reader.readGame(in, temp);
                if(temp->matchesPosition(pattern.posHash)) {
                    this->searchedOffsets.append(offset_i);
                }
                chess::NodePool::deleteNode(temp->getRootNode());
                temp->reset();
            }

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
