#include <QApplication>

#include "main_window.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    online_board::client_qt::MainWindow window;
    window.show();
    return app.exec();
}
