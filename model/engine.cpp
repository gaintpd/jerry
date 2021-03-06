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

#include "engine.h"
#include <QDebug>

Engine::Engine()
{
    this->name = QString("");
    this->path = QString("");
    //this->uciOptions = new QList<EngineOption*>();
    this->isInternal = false;
    this->isActive = false;
}

// create deep copy
/*
Engine::Engine(Engine *e) {
    this->name = QString(e->getName());
    this->path = QString(e->getPath());
    this->uciOptions = new QList<EngineOption*>();
    for(int i=0;i<e->getUciOptions()->count();i++) {
        EngineOption *copy = new EngineOption(*e->getUciOptions()->at(i));
        this->uciOptions->append(copy);
    }
    this->isActive = e->isActive;
}
*/


void Engine::setUciSpinOption(int opt_idx, int value) {
    if(opt_idx >= 0 && opt_idx < this->uciOptions.size()) {
        this->uciOptions[opt_idx].spin_val = value;
    }
}

void Engine::setUciComboOption(int opt_idx, QString value) {
    if(opt_idx >= 0 && opt_idx < this->uciOptions.size()) {
        this->uciOptions[opt_idx].combo_val = value;
    }
}

void Engine::setUciCheckOption(int opt_idx, bool value) {
    if(opt_idx >= 0 && opt_idx < this->uciOptions.size()) {
        this->uciOptions[opt_idx].check_val = value;
    }
}

void Engine::setUciStringOption(int opt_idx, QString value) {
    if(opt_idx >= 0 && opt_idx < this->uciOptions.size()) {
        this->uciOptions[opt_idx].string_val = value;
    }
}

void Engine::addEngineOption(EngineOption o) {
    this->uciOptions.append(o);
}

void Engine::removeEngineOption(int idx_option) {
    if(idx_option >= 0 && idx_option < this->uciOptions.size()) {
        this->uciOptions.removeAt(idx_option);
    }
}

void Engine::clearAllEngineOptions() {
    this->uciOptions.clear();
}

void Engine::setEngineOptions(QVector<EngineOption> opts) {
    this->uciOptions = opts;
}

QString Engine::getPath() {
    return this->path;
}

QString Engine::getName() {
    return this->name;
}

QVector<EngineOption> Engine::getUciOptions() {
    return this->uciOptions;
}

void Engine::setPath(QString &path) {
    this->path = path;
}

void Engine::setName(QString &name) {
    this->name = name;
}

// return -1 if not found, otherwise index of list
int Engine::existsEngineOption(QString &name) {
    for(int i=0;i<this->uciOptions.count();i++) {
        EngineOption ei = this->uciOptions.at(i);
        if(ei.name == name) {
            return i;
        }
    }
    return -1;
}
