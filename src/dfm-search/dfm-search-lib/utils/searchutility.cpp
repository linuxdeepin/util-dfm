// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "searchutility.h"

#include <unistd.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>

#include <lucene++/LuceneHeaders.h>
#include <lucene++/FSDirectory.h>

DFM_SEARCH_BEGIN_NS
using namespace Lucene;

namespace Global {

// 辅助递归函数，尝试所有可能的分割方法
static bool isPinyinSequenceHelper(const QString &str, int startPos, const QSet<QString> &validSyllables)
{
    int len = str.length();

    // 如果已经处理到字符串末尾，则匹配成功
    if (startPos >= len)
        return true;

    // 尝试从长到短匹配音节
    for (int syllableLen = qMin(6, len - startPos); syllableLen >= 1; syllableLen--) {
        QString syllable = str.mid(startPos, syllableLen);

        if (validSyllables.contains(syllable)) {
            // 当前音节匹配成功，递归处理剩余部分
            if (isPinyinSequenceHelper(str, startPos + syllableLen, validSyllables))
                return true;
        }
    }

    // 所有可能的分割方式都尝试失败
    return false;
}

// TODO (search): use dconfig
static const QSet<QString> &supportedExtensions()
{
    static const QSet<QString> kExtensions = {
        "rtf", "odt", "ods", "odp", "odg", "docx",
        "xlsx", "pptx", "ppsx", "md", "xls", "xlsb",
        "doc", "dot", "wps", "ppt", "pps", "txt",
        "pdf", "dps", "sh", "html", "htm", "xml",
        "xhtml", "dhtml", "shtm", "shtml", "json",
        "css", "yaml", "ini", "bat", "js", "sql",
        "uof", "ofd"
    };
    return kExtensions;
}

bool isPinyinSequence(const QString &input)
{
    if (input.isEmpty())
        return false;

    // 合法的拼音音节表（预先定义所有可能的拼音音节组合）
    static const QSet<QString> validSyllables = {
        // 单韵母音节 - 只有 a, o, e 可以单独成音节
        "a", "o", "e", "ai", "ei", "ao", "ou", "an", "en", "ang", "eng", "er",
        // b开头音节
        "ba", "bo", "bi", "bu", "bai", "bei", "bao", "ban", "ben", "bin", "bie", "biao", "bian", "bing", "bang", "beng",
        // p开头音节
        "pa", "po", "pi", "pu", "pai", "pei", "pao", "pan", "pen", "pin", "pie", "piao", "pian", "ping", "pang", "peng",
        // m开头音节
        "ma", "mo", "me", "mi", "mu", "mai", "mei", "mao", "mou", "man", "men", "min", "mie", "miao", "miu", "mian", "min", "ming", "mang", "meng",
        // f开头音节
        "fa", "fo", "fu", "fei", "fan", "fen", "fang", "feng",
        // d开头音节
        "da", "de", "di", "du", "dai", "dao", "dou", "dan", "den", "dang", "deng", "ding", "dong", "die", "diao", "diu", "dian", "duan", "dun", "duo",
        // t开头音节
        "ta", "te", "ti", "tu", "tai", "tao", "tou", "tan", "tang", "teng", "ting", "tong", "tie", "tiao", "tian", "tuan", "tun", "tuo",
        // n开头音节
        "na", "ne", "ni", "nu", "nv", "nai", "nei", "nao", "nou", "nan", "nen", "nang", "neng", "ning", "nong", "nie", "niao", "niu", "nian", "niang", "nuan", "nve", "nuo", "nun",
        // l开头音节
        "la", "le", "li", "lu", "lv", "lai", "lei", "lao", "lou", "lan", "lang", "leng", "ling", "long", "lie", "liao", "liu", "lian", "liang", "luan", "lun", "luo", "lve",
        // g开头音节
        "ga", "ge", "gu", "gai", "gei", "gao", "gou", "gan", "gen", "gang", "geng", "gong", "gua", "guo", "guai", "gui", "guan", "gun", "guang",
        // k开头音节
        "ka", "ke", "ku", "kai", "kao", "kou", "kan", "ken", "kang", "keng", "kong", "kua", "kuo", "kuai", "kui", "kuan", "kun", "kuang",
        // h开头音节
        "ha", "he", "hu", "hai", "hei", "hao", "hou", "han", "hen", "hang", "heng", "hong", "hua", "huo", "huai", "hui", "huan", "hun", "huang",
        // j开头音节
        "ji", "ju", "jue", "jiu", "jie", "jia", "jin", "jing", "jiang", "jiong", "juan", "jun", "jian", "jiao",
        // q开头音节
        "qi", "qu", "que", "qiu", "qie", "qia", "qin", "qing", "qiang", "qiong", "quan", "qun", "qian", "qiao",
        // x开头音节
        "xi", "xu", "xue", "xiu", "xie", "xia", "xin", "xing", "xiang", "xiong", "xuan", "xun", "xian", "xiao",
        // zh开头音节
        "zha", "zhe", "zhi", "zhu", "zhai", "zhao", "zhou", "zhan", "zhen", "zhang", "zheng", "zhong", "zhua", "zhuo", "zhuai", "zhui", "zhuan", "zhun", "zhuang",
        // ch开头音节
        "cha", "che", "chi", "chu", "chai", "chao", "chou", "chan", "chen", "chang", "cheng", "chong", "chua", "chuo", "chuai", "chui", "chuan", "chun", "chuang",
        // sh开头音节
        "sha", "she", "shi", "shu", "shai", "shao", "shou", "shan", "shen", "shang", "sheng", "shua", "shuo", "shuai", "shui", "shuan", "shun", "shuang",
        // r开头音节
        "ra", "re", "ri", "ru", "rao", "rou", "ran", "ren", "rang", "reng", "rong", "rua", "ruo", "rui", "ruan", "run",
        // z开头音节
        "za", "ze", "zi", "zu", "zai", "zei", "zao", "zou", "zan", "zen", "zang", "zeng", "zong", "zuo", "zui", "zuan", "zun",
        // c开头音节
        "ca", "ce", "ci", "cu", "cai", "cao", "cou", "can", "cen", "cang", "ceng", "cong", "cuo", "cui", "cuan", "cun",
        // s开头音节
        "sa", "se", "si", "su", "sai", "sao", "sou", "san", "sen", "sang", "seng", "song", "suo", "sui", "suan", "sun",
        // y开头音节 - 注意yi/you/yao等整体认读音节
        "ya", "ye", "yi", "yo", "yu", "yue", "yao", "you", "yan", "yin", "yang", "ying", "yong", "yuan", "yun",
        // w开头音节 - 注意wu/wei等整体认读音节
        "wa", "wo", "wu", "wai", "wei", "wan", "wen", "wang", "weng"
    };

    // 特殊处理规则：单个字母'i', 'u', 'v', 'ü'不能单独成音节
    if (input.length() == 1) {
        QChar ch = input.toLower()[0];
        if (ch == 'i' || ch == 'u' || ch == 'v' || ch == QChar(0x00FC))   // 0x00FC是ü的Unicode编码
            return false;
    }

    // 特殊处理规则：检查重复字母如'vvv'
    if (input.length() >= 3) {
        bool allSame = true;
        QChar firstChar = input.toLower()[0];
        for (int i = 1; i < input.length(); i++) {
            if (input.toLower()[i] != firstChar) {
                allSame = false;
                break;
            }
        }
        if (allSame)
            return false;
    }

    QString str = input.toLower();
    str.replace("ü", "v");   // 统一处理 ü

    // 尝试所有可能的分割方式
    return isPinyinSequenceHelper(str, 0, validSyllables);
}

bool isHiddenPathOrInHiddenDir(const QString &absolutePath)
{
    int start = 0;
    int len = absolutePath.length();

    // 跳过根目录标记（如果是绝对路径）
    if (len > 0 && absolutePath[0] == '/') {
        start = 1;
    }

    for (int i = start; i < len; ++i) {
        // 找到路径分隔符或到达字符串末尾
        if (absolutePath[i] == '/' || i == len - 1) {
            // 计算当前目录/文件名的起始位置
            int nameStart = (start == i) ? start : start + 1;

            // 如果目录/文件名以.开头且不是"."或".."
            if (nameStart < i && absolutePath[nameStart] == '.') {
                // 排除 "." 和 ".." 这两个特殊目录
                if (!(i - nameStart == 1 || (i - nameStart == 2 && absolutePath[nameStart + 1] == '.'))) {
                    return true;
                }
            }

            // 更新下一段的起始位置
            start = i;
        }
    }

    return false;
}

bool isSupportedContentSearchExtension(const QString &suffix)
{
    return supportedExtensions().contains(suffix.toLower());
}

QStringList defaultContentSearchExtensions()
{
    return supportedExtensions().values();
}

QStringList defaultIndexedDirectory()
{
    return { QDir::homePath() };
}

bool isPathInContentIndexDirectory(const QString &path)
{
    if (!isContentIndexAvailable())
        return false;

    const QStringList &dirs = defaultIndexedDirectory();
    return std::any_of(dirs.cbegin(), dirs.cend(),
                       [&path](const QString &dir) { return path.startsWith(dir); });
}

bool isContentIndexAvailable()
{
    const QString &dir = contentIndexDirectory();
    if (!IndexReader::indexExists(FSDirectory::open(dir.toStdWString())))
        return false;

    const QString &statusFile = dir + "/index_status.json";

    // 1. 尝试打开文件
    QFile file(statusFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;   // 文件无法打开
    }

    // 2. 读取并解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        return false;   // JSON 格式无效
    }

    // 3. 检查 lastUpdateTime 字段
    QJsonObject obj = doc.object();
    if (!obj.contains("lastUpdateTime")) {
        return false;   // 字段不存在
    }

    const QString lastUpdateTime = obj["lastUpdateTime"].toString();
    return !lastUpdateTime.isEmpty();   // 字段值非空则为有效
}

QString contentIndexDirectory()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QDir deepinDir(configDir);
    QString indexPath = deepinDir.filePath("deepin/dde-file-manager/index");
    return indexPath;
}

bool isPathInFileNameIndexDirectory(const QString &path)
{
    if (!isFileNameIndexDirectoryAvailable())
        return false;

    const QStringList &dirs = defaultIndexedDirectory();
    return std::any_of(dirs.cbegin(), dirs.cend(),
                       [&path](const QString &dir) { return path.startsWith(dir); });
}

bool isFileNameIndexDirectoryAvailable()
{
    return IndexReader::indexExists(FSDirectory::open(fileNameIndexDirectory().toStdWString()));
}

QString fileNameIndexDirectory()
{
    return QString("/run/user/%1/deepin-anything-server").arg(getuid());
}

}   //  namespace Global

namespace SearchUtility {

QStringList extractBooleanKeywords(const SearchQuery &query)
{
    QStringList keywords;

    if (query.type() == SearchQuery::Type::Boolean) {
        // 布尔查询，提取所有子查询的关键词
        for (const auto &subQuery : query.subQueries()) {
            keywords.append(subQuery.keyword());
        }
        // 如果没有子查询，添加主关键词
        if (keywords.isEmpty()) {
            keywords.append(query.keyword());
        }
    } else {
        // 简单查询，直接添加关键词
        keywords.append(query.keyword());
    }

    // 移除空关键词
    keywords.removeAll("");

    return keywords;
}

QStringList deepinAnythingFileTypes()
{
    static const QStringList kTypes { "app", "archive", "audio", "doc", "pic", "video", "dir", "other" };
    return kTypes;
}

}   // namespace SearchUtility
DFM_SEARCH_END_NS
