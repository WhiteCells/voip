#include "account.h"

#include <QLineEdit>
#include <QLabel>
#include <QFormLayout>
#include <QPushButton>

AccountDialog::AccountDialog(QWidget *parent) :
    QDialog(parent),
    m_account_name_label(new QLabel {"Account Name"}),
    m_account_name_edit(new QLineEdit {}),
    m_sip_server_label(new QLabel {"SIP Server"}),
    m_sip_server_edit(new QLineEdit {}),
    m_sip_proxy_label(new QLabel {"SIP Proxy"}),
    m_sip_proxy_edit(new QLineEdit {}),
    m_username_label(new QLabel {"Username"}),
    m_username_edit(new QLineEdit {}),
    m_domain_label(new QLabel {"Domain"}),
    m_domain_edit(new QLineEdit {}),
    m_password_label(new QLabel {"Password"}),
    m_password_edit(new QLineEdit {}),
    m_save_button(new QPushButton {"Save"}),
    m_cancel_button(new QPushButton {"Cancel"})
{
    setupUI();
    connectSignalSlot();
}

AccountDialog::~AccountDialog()
{
}

const QString AccountDialog::getAccountName() const {
    return m_account_name_edit->text();
}

const QString AccountDialog::getSipServer() const {
    return m_sip_server_edit->text();
}

const QString AccountDialog::getSipProxy() const {
    return m_sip_proxy_edit->text();
}

const QString AccountDialog::getUsername() const {
    return m_username_edit->text();
}

const QString AccountDialog::getDoamin() const {
    return m_domain_edit->text();
}

const QString AccountDialog::getPassword() const {
    return m_password_edit->text();
}

void AccountDialog::setupUI()
{
    this->setWindowTitle("Account Setting");
    this->setMinimumSize(QSize(430, 330));
    this->setMaximumSize(QSize(430, 330));

    QFormLayout *from_layout = new QFormLayout();
    from_layout->setLabelAlignment(Qt::AlignRight);
    from_layout->setFormAlignment(Qt::AlignCenter);
    from_layout->addRow(m_account_name_label, m_account_name_edit);
    from_layout->addRow(m_sip_server_label, m_sip_server_edit);
    from_layout->addRow(m_sip_proxy_label, m_sip_proxy_edit);
    from_layout->addRow(m_username_label, m_username_edit);
    from_layout->addRow(m_domain_label, m_domain_edit);
    from_layout->addRow(m_password_label, m_password_edit);

    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addStretch();
    button_layout->addWidget(m_save_button);
    button_layout->addWidget(m_cancel_button);

    QVBoxLayout *form_layout = new QVBoxLayout(this);
    form_layout->addLayout(from_layout);
    form_layout->addLayout(button_layout);
}

void AccountDialog::connectSignalSlot()
{
    this->connect(m_save_button, &QPushButton::clicked,
                  this, &QDialog::accept);
    this->connect(m_cancel_button, &QPushButton::clicked,
                  this, &QDialog::reject);
}
