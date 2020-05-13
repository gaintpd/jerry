#ifndef TABPOSSEARCH_H
#define TABPOSSEARCH_H

#include <QWidget>
#include <QPushButton>
#include <QSpinBox>
#include "model/game_model.h"
#include "viewController/enterposboard.h"

class TabPosSearch : public QWidget
{
    Q_OBJECT
public:
    explicit TabPosSearch(GameModel* model, QWidget *parent = nullptr);

    QSpinBox *firstMove;
    QSpinBox *lastMove;
    QSpinBox *occursAtLeast;

    QPushButton *buttonInit;
    QPushButton *buttonClear;
    QPushButton *buttonFlipBoard;
    QPushButton *buttonCurrent;

    chess::Board getBoard();

private:
    EnterPosBoard *enterPos;
    chess::Board board;

    signals:

public slots:
    void flipBoard();
    void clearBoard();
    void setToCurrentBoard();
    void setToInitialPosition();

};

#endif // TABPOSSEARCH_H
