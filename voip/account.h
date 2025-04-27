#ifndef _ACCOUNT_H_
#define _ACCOUNT_H_

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

class AccountDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AccountDialog(QWidget *parent = nullptr);
    ~AccountDialog();

    const QString getAccountName() const;
    const QString getSipServer() const;
    const QString getSipProxy() const;
    const QString getUsername() const;
    const QString getDoamin() const;
    const QString getPassword() const;

private:
    void setupUI();
    void connectSignalSlot();

private:
    QLabel *m_account_name_label;
    QLineEdit *m_account_name_edit;
    QLabel *m_sip_server_label;
    QLineEdit *m_sip_server_edit;
    QLabel *m_sip_proxy_label;
    QLineEdit *m_sip_proxy_edit;
    QLabel *m_username_label;
    QLineEdit *m_username_edit;
    QLabel *m_domain_label;
    QLineEdit *m_domain_edit;
    QLabel *m_password_label;
    QLineEdit *m_password_edit;

    QPushButton *m_save_button;
    QPushButton *m_cancel_button;
};

#endif // _ACCOUNT_H_