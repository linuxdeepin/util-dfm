// SPDX-FileCopyrightText: 2021 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DENUMERATORFUTURE_H
#define DENUMERATORFUTURE_H

#include <dfm-io/dfmio_global.h>
#include <dfm-io/denumerator.h>

#include <QObject>

BEGIN_IO_NAMESPACE
class DEnumerator;
// 使用这个异步时，创建的DEnumerator必须是QSharedPointer
/*使用示例
 * QSharedPointer<DEnumerator> enumerator(new DEnumerator(QUrl()));
 * DEnumeratorFuture *future = enumerator->asyncIterator();
 * 这样才能迭代出来文件。enumerator这里不需要手动析构，future不使用了手动析构
*/
class DEnumeratorFuture : public QObject
{
    Q_OBJECT
public:
    explicit DEnumeratorFuture(const QSharedPointer<DEnumerator> &enumerator, QObject *parent = nullptr);
    ~DEnumeratorFuture() override;

public:
    void startAsyncIterator();

Q_SIGNALS:
    void asyncIteratorOver();

public:
    bool isFinished();
    DFMIOError lastError() const;
    qint64 fileCount();

public Q_SLOTS:
    void onAsyncIteratorOver();

private:
    QSharedPointer<DEnumerator> enumerator { nullptr };
};

END_IO_NAMESPACE

#endif   // DENUMERATORFUTURE_H
