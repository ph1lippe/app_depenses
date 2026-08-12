#include <QApplication>
#include <cmath>
#include <iostream>

#include "ExpenseEditorWidget.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    ExpenseEditorWidget editor;

    editor.setAmount(-12.34);
    const double actual = editor.amount();

    if (std::abs(actual + 12.34) > 0.000001) {
        std::cerr << "Expected -12.34, got " << actual << std::endl;
        return 1;
    }

    std::cout << "Negative amount accepted: " << actual << std::endl;
    return 0;
}
