#include "phone.h"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

Phone::Phone(QWidget *parent) :
    QWidget(parent),
    m_backspace_button(new QPushButton {"Backspace"}),
    m_call_out_button(new QPushButton {"Call"})
{
    setupUI();
    connectSignalSlot();
}

Phone::~Phone()
{
}

const QString Phone::getPhoneNum() const
{
    return m_display->text();
}

void Phone::setupUI()
{
    QVBoxLayout *main_layout = new QVBoxLayout(this);
    m_display = new QLineEdit {this};
    m_display->setReadOnly(true);
    m_display->setAlignment(Qt::AlignCenter);
    m_display->setFixedHeight(50);
    m_display->setStyleSheet("font-size: 20px; padding: 10px;");
    main_layout->addWidget(m_display);

    QGridLayout *gridLayout = new QGridLayout {};

    QString buttons[12] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9",
        "*", "0", "#"};

    int pos = 0;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            QPushButton *btn = new QPushButton(buttons[pos], this);
            btn->setFixedSize(80, 80);
            btn->setStyleSheet("font-size: 20px; border-radius: 10px;");
            connect(btn, &QPushButton::clicked, this, &Phone::slot_number_to_line);
            gridLayout->addWidget(btn, row, col);
            ++pos;
        }
    }
    main_layout->addLayout(gridLayout);

    QHBoxLayout *bottomLayout = new QHBoxLayout {};
    m_backspace_button->setStyleSheet("background-color: lightcoral; color: white; font-size: 18px; border-radius: 10px; padding: 10px;");
    m_call_out_button->setStyleSheet("background-color: limegreen; color: white; font-size: 18px; border-radius: 10px; padding: 10px;");

    bottomLayout->addWidget(m_backspace_button);
    bottomLayout->addWidget(m_call_out_button);

    main_layout->addLayout(bottomLayout);
}

void Phone::connectSignalSlot()
{
    this->connect(m_backspace_button, &QPushButton::clicked,
                  this, &Phone::slot_number_backspace);
    this->connect(m_call_out_button, &QPushButton::clicked,
                  this, &Phone::slot_call_out);
}

void Phone::slot_number_to_line()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (btn) {
        m_display->setText(m_display->text() + btn->text());
    }
    m_call_out_button->setStyleSheet("background-color: limegreen; color: white; font-size: 18px; border-radius: 10px; padding: 10px;");
}

void Phone::slot_number_backspace()
{
    QString cur_display_text = m_display->text();
    if (!cur_display_text.isEmpty()) {
        cur_display_text.chop(1);
        m_display->setText(cur_display_text);
    }
}

void Phone::slot_call_out()
{
    QString number = m_display->text();
    if (number.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入号码！");
    }
    else {
        // QMessageBox::information(this, "拨打中", "正在拨打: " + number);
        emit signal_m_call_out_button_click(getPhoneNum());
    }
}
