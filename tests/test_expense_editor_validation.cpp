#include <QApplication>
#include <QLineEdit>
#include <QString>
#include <iostream>

#include "ExpenseEditorWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QLineEdit dateEdit;
    dateEdit.setInputMask("0000-00-00");
    dateEdit.insert("202604");
    if (dateEdit.text() != "2026-04") {
        std::cerr << "Expected 2026-04 after typing 202604, got " << dateEdit.text().toStdString() << std::endl;
        return 1;
    }

    dateEdit.clear();
    dateEdit.insert("20260415");
    if (dateEdit.text() != "2026-04-15") {
        std::cerr << "Expected 2026-04-15 after typing 20260415, got " << dateEdit.text().toStdString() << std::endl;
        return 1;
    }

    ExpenseEditorWidget editor;

    editor.setItemText("Groceries");
    editor.setAmount(12.34);
    editor.setDateText("2026-08-11");

    QString error;
    if (!editor.isFormDataValid(&error)) {
        std::cerr << "Valid entry rejected: " << error.toStdString() << std::endl;
        return 1;
    }

    editor.setDateText("2026/08/11");
    if (editor.isFormDataValid(&error)) {
        std::cerr << "Invalid date accepted" << std::endl;
        return 1;
    }

    editor.setDateText("2026-08-11");
    editor.setItemText("   ");
    if (editor.isFormDataValid(&error)) {
        std::cerr << "Blank item accepted" << std::endl;
        return 1;
    }

    std::cout << "Expense editor validation passed" << std::endl;
    return 0;
}
