#include "gui_main.h"

#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QFileSystemModel>
#include <QTreeView>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QFileInfo>
#include <QPixmap>
#include <QMessageBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QFontDatabase>

#include <vector>
#include <fstream>
#include <sstream>

#include "tex_parser.h"

// Define a simple main window without Q_OBJECT to avoid MOC issues in a single file
class MainWindow : public QMainWindow {
public:
    MainWindow() {
        setWindowTitle("IGI Game Asset Converter (Qt Advanced UI)");
        resize(1200, 800);

        QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
        setCentralWidget(splitter);

        // Left side: File browser
        QFileSystemModel* model = new QFileSystemModel(this);
        model->setRootPath("");
        QTreeView* treeView = new QTreeView(splitter);
        treeView->setModel(model);
        treeView->setRootIndex(model->index(QDir::currentPath()));
        treeView->setColumnWidth(0, 250);

        // Right side: Viewer and Controls
        QWidget* rightWidget = new QWidget(splitter);
        QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);

        viewerEdit = new QTextEdit();
        viewerEdit->setReadOnly(true);
        const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        viewerEdit->setFont(fixedFont);

        imageLabel = new QLabel();
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->hide();

        rightLayout->addWidget(viewerEdit, 3);
        rightLayout->addWidget(imageLabel, 3);

        // Controls
        QGroupBox* controlsGroup = new QGroupBox("Conversion Options");
        QVBoxLayout* controlsLayout = new QVBoxLayout(controlsGroup);

        QHBoxLayout* outDirLayout = new QHBoxLayout();
        outDirLayout->addWidget(new QLabel("Output Path (Optional):"));
        outDirEdit = new QLineEdit();
        outDirLayout->addWidget(outDirEdit);
        controlsLayout->addLayout(outDirLayout);

        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        btnAction1 = new QPushButton("Action 1");
        btnAction2 = new QPushButton("Action 2");
        btnAction1->hide();
        btnAction2->hide();
        buttonsLayout->addWidget(btnAction1);
        buttonsLayout->addWidget(btnAction2);
        buttonsLayout->addStretch();
        controlsLayout->addLayout(buttonsLayout);

        controlsLayout->addWidget(new QLabel("Console Output:"));
        consoleEdit = new QTextEdit();
        consoleEdit->setReadOnly(true);
        consoleEdit->setFont(fixedFont);
        controlsLayout->addWidget(consoleEdit, 1);

        rightLayout->addWidget(controlsGroup, 2);

        splitter->addWidget(treeView);
        splitter->addWidget(rightWidget);
        splitter->setSizes({300, 900});

        connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this, model](const QModelIndex& current) {
            if (!model->isDir(current)) {
                loadFile(model->filePath(current));
            }
        });

        connect(btnAction1, &QPushButton::clicked, this, [this]() { executeAction(1); });
        connect(btnAction2, &QPushButton::clicked, this, [this]() { executeAction(2); });
    }

private:
    QTextEdit* viewerEdit;
    QLabel* imageLabel;
    QLineEdit* outDirEdit;
    QPushButton* btnAction1;
    QPushButton* btnAction2;
    QTextEdit* consoleEdit;
    QString currentFile;
    QString currentExt;

    void loadFile(const QString& path) {
        currentFile = path;
        QFileInfo info(path);
        currentExt = info.suffix().toLower();

        viewerEdit->clear();
        imageLabel->clear();
        btnAction1->hide();
        btnAction2->hide();

        if (currentExt == "png" || currentExt == "jpg" || currentExt == "jpeg" || currentExt == "bmp") {
            QPixmap pix(path);
            if (!pix.isNull()) {
                imageLabel->setPixmap(pix.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                viewerEdit->hide();
                imageLabel->show();
            } else {
                viewerEdit->setText("Failed to load image.");
                imageLabel->hide();
                viewerEdit->show();
            }
        } else if (currentExt == "tex" || currentExt == "spr" || currentExt == "pic") {
            // Use native game format parser
            TEXFile tex = TEX_Parse(path.toStdString());
            if (tex.valid && !tex.images.empty()) {
                const auto& img = tex.images[0];
                QImage qimg(img.width, img.height, QImage::Format_ARGB32);
                if (img.mode == 3) {
                    for (size_t i = 0; i < img.pixels.size() / 4; ++i) {
                        qimg.setPixelColor(i % img.width, i / img.width, 
                            QColor(img.pixels[i*4], img.pixels[i*4+1], img.pixels[i*4+2], img.pixels[i*4+3]));
                    }
                } else if (img.mode == 2) {
                    for (size_t i = 0; i < img.pixels.size() / 2; ++i) {
                        uint16_t pix = (img.pixels[i*2+1] << 8) | img.pixels[i*2];
                        qimg.setPixelColor(i % img.width, i / img.width, 
                            QColor(((pix >> 11) & 0x1F) * 255 / 31, ((pix >> 5) & 0x3F) * 255 / 63, (pix & 0x1F) * 255 / 31, 255));
                    }
                }
                imageLabel->setPixmap(QPixmap::fromImage(qimg).scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                viewerEdit->hide();
                imageLabel->show();
            } else {
                viewerEdit->setText("Failed to load game texture.");
                imageLabel->hide();
                viewerEdit->show();
            }
            btnAction1->setText("Convert to PNG");
            btnAction2->setText("Convert to TGA");
            btnAction1->show();
            btnAction2->show();
        } else if (currentExt == "qsc" || currentExt == "txt" || currentExt == "json" || currentExt == "md" || currentExt == "h" || currentExt == "cpp" || currentExt == "dat") {
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                viewerEdit->setPlainText(QString::fromUtf8(file.readAll()));
            }
            imageLabel->hide();
            viewerEdit->show();

            if (currentExt == "qsc") {
                btnAction1->setText("Compile to QVM");
                btnAction1->show();
            } else if (currentExt == "dat") {
                btnAction1->setText("Dump DAT Info");
                btnAction1->show();
            }
        } else {
            // Hex view fallback
            QFile file(path);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                QString hexView;
                int lines = qMin(data.size() / 16 + 1, 4000);
                for (int i = 0; i < lines; ++i) {
                    int offset = i * 16;
                    hexView += QString("%1: ").arg(offset, 8, 16, QChar('0')).toUpper();
                    
                    for (int n = 0; n < 16; n++) {
                        if (offset + n < data.size()) {
                            hexView += QString("%1 ").arg((quint8)data[offset + n], 2, 16, QChar('0')).toUpper();
                        } else {
                            hexView += "   ";
                        }
                    }
                    hexView += "| ";
                    for (int n = 0; n < 16; n++) {
                        if (offset + n < data.size()) {
                            char c = data[offset + n];
                            hexView += (c >= 32 && c < 127) ? QChar(c) : QChar('.');
                        } else {
                            hexView += " ";
                        }
                    }
                    hexView += "\n";
                }
                if (data.size() > 4000 * 16) hexView += "\n... (Truncated) ...";
                viewerEdit->setPlainText(hexView);
            }
            imageLabel->hide();
            viewerEdit->show();

            if (currentExt == "qvm") {
                btnAction1->setText("Decompile to QSC");
                btnAction1->show();
            } else if (currentExt == "mef") {
                btnAction1->setText("Export to OBJ");
                btnAction1->show();
            } else if (currentExt == "res") {
                btnAction1->setText("Extract RES");
                btnAction1->show();
            } else if (currentExt == "fnt") {
                btnAction1->setText("Export PNG");
                btnAction2->setText("FNT Info");
                btnAction1->show();
                btnAction2->show();
            }
        }
    }

    void executeAction(int actionIndex) {
        if (currentFile.isEmpty()) return;

        QString outArg = outDirEdit->text().trimmed();
        QStringList args;

        if (currentExt == "qsc" && actionIndex == 1) {
            args << "qsc" << "compile" << currentFile;
        } else if (currentExt == "qvm" && actionIndex == 1) {
            args << "qvm" << "decompile" << currentFile;
        } else if ((currentExt == "tex" || currentExt == "spr" || currentExt == "pic") && actionIndex == 1) {
            args << "tex" << "to-png" << currentFile;
        } else if ((currentExt == "tex" || currentExt == "spr" || currentExt == "pic") && actionIndex == 2) {
            args << "tex" << "to-tga" << currentFile;
        } else if (currentExt == "mef" && actionIndex == 1) {
            args << "mef" << "export" << currentFile;
        } else if (currentExt == "res" && actionIndex == 1) {
            args << "res" << "extract" << currentFile;
        } else if (currentExt == "dat" && actionIndex == 1) {
            args << "dat" << "info" << currentFile;
        } else if (currentExt == "fnt" && actionIndex == 1) {
            args << "fnt" << "export" << currentFile;
        } else if (currentExt == "fnt" && actionIndex == 2) {
            args << "fnt" << "info" << currentFile;
        }

        if (args.isEmpty()) return;

        if (!outArg.isEmpty() && currentExt != "dat" && !(currentExt == "fnt" && actionIndex == 2)) {
            args << "-o" << outArg;
        }

        QProcess process;
        process.setProgram("igi1conv"); // Call itself
        process.setArguments(args);
        
        consoleEdit->append(QString("> igi1conv %1").arg(args.join(" ")));
        process.start();
        process.waitForFinished();

        QString output = process.readAllStandardOutput();
        QString err = process.readAllStandardError();
        
        consoleEdit->append(output);
        if (!err.isEmpty()) consoleEdit->append("ERROR: " + err);
        consoleEdit->append("--------------------------------------------------");
    }
};

int run_gui() {
    int argc = 1;
    char arg0[] = "igi1conv.exe";
    char* argv[] = { arg0, nullptr };

    QApplication app(argc, argv);

    // Apply a professional dark style
    app.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);

    MainWindow window;
    window.show();

    return app.exec();
}
