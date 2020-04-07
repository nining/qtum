#include <qt/superstakerpage.h>
#include <qt/forms/ui_superstakerpage.h>
#include <qt/superstakeritemmodel.h>
#include <qt/walletmodel.h>
#include <qt/platformstyle.h>
#include <qt/styleSheet.h>
#include <qt/superstakerlistwidget.h>
#include <qt/guiutil.h>

#include <QPainter>
#include <QAbstractItemDelegate>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSizePolicy>
#include <QMenu>
#include <QMessageBox>

SuperStakerPage::SuperStakerPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SuperStakerPage),
    m_model(0),
    m_clientModel(0)
{
    ui->setupUi(this);

    m_platformStyle = platformStyle;

    m_configSuperStakerPage = new SuperStakerConfigDialog(this);
    m_addSuperStakerPage = new AddSuperStakerPage(this);

    m_configSuperStakerPage->setEnabled(false);

    QAction *copyStakerAction = new QAction(tr("Copy staker address"), this);
    QAction *copyStekerFeeAction = new QAction(tr("Copy staker fee"), this);
    QAction *configSuperStakerAction = new QAction(tr("Config super staker"), this);

    m_superStakerList = new SuperStakerListWidget(platformStyle, this);
    m_superStakerList->setContextMenuPolicy(Qt::CustomContextMenu);
    new QVBoxLayout(ui->scrollArea);
    ui->scrollArea->setWidget(m_superStakerList);
    ui->scrollArea->setWidgetResizable(true);
    connect(m_superStakerList, &SuperStakerListWidget::configSuperStaker, this, &SuperStakerPage::on_configSuperStaker);
    connect(m_superStakerList, &SuperStakerListWidget::addSuperStaker, this, &SuperStakerPage::on_addSuperStaker);
    connect(m_superStakerList, &SuperStakerListWidget::removeSuperStaker, this, &SuperStakerPage::on_removeSuperStaker);

    contextMenu = new QMenu(m_superStakerList);
    contextMenu->addAction(copyStakerAction);
    contextMenu->addAction(copyStekerFeeAction);
    contextMenu->addAction(configSuperStakerAction);

    connect(copyStekerFeeAction, &QAction::triggered, this, &SuperStakerPage::copyStekerFee);
    connect(copyStakerAction, &QAction::triggered, this, &SuperStakerPage::copyStakerAddress);
    connect(configSuperStakerAction, &QAction::triggered, this, &SuperStakerPage::configSuperStaker);

    connect(m_superStakerList, &SuperStakerListWidget::customContextMenuRequested, this, &SuperStakerPage::contextualMenu);
}

SuperStakerPage::~SuperStakerPage()
{
    delete ui;
}

void SuperStakerPage::setModel(WalletModel *_model)
{
    m_model = _model;
    m_addSuperStakerPage->setModel(m_model);
    m_configSuperStakerPage->setModel(m_model);
    m_superStakerList->setModel(m_model);
    if(m_model && m_model->getSuperStakerItemModel())
    {
        // Set current super staker
        connect(m_superStakerList->superStakerModel(), &QAbstractItemModel::dataChanged, this, &SuperStakerPage::on_dataChanged);
        connect(m_superStakerList->superStakerModel(), &QAbstractItemModel::rowsInserted, this, &SuperStakerPage::on_rowsInserted);
        if(m_superStakerList->superStakerModel()->rowCount() > 0)
        {
            QModelIndex currentSuperStaker(m_superStakerList->superStakerModel()->index(0, 0));
            on_currentSuperStakerChanged(currentSuperStaker);
        }
    }
}

void SuperStakerPage::setClientModel(ClientModel *_clientModel)
{
    m_clientModel = _clientModel;
    m_configSuperStakerPage->setClientModel(_clientModel);
}

void SuperStakerPage::on_goToConfigSuperStakerPage()
{
    m_configSuperStakerPage->show();
}

void SuperStakerPage::on_goToAddSuperStakerPage()
{
    m_addSuperStakerPage->show();
}

void SuperStakerPage::on_currentSuperStakerChanged(QModelIndex index)
{
    if(m_superStakerList->superStakerModel())
    {
        if(index.isValid())
        {
            m_selectedSuperStakerHash = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::HashRole).toString();
            QString address = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::StakerRole).toString();
            QString hash = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::HashRole).toString();
            m_configSuperStakerPage->setSuperStakerData(address, hash);

            if(!m_configSuperStakerPage->isEnabled())
                m_configSuperStakerPage->setEnabled(true);
        }
        else
        {
            m_configSuperStakerPage->setEnabled(false);
            m_configSuperStakerPage->setSuperStakerData("", "");
            m_selectedSuperStakerHash = "";
        }
    }
}

void SuperStakerPage::on_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);

    if(m_superStakerList->superStakerModel())
    {
        QString superStakerHash = m_superStakerList->superStakerModel()->data(topLeft, SuperStakerItemModel::HashRole).toString();
        if(m_selectedSuperStakerHash.isEmpty() ||
                superStakerHash == m_selectedSuperStakerHash)
        {
            on_currentSuperStakerChanged(topLeft);
        }
    }
}

void SuperStakerPage::on_currentChanged(QModelIndex current, QModelIndex previous)
{
    Q_UNUSED(previous);

    on_currentSuperStakerChanged(current);
}

void SuperStakerPage::on_rowsInserted(QModelIndex index, int first, int last)
{
    Q_UNUSED(index);
    Q_UNUSED(first);
    Q_UNUSED(last);

    if(m_superStakerList->superStakerModel()->rowCount() == 1)
    {
        QModelIndex currentSuperStaker(m_superStakerList->superStakerModel()->index(0, 0));
        on_currentSuperStakerChanged(currentSuperStaker);
    }
}

void SuperStakerPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = m_superStakerList->indexAt(point);
    if(index.isValid())
    {
        indexMenu = index;
        contextMenu->exec(QCursor::pos());
    }
}

void SuperStakerPage::copyStekerFee()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(SuperStakerItemModel::FeeRole).toString());
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::copyStakerAddress()
{
    if(indexMenu.isValid())
    {
        GUIUtil::setClipboard(indexMenu.data(SuperStakerItemModel::StakerRole).toString());
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::configSuperStaker()
{
    if(indexMenu.isValid())
    {
        on_configSuperStaker(indexMenu);
        indexMenu = QModelIndex();
    }
}

void SuperStakerPage::on_configSuperStaker(const QModelIndex &index)
{
    on_currentSuperStakerChanged(index);
    on_goToConfigSuperStakerPage();
}

void SuperStakerPage::on_addSuperStaker()
{
    on_goToAddSuperStakerPage();
}

void SuperStakerPage::on_removeSuperStaker(const QModelIndex &index)
{
    if(index.isValid() && m_model)
    {
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm super staker removal"), tr("The selected super staker will be removed from the list. Are you sure?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(btnRetVal == QMessageBox::Yes)
        {
            QString hash = m_superStakerList->superStakerModel()->data(index, SuperStakerItemModel::HashRole).toString();
            m_model->wallet().removeSuperStakerEntry(hash.toStdString());
        }
    }
}
