#ifndef USERSETTINGS_DIALOG_H
#define USERSETTINGS_DIALOG_H

#include <QDialog>

class UserList;
class QTableWidget;
class QLineEdit;
class QSpinBox;
class QPushButton;

class UserSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit UserSettingsDialog(UserList& userList, QWidget* parent = nullptr);

signals:
    void usersChanged();

private slots:
    void onUserSelected(int row, int column);
    void onFormEdited();
    void addUser();
    void removeUser();

private:
    void refreshUserList();
    void clearForm();
    void updateSelectedUser();

    UserList& m_userList;
    QTableWidget* userTable;
    QLineEdit* nameEdit;
    QSpinBox* salaryEdit;
    QLineEdit* cardNumberEdit;
    QPushButton* addButton;
    QPushButton* removeButton;
    QPushButton* closeButton;
    bool m_updatingForm;
    int selectedRow;
};

#endif // USERSETTINGS_DIALOG_H
