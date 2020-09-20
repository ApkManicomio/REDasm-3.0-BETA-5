#pragma once

#include <QSortFilterProxyModel>
#include <QDialog>
#include <rdapi/rdapi.h>
#include "../hooks/idisassemblercommand.h"
#include "../models/signatures/signaturefilesmodel.h"
#include "../models/signatures/signaturesmodel.h"

namespace Ui {
class SignaturesDialog;
}

class SignaturesDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit SignaturesDialog(const RDDisassemblerPtr& disassembler, QWidget *parent = nullptr);
        ~SignaturesDialog();

    private slots:
        void loadSignature(bool);
        void readSignature(const QModelIndex& index);
        void browseSignatures();

    private:
        Ui::SignaturesDialog *ui;
        RDDisassemblerPtr m_disassembler;
        SignatureFilesModel* m_signaturefilesmodel;
        SignaturesModel* m_signaturesmodel;
        QSortFilterProxyModel* m_filtermodel;
};
