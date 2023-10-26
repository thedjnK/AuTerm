// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONVERTER_H
#define CONVERTER_H

#include <QIODevice>
#include <QList>
#include <QPair>
#include <QVariant>
#include <QVariantMap>

class VariantOrderedMap : public QList<QPair<QVariant, QVariant>>
{
public:
    VariantOrderedMap() = default;
    VariantOrderedMap(const QVariantMap &map)
    {
        reserve(map.size());
        for (auto it = map.begin(); it != map.end(); ++it)
            append({it.key(), it.value()});
    }
};

using Map = VariantOrderedMap;
Q_DECLARE_METATYPE(Map)

class Converter
{
protected:


public:
//    static Converter *null;

    enum class Direction { In = 1, Out = 2, InOut = In | Out };
    Q_DECLARE_FLAGS(Directions, Direction)

    enum Option { SupportsArbitraryMapKeys = 0x01 };
    Q_DECLARE_FLAGS(Options, Option)
    Converter(){};
    ~Converter(){};

    virtual QString name() const = 0;
    virtual Directions directions() const = 0;
    virtual Options outputOptions() const = 0;
    virtual const char *optionsHelp() const = 0;
    virtual bool probeFile(QIODevice *f) const = 0;
    virtual QVariant load(QByteArray data, const Converter *&outputConverter) const = 0;
    virtual QByteArray save(const QVariant &contents, const QStringList &options) const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Converter::Directions)
Q_DECLARE_OPERATORS_FOR_FLAGS(Converter::Options)

#endif // CONVERTER_H
