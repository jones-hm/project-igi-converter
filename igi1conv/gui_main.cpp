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
#include <QComboBox>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QDesktopServices>
#include <QUrl>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QCoreApplication>
#include <QSortFilterProxyModel>
#include <QInputDialog>
#include <QClipboard>
#include <QStyleFactory>
#include <QTextDocument>

#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "tex_parser.h"
#include "mef_parser.h"
#include "mef_native.h"

#include "../../third_party/tinygltf/stb_image.h"

static QImage loadImageSafe(const QString& path) {
    QImage img(path);
    if (img.isNull()) {
        int w, h, channels;
        unsigned char* data = stbi_load(path.toLocal8Bit().constData(), &w, &h, &channels, 4);
        if (data) {
            // Must copy because stbi_image_free will deallocate the memory
            img = QImage(data, w, h, w * 4, QImage::Format_RGBA8888).copy();
            stbi_image_free(data);
        }
    }
    return img;
}

static QString formatJson(const QJsonObject& obj, int indent = 0) {
    QString result;
    QString ind(indent, ' ');
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key().toLower() == "graph") continue;
        if (it.value().isObject()) {
            result += ind + it.key() + ":\n" + formatJson(it.value().toObject(), indent + 2);
        } else if (it.value().isArray()) {
            result += ind + it.key() + ": [Array]\n";
        } else {
            result += ind + it.key() + ": " + it.value().toVariant().toString() + "\n";
        }
    }
    return result;
}

class ModelViewer : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    struct SubMesh {
        int startIndex = 0;
        int count = 0;
        QString materialName;
        std::shared_ptr<QOpenGLTexture> texture;
    };

    QTextEdit* infoOverlay;
    QLabel* coordsOverlay;
    QString currentModelName;
    QString currentModelId;
    QString currentJsonInfo;
    std::vector<SubMesh> submeshes;
    std::map<QString, std::shared_ptr<QOpenGLTexture>> textureCache;

    ModelViewer(QWidget* parent = nullptr) : QOpenGLWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        infoOverlay = new QTextEdit(this);
        infoOverlay->setReadOnly(true);
        infoOverlay->setStyleSheet("background-color: rgba(0, 20, 0, 200); color: #00FF66; border: 1px solid #00FF66; padding: 5px; font-family: monospace;");
        infoOverlay->hide();

        coordsOverlay = new QLabel(this);
        coordsOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 150); color: white; padding: 4px; font-family: monospace;");
        coordsOverlay->hide();
    }
    
    ~ModelViewer() {
        makeCurrent();
        textureCache.clear();
        submeshes.clear();
        vbo.destroy();
        doneCurrent();
    }

    std::shared_ptr<QOpenGLTexture> loadCachedTexture(const QString& texStr, const QFileInfo& info) {
        if (texStr.isEmpty()) return nullptr;
        QString key = texStr;
        key.replace("\\", "/");
        if (textureCache.find(key) != textureCache.end()) return textureCache[key];

        QString texPath = info.absolutePath() + "/" + key;
        QImage img = loadImageSafe(texPath);
        if (img.isNull()) {
            texPath = info.absolutePath() + "/" + QFileInfo(key).fileName();
            img = loadImageSafe(texPath);
        }
        if (img.isNull()) {
            texPath = info.absolutePath() + "/" + QFileInfo(key).completeBaseName() + ".png";
            img = loadImageSafe(texPath);
        }
        if (img.isNull()) {
            texPath = info.absolutePath() + "/" + QFileInfo(key).completeBaseName() + ".tga";
            img = loadImageSafe(texPath);
        }
        if (!img.isNull()) {
            auto tex = std::make_shared<QOpenGLTexture>(img);
            tex->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
            tex->setMagnificationFilter(QOpenGLTexture::Linear);
            textureCache[key] = tex;
            return tex;
        }
        return nullptr;
    }

    void updateCoordsOverlay() {
        if (!coordsOverlay->isVisible()) coordsOverlay->show();
        coordsOverlay->setText(QString("%1. ID: %2\nPos: %3, %4, %5\nRot: %6, %7, %8")
            .arg(currentModelName).arg(currentModelId)
            .arg(transX, 0, 'f', 2).arg(transY, 0, 'f', 2).arg(-zoom, 0, 'f', 2)
            .arg(rotX, 0, 'f', 1).arg(rotY, 0, 'f', 1).arg(rotZ, 0, 'f', 1));
    }

    void loadModel(const QString& path) {
        makeCurrent();
        vertices.clear(); uvs.clear(); normals.clear();
        submeshes.clear();
        textureCache.clear();
        
        QFileInfo info(path);
        QString ext = info.suffix().toLower();
        
        updateModelInfo(info.completeBaseName());
        
        if (ext == "mef") {
            try {
                ParsedGeometry geo = ParseMefFile(path.toStdString());
                std::vector<std::shared_ptr<QOpenGLTexture>> loadedMaterials;
                for (const auto& tex : geo.pmtlTextures) {
                    loadedMaterials.push_back(loadCachedTexture(QString::fromStdString(tex), info));
                }

                for (const auto& block : geo.renderBlocks) {
                    SubMesh sm;
                    sm.startIndex = vertices.size() / 3;
                    int triCount = 0;
                    for (size_t i = 0; i < block.triangleCount; ++i) {
                        size_t tIdx = block.triangleStart + i;
                        if (tIdx < geo.triangles.size()) {
                            const auto& tri = geo.triangles[tIdx];
                            auto addVert = [&](uint32_t idx) {
                                if (idx < geo.vertices.size()) {
                                    const auto& v = geo.vertices[idx];
                                    vertices.push_back(v.pos.x); vertices.push_back(v.pos.y); vertices.push_back(v.pos.z);
                                    normals.push_back(v.normal.x); normals.push_back(v.normal.y); normals.push_back(v.normal.z);
                                    uvs.push_back(v.uv.x); uvs.push_back(1.0f - v.uv.y);
                                } else {
                                    vertices.push_back(0); vertices.push_back(0); vertices.push_back(0);
                                    normals.push_back(0); normals.push_back(1); normals.push_back(0);
                                    uvs.push_back(0); uvs.push_back(0);
                                }
                            };
                            addVert(tri[0]); addVert(tri[1]); addVert(tri[2]);
                            triCount += 3;
                        }
                    }
                    sm.count = triCount;
                    if (block.materialSlot >= 0 && block.materialSlot < loadedMaterials.size()) {
                        sm.texture = loadedMaterials[block.materialSlot];
                    }
                    if (sm.count > 0) submeshes.push_back(sm);
                }
            } catch (...) {}
        } else if (ext == "obj") {
            std::vector<QVector3D> temp_v;
            std::vector<QVector2D> temp_vt;
            std::vector<QVector3D> temp_vn;
            
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString mtlLib = "";
                while (!file.atEnd()) {
                    QString line = file.readLine().trimmed();
                    if (line.isEmpty() || line.startsWith("#")) continue;
                    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (parts.isEmpty()) continue;
                    
                    if (parts[0] == "v" && parts.size() >= 4) {
                        temp_v.push_back(QVector3D(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()));
                    } else if (parts[0] == "vt" && parts.size() >= 3) {
                        temp_vt.push_back(QVector2D(parts[1].toFloat(), parts[2].toFloat()));
                    } else if (parts[0] == "vn" && parts.size() >= 4) {
                        temp_vn.push_back(QVector3D(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()));
                    } else if (parts[0] == "usemtl" && parts.size() >= 2) {
                        if (!submeshes.empty() && submeshes.back().count == 0) {
                            submeshes.back().materialName = parts[1];
                        } else {
                            SubMesh sm;
                            sm.startIndex = vertices.size() / 3;
                            sm.count = 0;
                            sm.materialName = parts[1];
                            submeshes.push_back(sm);
                        }
                    } else if (parts[0] == "f" && parts.size() >= 4) {
                        if (submeshes.empty()) {
                            SubMesh sm;
                            sm.startIndex = vertices.size() / 3;
                            sm.count = 0;
                            submeshes.push_back(sm);
                        }
                        int startSize = vertices.size() / 3;
                        for (int i = 2; i < parts.size() - 1; ++i) {
                            parseFaceVertex(parts[1], temp_v, temp_vt, temp_vn);
                            parseFaceVertex(parts[i], temp_v, temp_vt, temp_vn);
                            parseFaceVertex(parts[i+1], temp_v, temp_vt, temp_vn);
                        }
                        submeshes.back().count += (vertices.size() / 3) - startSize;
                    } else if (parts[0] == "mtllib" && parts.size() >= 2) {
                        mtlLib = parts[1];
                    }
                }
                
                if (!mtlLib.isEmpty()) {
                    QString mtlPath = info.absolutePath() + "/" + mtlLib;
                    QFile mfile(mtlPath);
                    if (mfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString currentMaterial = "";
                        while (!mfile.atEnd()) {
                            QString mline = mfile.readLine().trimmed();
                            if (mline.startsWith("newmtl ")) {
                                currentMaterial = mline.mid(7).trimmed();
                            } else if (mline.startsWith("map_Kd ") && !currentMaterial.isEmpty()) {
                                QString texStr = mline.mid(7).trimmed();
                                auto tex = loadCachedTexture(texStr, info);
                                for (auto& sm : submeshes) {
                                    if (sm.materialName == currentMaterial) {
                                        sm.texture = tex;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        centerModel();
        setupBuffers();
        update();
    }

protected:
    void parseFaceVertex(const QString& str, const std::vector<QVector3D>& tv, const std::vector<QVector2D>& tvt, const std::vector<QVector3D>& tvn) {
        QStringList p = str.split('/');
        int vIdx = p.size() > 0 && !p[0].isEmpty() ? p[0].toInt() - 1 : -1;
        int vtIdx = p.size() > 1 && !p[1].isEmpty() ? p[1].toInt() - 1 : -1;
        int vnIdx = p.size() > 2 && !p[2].isEmpty() ? p[2].toInt() - 1 : -1;
        
        if (vIdx >= 0 && vIdx < tv.size()) {
            vertices.push_back(tv[vIdx].x()); vertices.push_back(tv[vIdx].y()); vertices.push_back(tv[vIdx].z());
        } else {
            vertices.push_back(0); vertices.push_back(0); vertices.push_back(0);
        }
        
        if (vtIdx >= 0 && vtIdx < tvt.size()) {
            uvs.push_back(tvt[vtIdx].x()); uvs.push_back(tvt[vtIdx].y());
        } else {
            uvs.push_back(0); uvs.push_back(0);
        }
        
        if (vnIdx >= 0 && vnIdx < tvn.size()) {
            normals.push_back(tvn[vnIdx].x()); normals.push_back(tvn[vnIdx].y()); normals.push_back(tvn[vnIdx].z());
        } else {
            normals.push_back(0); normals.push_back(1); normals.push_back(0);
        }
    }

    void centerModel() {
        if (vertices.isEmpty()) return;
        float minX = vertices[0], maxX = minX;
        float minY = vertices[1], maxY = minY;
        float minZ = vertices[2], maxZ = minZ;
        for (int i=0; i<vertices.size(); i+=3) {
            minX = std::min(minX, vertices[i]); maxX = std::max(maxX, vertices[i]);
            minY = std::min(minY, vertices[i+1]); maxY = std::max(maxY, vertices[i+1]);
            minZ = std::min(minZ, vertices[i+2]); maxZ = std::max(maxZ, vertices[i+2]);
        }
        float cx = (minX + maxX)/2.f; float cy = (minY + maxY)/2.f; float cz = (minZ + maxZ)/2.f;
        float maxDim = std::max({maxX - minX, maxY - minY, maxZ - minZ});
        if (maxDim == 0) maxDim = 1.0f;
        for (int i=0; i<vertices.size(); i+=3) {
            vertices[i] = (vertices[i]-cx)/maxDim;
            vertices[i+1] = (vertices[i+1]-cy)/maxDim;
            vertices[i+2] = (vertices[i+2]-cz)/maxDim;
        }
    }

    void setupBuffers() {
        if (vertices.isEmpty()) return;
        QVector<float> bufferData;
        for (int i=0; i<vertices.size()/3; ++i) {
            bufferData.push_back(vertices[i*3]); bufferData.push_back(vertices[i*3+1]); bufferData.push_back(vertices[i*3+2]);
            bufferData.push_back(uvs[i*2]); bufferData.push_back(uvs[i*2+1]);
            bufferData.push_back(normals[i*3]); bufferData.push_back(normals[i*3+1]); bufferData.push_back(normals[i*3+2]);
        }
        if (!vbo.isCreated()) vbo.create();
        vbo.bind();
        vbo.allocate(bufferData.data(), bufferData.size() * sizeof(float));
        vbo.release();
    }

    void updateModelInfo(const QString& baseName) {
        currentModelId = baseName;
        if (baseName.isEmpty()) {
            infoOverlay->hide();
            return;
        }

        static QJsonArray modelsArray;
        static QJsonObject allLevelsObj;
        static bool loaded = false;

        if (!loaded) {
            loaded = true;
            QString appDir = QCoreApplication::applicationDirPath();
            QFile f1(appDir + "/IGIModels.json");
            if (f1.open(QIODevice::ReadOnly)) {
                modelsArray = QJsonDocument::fromJson(f1.readAll()).array();
            }
            QFile f2(appDir + "/IGIModelsAllLevel.json");
            if (f2.open(QIODevice::ReadOnly)) {
                allLevelsObj = QJsonDocument::fromJson(f2.readAll()).object();
            }
        }

        // Find Model Name from IGIModels.json
        QString modelName = "Unknown";
        for (const QJsonValue& val : modelsArray) {
            QJsonObject obj = val.toObject();
            if (obj["ModelId"].toString().toLower() == baseName.toLower()) {
                modelName = obj["ModelName"].toString();
                break;
            }
        }
        currentModelName = modelName;

        QJsonObject firstInstance;
        bool found = false;
        
        for (auto it = allLevelsObj.begin(); it != allLevelsObj.end() && !found; ++it) {
            QJsonObject levelObj = it.value().toObject();
            for (auto catIt = levelObj.begin(); catIt != levelObj.end() && !found; ++catIt) {
                QJsonArray items = catIt.value().toArray();
                for (const QJsonValue& itemVal : items) {
                    QJsonObject item = itemVal.toObject();
                    bool match = false;
                    if (item.contains("Model ID") && item["Model ID"].toString().toLower() == baseName.toLower()) {
                        match = true;
                    } else if (item.contains("Model") && item["Model"].toObject()["ID"].toString().toLower() == baseName.toLower()) {
                        match = true;
                    }

                    if (match) {
                        firstInstance = item;
                        found = true;
                        break;
                    }
                }
            }
        }

        currentJsonInfo = "";
        if (found) {
            currentJsonInfo = formatJson(firstInstance);
        }

        infoOverlay->setPlainText(QString("Model Name: %1\nModel ID: %2\n\n--- Properties ---\n%3")
            .arg(currentModelName).arg(currentModelId).arg(currentJsonInfo));
        infoOverlay->show();
    }

    void initializeGL() override {
        initializeOpenGLFunctions();
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        const char* vsrc =
            "#version 330 core\n"
            "layout(location = 0) in vec3 aPos;\n"
            "layout(location = 1) in vec2 aTex;\n"
            "layout(location = 2) in vec3 aNorm;\n"
            "out vec2 TexCoord;\n"
            "out vec3 FragPos;\n"
            "out vec3 Normal;\n"
            "uniform mat4 model;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "    FragPos = vec3(view * model * vec4(aPos, 1.0));\n"
            "    Normal = mat3(transpose(inverse(view * model))) * aNorm;\n"
            "    TexCoord = aTex;\n"
            "    gl_Position = projection * vec4(FragPos, 1.0);\n"
            "}\n";
            
        const char* fsrc =
            "#version 330 core\n"
            "in vec2 TexCoord;\n"
            "in vec3 FragPos;\n"
            "in vec3 Normal;\n"
            "out vec4 FragColor;\n"
            "uniform sampler2D texture1;\n"
            "uniform bool hasTexture;\n"
            "void main() {\n"
            "    vec3 norm = normalize(Normal);\n"
            "    vec3 viewDir = normalize(-FragPos);\n" // Headlight from camera
            "    float diff = max(dot(norm, viewDir), 0.1);\n"
            "    float ambient = 0.3;\n"
            "    float lighting = min(diff + ambient, 1.0);\n"
            "    vec4 baseColor = hasTexture ? texture(texture1, TexCoord) : vec4(0.8, 0.8, 0.8, 1.0);\n"
            "    FragColor = vec4(lighting * baseColor.rgb, baseColor.a);\n"
            "}\n";

        program.addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);
        program.addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);
        program.link();
    }

    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (vertices.isEmpty()) return;

        program.bind();
        vbo.bind();

        program.enableAttributeArray(0);
        program.setAttributeBuffer(0, GL_FLOAT, 0, 3, 8 * sizeof(float));
        program.enableAttributeArray(1);
        program.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 8 * sizeof(float));
        program.enableAttributeArray(2);
        program.setAttributeBuffer(2, GL_FLOAT, 5 * sizeof(float), 3, 8 * sizeof(float));

        QMatrix4x4 projection, view, model;
        projection.perspective(45.0f, float(width()) / float(height() ? height() : 1), 0.1f, 100.0f);
        view.translate(transX, transY, -zoom);
        model.rotate(rotX, 1.0f, 0.0f, 0.0f);
        model.rotate(rotY, 0.0f, 1.0f, 0.0f);
        model.rotate(rotZ, 0.0f, 0.0f, 1.0f);

        program.setUniformValue("projection", projection);
        program.setUniformValue("view", view);
        program.setUniformValue("model", model);

        if (submeshes.empty()) {
            program.setUniformValue("hasTexture", false);
            glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);
        } else {
            for (const auto& sm : submeshes) {
                if (sm.count == 0) continue;
                if (sm.texture && sm.texture->isCreated()) {
                    sm.texture->bind();
                    program.setUniformValue("hasTexture", true);
                    program.setUniformValue("texture1", 0);
                } else {
                    program.setUniformValue("hasTexture", false);
                }
                glDrawArrays(GL_TRIANGLES, sm.startIndex, sm.count);
            }
        }

        program.disableAttributeArray(0);
        program.disableAttributeArray(1);
        program.disableAttributeArray(2);
        vbo.release();
        program.release();
    }

    void resizeEvent(QResizeEvent *event) override {
        QOpenGLWidget::resizeEvent(event);
        infoOverlay->setGeometry(10, 10, 320, height() - 20);
        coordsOverlay->setGeometry(10, height() - 70, 400, 60);
    }

    float wrapAngle(float angle) {
        angle = fmod(angle, 360.0f);
        if (angle < 0) angle += 360.0f;
        return angle;
    }

    void mousePressEvent(QMouseEvent *event) override { lastPos = event->pos(); }
    void mouseMoveEvent(QMouseEvent *event) override {
        int dx = event->pos().x() - lastPos.x();
        int dy = event->pos().y() - lastPos.y();
        if (event->buttons() & Qt::LeftButton) {
            rotY = wrapAngle(rotY + dx);
            rotX = wrapAngle(rotX + dy);
        } else if (event->buttons() & Qt::RightButton) {
            transX += dx * 0.01f;
            transY -= dy * 0.01f;
        } else if (event->buttons() & Qt::MiddleButton) {
            rotZ = wrapAngle(rotZ + dx);
        }
        update();
        lastPos = event->pos();
        updateCoordsOverlay();
    }
    void wheelEvent(QWheelEvent *event) override {
        zoom -= event->angleDelta().y() / 120.0f * 0.2f;
        if (zoom < 0.1f) zoom = 0.1f;
        update();
        updateCoordsOverlay();
    }

private:
    QVector<float> vertices;
    QVector<float> uvs;
    QVector<float> normals;
    QOpenGLShaderProgram program;
    QOpenGLBuffer vbo;
    std::unique_ptr<QOpenGLTexture> texture;

    QPoint lastPos;
    float rotX = 0, rotY = 0, rotZ = 0;
    float transX = 0, transY = 0;
    float zoom = 3.0f;
};

class MainWindow : public QMainWindow {
public:
    MainWindow() {
        setWindowTitle("IGI Game Asset Converter (Qt Advanced UI)");
        resize(1200, 800);

        fileModel = new QFileSystemModel(this);
        fileModel->setRootPath("");
        proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(fileModel);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        // Standard Menu Bar
        QMenu* fileMenu = menuBar()->addMenu("&File");
        fileMenu->addAction(QIcon::fromTheme("document-open"), "&Open Folder...", this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Workspace Folder");
            if (!dir.isEmpty()) {
                fileModel->setRootPath(dir);
                treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(dir)));
            }
        }, QKeySequence::Open);
        
        fileMenu->addAction(QIcon::fromTheme("document-save"), "&Save", this, [this]() {
            if (viewerEdit->isVisible() && !currentFile.isEmpty() && !viewerEdit->isReadOnly()) {
                QFile f(currentFile);
                if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    f.write(viewerEdit->toPlainText().toUtf8());
                    QMessageBox::information(this, "Saved", "File saved successfully!");
                }
            }
        }, QKeySequence::Save);
        
        fileMenu->addSeparator();
        fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

        QMenu* editMenu = menuBar()->addMenu("&Edit");
        editMenu->addAction("Undo", this, [this](){ viewerEdit->undo(); }, QKeySequence::Undo);
        editMenu->addAction("Redo", this, [this](){ viewerEdit->redo(); }, QKeySequence::Redo);
        editMenu->addSeparator();
        editMenu->addAction("Cut", this, [this](){ viewerEdit->cut(); }, QKeySequence::Cut);
        editMenu->addAction("Copy", this, [this](){ viewerEdit->copy(); }, QKeySequence::Copy);
        editMenu->addAction("Paste", this, [this](){ viewerEdit->paste(); }, QKeySequence::Paste);
        editMenu->addSeparator();
        editMenu->addAction("Find File", this, [this](){
            fileSearchBox->setVisible(!fileSearchBox->isVisible());
            if (fileSearchBox->isVisible()) fileSearchBox->setFocus();
            else { fileSearchBox->clear(); treeView->setFocus(); }
        }, QKeySequence("Ctrl+F"));
        editMenu->addAction("Find in Text", this, [this](){
            textSearchWidget->setVisible(!textSearchWidget->isVisible());
            if (textSearchWidget->isVisible()) textSearchBox->setFocus();
            else textSearchWidget->hide();
        }, QKeySequence("Ctrl+Shift+F"));

        QMenu* viewMenu = menuBar()->addMenu("&View");
        viewMenu->addAction("Auto", [this]() { viewModeCombo->setCurrentIndex(0); });
        viewMenu->addAction("Text View", [this]() { viewModeCombo->setCurrentIndex(1); });
        viewMenu->addAction("Hex View", [this]() { viewModeCombo->setCurrentIndex(2); });
        viewMenu->addAction("Image View", [this]() { viewModeCombo->setCurrentIndex(3); });
        viewMenu->addAction("3D View", [this]() { viewModeCombo->setCurrentIndex(4); });
        viewMenu->addSeparator();
        viewMenu->addAction("Light Theme", [this]() {
            qApp->setPalette(qApp->style()->standardPalette());
        });
        viewMenu->addAction("Dark Theme", [this]() {
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
            qApp->setPalette(darkPalette);
        });

        QMenu* helpMenu = menuBar()->addMenu("&Help");
        helpMenu->addAction("About", this, [this]() {
            QMessageBox::about(this, "About", "IGI Game Asset Converter (Qt)\nVersion 1.2.0\nAdvanced Edition\nWith MEF Native Viewer and full CLI action integration.");
        });

        QToolBar* toolbar = addToolBar("Main Toolbar");
        QAction* openAction = toolbar->addAction("Open Folder");
        connect(openAction, &QAction::triggered, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Workspace Folder");
            if (!dir.isEmpty()) {
                fileModel->setRootPath(dir);
                treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(dir)));
            }
        });

        QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
        setCentralWidget(splitter);

        // Left side: File browser
        QWidget* leftWidget = new QWidget(splitter);
        QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(0,0,0,0);
        fileSearchBox = new QLineEdit();
        fileSearchBox->setPlaceholderText("Search file by name...");
        fileSearchBox->hide();
        leftLayout->addWidget(fileSearchBox);
        proxyModel->setRecursiveFilteringEnabled(true);

        connect(fileSearchBox, &QLineEdit::textChanged, this, [this](const QString& text) {
            proxyModel->setFilterWildcard(text);
            QString currentRoot = fileModel->rootPath();
            if (currentRoot.isEmpty()) currentRoot = QDir::currentPath();
            treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(currentRoot)));
        });
        treeView = new QTreeView(leftWidget);
        treeView->setModel(proxyModel);
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(QDir::currentPath())));
        treeView->setColumnWidth(0, 250);
        treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        leftLayout->addWidget(treeView);

        connect(treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);

        // Right side: Viewer and Controls
        QWidget* rightWidget = new QWidget(splitter);
        QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);

        textSearchWidget = new QWidget();
        QHBoxLayout* textSearchLayout = new QHBoxLayout(textSearchWidget);
        textSearchLayout->setContentsMargins(0,0,0,0);
        textSearchBox = new QLineEdit();
        textSearchBox->setPlaceholderText("Search text inside file...");
        QPushButton* findNextBtn = new QPushButton("Find Next");
        textSearchLayout->addWidget(textSearchBox);
        textSearchLayout->addWidget(findNextBtn);
        textSearchWidget->hide();
        connect(textSearchBox, &QLineEdit::returnPressed, findNextBtn, &QPushButton::click);
        connect(findNextBtn, &QPushButton::clicked, this, [this]() {
            QString query = textSearchBox->text();
            if (!query.isEmpty() && viewerEdit->isVisible()) {
                if (!viewerEdit->find(query)) {
                    viewerEdit->moveCursor(QTextCursor::Start);
                    viewerEdit->find(query);
                }
            }
        });
        rightLayout->addWidget(textSearchWidget);

        QHBoxLayout* viewModeLayout = new QHBoxLayout();
        viewModeLayout->addWidget(new QLabel("View Mode:"));
        viewModeCombo = new QComboBox();
        viewModeCombo->addItems({"Auto", "Text View", "Hex View", "Image View", "3D View"});
        viewModeLayout->addWidget(viewModeCombo);
        viewModeLayout->addStretch();
        rightLayout->addLayout(viewModeLayout);

        viewerEdit = new QTextEdit();
        viewerEdit->setReadOnly(true);
        const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        viewerEdit->setFont(fixedFont);

        imageLabel = new QLabel();
        imageLabel->setAlignment(Qt::AlignCenter);

        modelViewer = new ModelViewer();

        rightLayout->addWidget(viewerEdit, 3);
        rightLayout->addWidget(imageLabel, 3);
        rightLayout->addWidget(modelViewer, 3);

        // Debug Output
        QGroupBox* debugGroup = new QGroupBox("Conversion Debug Output");
        QVBoxLayout* debugLayout = new QVBoxLayout(debugGroup);
        consoleEdit = new QTextEdit();
        consoleEdit->setReadOnly(true);
        consoleEdit->setFont(fixedFont);
        debugLayout->addWidget(consoleEdit);

        rightLayout->addWidget(debugGroup, 1);

        splitter->addWidget(leftWidget);
        splitter->addWidget(rightWidget);
        splitter->setSizes({300, 900});

        connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex& current) {
            QModelIndex srcIndex = proxyModel->mapToSource(current);
            if (!fileModel->isDir(srcIndex)) {
                loadFile(fileModel->filePath(srcIndex));
            }
        });

        connect(viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!currentFile.isEmpty()) {
                loadFile(currentFile, index);
            }
        });
        
        hideAllViewers();
    }

private:
    QFileSystemModel* fileModel;
    QSortFilterProxyModel* proxyModel;
    QTreeView* treeView;
    QLineEdit* fileSearchBox;
    QWidget* textSearchWidget;
    QLineEdit* textSearchBox;
    QComboBox* viewModeCombo;
    QTextEdit* viewerEdit;
    QLabel* imageLabel;
    ModelViewer* modelViewer;
    QTextEdit* consoleEdit;
    QString currentFile;
    QString currentExt;
    QString clipboardFilePath;
    bool clipboardIsCut = false;

    void hideAllViewers() {
        viewerEdit->hide();
        imageLabel->hide();
        modelViewer->hide();
    }

    void showContextMenu(const QPoint& pos) {
        QModelIndex index = treeView->indexAt(pos);
        QModelIndex srcIndex = proxyModel->mapToSource(index);
        if (!srcIndex.isValid() || fileModel->isDir(srcIndex)) return;

        QString path = fileModel->filePath(srcIndex);
        QString ext = QFileInfo(path).suffix().toLower();

        QMenu menu;
        menu.addAction("Open in Native App", [path]() { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); });

        QMenu* viewMenu = menu.addMenu("View As");
        viewMenu->addAction("Text View", [this, path]() { loadFile(path, 1); });
        viewMenu->addAction("Hex View",  [this, path]() { loadFile(path, 2); });
        viewMenu->addAction("Image View",[this, path]() { loadFile(path, 3); });
        viewMenu->addAction("3D View",   [this, path]() { loadFile(path, 4); });
        menu.addSeparator();

        QMenu* editFileMenu = menu.addMenu("File Operations");
        editFileMenu->addAction("Rename...", [this, path]() {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename File", "New name:", QLineEdit::Normal, QFileInfo(path).fileName(), &ok);
            if (ok && !newName.isEmpty()) {
                QFile::rename(path, QFileInfo(path).absolutePath() + "/" + newName);
            }
        });
        editFileMenu->addAction("Delete", [this, path]() {
            if (QMessageBox::question(this, "Delete", "Are you sure you want to delete " + QFileInfo(path).fileName() + "?") == QMessageBox::Yes) {
                QFile(path).remove();
            }
        });
        editFileMenu->addAction("Cut", [this, path]() { clipboardFilePath = path; clipboardIsCut = true; });
        editFileMenu->addAction("Copy", [this, path]() { clipboardFilePath = path; clipboardIsCut = false; });
        if (!clipboardFilePath.isEmpty()) {
            editFileMenu->addAction("Paste Here", [this, path]() {
                QString dest = QFileInfo(path).absolutePath() + "/" + QFileInfo(clipboardFilePath).fileName();
                QFile::copy(clipboardFilePath, dest);
                if (clipboardIsCut) {
                    QFile(clipboardFilePath).remove();
                    clipboardFilePath = "";
                }
            });
        }
        menu.addSeparator();

        if (ext == "tex" || ext == "spr" || ext == "pic") {
            menu.addAction("Convert to PNG", [this, path]() { loadFile(path); executeCommand("tex to-png"); });
            menu.addAction("Convert to TGA", [this, path]() { loadFile(path); executeCommand("tex to-tga"); });
            menu.addAction("Info",           [this, path]() { loadFile(path); executeCommand("tex info"); });
            menu.addAction("Decode Batch",   [this, path]() { loadFile(path); executeCommand("tex decode-batch"); });
        } else if (ext == "qsc") {
            menu.addAction("Compile to QVM", [this, path]() { loadFile(path); executeCommand("qsc compile"); });
            menu.addAction("Validate",       [this, path]() { loadFile(path); executeCommand("qsc validate"); });
        } else if (ext == "qvm") {
            menu.addAction("Decompile to QSC", [this, path]() { loadFile(path); executeCommand("qvm decompile"); });
            menu.addAction("Disasm",           [this, path]() { loadFile(path); executeCommand("qvm disasm"); });
            menu.addAction("Info",             [this, path]() { loadFile(path); executeCommand("qvm info"); });
        } else if (ext == "mef") {
            menu.addAction("Export to OBJ",    [this, path]() { loadFile(path); executeCommand("mef export"); });
            menu.addAction("Bundle OBJ+TEX",   [this, path]() { loadFile(path); executeCommand("mef bundle"); });
            menu.addAction("Dump to TXT",      [this, path]() { loadFile(path); executeCommand("mef dump"); });
            menu.addAction("Info",             [this, path]() { loadFile(path); executeCommand("mef info"); });
        } else if (ext == "res") {
            menu.addAction("Extract", [this, path]() { loadFile(path); executeCommand("res extract"); });
            menu.addAction("List",    [this, path]() { loadFile(path); executeCommand("res list"); });
            menu.addAction("Unpack",  [this, path]() { loadFile(path); executeCommand("res unpack"); });
        } else if (ext == "dat") {
            menu.addAction("Info", [this, path]() { loadFile(path); executeCommand("dat info"); });
        } else if (ext == "fnt") {
            menu.addAction("Export PNG", [this, path]() { loadFile(path); executeCommand("fnt export"); });
            menu.addAction("Info",       [this, path]() { loadFile(path); executeCommand("fnt info"); });
        }
        menu.exec(treeView->mapToGlobal(pos));
    }

    void loadFile(const QString& path, int overrideViewMode = 0) {
        currentFile = path;
        QFileInfo info(path);
        currentExt = info.suffix().toLower();

        hideAllViewers();

        int mode = overrideViewMode;
        if (mode == 0) {
            if (currentExt == "png" || currentExt == "jpg" || currentExt == "jpeg" || currentExt == "bmp" || currentExt == "tex" || currentExt == "spr" || currentExt == "pic" || currentExt == "tga") {
                mode = 3; // Image
            } else if (currentExt == "mef" || currentExt == "obj") {
                mode = 4; // 3D
            } else if (currentExt == "qsc" || currentExt == "txt" || currentExt == "json" || currentExt == "md" || currentExt == "h" || currentExt == "cpp" || currentExt == "dat") {
                mode = 1; // Text
            } else {
                mode = 2; // Hex
            }
            viewModeCombo->blockSignals(true);
            viewModeCombo->setCurrentIndex(mode);
            viewModeCombo->blockSignals(false);
        }

        if (mode == 1) { // Text
            QFile file(path);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                viewerEdit->setPlainText(QString::fromUtf8(file.readAll()));
            }
            viewerEdit->setReadOnly(false);
            viewerEdit->show();
        } else if (mode == 2) { // Hex
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
            viewerEdit->setReadOnly(true);
            viewerEdit->show();
        } else if (mode == 3) { // Image
            if (currentExt == "tex" || currentExt == "spr" || currentExt == "pic") {
                TEXFile tex = TEX_Parse(path.toStdString());
                if (tex.valid && !tex.images.empty()) {
                    const auto& img = tex.images[0];
                    QImage qimg(img.width, img.height, QImage::Format_ARGB32);
                    if (img.mode == 3 || img.mode == 67) {
                        for (uint32_t y = 0; y < img.height; ++y) {
                            for (uint32_t x = 0; x < img.width; ++x) {
                                size_t i = (y * img.width + x) * 4;
                                qimg.setPixelColor(x, y, QColor(img.pixels[i+2], img.pixels[i+1], img.pixels[i], img.pixels[i+3]));
                            }
                        }
                    } else if (img.mode == 2) {
                        for (uint32_t y = 0; y < img.height; ++y) {
                            for (uint32_t x = 0; x < img.width; ++x) {
                                size_t i = (y * img.width + x) * 2;
                                uint16_t c = (img.pixels[i+1] << 8) | img.pixels[i];
                                int r = (c >> 10) & 0x1F; r = (r << 3) | (r >> 2);
                                int g = (c >> 5) & 0x1F;  g = (g << 3) | (g >> 2);
                                int b = c & 0x1F;         b = (b << 3) | (b >> 2);
                                qimg.setPixelColor(x, y, QColor(r, g, b, 255));
                            }
                        }
                    }
                    imageLabel->setPixmap(QPixmap::fromImage(qimg).scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            } else {
                QImage img = loadImageSafe(path);
                if (!img.isNull()) {
                    imageLabel->setPixmap(QPixmap::fromImage(img).scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                } else {
                    imageLabel->setText("Failed to load image");
                }
            }
            imageLabel->show();
        } else if (mode == 4) { // 3D
            modelViewer->loadModel(path);
            modelViewer->show();
        }
    }

    void executeCommand(const QString& cmd) {
        if (currentFile.isEmpty()) return;

        QFileInfo info(currentFile);
        QString outDir = info.absolutePath();
        QString baseName = info.completeBaseName();

        QStringList args = cmd.split(" ", Qt::SkipEmptyParts);

        if (cmd == "tex decode-batch") {
            args.clear();
            args << "tex" << "decode" << info.absoluteDir().absolutePath() << "-o" << outDir << "--batch";
        } else if (cmd == "mef bundle") {
            QString outBundle = outDir + "/" + baseName + "_bundle";
            QString datFile = QFileDialog::getOpenFileName(this, "Select DAT File for Bundle", outDir, "DAT Files (*.dat)");
            if (datFile.isEmpty()) return;
            QString texDir = QFileDialog::getExistingDirectory(this, "Select Textures Directory for Bundle", outDir);
            if (texDir.isEmpty()) return;
            
            args << currentFile << "-o" << outBundle << "--dat" << datFile << "--texdir" << texDir;
        } else if (cmd == "res unpack") {
            args << currentFile << (outDir + "/" + baseName + "_unpacked");
        } else {
            args << currentFile;
            if (cmd == "qsc compile") args << "-o" << (outDir + "/" + baseName + ".qvm");
            else if (cmd == "qvm decompile") args << "-o" << (outDir + "/" + baseName + ".qsc");
            else if (cmd == "mef export") args << "-o" << (outDir + "/" + baseName + ".obj");
            else if (cmd == "mef dump") args << "-o" << (outDir + "/" + baseName + "_dump.txt");
            else if (cmd == "res extract") args << "-o" << outDir;
            else if (cmd == "fnt export") args << "-o" << (outDir + "/" + baseName + ".png");
            else if (cmd == "qvm disasm") args << "-o" << (outDir + "/" + baseName + "_disasm.txt");
        }

        if (args.isEmpty()) return;

        QProcess process;
        process.setProgram("igi1conv");
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
