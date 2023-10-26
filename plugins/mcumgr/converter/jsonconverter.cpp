// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "jsonconverter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using namespace Qt::StringLiterals;


static const char jsonOptionHelp[] = "compact=no|yes              Use compact JSON form.\n";

static QJsonDocument convertFromVariant(const QVariant &v)
{
    QJsonDocument doc = QJsonDocument::fromVariant(v);
    if (!doc.isObject() && !doc.isArray()) {
        qDebug()<< "Could not convert contents to JSON.";
        exit(EXIT_FAILURE);
    }
    return doc;
}

QString JsonConverter::name() const
{
    return "json"_L1;
}

Converter::Directions JsonConverter::directions() const
{
    return Direction::InOut;
}

Converter::Options JsonConverter::outputOptions() const
{
    return {};
}

const char *JsonConverter::optionsHelp() const
{
    return jsonOptionHelp;
}

bool JsonConverter::probeFile(QIODevice *f) const
{
    if (QFile *file = qobject_cast<QFile *>(f)) {
        if (file->fileName().endsWith(".json"_L1))
            return true;
    }

    if (f->isReadable()) {
        QByteArray ba = f->peek(1);
        return ba == "{" || ba == "[";
    }
    return false;
}

QVariant JsonConverter::load(QByteArray data, const Converter *&outputConverter) const
{
    if (!outputConverter)
        outputConverter = this;

    QJsonParseError error;
    QJsonDocument doc;
    

    doc = QJsonDocument::fromJson(data, &error);
 
    if (error.error) {
        fprintf(stderr, "Could not parse JSON content: offset %d: %s",
                error.offset, qPrintable(error.errorString()));
        exit(EXIT_FAILURE);
    }
    if (outputConverter == nullptr)
        return QVariant();
    return doc.toVariant();
}

QByteArray JsonConverter::save(const QVariant &contents, const QStringList &options) const
{
    QJsonDocument::JsonFormat format = QJsonDocument::Indented;
    for (const QString &s : options) {
        if (s == "compact=no"_L1) {
            format = QJsonDocument::Indented;
        } else if (s == "compact=yes"_L1) {
            format = QJsonDocument::Compact;
        } else {
            fprintf(stderr, "Unknown option '%s' to JSON output. Valid options are:\n%s",
                    qPrintable(s), jsonOptionHelp);
            exit(EXIT_FAILURE);
        }
    }

    return convertFromVariant(contents).toJson(format);
}
