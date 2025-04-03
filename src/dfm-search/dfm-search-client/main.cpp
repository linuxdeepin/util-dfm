// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>

#include <dfm-search/dsearch_global.h>

using namespace dfmsearch;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    QStringList args = a.arguments();
    
    if (args.size() < 3) {
        qDebug() << "Usage: dfm6-search-client <keyword> <path1> [path2] ...";
        return 1;
    }
    
    QString keyword = args.at(1);
    QStringList paths = args.mid(2);
    
    qDebug() << "Searching for:" << keyword;
    qDebug() << "In paths:" << paths;
    
    
    
    return a.exec();
} 