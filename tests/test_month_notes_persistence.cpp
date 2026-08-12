#include <cassert>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "appdata_persistence.h"

int main() {
    const QString filePath = QDir::tempPath() + "/app_depenses_test_month_notes.txt";
    QMap<QString, QString> notes;
    notes["2026-08"] = "Kitchen expenses";

    const bool saved = app_persistence::saveMonthNotes(notes, filePath);
    assert(saved);

    QMap<QString, QString> loaded;
    const bool loadedOk = app_persistence::loadMonthNotes(loaded, filePath);
    assert(loadedOk);
    assert(loaded["2026-08"] == "Kitchen expenses");

    QFile::remove(filePath);
    return 0;
}
