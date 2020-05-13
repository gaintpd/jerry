#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include "viewController/enterposboard.h"
#include "tab_pos_search.h"

TabPosSearch::TabPosSearch(GameModel *model, QWidget *parent) : QWidget(parent)
{
       QFontMetrics f = this->fontMetrics();
       int len_moves = f.width("99")*3;

       this->firstMove = new QSpinBox(this);
       this->lastMove = new QSpinBox(this);
       this->occursAtLeast = new QSpinBox(this);

       firstMove->setFixedWidth(len_moves);
       lastMove->setFixedWidth(len_moves);
       occursAtLeast->setFixedWidth(len_moves);

       firstMove->setRange(0,999);
       lastMove->setRange(0,999);
       occursAtLeast->setRange(0,999);

       QLabel *lblFirstMove = new QLabel("after move:", this);
       QLabel *lblLastMove = new QLabel("before move:", this);
       QLabel *lblOccursLast = new QLabel("occurs at least:", this);

       firstMove->setValue(1);
       lastMove->setValue(40);
       occursAtLeast->setValue(1);

       chess::Board board(*model->getGame()->getCurrentNode()->getBoard());
       ColorStyle cs = model->colorStyle;
       this->enterPos = new EnterPosBoard(cs, board, this);

       this->buttonInit = new QPushButton(tr("Initial Position"), this);
       this->buttonClear = new QPushButton(tr("Clear Board"), this);
       this->buttonCurrent = new QPushButton(tr("Current Position"), this);
       this->buttonFlipBoard = new QPushButton(tr("Flip Board"), this);
       this->buttonFlipBoard->setCheckable(true);
       this->buttonFlipBoard->setChecked(false);

       QGridLayout *layoutSpinBoxes = new QGridLayout();
       layoutSpinBoxes->addWidget(lblFirstMove, 0, 0);
       layoutSpinBoxes->addWidget(firstMove, 0, 1);
       layoutSpinBoxes->addWidget(lblLastMove, 1, 0);
       layoutSpinBoxes->addWidget(lastMove, 1, 1);
       layoutSpinBoxes->addWidget(lblOccursLast, 2, 0);
       layoutSpinBoxes->addWidget(occursAtLeast, 2, 1);


       QVBoxLayout *vbox_config = new QVBoxLayout();
       vbox_config->addLayout(layoutSpinBoxes);
       vbox_config->addStretch(1);
       vbox_config->addWidget(buttonInit);
       vbox_config->addWidget(buttonClear);
       vbox_config->addWidget(buttonCurrent);
       vbox_config->addWidget(buttonFlipBoard);

       QHBoxLayout *hbox = new QHBoxLayout();
       hbox->addWidget(enterPos);
       hbox->addLayout(vbox_config);

       connect(this->buttonInit, &QPushButton::clicked, this, &TabPosSearch::setToInitialPosition);
       connect(this->buttonFlipBoard, &QPushButton::clicked, this, &TabPosSearch::flipBoard);
       connect(this->buttonClear, &QPushButton::clicked, this, &TabPosSearch::clearBoard);
       connect(this->buttonCurrent, &QPushButton::clicked, this, &TabPosSearch::setToCurrentBoard);

       this->setLayout(hbox);

}


void TabPosSearch::flipBoard() {

    if(this->buttonFlipBoard->isChecked()) {
        this->enterPos->setFlipBoard(true);
    } else {
        this->enterPos->setFlipBoard(false);
    }
    this->update();
}

void TabPosSearch::setToInitialPosition() {
    this->enterPos->setToInitialPosition();
}

void TabPosSearch::clearBoard() {
    this->enterPos->clearBoard();
    this->update();
}

void TabPosSearch::setToCurrentBoard() {
    this->enterPos->setToCurrentBoard();
}

chess::Board TabPosSearch::getBoard() {
    return this->enterPos->getCurrentBoard();
}
