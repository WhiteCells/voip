#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include "voipcore.h"

#include <QMainWindow>

class QWidget;
class QMenu;
class Phone;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUI();
    void connectSignalSlot();

private:
    Phone *m_phone;
    QMenu *m_prop_menu;
    QAction *m_account_action;
    std::shared_ptr<VoipCore> m_voip_core;

signals:
    void signal_m_account_action();

private slots:
    void slot_triggered_m_account_action();
};

#endif // _MAINWINDOW_H_