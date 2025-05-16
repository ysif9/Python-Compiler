#ifndef PARSERTREEDIALOG_HPP
#define PARSERTREEDIALOG_HPP
#include <QLabel>

#include "ErrorDialog.hpp"


class ParserTreeDialog final : public QDialog {
    Q_OBJECT


public:
    explicit ParserTreeDialog(const QString &dotFilePath = "../dot_example.dot", QWidget *parent = nullptr);
    ~ParserTreeDialog() override = default; // Use default destructor

private:
    QLabel *imageLabel;

    void setupUi();

    static QPixmap renderDotToPixmap(const QString &dotFilePath);

};

#endif //PARSERTREEDIALOG_HPP
