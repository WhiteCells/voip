#ifndef _PHONE_H_
#define _PHONE_H_

#include <QWidget>

class QLineEdit;
class QPushButton;

class Phone : public QWidget
{
    Q_OBJECT
public:
    explicit Phone(QWidget *parent = nullptr);
    ~Phone();

    const QString getPhoneNum() const;

private:
    void setupUI();
    void connectSignalSlot();

private:
    QLineEdit *m_display;
    QPushButton *m_backspace_button;
    QPushButton *m_call_out_button;

signals:
    void signal_m_call_out_button_click(const QString phone_num);

private slots:
    void slot_number_to_line();
    void slot_number_backspace();
    void slot_call_out();
};

#endif // _PHONE_H_