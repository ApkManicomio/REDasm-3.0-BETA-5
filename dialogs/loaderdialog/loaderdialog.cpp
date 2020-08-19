#include "loaderdialog.h"
#include "ui_loaderdialog.h"
#include "../themeprovider.h"
#include <QPushButton>

LoaderDialog::LoaderDialog(const RDLoaderRequest* request, QWidget *parent) : QDialog(parent), ui(new Ui::LoaderDialog), m_request(request)
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(1, 1);

    RD_GetLoaders(request, [](RDLoaderPlugin* descriptor, void* userdata) {
        LoaderDialog* thethis = reinterpret_cast<LoaderDialog*>(userdata);
        thethis->m_loaders.push_back(descriptor);
    }, this);

    m_loadersmodel = new QStandardItemModel(ui->lvLoaders);
    m_analyzersmodel = new QStandardItemModel(ui->tvAnalyzers);

    for(const RDLoaderPlugin* d : m_loaders)
        m_loadersmodel->appendRow(new QStandardItem(d->name));

    ui->lvLoaders->setModel(m_loadersmodel);
    ui->lvLoaders->setCurrentIndex(m_loadersmodel->index(0, 0));
    ui->tvAnalyzers->setModel(m_analyzersmodel);

    this->populateAssemblers();
    this->checkFlags();
    this->updateInputMask();
    this->syncAssembler();
    this->syncAnalyzers();

    connect(m_analyzersmodel, &QStandardItemModel::itemChanged, this, &LoaderDialog::onAnalyzerItemChanged);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LoaderDialog::onAccepted);
    connect(ui->leBaseAddress, &QLineEdit::textEdited, this, [&](const QString&) { this->validateInput(); });
    connect(ui->leEntryPoint, &QLineEdit::textEdited, this, [&](const QString&)  { this->validateInput(); });
    connect(ui->leOffset, &QLineEdit::textEdited, this, [&](const QString&)  { this->validateInput(); });

    connect(ui->lvLoaders, &QListView::clicked, this, [&](const QModelIndex&) {
        this->checkFlags();
        this->validateInput();
        this->syncAssembler();
        this->syncAnalyzers();
    });
}

LoaderDialog::~LoaderDialog()
{
    if(!m_assemblers.empty() && this->selectedAssembler())
        m_assemblers.removeOne(this->selectedAssembler()); // Keep selected assembler

    if(!m_loaders.empty())
        m_loaders.removeOne(this->selectedLoader()); // Keep selected loader

    std::for_each(m_loaders.begin(), m_loaders.end(), [](RDLoaderPlugin* plugin) {
        RDPlugin_Free(reinterpret_cast<RDPluginHeader*>(plugin));
    });

    std::for_each(m_assemblers.begin(), m_assemblers.end(), [](RDAssemblerPlugin* plugin) {
        RDPlugin_Free(reinterpret_cast<RDPluginHeader*>(plugin));
    });

    delete ui;
}

RDLoaderBuildRequest LoaderDialog::buildRequest() const { return { this->offset(), this->baseAddress(), this->entryPoint() }; }

RDLoaderPlugin *LoaderDialog::selectedLoader() const
{
    QModelIndex index = ui->lvLoaders->currentIndex();
    if(!index.isValid()) return nullptr;
    return m_loaders.at(index.row());
}

RDAssemblerPlugin* LoaderDialog::selectedAssembler() const
{
    rd_flag flags = this->selectedLoaderFlags();
    if(flags & LoaderFlags_CustomAssembler) return m_assemblers.at(ui->cbAssembler->currentIndex());
    return RDLoader_GetAssemblerPlugin(this->selectedLoader());
}

rd_address LoaderDialog::baseAddress() const { return ui->leBaseAddress->text().toULongLong(nullptr, 16); }
rd_address LoaderDialog::entryPoint() const { return ui->leEntryPoint->text().toULongLong(nullptr, 16); }
rd_offset LoaderDialog::offset() const { return ui->leOffset->text().toULongLong(nullptr, 16); }

rd_flag LoaderDialog::selectedLoaderFlags() const
{
    QModelIndex index = ui->lvLoaders->currentIndex();
    return index.isValid() ? m_loaders[index.row()]->flags : LoaderFlags_None;
}

void LoaderDialog::checkFlags()
{
    QModelIndex index = ui->lvLoaders->currentIndex();

    if(!index.isValid())
    {
        ui->cbAssembler->setEnabled(false);
        ui->tabAddressing->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    rd_flag flags = this->selectedLoaderFlags();
    ui->cbAssembler->setEnabled(flags & LoaderFlags_CustomAssembler);
    ui->tabAddressing->setEnabled(flags & LoaderFlags_CustomAddressing);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void LoaderDialog::validateInput()
{
    bool okenabled = true;
    rd_flag flags = this->selectedLoaderFlags();

    if(flags & LoaderFlags_CustomAddressing)
    {
        if(ui->leOffset->text().isEmpty() || (this->offset() >= RDBuffer_Size(m_request->buffer)))
            okenabled = false;
        if((ui->leEntryPoint->text().isEmpty() || ui->leBaseAddress->text().isEmpty()) || (this->entryPoint() > this->baseAddress()))
            okenabled = false;
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(okenabled);
}

void LoaderDialog::updateInputMask()
{
    QString mask = "H" + QString("h").repeated(16);
    ui->leEntryPoint->setInputMask(mask);
    ui->leOffset->setInputMask(mask);
    ui->leBaseAddress->setInputMask(mask);

    ui->leEntryPoint->setText(QString("0").repeated(16));
    ui->leOffset->setText(QString("0").repeated(16));
    ui->leBaseAddress->setText(QString("0").repeated(16));
}

void LoaderDialog::syncAssembler()
{
    const RDLoaderPlugin* ploader = this->selectedLoader();
    if(!ploader) return;

    const RDAssemblerPlugin* selassembler = RDLoader_GetAssemblerPlugin(ploader);
    if(selassembler) ui->cbAssembler->setCurrentText(selassembler->name);
    else ui->cbAssembler->setCurrentIndex(-1);
}

void LoaderDialog::syncAnalyzers()
{
    m_analyzersmodel->clear();
    m_analyzersmodel->setHorizontalHeaderLabels({"Name", "Description"});

    RD_GetAnalyzers(this->selectedLoader(), this->selectedAssembler(), [](const RDAnalyzerPlugin* a, void* userdata) {
        auto* thethis = reinterpret_cast<LoaderDialog*>(userdata);
        auto* nameitem = new QStandardItem(QString::fromUtf8(a->name));
        auto* descritem = new QStandardItem(QString::fromUtf8(a->description));

        nameitem->setData(QVariant::fromValue(static_cast<void*>(const_cast<RDAnalyzerPlugin*>(a))));
        nameitem->setCheckable(true);
        nameitem->setCheckState(HAS_FLAG(a, AnalyzerFlags_Selected) ? Qt::Checked : Qt::Unchecked);
        if(HAS_FLAG(a, AnalyzerFlags_Experimental)) nameitem->setForeground(THEME_VALUE(Theme_GraphEdgeFalse));

        qDebug() << nameitem->text();
        thethis->m_analyzersmodel->appendRow({nameitem, descritem});
    }, this);

    ui->tvAnalyzers->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

void LoaderDialog::populateAssemblers()
{
    ui->cbAssembler->addItem(QString()); // Empty placeholder

    RD_GetAssemblers([](RDAssemblerPlugin* plugin, void* userdata) {
        LoaderDialog* thethis = reinterpret_cast<LoaderDialog*>(userdata);
        thethis->m_assemblers.push_back(plugin);
        thethis->ui->cbAssembler->addItem(plugin->name, plugin->id);
    }, this);
}

void LoaderDialog::onAnalyzerItemChanged(QStandardItem* item)
{
    const auto* panalyzer = reinterpret_cast<const RDAnalyzerPlugin*>(item->data().value<void*>());
    if(panalyzer) RDAnalyzer_Select(panalyzer, (item->checkState() == Qt::Checked));
}

void LoaderDialog::onAccepted()
{
    rd_flag flags = ContextFlag_None;

    if(ui->chkNoCFG->isChecked()) flags |= ContextFlag_DisableCFG;
    if(ui->chkNoAnalysis->isChecked()) flags |= ContextFlag_DisableAnalyzer;
    if(ui->chkNoSignatures->isChecked()) flags |= ContextFlag_DisableSignature;
    if(ui->chkNoUnexplored->isChecked()) flags |= ContextFlag_DisableUnexplored;

    RD_InitContextFlags(flags);
}
