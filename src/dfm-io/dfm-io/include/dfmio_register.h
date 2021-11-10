/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
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
