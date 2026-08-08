#ifndef EXPENSEEDITORWIDGET_H
#define EXPENSEEDITORWIDGET_H

#include <QWidget>

class QLineEdit;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QPushButton;

class ExpenseEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ExpenseEditorWidget(QWidget* parent = nullptr);

    QString itemText() const;
    double amount() const;
    QString dateText() const;
    QString cardholderText() const;
    QString payerText() const;
    QString paidForText() const;
    bool equalSplit() const;

    void setUserOptions(const QStringList& users);
    void clearForm();
    void clearForMultiSelection();

    void setItemText(const QString& text);
    void setAmount(double amount);
    void setDateText(const QString& text);
    void setCardholder(const QString& cardholder);
    void setPayer(const QString& payer);
    void setPaidFor(const QString& paidFor);
    void setEqualSplit(bool checked);

signals:
    void itemEdited();
    void amountEdited();
    void dateEdited();
    void cardholderChanged(int index);
    void payerChanged(int index);
    void paidForChanged(int index);
    void equalSplitToggled(bool checked);
    void addExpenseRequested();
    void removeExpenseRequested();

private:
    void ensureComboPlaceholder(QComboBox* combo, const QString& placeholder);
    void removeComboPlaceholder(QComboBox* combo, const QString& placeholder);
    bool hasPlaceholder(QComboBox* combo, const QString& placeholder) const;

    bool m_updatingForm;
    QLineEdit* itemEdit;
    QDoubleSpinBox* amountEdit;
    QLineEdit* dateEdit;
    QComboBox* cardholderCombo;
    QComboBox* payerCombo;
    QComboBox* paidForCombo;
    QCheckBox* equalSplitCheck;
    QPushButton* addButton;
    QPushButton* removeButton;
};

#endif // EXPENSEEDITORWIDGET_H
