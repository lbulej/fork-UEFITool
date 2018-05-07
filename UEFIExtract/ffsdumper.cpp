/* ffsdumper.cpp

Copyright (c) 2015, Nikolaj Schlej. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "ffsdumper.h"

USTATUS FfsDumper::dump(const QModelIndex & root, const QString & path, const bool dumpAll, const QString & guid)
{
    dumped = false;
    UINT8 result = recursiveDump(root, path, dumpAll, guid);
    if (result)
        return result;
    else if (!dumped)
        return U_ITEM_NOT_FOUND;
    return U_SUCCESS;
}

USTATUS FfsDumper::recursiveDump(const QModelIndex & index, const QString & path, const bool dumpAll, const QString & guid)
{
    if (!index.isValid())
        return U_INVALID_PARAMETER;

    QDir dir;
    if (guid.isEmpty() ||
        guidToUString(*(const EFI_GUID*)model->header(index).constData()) == guid ||
        guidToUString(*(const EFI_GUID*)model->header(model->findParentOfType(index, Types::File)).constData()) == guid) {

        if (dir.cd(path))
            return U_DIR_ALREADY_EXIST;

        if (!dir.mkpath(path))
            return U_DIR_CREATE;

        QFile file;
        if (dumpAll || model->rowCount(index) == 0)  { // Dump if leaf item or dumpAll is true
            if (!model->header(index).isEmpty()) {
                file.setFileName(QObject::tr("%1/header.bin").arg(path));
                if (!file.open(QFile::WriteOnly))
                    return U_FILE_OPEN;

                file.write(model->header(index));
                file.close();
            }

            if (!model->body(index).isEmpty()) {
                file.setFileName(QObject::tr("%1/body.bin").arg(path));
                if (!file.open(QFile::WriteOnly))
                    return U_FILE_OPEN;

                file.write(model->body(index));
                file.close();
            }
        }

        // Always dump info
        QString info = QObject::tr("Type: %1\nSubtype: %2\n%3%4\n")
            .arg(itemTypeToUString(model->type(index)))
            .arg(itemSubtypeToUString(model->type(index), model->subtype(index)))
            .arg(model->text(index).isEmpty() ? QObject::tr("") : QObject::tr("Text: %1\n").arg(model->text(index)))
            .arg(model->info(index));
        file.setFileName(QObject::tr("%1/info.txt").arg(path));
        if (!file.open(QFile::Text | QFile::WriteOnly))
            return U_FILE_OPEN;

        file.write(info.toLatin1());
        file.close();
        dumped = true;
    }

    UINT8 result;
    for (int i = 0; i < model->rowCount(index); i++) {
        QModelIndex childIndex = index.child(i, 0);
        bool useText = FALSE;
        if (model->type(childIndex) != Types::Volume)
            useText = !model->text(childIndex).isEmpty();

        QString childPath = QString("%1/%2 %3").arg(path).arg(i).arg(useText ? model->text(childIndex) : model->name(childIndex));
        result = recursiveDump(childIndex, childPath, dumpAll, guid);
        if (result)
            return result;
    }

    return U_SUCCESS;
}