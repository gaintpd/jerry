/* Jerry - A Chess Graphical User Interface
 * Copyright (C) 2014-2016 Dominik Klein
 * Copyright (C) 2015-2016 Karl Josef Klein
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <QPainter>
#include <QGuiApplication>
#include "boardviewcontroller.h"
#include <QDebug>
#include <QMouseEvent>
#include <iostream>
#include <QGraphicsLineItem>
#include <assert.h>
#include "dialogs/dialog_promotion.h"
#include "various/resource_finder.h"
#include <math.h>

BoardViewController::BoardViewController(GameModel *gameModel, QWidget *parent) :
    Chessboard(parent)
{
    QSizePolicy *policy = new QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    this->setSizePolicy(*policy);

    this->colorClickSrc = new QPoint(-1,-1);

    this->transpRed = QColor(200,0,0,100);
    this->transpGreen = QColor(0,200,0,64);
    this->transpYellow = QColor(200,200,0,64);
    this->yellow = QColor(200,200,0);
    this->green = QColor(50,88,0);
    this->red = QColor(200,0,0);

    this->selArrowColor = this->green;
    this->selFieldColor = this->transpGreen;
    this->colorClick = false;

    this->gameModel = gameModel;

    this->show();
}


void BoardViewController::paintEvent(QPaintEvent *event) {
    QPainter *painter = new QPainter();
    painter->begin(this);
    this->drawBoard(event, painter);
    painter->end();
}

/*
void BoardViewController::resizeEvent(QResizeEvent* ) {
    this->setMinimumWidth(this->height());
}*/

void BoardViewController::flipBoard() {
    this->gameModel->flipBoard = !this->gameModel->flipBoard;
    this->setFlipBoard(this->gameModel->flipBoard);
    this->update();
}

void BoardViewController::resetMove() {
    this->moveSrc.setX(-1);
    this->moveSrc.setY(-1);
    this->grabbedPiece.piece_type = chess::EMPTY;
    this->drawGrabbedPiece = false;
}

void BoardViewController::touchPiece(int x, int y, int mouse_x, int mouse_y) {
    this->moveSrc.setX(x);
    this->moveSrc.setY(y);
    int idx = this->xyToBoardIdx(x, y);
    this->grabbedPiece.piece_type = this->gameModel->getGame()->getCurrentNode()->getBoard()->get_piece_type(idx);
    this->grabbedPiece.color = this->gameModel->getGame()->getCurrentNode()->getBoard()->get_piece_color(idx);
    this->grabbedPiece.x = mouse_x;
    this->grabbedPiece.y = mouse_y;
    this->drawGrabbedPiece = true;
}

QPoint BoardViewController::getBoardPosition(int x, int y) {
    QPoint q;
    q.setX(-1);
    q.setY(-1);
    QPoint qpSize = Chessboard::calculateBoardSize();
    int boardSize = qpSize.x();
    int squareSize = qpSize.y();
    int addLeft = (this->width() - boardSize) / 2;
    if(addLeft < 0) {
        addLeft = 0;
    }
    if(x > (this->borderWidth + addLeft) && y > this->borderWidth
            && x < ((boardSize +addLeft) - this->borderWidth) && y < (boardSize - this->borderWidth)) {
        x = x - (this->borderWidth + addLeft);
        y = y - (this->borderWidth);
        x = x / squareSize;
        y = 7 - (y / squareSize);
        q.setX(x);
        q.setY(y);
    }
    return q;
}

int BoardViewController::xyToBoardIdx(int x, int y) {
    int board_idx = ((y+2)*10)+x+1;
    if(this->gameModel->flipBoard) {
        board_idx = 119 - board_idx;
    }
    return board_idx;
}

void BoardViewController::handleColoringOnKeyPress(const QPoint &pos) {
        // user clicked and is going to draw arrow or mark a field
        if(pos.x() != -1 && pos.y() != -1 && !this->colorClick) {
            int x = pos.x();
            int y = pos.y();
            if(this->gameModel->flipBoard) {
                x = 7 - x;
                y = 7 - y;
            }
            this->colorClickSrc->setX(x);
            this->colorClickSrc->setY(y);
            this->colorClick = true;
            this->setGrabbedArrowFrom(x,y);
            this->drawGrabbedArrow = true;
        }
}

void BoardViewController::applyMove(chess::Move &m) {
    this->gameModel->getGame()->applyMove(m);
    this->gameModel->triggerStateChange();
}

void BoardViewController::mousePressEvent(QMouseEvent *me) {
    QPoint pos = this->getBoardPosition(me->x(), me->y());
    if(me->button() == Qt::RightButton)
    {
        this->handleColoringOnKeyPress(pos);
    } else if(me->button() == Qt::LeftButton) {
        chess::Board *b = this->gameModel->getGame()->getCurrentNode()->getBoard();
        int to_idx = this->xyToBoardIdx(pos.x(), pos.y());
        if(pos.x() != -1 && pos.y() != -1) {
            if(this->grabbedPiece.piece_type != -1) {
                int from_idx = this->xyToBoardIdx(this->moveSrc.x(), this->moveSrc.y());
                chess::Move m(from_idx,to_idx);
                if(b->is_legal_and_promotes(m)) {
                    DialogPromotion d(b->turn,this->parentWidget());
                    d.exec();
                    m.promotion_piece = d.promotesTo;
                    this->applyMove(m);
                    this->resetMove();
                } else if(b->is_legal_move(m)) {
                    this->applyMove(m);
                    this->resetMove();
                } else {
                    this->resetMove();
                    if(b->get_piece_type(to_idx) != chess::EMPTY) {
                        this->touchPiece(pos.x(),pos.y(),me->x(),me->y());
                    }
                }
            } else {
                if(b->get_piece_type(to_idx) != chess::EMPTY) {
                    this->touchPiece(pos.x(),pos.y(),me->x(),me->y());
                }
            }
        }
    }
}

void BoardViewController::mouseMoveEvent(QMouseEvent *m) {

    Qt::MouseButton button = m->button();
    if(button == Qt::NoButton &&
            this->drawGrabbedPiece && this->grabbedPiece.piece_type != chess::EMPTY && !this->colorClick) {
        this->grabbedPiece.x = m->x();
        this->grabbedPiece.y = m->y();
        this->update();
    }
    if(button == Qt::NoButton && this->drawGrabbedArrow) {
        QPoint xy = this->getBoardPosition(m->x(),m->y());
        if(this->gameModel->flipBoard) {
            xy.setX(7-xy.x());
            xy.setY(7-xy.y());
        }
        this->setGrabbedArrowTo(xy.x(), xy.y());
        this->update();
    }

}

void BoardViewController::handleColoringonKeyRelease(const QPoint &pos) {
        // user clicked and is going to draw arrow
        if(pos.x() != -1 && pos.y() != -1 && this->colorClick) {
            int x = pos.x();
            int y = pos.y();
            if(this->gameModel->flipBoard) {
                x = 7 - x;
                y = 7 - y;
            }
            // arrow case
            if(pos.x() != this->colorClickSrc->x() || pos.y() != this->colorClickSrc->y()) {
                chess::Arrow a;
                a.from = QPoint(this->colorClickSrc->x(), this->colorClickSrc->y());
                a.to = QPoint(x, y);
                a.color = this->green;
                this->gameModel->getGame()->getCurrentNode()->addOrDelArrow(a);
            } else {
                chess::ColoredField c;
                c.field = QPoint(x, y);
                c.color = this->transpRed;
                this->gameModel->getGame()->getCurrentNode()->addOrDelColoredField(c);
            }
        }
        this->drawGrabbedArrow = false;
}

void BoardViewController::mouseReleaseEvent(QMouseEvent *m) {
    this->drawGrabbedPiece = false;
    QPoint pos = this->getBoardPosition(m->x(), m->y());
    if(m->button() == Qt::RightButton)
    {
        this->handleColoringonKeyRelease(pos);

    } else if(m->button() == Qt::LeftButton){
        chess::Board *b = this->gameModel->getGame()->getCurrentNode()->getBoard();
        if(pos.x() != -1 && pos.y() != -1 && this->grabbedPiece.piece_type != chess::EMPTY) {
            if(!(pos.x() == this->moveSrc.x() && pos.y() == this->moveSrc.y())) {
                int from_idx = this->xyToBoardIdx(this->moveSrc.x(), this->moveSrc.y());
                int to_idx = this->xyToBoardIdx(pos.x(), pos.y());
                chess::Move m(from_idx,to_idx);
                if(b->is_legal_and_promotes(m)) {
                    DialogPromotion d(b->turn,this->parentWidget());
                    d.exec();
                    m.promotion_piece = d.promotesTo;
                    this->applyMove(m);
                } else if(b->is_legal_move(m)) {
                    this->applyMove(m);
                } else {
                    this->resetMove();
                }
            }
        }
    }
    this->colorClick = false;
    this->update();
}


void BoardViewController::onStateChange() {
    this->lastMove = this->gameModel->getGame()->getCurrentNode()->getMove();
    this->style = this->gameModel->colorStyle;
    this->setFlipBoard(this->gameModel->flipBoard);
    this->update();
}

void BoardViewController::drawBoard(QPaintEvent *event, QPainter *painter) {

    //Chessboard::board = this->gameModel->getGame()->getCurrentNode()->getBoard();
    //Chessboard::currentArrows = this->gameModel->getGame()->getCurrentNode()->getArrows();
    //Chessboard::currentColoredFields = this->gameModel->getGame()->getCurrentNode()->getColoredFields();
    chess::GameNode *node = this->gameModel->getGame()->getCurrentNode();
    this->setBoard(*(node->getBoard()));
    this->setArrows(node->getArrows());
    this->setColoredFields(node->getColoredFields());

    Chessboard::drawBoard(event, painter);

}
