#pragma once

#include <QWidget>
#include <QHeaderView>
#include "../hooks/itabletab.h"

namespace Ui {
class TableTab;
}

class ListingItemModel;
class QSortFilterProxyModel;

class TableTab : public QWidget, public ITableTab
{
    Q_OBJECT

    public:
        explicit TableTab(ListingItemModel* model, QWidget *parent = nullptr);
        virtual ~TableTab();
        void setSectionResizeMode(int idx, QHeaderView::ResizeMode mode);
        void setColumnHidden(int idx);
        void resizeColumn(int idx);
        void moveSection(int from, int to);

    public slots:
        void resizeAllColumns();

    private slots:
        void onTableDoubleClick(const QModelIndex& index);

    protected:
        bool event(QEvent* e) override;

    public: // ITableTab implementation
        void toggleFilter() override;
        ListingItemModel* model() const override;

    signals:
        void resizeColumns();

    private:
        Ui::TableTab *ui;
        ListingItemModel* m_listingitemmodel;
        QSortFilterProxyModel* m_filtermodel;
};

