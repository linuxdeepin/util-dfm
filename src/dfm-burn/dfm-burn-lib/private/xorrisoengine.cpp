/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "xorrisoengine.h"

#include <QHash>
#include <QRegularExpression>

#include <functional>

DFM_BURN_USE_NS

static inline char *PCHAR(const char *c)
{
    return const_cast<char *>(c);
}

static inline int XORRISO_OPT(struct XorrisO *x, std::function<int()> opt)
{
    Xorriso_set_problem_status(x, PCHAR(""), 0);
    int r = opt();
    return Xorriso_eval_problem_status(x, r, 0);
}

static int xorrisoResultHandler(void *handle, char *text)
{
    (static_cast<XorrisoEngine *>(handle))->messageReceived(0, text);
    return 1;
}
static int xorrisoInfoHandler(void *handle, char *text)
{
    // working around xorriso passing wrong handle to the callback
    if (strstr(text, "DEBUG : Concurrent message watcher")) {
        return 1;
    }
    (static_cast<XorrisoEngine *>(handle))->messageReceived(1, text);
    return 1;
}

XorrisoEngine::XorrisoEngine(QObject *parent)
    : QObject(parent)
{
    int r = Xorriso_new(&xorriso, PCHAR("xorriso"), 0);
    if (r <= 0) {
        xorriso = nullptr;
        return;
    }

    r = Xorriso_startup_libraries(xorriso, 0);
    if (r <= 0) {
        Xorriso_destroy(&xorriso, 0);
        return;
    }

    Xorriso_sieve_big(xorriso, 0);
    Xorriso_start_msg_watcher(xorriso, xorrisoResultHandler, this, xorrisoInfoHandler, this, 0);
}

XorrisoEngine::~XorrisoEngine()
{
    if (xorriso) {
        Xorriso_stop_msg_watcher(xorriso, 0);
        Xorriso_destroy(&xorriso, 0);
    }
}

/*!
 * \brief Acquire an optical drive.
 * \param dev The device identifier of the drive to be
 * acquired (e.g. "/dev/sr0")
 *
 * Unless otherwise stated, all methods below requires
 * a drive acquired.
 * XorrisoEngine will take exclusive control of the device
 * until it is released by calling releaseDevice().
 *
 * \return true on success. If this function fails, it
 * is usually because the device is currently in use.
 */
bool XorrisoEngine::acquireDevice(QString dev)
{
    if (!dev.isEmpty()) {
        curDev = dev;

        int result = XORRISO_OPT(xorriso, [this, dev]() {
            int r = Xorriso_option_dev(xorriso, dev.toUtf8().data(), 3);
            return r;
        });
        if (result <= 0) {
            curDev = "";
            return false;
        }
        return true;
    }
    return false;
}

/*!
 * \brief Release the drive currently held.
 */
void XorrisoEngine::releaseDevice()
{
    curDev = "";
    Xorriso_option_end(xorriso, 0);
}

void XorrisoEngine::clearResult()
{
    Xorriso_sieve_clear_results(xorriso, 0);
}

MediaType XorrisoEngine::mediaTypeProperty() const
{
    MediaType devMedia { MediaType::kNoMedia };
    if (curDev.isEmpty())
        return devMedia;

    int ac, avail;
    char **av;
    Xorriso_sieve_get_result(xorriso, PCHAR("Media current:"), &ac, &av, &avail, 0);
    if (ac < 1) {
        Xorriso__dispose_words(&ac, &av);
        return devMedia;
    }
    QString mt = av[0];
    const static QHash<QString, MediaType> typemap = {
        { "CD-ROM", MediaType::kCD_ROM },
        { "CD-R", MediaType::kCD_R },
        { "CD-RW", MediaType::kCD_RW },
        { "DVD-ROM", MediaType::kDVD_ROM },
        { "DVD-R", MediaType::kDVD_R },
        { "DVD-RW", MediaType::kDVD_RW },
        { "DVD+R", MediaType::kDVD_PLUS_R },
        { "DVD+R/DL", MediaType::kDVD_PLUS_R_DL },
        { "DVD-RAM", MediaType::kDVD_RAM },
        { "DVD+RW", MediaType::kDVD_PLUS_RW },
        { "BD-ROM", MediaType::kBD_ROM },
        { "BD-R", MediaType::kBD_R },
        { "BD-RE", MediaType::kBD_RE }
    };

    mt = mt.left(mt.indexOf(' '));
    if (typemap.find(mt) != typemap.end()) {
        devMedia = typemap[mt];
    } else {
        devMedia = MediaType::kNoMedia;
    }

    Xorriso__dispose_words(&ac, &av);

    return devMedia;
}

void XorrisoEngine::mediaStorageProperty(quint64 *usedSize, quint64 *availSize, quint64 *blocks) const
{
    if (curDev.isEmpty())
        return;

    int ac, avail;
    char **av;
    Xorriso_sieve_get_result(xorriso, PCHAR("Media summary:"), &ac, &av, &avail, 0);
    if (ac == 4) {
        const QString units = "kmg";
        *blocks = static_cast<quint64>(atoll(av[1]));
        *usedSize = static_cast<quint64>(atof(av[2]) * (1 << ((units.indexOf(*(QString(av[2]).rbegin())) + 1) * 10)));
        *availSize = static_cast<quint64>(atof(av[3]) * (1 << ((units.indexOf(*(QString(av[3]).rbegin())) + 1) * 10)));
    }
    Xorriso__dispose_words(&ac, &av);
}

bool XorrisoEngine::mediaFormattedProperty() const
{
    bool formatted { true };
    if (curDev.isEmpty())
        return formatted;

    int ac, avail;
    char **av;
    Xorriso_sieve_get_result(xorriso, PCHAR("Media status :"), &ac, &av, &avail, 0);

    if (ac == 1)
        formatted = QString(av[0]).contains("is blank");

    Xorriso__dispose_words(&ac, &av);

    return formatted;
}

QString XorrisoEngine::mediaVolIdProperty() const
{
    QString volId;
    if (curDev.isEmpty())
        return volId;

    int ac, avail;
    char **av;
    Xorriso_sieve_get_result(xorriso, PCHAR("Volume id    :"), &ac, &av, &avail, 0);
    if (ac == 1)
        volId = QString(av[0]);
    Xorriso__dispose_words(&ac, &av);
    return volId;
}

QStringList XorrisoEngine::mediaSpeedProperty() const
{
    QStringList writeSpeed;
    if (curDev.isEmpty())
        return writeSpeed;

    int ac, avail;
    char **av;
    do {
        Xorriso_sieve_get_result(xorriso, PCHAR("Write speed  :"), &ac, &av, &avail, 0);
        if (ac == 2) {
            writeSpeed.push_back(QString(av[0]) + '\t' + QString(av[1]));
        }
        Xorriso__dispose_words(&ac, &av);
    } while (avail > 0);

    return writeSpeed;
}

/*!
 * \brief Get a list of messages from xorriso.
 *
 * This will clear the internal message buffer.
 *
 * \return a list of messages from xorriso since the last command.
 */
QStringList XorrisoEngine::takeInfoMessages()
{
    QStringList ret = xorrisomsg;
    xorrisomsg.clear();
    return ret;
}

QString XorrisoEngine::getCurSpeed() const
{
    return curspeed;
}

void XorrisoEngine::messageReceived(int type, char *text)
{
    Q_UNUSED(type);

    QString msg(text);
    msg = msg.trimmed();
    fprintf(stderr, "msg from xorriso (%s) : %s\n", type ? " info " : "result", msg.toStdString().c_str());
    xorrisomsg.push_back(msg);

    // closing session
    if (msg.contains("UPDATE : Closing track/session.")) {
        Q_EMIT jobStatusChanged(JobStatus::kStalled, 1);
        return;
    }

    // stalled
    if (msg.contains("UPDATE : Thank you for being patient.")) {
        Q_EMIT jobStatusChanged(JobStatus::kStalled, 0);
        return;
    }

    //cdrecord / blanking
    QRegularExpression r("([0-9.]*)%\\s*(fifo|done)");
    QRegularExpressionMatch m = r.match(msg);
    if (m.hasMatch()) {
        double percentage = m.captured(1).toDouble();
        Q_EMIT jobStatusChanged(JobStatus::kRunning, static_cast<int>(percentage));
    }

    //commit
    r = QRegularExpression("([0-9]*)\\s*of\\s*([0-9]*) MB written");
    m = r.match(msg);
    if (m.hasMatch()) {
        double percentage = 100. * m.captured(1).toDouble() / m.captured(2).toDouble();
        Q_EMIT jobStatusChanged(JobStatus::kRunning, static_cast<int>(percentage));
    }

    //check media
    r = QRegularExpression("([0-9]*) blocks read in ([0-9]*) seconds , ([0-9.]*)x");
    m = r.match(msg);
    if (m.hasMatch() && curDatablocks != 0) {
        double percentage = 100. * m.captured(1).toDouble() / curDatablocks;
        Q_EMIT jobStatusChanged(JobStatus::kRunning, static_cast<int>(percentage));
    }

    //current speed
    r = QRegularExpression("([0-9]*\\.[0-9]x)[bBcCdD.]");
    m = r.match(msg);
    if (m.hasMatch()) {
        curspeed = m.captured(1);
    } else {
        curspeed.clear();
    }

    //operation complete
    if (msg.contains("Blanking done") || msg.contains(QRegularExpression("Writing to .* completed successfully."))) {
        Q_EMIT jobStatusChanged(JobStatus::kFinished, 0);
    }
}
