// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef JSONCONVERTER_H
#define JSONCONVERTER_H

#include "converter.h"

class JsonConverter : public Converter
{
    // Converter interface
public:
    QString name() const override;
    Directions directions() const override;
    Options outputOptions() const override;
    const char *optionsHelp() const override;
    bool probeFile(QIODevice *f) const override;
    QVariant load(QByteArray data, const Converter *&outputConverter) const override;
    QByteArray save(const QVariant &contents, const QStringList &options) const override;
};

#endif // JSONCONVERTER_H