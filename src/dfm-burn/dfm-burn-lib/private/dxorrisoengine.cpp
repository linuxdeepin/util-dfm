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
#include "dxorrisoengine.h"

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

static inline bool JOBFAILED_IF(DXorrisoEngine *engine, int r, struct XorrisO *x)
{
    if (r <= 0) {
        Xorriso_option_end(x, 1);
        Q_EMIT engine->jobStatusChanged(JobStatus::kFailed, -1, "");
        return true;
    }
    return false;
}

static int xorrisoResultHandler(void *handle, char *text)
{
    (static_cast<DXorrisoEngine *>(handle))->messageReceived(0, text);
    return 1;
}
static int xorrisoInfoHandler(void *handle, char *text)
{
    // working around xorriso passing wrong handle to the callback
    if (strstr(text, "DEBUG : Concurrent message watcher")) {
        return 1;
    }
    (static_cast<DXorrisoEngine *>(handle))->messageReceived(1, text);
    return 1;
}

DXorrisoEngine::DXorrisoEngine(QObject *parent)
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

DXorrisoEngine::~DXorrisoEngine()
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
bool DXorrisoEngine::acquireDevice(QString dev)
{
    if (!dev.isEmpty()) {
        curDev = dev;

        int result = XORRISO_OPT(xorriso, [this, dev]() {
            return Xorriso_option_dev(xorriso, dev.toUtf8().data(), 3);
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
void DXorrisoEngine::releaseDevice()
{
    curDev = "";
    Xorriso_option_end(xorriso, 0);
}

void DXorrisoEngine::clearResult()
{
    Xorriso_sieve_clear_results(xorriso, 0);
}

MediaType DXorrisoEngine::mediaTypeProperty() const
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

void DXorrisoEngine::mediaStorageProperty(quint64 *usedSize, quint64 *availSize, quint64 *blocks) const
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

bool DXorrisoEngine::mediaFormattedProperty() const
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

QString DXorrisoEngine::mediaVolIdProperty() const
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

QStringList DXorrisoEngine::mediaSpeedProperty() const
{
    QStringList writeSpeed;
    if (curDev.isEmpty())
        return writeSpeed;

    int r = XORRISO_OPT(xorriso, [this]() {
        return Xorriso_option_list_speeds(xorriso, 0);
    });
    if (r < 0)
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
QStringList DXorrisoEngine::takeInfoMessages()
{
    QStringList ret = xorrisomsg;
    xorrisomsg.clear();
    return ret;
}

bool DXorrisoEngine::doErase()
{
    Q_EMIT jobStatusChanged(JobStatus::kRunning, 0, curspeed);
    xorrisomsg.clear();

    XORRISO_OPT(xorriso, [this]() {
        return Xorriso_option_abort_on(xorriso, PCHAR("ABORT"), 0);
    });
    int r = XORRISO_OPT(xorriso, [this]() {
        return Xorriso_option_blank(xorriso, PCHAR("as_needed"), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    return true;
}

bool DXorrisoEngine::doWriteISO(const QString &isoPath, int speed)
{
    Q_EMIT jobStatusChanged(JobStatus::kStalled, 0, curspeed);
    xorrisomsg.clear();

    QString spd = QString::number(speed) + "k";
    if (speed == 0)
        spd = "0";

    char **av = new char *[6];
    av[0] = strdup("cdrecord");
    av[1] = strdup("-v");
    av[2] = strdup((QString("dev=") + curDev).toUtf8().data());
    av[3] = strdup("blank=as_needed");
    av[4] = strdup((QString("speed=") + spd).toUtf8().data());
    av[5] = strdup(isoPath.toUtf8().data());

    int r = XORRISO_OPT(xorriso, [this, av]() {
        int dummy { 0 };
        return Xorriso_option_as(xorriso, 6, av, &dummy, 1);
    });

    for (int i = 0; i < 6; ++i)
        free(av[i]);
    delete[] av;

    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    return true;
}

bool DXorrisoEngine::doDumpISO(quint64 dataBlocks, const QString &isoPath)
{
    curDatablocks = dataBlocks;
    if (dataBlocks == 0)
        return false;

    Q_EMIT jobStatusChanged(JobStatus::kStalled, 0, curspeed);
    xorrisomsg.clear();

    char **av = new char *[2];
    av[0] = strdup((QString("use=outdev")).toUtf8().data());
    av[1] = strdup((QString("data_to=") + isoPath).toUtf8().data());

    int r = XORRISO_OPT(xorriso, [this, av]() {
        int dummy { 0 };
        return Xorriso_option_check_media(xorriso, 2, av, &dummy, 0);
    });

    for (int i = 0; i < 2; ++i)
        free(av[i]);
    delete[] av;

    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    return true;
}

/*!
 * \brief Perform a data integration check for the disc.
 * \param dataBlocks is current device's blocks
 * \param qgood if not null, will be set to the portion of sectors that can be read fast.
 * \param qslow if not null, will be set to the portion of sectors that can still be read, but slowly.
 * \param qbad if not null, will be set to the portion of sectors that are corrupt.
 * \return true on success, false on failure (if for some reason the disc could not be checked)
 *
 * The values returned should add up to 1 (or very close to 1).
 */
bool DXorrisoEngine::doCheckmedia(quint64 dataBlocks, double *qgood, double *qslow, double *qbad)
{
    curDatablocks = dataBlocks;
    if (dataBlocks == 0)
        return false;

    Q_EMIT jobStatusChanged(JobStatus::kRunning, 0, curspeed);

    int r = XORRISO_OPT(xorriso, [this]() {
        int dummy { 0 };
        return Xorriso_option_check_media(xorriso, 0, nullptr, &dummy, 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    quint64 ngood = 0;
    quint64 nslow = 0;
    quint64 nbad = 0;

    int ac, avail;
    char **av;

    do {
        Xorriso_sieve_get_result(xorriso, PCHAR("Media region :"), &ac, &av, &avail, 0);
        if (ac == 3) {
            quint64 szblk = static_cast<quint64>(QString(av[1]).toLongLong());
            if (av[2][0] == '-') {
                nbad += szblk;
            } else if (av[2][0] == '0') {
                ngood += szblk;
            } else {
                if (QString(av[2]).contains("slow")) {
                    nslow += szblk;
                } else {
                    ngood += szblk;
                }
            }
        }
        Xorriso__dispose_words(&ac, &av);
    } while (avail > 0);

    if (qgood) {
        *qgood = 1. * ngood / dataBlocks;
    }
    if (qslow) {
        *qslow = 1. * nslow / dataBlocks;
    }
    if (qbad) {
        *qbad = 1. * nbad / dataBlocks;
    }
    Xorriso_sieve_clear_results(xorriso, 0);
    Q_EMIT jobStatusChanged(JobStatus::kFinished, 0, curspeed);

    return true;
}

bool DXorrisoEngine::doBurn(const QPair<QString, QString> files, int speed, QString volId, DXorrisoEngine::JolietSupport joliet, DXorrisoEngine::RockRageSupport rockRage, DXorrisoEngine::KeepAppendable appendable)
{
    if (files.first.isEmpty())
        return false;

    Q_EMIT jobStatusChanged(JobStatus::kStalled, 0, curspeed);
    xorrisomsg.clear();

    QString spd = QString::number(speed) + "k";
    if (speed == 0)
        spd = "0";

    // speed
    int r = XORRISO_OPT(xorriso, [this, spd]() {
        return Xorriso_option_speed(xorriso, spd.toUtf8().data(), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    // volid
    r = XORRISO_OPT(xorriso, [this, volId]() {
        return Xorriso_option_volid(xorriso, volId.toUtf8().data(), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    // overwrite
    r = XORRISO_OPT(xorriso, [this]() {
        return Xorriso_option_overwrite(xorriso, PCHAR("off"), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    // joliet
    r = XORRISO_OPT(xorriso, [this, joliet]() {
        return Xorriso_option_joliet(xorriso, PCHAR(joliet == JolietSupport::kTrue ? "on" : "off"), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    // rockridge
    r = XORRISO_OPT(xorriso, [this, rockRage]() {
        return Xorriso_option_rockridge(xorriso, PCHAR(rockRage == RockRageSupport::kTrue ? "on" : "off"), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    // add files
    r = XORRISO_OPT(xorriso, [this, files]() {
        return Xorriso_option_map(xorriso, files.first.toUtf8().data(),
                                  files.second.toUtf8().data(), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    // close
    r = XORRISO_OPT(xorriso, [this, appendable]() {
        return Xorriso_option_close(xorriso, PCHAR(appendable == KeepAppendable::kTrue ? "off" : "on"), 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    // commit
    r = XORRISO_OPT(xorriso, [this]() {
        return Xorriso_option_commit(xorriso, 0);
    });
    if (JOBFAILED_IF(this, r, xorriso))
        return false;

    return true;
}

void DXorrisoEngine::messageReceived(int type, char *text)
{
    Q_UNUSED(type);

    QString msg(text);
    msg = msg.trimmed();
    fprintf(stderr, "msg from xorriso (%s) : %s\n", type ? " info " : "result", msg.toStdString().c_str());
    xorrisomsg.push_back(msg);

    // closing session
    if (msg.contains("UPDATE : Closing track/session.")) {
        Q_EMIT jobStatusChanged(JobStatus::kStalled, 1, curspeed);
        return;
    }

    // stalled
    if (msg.contains("UPDATE : Thank you for being patient.")) {
        Q_EMIT jobStatusChanged(JobStatus::kStalled, 0, curspeed);
        return;
    }

    // cdrecord / blanking
    QRegularExpression r("([0-9.]*)%\\s*(fifo|done)");
    QRegularExpressionMatch m = r.match(msg);
    if (m.hasMatch()) {
        double percentage = m.captured(1).toDouble();
        Q_EMIT jobStatusChanged(JobStatus::kRunning, static_cast<int>(percentage), curspeed);
    }

    // current speed
    r = QRegularExpression("([0-9]*\\.[0-9]x)[bBcCdD.]");
    m = r.match(msg);
    if (m.hasMatch()) {
        curspeed = m.captured(1);
    } else {
        curspeed.clear();
    }

    // commit
    r = QRegularExpression("([0-9]*)\\s*of\\s*([0-9]*) MB written");
    m = r.match(msg);
    if (m.hasMatch()) {
        double percentage = 100. * m.captured(1).toDouble() / m.captured(2).toDouble();
        Q_EMIT jobStatusChanged(JobStatus::kRunning, static_cast<int>(percentage), curspeed);
    }

    // check media
    r = QRegularExpression("([0-9]*) blocks read in ([0-9]*) seconds , ([0-9.]*)x");
    m = r.match(msg);
    if (m.hasMatch() && curDatablocks != 0) {
        double percentage = 100. * m.captured(1).toDouble() / curDatablocks;
        Q_EMIT jobStatusChanged(JobStatus::kRunning, static_cast<int>(percentage), curspeed);
    }

    // operation complete
    if (msg.contains("Blanking done") || msg.contains(QRegularExpression("Writing to .* completed successfully."))) {
        Q_EMIT jobStatusChanged(JobStatus::kFinished, 0, curspeed);
    }
}
