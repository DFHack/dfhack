#ifndef memxmlModel_H
#define memxmlModel_H

#include <qabstractitemmodel.h>
#include <qdom.h>

class DomItem;

class MemXMLModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    MemXMLModel(QDomDocument document, QObject *parent = 0);
    ~MemXMLModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QDomDocument domDocument;
    DomItem *rootItem;
};
#endif // memxmlModel
