#include "ExpenseEditorWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

ExpenseEditorWidget::ExpenseEditorWidget(QWidget* parent)
    : QWidget(parent),
      m_updatingForm(false),
      itemEdit(new QLineEdit(this)),
      amountEdit(new QDoubleSpinBox(this)),
      dateEdit(new QLineEdit(this)),
      cardholderCombo(new QComboBox(this)),
      payerCombo(new QComboBox(this)),
      paidForCombo(new QComboBox(this)),
      equalSplitCheck(new QCheckBox("Equal split", this)),
      addButton(new QPushButton("New expense", this)),
      removeButton(new QPushButton("Remove expense", this)) {
    QFormLayout* expenseFormLayout = new QFormLayout(this);
    amountEdit->setRange(0.0, 10000000.0);
    amountEdit->setDecimals(2);
    amountEdit->setSpecialValueText("");
    dateEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(R"(^\d{4}-\d{2}-\d{2}$)"), this));

    paidForCombo->addItem("Both");
    equalSplitCheck->setTristate(false);

    expenseFormLayout->addRow("Item", itemEdit);
    expenseFormLayout->addRow("Amount", amountEdit);
    expenseFormLayout->addRow("Date", dateEdit);
    expenseFormLayout->addRow("Cardholder", cardholderCombo);
    expenseFormLayout->addRow("Paid by", payerCombo);
    expenseFormLayout->addRow("Paid for", paidForCombo);
    expenseFormLayout->addRow("", equalSplitCheck);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    expenseFormLayout->addRow(buttonLayout);

    connect(itemEdit, &QLineEdit::textEdited, this, [this]() {
        if (!m_updatingForm) emit itemEdited();
    });
    connect(amountEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() {
        if (!m_updatingForm) emit amountEdited();
    });
    connect(dateEdit, &QLineEdit::editingFinished, this, [this]() {
        if (!m_updatingForm) emit dateEdited();
    });
    connect(cardholderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!m_updatingForm) emit cardholderChanged(index);
    });
    connect(payerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!m_updatingForm) emit payerChanged(index);
    });
    connect(paidForCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!m_updatingForm) emit paidForChanged(index);
    });
    connect(equalSplitCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_updatingForm) emit equalSplitToggled(checked);
    });
    connect(addButton, &QPushButton::clicked, this, &ExpenseEditorWidget::addExpenseRequested);
    connect(removeButton, &QPushButton::clicked, this, &ExpenseEditorWidget::removeExpenseRequested);
}

QString ExpenseEditorWidget::itemText() const {
    return itemEdit->text();
}

double ExpenseEditorWidget::amount() const {
    return amountEdit->value();
}

QString ExpenseEditorWidget::dateText() const {
    return dateEdit->text();
}

QString ExpenseEditorWidget::cardholderText() const {
    return cardholderCombo->currentText();
}

QString ExpenseEditorWidget::payerText() const {
    return payerCombo->currentText();
}

QString ExpenseEditorWidget::paidForText() const {
    return paidForCombo->currentText();
}

bool ExpenseEditorWidget::equalSplit() const {
    return equalSplitCheck->isChecked();
}

void ExpenseEditorWidget::setUserOptions(const QStringList& users) {
    const bool cardholderPlaceholder = hasPlaceholder(cardholderCombo, "(multiple)");
    const bool payerPlaceholder = hasPlaceholder(payerCombo, "(multiple)");
    const bool paidForPlaceholder = hasPlaceholder(paidForCombo, "(multiple)");

    cardholderCombo->blockSignals(true);
    payerCombo->blockSignals(true);
    paidForCombo->blockSignals(true);

    cardholderCombo->clear();
    payerCombo->clear();
    paidForCombo->clear();

    if (cardholderPlaceholder) {
        cardholderCombo->addItem("(multiple)");
    }
    if (payerPlaceholder) {
        payerCombo->addItem("(multiple)");
    }
    if (paidForPlaceholder) {
        paidForCombo->addItem("(multiple)");
    }

    cardholderCombo->addItems(users);
    payerCombo->addItems(users);

    paidForCombo->addItem("Both");
    paidForCombo->addItems(users);

    cardholderCombo->blockSignals(false);
    payerCombo->blockSignals(false);
    paidForCombo->blockSignals(false);
}

void ExpenseEditorWidget::clearForm() {
    m_updatingForm = true;
    itemEdit->clear();
    itemEdit->setPlaceholderText("");
    amountEdit->setSpecialValueText("");
    amountEdit->setValue(0.0);
    dateEdit->clear();
    dateEdit->setPlaceholderText("");
    if (cardholderCombo) {
        removeComboPlaceholder(cardholderCombo, "(multiple)");
        cardholderCombo->setCurrentIndex(-1);
    }
    if (payerCombo) {
        removeComboPlaceholder(payerCombo, "(multiple)");
        payerCombo->setCurrentIndex(-1);
    }
    if (paidForCombo) {
        removeComboPlaceholder(paidForCombo, "(multiple)");
        paidForCombo->setCurrentIndex(0);
    }
    equalSplitCheck->setCheckState(Qt::Unchecked);
    m_updatingForm = false;
}

void ExpenseEditorWidget::clearForMultiSelection() {
    m_updatingForm = true;
    itemEdit->clear();
    itemEdit->setPlaceholderText("(multiple)");
    amountEdit->setSpecialValueText("(multiple)");
    amountEdit->setValue(amountEdit->minimum());
    dateEdit->clear();
    dateEdit->setPlaceholderText("(multiple)");
    ensureComboPlaceholder(cardholderCombo, "(multiple)");
    ensureComboPlaceholder(payerCombo, "(multiple)");
    ensureComboPlaceholder(paidForCombo, "(multiple)");
    cardholderCombo->setCurrentIndex(0);
    payerCombo->setCurrentIndex(0);
    paidForCombo->setCurrentIndex(0);
    equalSplitCheck->setCheckState(Qt::Unchecked);
    m_updatingForm = false;
}

void ExpenseEditorWidget::setItemText(const QString& text) {
    m_updatingForm = true;
    itemEdit->setText(text);
    itemEdit->setPlaceholderText("");
    m_updatingForm = false;
}

void ExpenseEditorWidget::setAmount(double amount) {
    m_updatingForm = true;
    amountEdit->setSpecialValueText("");
    amountEdit->setValue(amount);
    m_updatingForm = false;
}

void ExpenseEditorWidget::setDateText(const QString& text) {
    m_updatingForm = true;
    dateEdit->setText(text);
    dateEdit->setPlaceholderText("");
    m_updatingForm = false;
}

void ExpenseEditorWidget::setCardholder(const QString& cardholder) {
    m_updatingForm = true;
    removeComboPlaceholder(cardholderCombo, "(multiple)");
    const int index = cardholderCombo->findText(cardholder);
    cardholderCombo->setCurrentIndex(index >= 0 ? index : -1);
    m_updatingForm = false;
}

void ExpenseEditorWidget::setPayer(const QString& payer) {
    m_updatingForm = true;
    removeComboPlaceholder(payerCombo, "(multiple)");
    const int index = payerCombo->findText(payer);
    payerCombo->setCurrentIndex(index >= 0 ? index : -1);
    m_updatingForm = false;
}

void ExpenseEditorWidget::setPaidFor(const QString& paidFor) {
    m_updatingForm = true;
    removeComboPlaceholder(paidForCombo, "(multiple)");
    const int index = paidForCombo->findText(paidFor);
    paidForCombo->setCurrentIndex(index >= 0 ? index : 0);
    m_updatingForm = false;
}

void ExpenseEditorWidget::setEqualSplit(bool checked) {
    m_updatingForm = true;
    equalSplitCheck->setChecked(checked);
    m_updatingForm = false;
}

void ExpenseEditorWidget::ensureComboPlaceholder(QComboBox* combo, const QString& placeholder) {
    if (!combo) {
        return;
    }
    if (!hasPlaceholder(combo, placeholder)) {
        combo->insertItem(0, placeholder);
    }
}

void ExpenseEditorWidget::removeComboPlaceholder(QComboBox* combo, const QString& placeholder) {
    if (!combo) {
        return;
    }
    const int index = combo->findText(placeholder);
    if (index >= 0) {
        combo->removeItem(index);
    }
}

bool ExpenseEditorWidget::hasPlaceholder(QComboBox* combo, const QString& placeholder) const {
    if (!combo) {
        return false;
    }
    return combo->findText(placeholder) >= 0;
}
