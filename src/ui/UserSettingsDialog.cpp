#include "UserSettingsDialog.h"
#include "userList.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>
#include <QHeaderView>
#include <algorithm>

UserSettingsDialog::UserSettingsDialog(UserList& userList, QWidget* parent)
    : QDialog(parent),
      m_userList(userList),
      userTable(new QTableWidget(this)),
      nameEdit(new QLineEdit(this)),
      salaryEdit(new QSpinBox(this)),
      cardNumberEdit(new QLineEdit(this)),
      addButton(new QPushButton("Add user", this)),
      removeButton(new QPushButton("Remove user", this)),
      closeButton(new QPushButton("Close", this)),
      m_updatingForm(false),
      selectedRow(-1) {
    setWindowTitle("User settings");
    resize(400, 300);

    userTable->setColumnCount(3);
    userTable->setHorizontalHeaderLabels({"Name", "Salary", "Card number"});
    userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    userTable->setAlternatingRowColors(true);
    userTable->horizontalHeader()->setStretchLastSection(true);

    QFormLayout* formLayout = new QFormLayout();
    salaryEdit->setRange(0, 10000000);
    nameEdit->setPlaceholderText("Enter user name");
    cardNumberEdit->setPlaceholderText("4 digits");
    formLayout->addRow("Name", nameEdit);
    formLayout->addRow("Salary", salaryEdit);
    formLayout->addRow("Card number", cardNumberEdit);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(removeButton);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(userTable);
    dialogLayout->addLayout(formLayout);
    dialogLayout->addLayout(buttonsLayout);

    connect(userTable, &QTableWidget::cellClicked, this, &UserSettingsDialog::onUserSelected);
    connect(nameEdit, &QLineEdit::editingFinished, this, &UserSettingsDialog::onFormEdited);
    connect(salaryEdit, QOverload<int>::of(&QSpinBox::valueChanged), this, &UserSettingsDialog::onFormEdited);
    connect(cardNumberEdit, &QLineEdit::editingFinished, this, &UserSettingsDialog::onFormEdited);
    connect(addButton, &QPushButton::clicked, this, &UserSettingsDialog::addUser);
    connect(removeButton, &QPushButton::clicked, this, &UserSettingsDialog::removeUser);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    refreshUserList();
}

void UserSettingsDialog::refreshUserList() {
    userTable->setRowCount(0);
    userTable->setRowCount(m_userList.size());
    for (int i = 1; i <= m_userList.size(); ++i) {
        const User user = m_userList.getUser(i);
        userTable->setItem(i - 1, 0, new QTableWidgetItem(QString::fromStdString(user.getName())));
        userTable->setItem(i - 1, 1, new QTableWidgetItem(QString::number(user.getSalary())));
        userTable->setItem(i - 1, 2, new QTableWidgetItem(QString::fromStdString(user.getCardNumber())));
    }
}

void UserSettingsDialog::clearForm() {
    nameEdit->clear();
    salaryEdit->setValue(0);
    cardNumberEdit->clear();
}

void UserSettingsDialog::onUserSelected(int row, int) {
    if (row < 0) {
        selectedRow = -1;
        return;
    }
    selectedRow = row;
    m_updatingForm = true;
    const User user = m_userList.getUser(row + 1);
    nameEdit->setText(QString::fromStdString(user.getName()));
    salaryEdit->setValue(user.getSalary());
    cardNumberEdit->setText(QString::fromStdString(user.getCardNumber()));
    m_updatingForm = false;
    userTable->selectRow(row);
}

void UserSettingsDialog::onFormEdited() {
    if (m_updatingForm || selectedRow < 0) {
        return;
    }
    updateSelectedUser();
}

void UserSettingsDialog::updateSelectedUser() {
    if (selectedRow < 0) {
        return;
    }

    const User oldUser = m_userList.getUser(selectedRow + 1);
    const QString name = nameEdit->text().trimmed();
    const QString cardNumber = cardNumberEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Missing data", "Please enter a user name.");
        m_updatingForm = true;
        nameEdit->setText(QString::fromStdString(oldUser.getName()));
        m_updatingForm = false;
        return;
    }

    if (cardNumber.size() != 4 || !std::all_of(cardNumber.begin(), cardNumber.end(), [](QChar ch) { return ch.isDigit(); })) {
        QMessageBox::warning(this, "Invalid input", "Please enter a 4-digit card number.");
        m_updatingForm = true;
        cardNumberEdit->setText(QString::fromStdString(oldUser.getCardNumber()));
        m_updatingForm = false;
        return;
    }

    if (name == QString::fromStdString(oldUser.getName()) && salaryEdit->value() == oldUser.getSalary() && cardNumber == QString::fromStdString(oldUser.getCardNumber())) {
        return;
    }

    m_userList.modifyUser(selectedRow + 1, name.toStdString(), salaryEdit->value(), cardNumber.toStdString());
    refreshUserList();
    userTable->selectRow(selectedRow);
    emit usersChanged();
}

void UserSettingsDialog::addUser() {
    const QString name = nameEdit->text().trimmed();
    const QString cardNumber = cardNumberEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Missing data", "Please enter a user name.");
        return;
    }
    if (cardNumber.size() != 4 || !std::all_of(cardNumber.begin(), cardNumber.end(), [](QChar ch) { return ch.isDigit(); })) {
        QMessageBox::warning(this, "Invalid input", "Please enter a 4-digit card number.");
        return;
    }

    User user;
    user.setName(name.toStdString());
    user.setSalary(salaryEdit->value());
    user.setCardNumber(cardNumber.toStdString());
    m_userList.addUser(user);
    refreshUserList();
    clearForm();
    emit usersChanged();
}

void UserSettingsDialog::removeUser() {
    if (userTable->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select a user first.");
        return;
    }

    m_userList.deleteUser(userTable->currentRow() + 1);
    refreshUserList();
    clearForm();
    emit usersChanged();
}
