// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DFMIO_REGISTER_H
#define DFMIO_REGISTER_H

#include "dfmio_global.h"

#include <functional>
#include <memory>

#include <QMap>
#include <QSet>
#include <QSharedPointer>

BEGIN_IO_NAMESPACE

class DIOFactory;

template<typename... U>
struct Factory {
    template<typename T>
    struct Register {
        Register(const QString &key) {
            Factory<U...>::get().map.insert(key, [](U&&... u){ return new T(std::forward<U>(u)...);});
        }

        Register(const QString &key, std::function<DIOFactory*(U&&...)>&& f) {
            Factory<U...>::get().map.insert(key, std::move(f));
        }
    };

    inline DIOFactory* produce(const QString& key, U&&... u) {
        if (map.find(key) == map.end())
            return nullptr;

        return map[key](std::forward<U>(u)...);
    }

    inline QSharedPointer<DIOFactory> produceQShared(const QString& key, U&&... u) {
        return QSharedPointer<DIOFactory>(produce(key, std::forward<U>(u)...));
    }

    inline std::shared_ptr<DIOFactory> produceShared(const QString& key, U&&... u) {
        return std::shared_ptr<DIOFactory>(produce(key, std::forward<U>(u)...));
    }

    inline static Factory<U...>& get() {
        static Factory<U...> instance;
        return instance;
    }

private:
    Factory<U...>() {}
    Factory<U...>(const Factory<U...>&) = delete;
    Factory<U...>(Factory<U...>&&) = delete;

    QMap<QString, std::function<DIOFactory*(U&&...)>> map;
};

bool dfmio_init();
QSet<QString> schemesInited();

END_IO_NAMESPACE

// example:
//      REGISTER_FACTORY(DFtpFactory, "ftp")
#define REGISTER_FACTORY(T, key) DFMIO::Factory<>::Register<T> REGISTER_FACTORY_VNAME(T)(key);
#define STATIC_REGISTER_FACTORY(T, key) static REGISTER_FACTORY(T, key)

// example:
//      REGISTER_FACTORY1(DLocalFactory, "file", QUrl)
#define REGISTER_FACTORY1(T, key, ...) DFMIO::Factory<__VA_ARGS__>::Register<T> REGISTER_FACTORY_VNAME(T)(key);
#define STATIC_REGISTER_FACTORY1(T, key, ...) static REGISTER_FACTORY1(T, key, ...)

// example:
//      REGISTER_FACTORY2(DMtpFactory, "mtp", [](QUrl uri) {return DMtpFactory(uri};}, QUrl)
#define REGISTER_FACTORY2(T, key, f, ...) DFMIO::Factory<__VA_ARGS__>::Register<T> REGISTER_FACTORY_VNAME(T)(key, f);
#define STATIC_REGISTER_FACTORY2(T, key, f, ...) static REGISTER_FACTORY2(T, key, f, ...)

#define REGISTER_FACTORY_VNAME(T) reg_factory_##T##_

template<typename... Args>
inline DFMIO::DIOFactory* produceIOFactory(const QString &key, Args &&... args){
    return DFMIO::Factory<Args...>::get().produce(key, std::forward<Args>(args)...);
}
template<typename... Args>
inline QSharedPointer<DFMIO::DIOFactory> produceQSharedIOFactory(const QString &key, Args &&... args){
    return DFMIO::Factory<Args...>::get().produceQShared(key, std::forward<Args>(args)...);
}

template<typename... Args>
inline std::shared_ptr<DFMIO::DIOFactory> produceSharedIOFactory(const QString &key, Args &&... args){
    return DFMIO::Factory<Args...>::get().produceShared(key, std::forward<Args>(args)...);
}

#endif // DFMIO_REGISTER_H
