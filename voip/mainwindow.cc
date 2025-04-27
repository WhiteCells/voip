#include "mainwindow.h"
#include "account.h"
#include "phone.h"

#include <QWidget>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_phone(new Phone {}),
    m_account_action(new QAction {"账户", this}),
    m_voip_core(std::make_shared<VoipCore>())
{
    setupUI();
    connectSignalSlot();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    this->setMaximumSize(QSize(300, 550));
    this->setMinimumSize(QSize(300, 550));
    this->setWindowTitle("voip");
    this->setCentralWidget(m_phone);

    // menu
    m_prop_menu = menuBar()->addMenu("编辑");
    m_prop_menu->addAction(m_account_action);
}

void MainWindow::connectSignalSlot()
{
    this->connect(m_account_action, &QAction::triggered,
                  this, &MainWindow::slot_triggered_m_account_action);
    this->connect(m_phone, &Phone::signal_m_call_out_button_click,
                  m_voip_core.get(), &VoipCore::slot_call_out);
}

void MainWindow::slot_triggered_m_account_action()
{
    // emit signal_m_account_action();
    static AccountDialog *dlg = nullptr;
    if (!dlg) {
        dlg = new AccountDialog(this);
    }
    if (dlg->isVisible()) {
        dlg->activateWindow();
        dlg->raise();
        return;
    }
    dlg->exec();
    const QString account_name = dlg->getAccountName();
    qDebug() << account_name;
}