// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "searchutility.h"

#include <unistd.h>

#include <DConfig>

#include <QDebug>
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

// Index version threshold constants
namespace IndexVersionThresholds {
constexpr int FILENAME_ANCESTOR_PATHS = 3;
constexpr int CONTENT_ANCESTOR_PATHS = 1;
}

/**
 * @brief Read index version from a JSON status file
 * @param indexDir The index directory path
 * @param statusFile The name of the status JSON file
 * @return The version number, or -1 if reading fails
 */
static int readIndexVersion(const QString &indexDir, const QString &statusFile)
{
    const QString versionFilePath = QDir(indexDir).filePath(statusFile);

    QFile file(versionFilePath);
    if (!file.exists()) {
        qWarning() << "Index version file not found:" << versionFilePath;
        return -1;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open index version file:" << file.errorString();
        return -1;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse index version JSON:" << parseError.errorString();
        return -1;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj.contains("version")) {
        qWarning() << "Index version JSON missing 'version' field";
        return -1;
    }

    // Handle both numeric and string version values
    const QJsonValue versionValue = jsonObj.value("version");
    return versionValue.isDouble() ? versionValue.toInt() : versionValue.toString().toInt();
}

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

// This function is internal to this unit (static) and handles the core DConfig loading.
static std::optional<QStringList> tryLoadStringListFromDConfigInternal(
        const QString &appId,
        const QString &schemaId,
        const QString &keyName)
{
    // Dtk::Core::DConfig::create returns a pointer and ideally needs a parent
    // for memory management. For a short-lived object within a function,
    // a local QObject can serve as a temporary parent.
    QObject dconfigParent;   // Temporary parent for the DConfig instance
    Dtk::Core::DConfig *dconfigPtr = Dtk::Core::DConfig::create(appId, schemaId, "", &dconfigParent);

    // Check if DConfig object was created successfully
    if (!dconfigPtr) {
        qWarning() << "DConfig: Failed to create DConfig instance for appId:" << appId << "schemaId:" << schemaId;
        return std::nullopt;
    }

    // Check if the created DConfig instance is valid
    if (!dconfigPtr->isValid()) {
        qWarning() << "DConfig: Instance is invalid for appId:" << appId << "schemaId:" << schemaId;
        // No need to delete dconfigPtr, dconfigParent will manage it.
        return std::nullopt;
    }

    QVariant value = dconfigPtr->value(keyName);

    if (!value.isValid()) {
        qDebug() << "DConfig: Key '" << keyName << "' not found in appId:" << appId << "schemaId:" << schemaId;
        return std::nullopt;   // Key not found
    }

    if (!value.canConvert<QStringList>()) {
        qWarning() << "DConfig: Value for key '" << keyName << "' in appId:" << appId << "schemaId:" << schemaId
                   << "cannot be converted to QStringList. Actual type:" << value.typeName();
        return std::nullopt;   // Type mismatch
    }
    // No need to delete dconfigPtr, dconfigParent will manage it when it goes out of scope.
    return value.toStringList();
}

// --- Specific Loader for "supportedFileExtensions" ---
static std::optional<QSet<QString>> tryLoadSupportedFileExtensionsFromDConfig()
{
    const QString appId = "org.deepin.dde.file-manager";   // Assuming this is the correct appId
    const QString schemaId = "org.deepin.dde.file-manager.textindex";
    const QString keyName = "supportedFileExtensions";

    std::optional<QStringList> stringListOpt = tryLoadStringListFromDConfigInternal(appId, schemaId, keyName);

    if (!stringListOpt) {
        return std::nullopt;   // Loading failed at the generic level
    }

    const QStringList &extensionsFromDConfigList = *stringListOpt;
    QSet<QString> extensionsSet;
    for (const QString &ext : extensionsFromDConfigList) {
        QString cleanedExt = ext.startsWith('.') ? ext.mid(1) : ext;
        if (!cleanedExt.isEmpty()) {
            extensionsSet.insert(cleanedExt);
        }
    }

    if (extensionsFromDConfigList.isEmpty()) {
        qDebug() << "DConfig: Key '" << keyName << "' in schema '" << schemaId << "' provided an empty list.";
    } else if (extensionsSet.isEmpty() && !extensionsFromDConfigList.isEmpty()) {
        qDebug() << "DConfig: Key '" << keyName << "' in schema '" << schemaId << "' contained only invalid/empty entries after cleaning.";
    } else if (!extensionsSet.isEmpty()) {
        qDebug() << "DConfig: Successfully processed" << extensionsSet.size() << "supported extensions from DConfig.";
    }
    return extensionsSet;   // Return the processed set (might be empty if DConfig list was empty or all invalid)
}

// --- Specific Loader for "indexing_paths" ---
static std::optional<QStringList> tryLoadIndexingNamesFromDConfig()
{
    const QString appId = "org.deepin.anything";
    const QString schemaId = "org.deepin.anything";   // As per your requirement
    const QString keyName = "indexing_paths";

    std::optional<QStringList> stringListOpt = tryLoadStringListFromDConfigInternal(appId, schemaId, keyName);

    if (!stringListOpt) {
        return std::nullopt;   // Loading failed
    }

    const QStringList &namesFromDConfigList = *stringListOpt;

    if (namesFromDConfigList.isEmpty()) {
        qDebug() << "DConfig: Key '" << keyName << "' in schema '" << schemaId << "' provided an empty list.";
    }
    return namesFromDConfigList;   // Return the processed set
}

// --- Specific Loader for "blacklist_paths" ---
static std::optional<QStringList> tryLoadBlacklistPathsFromDConfig()
{
    const QString appId = "org.deepin.anything";
    const QString schemaId = "org.deepin.anything";
    const QString keyName = "blacklist_paths";

    std::optional<QStringList> stringListOpt = tryLoadStringListFromDConfigInternal(appId, schemaId, keyName);

    if (!stringListOpt) {
        return std::nullopt;   // Loading failed
    }

    const QStringList &pathsFromDConfigList = *stringListOpt;

    if (pathsFromDConfigList.isEmpty()) {
        qDebug() << "DConfig: Key '" << keyName << "' in schema '" << schemaId << "' provided an empty list.";
    }
    return pathsFromDConfigList;   // Return the processed list
}

// 辅助函数：提供硬编码的默认扩展名集合
static const QSet<QString> &getDefaultSupportedExtensions()
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

// 主接口：获取支持的文件扩展名
// 优先从 DConfig 获取，如果失败或DConfig中的列表为空，则使用硬编码的默认值。
static const QSet<QString> &supportedExtensions()
{
    // 静态局部变量的初始化只发生一次，并且是线程安全的。
    // 使用立即调用的lambda表达式来执行初始化逻辑。
    static const QSet<QString> kResolvedExtensions = []() -> QSet<QString> {
        std::optional<QSet<QString>> dconfigExtensionsOpt = tryLoadSupportedFileExtensionsFromDConfig();

        // 如果DConfig成功返回了一个【非空】的集合，则使用它
        if (dconfigExtensionsOpt && !dconfigExtensionsOpt->isEmpty()) {
            qDebug() << "Using extensions from DConfig.";
            return *dconfigExtensionsOpt;
        }

        // 否则 (DConfig读取失败，或DConfig返回了一个空集合)，使用默认值
        if (dconfigExtensionsOpt && dconfigExtensionsOpt->isEmpty()) {
            qDebug() << "DConfig provided an empty list of extensions. Using fallback default extensions.";
        } else if (!dconfigExtensionsOpt) {   // implies dconfigExtensionsOpt is std::nullopt
            qDebug() << "Failed to load extensions from DConfig. Using fallback default extensions.";
        }
        return getDefaultSupportedExtensions();
    }();   // 注意这里的 ()，立即调用lambda

    return kResolvedExtensions;
}

static QStringList getResolvedIndexedDirectories()   // Renamed for clarity
{
    const QString homePath = QDir::homePath();   // Cache for frequent use
    const QStringList fallbackResult { homePath };

    std::optional<QStringList> dconfigNamesOpt = tryLoadIndexingNamesFromDConfig();

    if (!dconfigNamesOpt) {
        qDebug() << "Failed to load indexing names from DConfig or DConfig instance invalid, using fallback.";
        return fallbackResult;
    }

    const QStringList &rawPathsFromDConfig = *dconfigNamesOpt;

    if (rawPathsFromDConfig.isEmpty()) {
        qDebug() << "DConfig provided an empty list for indexing names, using fallback.";
        return fallbackResult;
    }

    QList<QString> processedPaths;   // Use QList to maintain order before final sort
    QSet<QString> uniquePathsChecker;   // To ensure uniqueness during processing

    // Special handling for homePath if it's intended to be first
    bool homePathEncounteredAndAdded = false;

    for (const QString &rawPath : rawPathsFromDConfig) {
        QString currentPath = rawPath.trimmed();   // Remove leading/trailing whitespace

        // 1. Expand environment variables (specifically $HOME and ~)
        if (currentPath == QLatin1String("$HOME") || currentPath == QLatin1String("~")) {
            currentPath = homePath;
        } else if (currentPath.startsWith(QLatin1String("$HOME/"))) {
            currentPath = QDir(homePath).filePath(currentPath.mid(6));   // Length of "$HOME/"
        } else if (currentPath.startsWith(QLatin1String("~/"))) {
            currentPath = QDir(homePath).filePath(currentPath.mid(2));   // Length of "~/"
        }
        // Add more generic environment variable expansion here if needed:
        // e.g., using QProcessEnvironment::systemEnvironment().value("VAR_NAME")
        // or a regex for ${VAR_NAME} patterns.

        // 2. Normalize and validate path (basic validation: non-empty)
        if (currentPath.isEmpty()) {
            qDebug() << "Skipping empty path string after processing raw entry:" << rawPath;
            continue;
        }

        currentPath = QDir::cleanPath(currentPath);   // Normalize path separators (e.g. \\ to /)

        // Further validation could be done here, e.g., check if it's an absolute path
        // QFileInfo fileInfo(currentPath);
        // if (!fileInfo.isAbsolute()) {
        //     qWarning() << "Skipping relative path:" << currentPath;
        //     continue;
        // }
        // if (!fileInfo.isDir() || !fileInfo.exists()) { // Stricter: must be an existing directory
        //     qWarning() << "Skipping non-existent or non-directory path:" << currentPath;
        //     continue;
        // }

        // 3. Add to list if unique and handle homePath ordering
        if (currentPath == homePath) {
            if (!homePathEncounteredAndAdded) {
                processedPaths.prepend(currentPath);   // Ensure homePath is first if encountered
                uniquePathsChecker.insert(currentPath);
                homePathEncounteredAndAdded = true;
            }
            // If homePath was already added (e.g. from a previous "$HOME"), skip subsequent identical entries.
            // The uniquePathsChecker will also prevent adding it again if it wasn't the first home path encountered.
        } else {
            if (!uniquePathsChecker.contains(currentPath)) {
                processedPaths.append(currentPath);
                uniquePathsChecker.insert(currentPath);
            }
        }
    }

    // Ensure home path is first if it was added but not via prepend (e.g. if it wasn't the first item processed)
    // This can happen if DConfig has ["/foo", "$HOME"] and homePath wasn't prepended.
    // The above loop tries to prepend, but this is a safeguard if the logic changes or has subtle bugs.
    if (homePathEncounteredAndAdded && !processedPaths.isEmpty() && processedPaths.first() != homePath) {
        processedPaths.removeAll(homePath);   // Remove from other positions
        processedPaths.prepend(homePath);   // Add to front
    }

    if (processedPaths.isEmpty()) {
        qDebug() << "All paths from DConfig were invalid or empty after processing, using fallback.";
        return fallbackResult;
    }

    qDebug() << "Resolved indexed directories:" << processedPaths;
    return processedPaths;
}

bool isPinyinSequence(const QString &input)
{
    if (input.isEmpty())
        return false;

    // 清洗输入：移除所有非字母字符，只保留字母用于拼音检验
    QString cleanedInput = input;
    cleanedInput.remove(QRegularExpression("[^a-zA-Z]"));

    if (cleanedInput.isEmpty())
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
    if (cleanedInput.length() == 1) {
        QChar ch = cleanedInput.toLower()[0];
        if (ch == 'i' || ch == 'u' || ch == 'v' || ch == QChar(0x00FC))   // 0x00FC是ü的Unicode编码
            return false;
    }

    // 特殊处理规则：检查重复字母如'vvv'
    if (cleanedInput.length() >= 3) {
        bool allSame = true;
        QChar firstChar = cleanedInput.toLower()[0];
        for (int i = 1; i < cleanedInput.length(); i++) {
            if (cleanedInput.toLower()[i] != firstChar) {
                allSame = false;
                break;
            }
        }
        if (allSame)
            return false;
    }

    QString str = cleanedInput.toLower();
    str.replace("ü", "v");   // 统一处理 ü

    // 尝试所有可能的分割方式
    return isPinyinSequenceHelper(str, 0, validSyllables);
}

bool isPinyinAcronymSequence(const QString &input)
{
    if (input.isEmpty())
        return false;

    QString str = input.trimmed();

    // 长度检查
    if (str.length() == 0 || str.length() > 255)
        return false;

    // 核心验证：
    // 1. 必须包含至少一个英文字母
    // 2. 不能包含中文字符
    // 3. 允许数字、符号等其他任意字符

    bool hasLetter = false;
    for (const QChar &ch : str) {
        // 检查是否为中文（拼音缩写不应包含中文）
        if (ch.script() == QChar::Script_Han)
            return false;

        // 检查是否为英文字母（拉丁字母）
        if (ch.isLetter() && ch.script() == QChar::Script_Latin)
            hasLetter = true;
    }

    return hasLetter;
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
    const QStringList &dirs = getResolvedIndexedDirectories();

    if (dirs.isEmpty()) {
        return QStringList();
    }

    // 创建一个新列表来存储结果
    QStringList result;

    // 先将所有路径规范化并排序，以便父目录出现在子目录之前
    QStringList normalizedDirs;
    for (const QString &dir : dirs) {
        QString normalizedPath = QDir(dir).absolutePath();
        // 确保路径以 '/' 结尾，便于后续比较
        if (!normalizedPath.endsWith('/')) {
            normalizedPath += '/';
        }
        normalizedDirs.append(normalizedPath);
    }

    // 排序，确保短路径（潜在的父路径）优先
    std::sort(normalizedDirs.begin(), normalizedDirs.end(),
              [](const QString &a, const QString &b) { return a.length() < b.length(); });

    // 检查路径之间的父子关系
    for (const QString &currentPath : normalizedDirs) {
        bool isSubdirectory = false;

        // 检查当前路径是否是已添加路径的子目录
        for (const QString &addedPath : result) {
            if (currentPath.startsWith(addedPath)) {
                isSubdirectory = true;
                break;
            }
        }

        // 如果不是子目录，则添加到结果中
        if (!isSubdirectory) {
            // 移除末尾的 '/'，除非是根目录 "/"
            QString pathToAdd = currentPath;
            if (pathToAdd.length() > 1 && pathToAdd.endsWith('/')) {
                pathToAdd.chop(1);
            }
            result.append(pathToAdd);
        }
    }

    return result;
}

QStringList defaultBlacklistPaths()
{
    std::optional<QStringList> dconfigPathsOpt = tryLoadBlacklistPathsFromDConfig();

    if (!dconfigPathsOpt) {
        qDebug() << "Failed to load blacklist paths from DConfig or DConfig instance invalid, returning empty list.";
        return QStringList();
    }

    const QStringList &pathsFromDConfig = *dconfigPathsOpt;
    qDebug() << "Resolved blacklist paths:" << pathsFromDConfig;
    return pathsFromDConfig;
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
    try {
        const QString &indexDir = fileNameIndexDirectory();
        bool exists = IndexReader::indexExists(FSDirectory::open(indexDir.toStdWString()));
        return exists;
    } catch (const LuceneException &e) {
        qWarning() << "Failed to check index existence:" << QString::fromStdWString(e.getError());
        return false;
    }
}

bool isFileNameIndexReadyForSearch()
{
    // First check if the index physically exists
    if (!isFileNameIndexDirectoryAvailable()) {
        qDebug() << "Index directory does not exist physically.";
        return false;
    }

    // Then check the status to ensure it's in monitoring state
    std::optional<QString> currentStatus = fileNameIndexStatus();
    if (!currentStatus) {
        qWarning() << "Failed to get file name index status.";
        return false;
    }

    const QStringList &validStatus = { "monitoring" };
    const QString &status = currentStatus.value();
    if (!validStatus.contains(status)) {
        qDebug() << "Index status is '" << status
                 << "', expected or 'monitoring'. Index not ready for search.";
        return false;
    }

    return true;
}

std::optional<QString> fileNameIndexStatus()
{
    if (!isFileNameIndexDirectoryAvailable()) {
        qWarning() << "Index directory not available";
        return std::nullopt;
    }

    const QString &statusFilePath = QDir(fileNameIndexDirectory()).filePath("status.json");
    QFile statusFile(statusFilePath);

    // 检查文件是否存在和可读
    if (!statusFile.exists()) {
        qWarning() << "Status file does not exist:" << statusFilePath;
        return std::nullopt;
    }
    if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open status file:" << statusFile.errorString();
        return std::nullopt;
    }

    // 读取和解析JSON
    QJsonParseError jsonError;
    const QJsonDocument doc = QJsonDocument::fromJson(statusFile.readAll(), &jsonError);
    statusFile.close();

    if (jsonError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << jsonError.errorString()
                   << "at offset:" << jsonError.offset;
        return std::nullopt;
    }

    // 检查JSON结构
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON format: root is not an object";
        return std::nullopt;
    }

    const QJsonObject root = doc.object();
    if (!root.contains("status") || !root["status"].isString()) {
        qWarning() << "Missing or invalid 'status' field";
        return std::nullopt;
    }

    // 返回小写状态
    return root["status"].toString().toLower();
}

QString fileNameIndexDirectory()
{
    return QString("/run/user/%1/deepin-anything-server").arg(getuid());
}

int fileNameIndexVersion()
{
    return readIndexVersion(fileNameIndexDirectory(), "status.json");
}

int contentIndexVersion()
{
    return readIndexVersion(contentIndexDirectory(), "index_status.json");
}

}   //  namespace Global

namespace SearchUtility {

bool isFilenameIndexAncestorPathsSupported()
{
    return Global::fileNameIndexVersion() > Global::IndexVersionThresholds::FILENAME_ANCESTOR_PATHS;
}

bool isContentIndexAncestorPathsSupported()
{
    return Global::contentIndexVersion() > Global::IndexVersionThresholds::CONTENT_ANCESTOR_PATHS;
}

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

bool shouldUsePathPrefixQuery(const QString &searchPath)
{
    // Don't use path prefix query for root directory
    if (searchPath == "/" || searchPath.isEmpty()) {
        return false;
    }

    // Check if it's one of the default indexed directories
    const QStringList &defaultDirs = Global::defaultIndexedDirectory();
    for (const QString &defaultDir : defaultDirs) {
        QString normalizedDefault = QDir::cleanPath(defaultDir);
        QString normalizedSearch = QDir::cleanPath(searchPath);

        // Don't use path prefix query if search path is one of the default indexed directories
        if (normalizedSearch == normalizedDefault) {
            return false;
        }
    }

    return true;
}

}   // namespace SearchUtility
DFM_SEARCH_END_NS
