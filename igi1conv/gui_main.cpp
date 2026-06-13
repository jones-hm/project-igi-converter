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
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QLineEdit>
#include <QGroupBox>
#include <QFontDatabase>
#include <QComboBox>
#include <QToolBar>
#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QShortcut>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QCloseEvent>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QToolTip>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOpenGLFunctions_3_2_Core>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLVersionFunctionsFactory>
#endif
#include <QJsonArray>
#include <QJsonValue>
#include <QCoreApplication>
#include <QSortFilterProxyModel>
#include <QInputDialog>
#include <QClipboard>
#include <QStyleFactory>
#include <QTextDocument>
#include <QPainter>
#include <QScrollArea>
#include <QSpinBox>
#include <QColorDialog>
#include <QProgressDialog>

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
    QLabel* statsOverlay;
    QString currentModelName;
    QString currentModelId;
    QString currentJsonInfo;
    std::vector<SubMesh> submeshes;
    std::map<QString, std::shared_ptr<QOpenGLTexture>> textureCache;
    bool showWireframe = false;
    bool showGrid = true;
    QString currentModelPath;
    QString cacheDir;

    ModelViewer(QWidget* parent = nullptr) : QOpenGLWidget(parent) {
        setFocusPolicy(Qt::StrongFocus);
        
        infoOverlay = new QTextEdit(this);
        infoOverlay->setReadOnly(true);
        infoOverlay->setStyleSheet("background-color: rgba(20, 20, 20, 180); color: #FFF; font-family: Consolas; font-size: 11px; border: none; padding: 5px;");
        infoOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
        infoOverlay->hide();

        coordsOverlay = new QLabel(this);
        coordsOverlay->setStyleSheet("background-color: rgba(0,0,0,180); color:#AAFFCC; padding:4px 8px; font-family:'Consolas',monospace; font-size:10px; border:1px solid #444;");
        coordsOverlay->hide();
        
        statsOverlay = new QLabel(this);
        statsOverlay->setStyleSheet("background-color: rgba(0,0,0,160); color:#FFCC66; padding:4px 8px; font-family:'Consolas',monospace; font-size:10px; border:1px solid #444; border-radius:3px;");
        statsOverlay->hide();
        
        // Keyboard shortcuts for 3D viewer
        auto addKey = [this](Qt::Key k, auto fn) {
            QShortcut* sc = new QShortcut(k, this);
            sc->setContext(Qt::WidgetShortcut);
            connect(sc, &QShortcut::activated, fn);
        };
        addKey(Qt::Key_W, [this]() { showWireframe = !showWireframe; update(); });
        addKey(Qt::Key_G, [this]() { showGrid = !showGrid; update(); });
        addKey(Qt::Key_R, [this]() { rotX=rotY=rotZ=0; transX=transY=0; zoom=3.0f; update(); updateCoordsOverlay(); });
        addKey(Qt::Key_I, [this]() { infoOverlay->setVisible(!infoOverlay->isVisible()); });
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
        coordsOverlay->setText(QString(
            "Model: %1 (%2)\n"
            "Cam: X%3 Y%4 Z%5 | Rot: X%6 Y%7 Z%8\n"
            "[W]ire [G]rid [R]eset [I]nfo | LMB=Rotate RMB=Pan Wheel=Zoom")
            .arg(currentModelName).arg(currentModelId)
            .arg(transX, 0,'f',2).arg(transY, 0,'f',2).arg(-zoom, 0,'f',2)
            .arg(rotX, 0,'f',0).arg(rotY, 0,'f',0).arg(rotZ, 0,'f',0));
    }

    void loadMefRecursive(const QString& path, const QMatrix4x4& transform, int depth) {
        if (depth > 20) return;
        
        QFileInfo info(path);
        if (!info.exists()) return;
        
        ParsedGeometry geo;
        try {
            geo = ParseMefFile(path.toStdString());
        } catch(...) { return; }
        
        std::vector<std::shared_ptr<QOpenGLTexture>> loadedMaterials;
        if (!geo.pmtlTextures.empty()) {
            for (const auto& tex : geo.pmtlTextures) {
                loadedMaterials.push_back(loadCachedTexture(QString::fromStdString(tex), info));
            }
        } else {
            int maxSlot = -1;
            for (const auto& b : geo.renderBlocks) if (b.materialSlot > maxSlot) maxSlot = b.materialSlot;
            for (int i = 0; i <= maxSlot; ++i) {
                QString matName = QString("mat_%1.tga").arg(i);
                auto tex = loadCachedTexture(matName, info);
                if (!tex) {
                    QString tempDir = cacheDir + "/bundle/" + info.completeBaseName() + "/";
                    QFileInfo tempInfo(tempDir + matName);
                    tex = loadCachedTexture(tempInfo.fileName(), tempInfo);
                }
                loadedMaterials.push_back(tex);
            }
        }

        bool hasRawPos = false;
        for (const auto& v : geo.vertices) {
            if (v.rawPos.x != 0 || v.rawPos.y != 0 || v.rawPos.z != 0) { hasRawPos = true; break; }
        }
        bool isBoneModel = (geo.renderLayout.find("type1") != std::string::npos);

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
                            // Bone models (type1): use v.pos which has bone world offsets baked in.
                            // Other XTRV models: use rawPos (raw game units). Collision fallback: v.pos * 40.96f.
                            glm::vec3 p = (isBoneModel || !hasRawPos) ? (v.pos * 40.96f) : v.rawPos;
                            QVector4D tp = transform * QVector4D(p.x, p.y, p.z, 1.0f);
                            vertices.push_back(tp.x() / 40.96f); vertices.push_back(tp.y() / 40.96f); vertices.push_back(tp.z() / 40.96f);

                            QVector4D tn = transform * QVector4D(v.normal.x, v.normal.y, v.normal.z, 0.0f);
                            QVector3D norm(tn.x(), tn.y(), tn.z());
                            norm.normalize();
                            normals.push_back(norm.x()); normals.push_back(norm.y()); normals.push_back(norm.z());

                            uvs.push_back(v.uv.x); uvs.push_back(isBoneModel ? v.uv.y : (1.0f - v.uv.y));
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

        for (const auto& atta : geo.mefAttachments) {
            QMatrix4x4 aMat(
                atta.r00, atta.r01, atta.r02, atta.px,
                atta.r03, atta.r04, atta.r05, atta.py,
                atta.r06, atta.r07, atta.r08, atta.pz,
                0.0f,     0.0f,     0.0f,     1.0f
            );
            QMatrix4x4 childTransform = transform * aMat;
            
            QString childName = QString::fromLocal8Bit(atta.name, strnlen(atta.name, 16));
            QDir dir = info.absoluteDir();
            QString path1 = dir.filePath(childName + ".mef");
            QString path2 = dir.filePath(childName + ".MEF");
            QString path3 = dir.filePath(childName + ".mex");
            QString path4 = dir.filePath(childName + ".MEX");
            
            if (QFileInfo::exists(path1)) loadMefRecursive(path1, childTransform, depth + 1);
            else if (QFileInfo::exists(path2)) loadMefRecursive(path2, childTransform, depth + 1);
            else if (QFileInfo::exists(path3)) loadMefRecursive(path3, childTransform, depth + 1);
            else if (QFileInfo::exists(path4)) loadMefRecursive(path4, childTransform, depth + 1);
        }
    }

    void loadModel(const QString& path) {
        makeCurrent();
        vertices.clear(); uvs.clear(); normals.clear();
        submeshes.clear();
        textureCache.clear();
        currentModelPath = path;
        
        QFileInfo info(path);
        QString ext = info.suffix().toLower();
        
        updateModelInfo(info.completeBaseName());
        
        if (ext == "mef" || ext == "mex") {
            QMatrix4x4 identity;
            identity.setToIdentity();
            loadMefRecursive(path, identity, 0);
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

        infoOverlay->setPlainText(QString(
            "=== %1 (%2) ===\n"
            "%3\n"
            "--- DAT Properties ---\n%4")
            .arg(currentModelName).arg(currentModelId)
            .arg(currentModelPath.isEmpty() ? "" : "Path: " + currentModelPath)
            .arg(currentJsonInfo));
        infoOverlay->show();
        
        // Update stats overlay
        statsOverlay->show();
        statsOverlay->setText(QString("V:%1 T:%2 M:%3")
            .arg(vertices.size()/3)
            .arg(vertices.size()/9)
            .arg(submeshes.size()));
    }

    void initializeGL() override {
        initializeOpenGLFunctions();
        // clear color set per-frame in paintGL to match current theme
        glEnable(GL_DEPTH_TEST);

        QVector<float> gridLines;
        for (int i = -10; i <= 10; ++i) {
            gridLines << i << 0.0f << -10.0f << 0.0f<<0.0f << 0.0f<<1.0f<<0.0f;
            gridLines << i << 0.0f << 10.0f  << 0.0f<<0.0f << 0.0f<<1.0f<<0.0f;
            gridLines << -10.0f << 0.0f << i << 0.0f<<0.0f << 0.0f<<1.0f<<0.0f;
            gridLines << 10.0f  << 0.0f << i << 0.0f<<0.0f << 0.0f<<1.0f<<0.0f;
        }
        gridVertexCount = gridLines.size() / 8;
        gridVbo.create();
        gridVbo.bind();
        gridVbo.allocate(gridLines.data(), gridLines.size() * sizeof(float));
        gridVbo.release();

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
            "    vec3 viewDir = vec3(0.0, 0.0, 1.0);\n" // Directional headlight
            "    float diff = max(abs(dot(norm, viewDir)), 0.1);\n" // Two-sided lighting
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
        QColor bg = QApplication::palette().color(QPalette::Window);
        glClearColor(bg.redF(), bg.greenF(), bg.blueF(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (vertices.isEmpty()) return;
        
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto* gl = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_2_Core>(QOpenGLContext::currentContext());
#else
        auto* gl = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
#endif
        if (gl) {
            if (showWireframe) gl->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else gl->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        program.bind();
        QMatrix4x4 projection, view, model;
        projection.perspective(45.0f, float(width()) / float(height() ? height() : 1), 0.1f, 100.0f);
        view.translate(transX, transY, -zoom);
        
        if (showGrid && gridVertexCount > 0) {
            gridVbo.bind();
            program.enableAttributeArray(0);
            program.setAttributeBuffer(0, GL_FLOAT, 0, 3, 8 * sizeof(float));
            program.enableAttributeArray(1);
            program.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 8 * sizeof(float));
            program.enableAttributeArray(2);
            program.setAttributeBuffer(2, GL_FLOAT, 5 * sizeof(float), 3, 8 * sizeof(float));
            program.setUniformValue("projection", projection);
            program.setUniformValue("view", view);
            QMatrix4x4 gridModel;
            program.setUniformValue("model", gridModel);
            program.setUniformValue("hasTexture", false);
            glDrawArrays(GL_LINES, 0, gridVertexCount);
            gridVbo.release();
        }

        vbo.bind();
        program.enableAttributeArray(0);
        program.setAttributeBuffer(0, GL_FLOAT, 0, 3, 8 * sizeof(float));
        program.enableAttributeArray(1);
        program.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 2, 8 * sizeof(float));
        program.enableAttributeArray(2);
        program.setAttributeBuffer(2, GL_FLOAT, 5 * sizeof(float), 3, 8 * sizeof(float));

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
        
        if (gl && showWireframe) {
            gl->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }

    void resizeEvent(QResizeEvent *event) override {
        QOpenGLWidget::resizeEvent(event);
        infoOverlay->setGeometry(10, 10, 300, height() - 90);
        coordsOverlay->setGeometry(0, height() - 72, width(), 72);
        statsOverlay->setGeometry(width() - 250, 10, 240, 60);
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
    QOpenGLBuffer gridVbo;
    int gridVertexCount = 0;
    std::unique_ptr<QOpenGLTexture> texture;

    QPoint lastPos;
    float rotX = 0, rotY = 0, rotZ = 0;
    float transX = 0, transY = 0;
    float zoom = 3.0f;
};

class ImageEditor : public QWidget {
public:
    ImageEditor(QWidget* parent = nullptr) : QWidget(parent) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        // ── Toolbar row ──────────────────────────────────────
        QWidget* toolsWidget = new QWidget();
        toolsWidget->setStyleSheet("background:#2a2a2a; border-bottom:1px solid #444;");
        QHBoxLayout* tools = new QHBoxLayout(toolsWidget);
        tools->setContentsMargins(6, 4, 6, 4);
        tools->setSpacing(4);
        
        auto mkBtn = [](const QString& text, const QString& tip = "") {
            QPushButton* b = new QPushButton(text);
            b->setToolTip(tip);
            b->setFixedHeight(26);
            b->setStyleSheet("QPushButton{background:#3a3a3a;color:#ddd;border:1px solid #555;border-radius:3px;padding:2px 8px;}"
                             "QPushButton:hover{background:#4a4a4a;}"
                             "QPushButton:checked{background:#0066aa;border-color:#0088cc;}");
            b->setCheckable(true);
            return b;
        };
        auto mkBtnNorm = [](const QString& text, const QString& tip = "") {
            QPushButton* b = new QPushButton(text);
            b->setToolTip(tip);
            b->setFixedHeight(26);
            b->setStyleSheet("QPushButton{background:#3a3a3a;color:#ddd;border:1px solid #555;border-radius:3px;padding:2px 8px;}"
                             "QPushButton:hover{background:#4a4a4a;}");
            return b;
        };
        
        btnDraw    = mkBtn("✏ Draw",   "Toggle pencil tool [D]");
        btnEraser  = mkBtn("⌫ Erase",  "Toggle eraser tool [E]");
        QPushButton* btnColor  = mkBtnNorm("🎨 Color",  "Pick pen color");
        QPushButton* btnFit    = mkBtnNorm("⊡ Fit",     "Fit to window");
        QPushButton* btnZoomIn = mkBtnNorm("+ Zoom",    "Zoom in");
        QPushButton* btnZoomOut= mkBtnNorm("- Zoom",    "Zoom out");
        QPushButton* btnClear  = mkBtnNorm("⟳ Reset",  "Reset to original");
        QPushButton* btnSave   = mkBtnNorm("💾 Save",  "Save (Ctrl+S)");
        
        // Pen size spinner
        QLabel* lblSize = new QLabel("Size:");
        lblSize->setStyleSheet("color:#aaa; font-size:11px;");
        QSpinBox* penSizeSpin = new QSpinBox();
        penSizeSpin->setRange(1, 40);
        penSizeSpin->setValue(3);
        penSizeSpin->setFixedWidth(50);
        penSizeSpin->setFixedHeight(24);
        penSizeSpin->setStyleSheet("background:#333;color:#ddd;border:1px solid #555;border-radius:2px;");
        
        // Info label
        infoLabel = new QLabel("No image");
        infoLabel->setStyleSheet("color:#888;font-size:10px;font-family:Consolas;");
        
        tools->addWidget(btnDraw);
        tools->addWidget(btnEraser);
        tools->addWidget(lblSize);
        tools->addWidget(penSizeSpin);
        tools->addWidget(btnColor);
        tools->addSpacing(10);
        tools->addWidget(btnFit);
        tools->addWidget(btnZoomIn);
        tools->addWidget(btnZoomOut);
        tools->addSpacing(10);
        tools->addWidget(btnClear);
        tools->addWidget(btnSave);
        tools->addStretch();
        tools->addWidget(infoLabel);
        layout->addWidget(toolsWidget);
        
        // ── Scroll area for zoomable canvas ──────────────────
        scrollArea = new QScrollArea();
        scrollArea->setAlignment(Qt::AlignCenter);
        scrollArea->setStyleSheet("background:#1a1a1a; border:none;");
        imageLabel = new QLabel();
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        scrollArea->setWidget(imageLabel);
        scrollArea->setWidgetResizable(false);
        layout->addWidget(scrollArea, 1);
        
        // ── Connect buttons ───────────────────────────────────
        connect(btnDraw, &QPushButton::clicked, this, [this]() {
            if (btnDraw->isChecked()) { isDrawing = true; isErasing = false; btnEraser->setChecked(false); }
            else { isDrawing = false; }
        });
        connect(btnEraser, &QPushButton::clicked, this, [this]() {
            if (btnEraser->isChecked()) { isErasing = true; isDrawing = false; btnDraw->setChecked(false); }
            else { isErasing = false; }
        });
        connect(btnColor, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(penColor, this, "Pick Pen Color");
            if (c.isValid()) penColor = c;
        });
        connect(btnFit, &QPushButton::clicked, this, [this]() {
            if (!currentImage.isNull()) {
                zoomFactor = std::min(
                    (double)scrollArea->width()  / currentImage.width(),
                    (double)scrollArea->height() / currentImage.height());
                updateDisplay();
            }
        });
        connect(btnZoomIn,  &QPushButton::clicked, this, [this]() { zoomFactor = qMin(zoomFactor * 1.25, 8.0); updateDisplay(); });
        connect(btnZoomOut, &QPushButton::clicked, this, [this]() { zoomFactor = qMax(zoomFactor / 1.25, 0.1); updateDisplay(); });
        connect(penSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { penSize = v; });
        connect(btnClear, &QPushButton::clicked, this, [this]() { currentImage = originalImage; updateDisplay(); });
        connect(btnSave, &QPushButton::clicked, this, [this]() {
            if (!currentPath.isEmpty() && !currentImage.isNull()) {
                bool isGameFormat = currentPath.endsWith(".tex", Qt::CaseInsensitive)
                                 || currentPath.endsWith(".spr", Qt::CaseInsensitive)
                                 || currentPath.endsWith(".pic", Qt::CaseInsensitive);
                if (isGameFormat) {
                    if (saveAsTex(currentImage, currentPath))
                        QMessageBox::information(this, "Saved", "Saved to game format:\n" + currentPath);
                    else
                        QMessageBox::critical(this, "Error", "Failed to save:\n" + currentPath);
                } else {
                    if (currentImage.save(currentPath))
                        QMessageBox::information(this, "Saved", "Saved:\n" + currentPath);
                    else
                        QMessageBox::critical(this, "Error", "Failed to save:\n" + currentPath);
                }
            }
        });
        
        imageLabel->installEventFilter(this);
        scrollArea->viewport()->installEventFilter(this);
    }

    
    bool saveAsTex(const QImage& img, const QString& outPath) {
        // Write a valid LOOP v11 TEX file: 32-byte header + ARGB8888 pixels
        // Header layout: sig(4s) version(I) mode(I) multi(I) _0(I) _1(H) _2(H) _3(H) width(H) height(H) depth(H)
        QFile f(outPath);
        if (!f.open(QIODevice::WriteOnly)) return false;
        
        QImage texImg = img.convertToFormat(QImage::Format_ARGB32);
        uint16_t w = (uint16_t)texImg.width();
        uint16_t h = (uint16_t)texImg.height();
        
        bool isArgb = outPath.toLower().contains("argb8888");
        uint32_t mode = isArgb ? 3 : 2; // 3=ARGB8888, 2=RGB565
        
        uint8_t hdr[32] = {0};
        hdr[0] = 'L'; hdr[1] = 'O'; hdr[2] = 'O'; hdr[3] = 'P';
        uint32_t version = 11;
        std::memcpy(hdr + 4, &version, 4);
        std::memcpy(hdr + 8, &mode, 4);
        uint32_t multi = 0;
        std::memcpy(hdr + 12, &multi, 4);
        // _0 at [16] = 0 (already zeroed)
        uint16_t _1 = 5;           // constant in all original game TEX files
        uint16_t _2 = w;           // redundant width copy
        uint16_t _3 = h;           // redundant height copy
        uint16_t depthBytes = (mode == 3) ? 4 : 2; // bytes per pixel: 4=ARGB8888, 2=ARGB1555
        std::memcpy(hdr + 20, &_1, 2);
        std::memcpy(hdr + 22, &_2, 2);
        std::memcpy(hdr + 24, &_3, 2);
        std::memcpy(hdr + 26, &w, 2);
        std::memcpy(hdr + 28, &h, 2);
        std::memcpy(hdr + 30, &depthBytes, 2);
        
        f.write((const char*)hdr, 32);
        
        QByteArray pixels;
        if (mode == 3) {
            pixels.resize(w * h * 4);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    QRgb color = texImg.pixel(x, y);
                    int i = (y * w + x) * 4;
                    pixels[i]   = (char)qBlue(color);
                    pixels[i+1] = (char)qGreen(color);
                    pixels[i+2] = (char)qRed(color);
                    pixels[i+3] = (char)qAlpha(color);
                }
            }
        } else {
            // ARGB1555: bit15=A(opaque=1), bits14-10=R, bits9-5=G, bits4-0=B
            pixels.resize(w * h * 2);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    QRgb color = texImg.pixel(x, y);
                    int r = qRed(color)   >> 3; // 5 bits
                    int g = qGreen(color) >> 3; // 5 bits
                    int b = qBlue(color)  >> 3; // 5 bits
                    uint16_t argb1555 = (uint16_t)(0x8000u | (r << 10) | (g << 5) | b);
                    int i = (y * w + x) * 2;
                    std::memcpy(pixels.data() + i, &argb1555, 2);
                }
            }
        }
        f.write(pixels);
        f.close();
        return true;
    }
    
    void loadImage(const QString& path, const QImage& img) {
        currentPath = path;
        originalImage = img;
        currentImage = img;
        zoomFactor = 1.0;
        updateDisplay();
        if (infoLabel) {
            infoLabel->setText(QString("%1x%2 | %3")
                .arg(img.width()).arg(img.height())
                .arg(QFileInfo(path).fileName()));
        }
    }
    
    void clear() {
        imageLabel->clear();
        currentPath.clear();
        currentImage = QImage();
        if (infoLabel) infoLabel->setText("No image");
    }

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent* we = static_cast<QWheelEvent*>(event);
            if (we->angleDelta().y() > 0) {
                zoomFactor = qMin(zoomFactor * 1.25, 8.0);
            } else {
                zoomFactor = qMax(zoomFactor / 1.25, 0.1);
            }
            updateDisplay();
            return true;
        }

        // Drawing on the image label
        if (obj == imageLabel && (isDrawing || isErasing) && !currentImage.isNull()) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                pushUndo();
                lastPoint = getMappedPoint(me->pos());
                drawingNow = true;
                return true;
            } else if (event->type() == QEvent::MouseMove) {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                if (drawingNow) {
                    QPoint pt = getMappedPoint(me->pos());
                    QPainter painter(&currentImage);
                    painter.setRenderHint(QPainter::Antialiasing);
                    if (isErasing) {
                        // Restore original pixels along the erased line
                        int r = penSize;
                        for (int dy = -r; dy <= r; dy++) {
                            for (int dx = -r; dx <= r; dx++) {
                                if (dx*dx + dy*dy <= r*r) {
                                    // Sample along the line from lastPoint to pt
                                    int steps = std::max(std::abs(pt.x()-lastPoint.x()), std::abs(pt.y()-lastPoint.y())) + 1;
                                    for (int s = 0; s <= steps; s++) {
                                        int cx = lastPoint.x() + (pt.x()-lastPoint.x())*s/steps + dx;
                                        int cy = lastPoint.y() + (pt.y()-lastPoint.y())*s/steps + dy;
                                        if (cx >= 0 && cy >= 0 && cx < currentImage.width() && cy < currentImage.height())
                                            currentImage.setPixel(cx, cy, originalImage.pixel(cx, cy));
                                    }
                                }
                            }
                        }
                        lastPoint = pt;
                        updateDisplay();
                        return true;
                    } else {
                        painter.setPen(QPen(penColor, penSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                    }
                    painter.drawLine(lastPoint, pt);
                    lastPoint = pt;
                    updateDisplay();
                }
                return true;
            } else if (event->type() == QEvent::MouseButtonRelease) {
                drawingNow = false;
                return true;
            }
        }
        // Zoom with Ctrl+Wheel on scroll viewport
        if (obj == scrollArea->viewport() && event->type() == QEvent::Wheel) {
            QWheelEvent* we = static_cast<QWheelEvent*>(event);
            if (we->modifiers() & Qt::ControlModifier) {
                double factor = (we->angleDelta().y() > 0) ? 1.15 : (1.0/1.15);
                zoomFactor = qBound(0.1, zoomFactor * factor, 8.0);
                updateDisplay();
                return true;
            }
        }
        return QWidget::eventFilter(obj, event);
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        updateDisplay();
        QWidget::resizeEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (event->matches(QKeySequence::Undo)) {
            imgUndo();
        } else if (event->matches(QKeySequence::Redo)) {
            imgRedo();
        } else if (event->key() == Qt::Key_Delete) {
            pushUndo();
            currentImage.fill(Qt::transparent);
            updateDisplay();
        } else if (event->matches(QKeySequence::Copy)) {
            if (!currentImage.isNull())
                QApplication::clipboard()->setImage(currentImage);
        } else if (event->matches(QKeySequence::Paste)) {
            QImage img = QApplication::clipboard()->image();
            if (!img.isNull()) {
                pushUndo();
                currentImage = img;
                updateDisplay();
            }
        } else {
            QWidget::keyPressEvent(event);
        }
    }

private:
    void updateDisplay() {
        if (currentImage.isNull()) return;
        int dw = (int)(currentImage.width()  * zoomFactor);
        int dh = (int)(currentImage.height() * zoomFactor);
        imageLabel->setFixedSize(dw, dh);
        imageLabel->setPixmap(QPixmap::fromImage(currentImage).scaled(dw, dh, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QPoint getMappedPoint(const QPoint& p) {
        if (currentImage.isNull()) return p;
        int dw = (int)(currentImage.width()  * zoomFactor);
        int dh = (int)(currentImage.height() * zoomFactor);
        int x = (int)((double)p.x() / dw * currentImage.width());
        int y = (int)((double)p.y() / dh * currentImage.height());
        return QPoint(qBound(0,x,currentImage.width()-1), qBound(0,y,currentImage.height()-1));
    }

    void pushUndo() {
        if (!currentImage.isNull()) {
            undoStack.append(currentImage);
            if (undoStack.size() > 50) undoStack.removeFirst();
            redoStack.clear();
        }
    }

    void imgUndo() {
        if (!undoStack.isEmpty()) {
            redoStack.append(currentImage);
            currentImage = undoStack.takeLast();
            updateDisplay();
        }
    }

    void imgRedo() {
        if (!redoStack.isEmpty()) {
            undoStack.append(currentImage);
            currentImage = redoStack.takeLast();
            updateDisplay();
        }
    }

    QLabel*      imageLabel  = nullptr;
    QLabel*      infoLabel   = nullptr;
    QScrollArea* scrollArea  = nullptr;
    QPushButton* btnDraw     = nullptr;
    QPushButton* btnEraser   = nullptr;
    QImage originalImage;
    QImage currentImage;
    QString currentPath;
    bool isDrawing  = false;
    bool isErasing  = false;
    bool drawingNow = false;
    QPoint lastPoint;
    QColor penColor = Qt::red;
    int    penSize  = 3;
    double zoomFactor = 1.0;
    QList<QImage> undoStack;
    QList<QImage> redoStack;
};


class QscSyntaxHighlighter : public QSyntaxHighlighter {
public:
    QscSyntaxHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {
        HighlightingRule rule;
        keywordFormat.setForeground(QColor("#569CD6"));
        keywordFormat.setFontWeight(QFont::Bold);
        QStringList keywordPatterns = {
            "\\bint\\b", "\\bfloat\\b", "\\bvoid\\b", "\\bif\\b", "\\belse\\b",
            "\\bwhile\\b", "\\bfor\\b", "\\breturn\\b", "\\bTRUE\\b", "\\bFALSE\\b",
            "\\bstring\\b"
        };
        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        classFormat.setForeground(QColor("#4EC9B0"));
        classFormat.setFontWeight(QFont::Bold);
        rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
        rule.format = classFormat;
        highlightingRules.append(rule);

        singleLineCommentFormat.setForeground(QColor("#6A9955"));
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = singleLineCommentFormat;
        highlightingRules.append(rule);

        multiLineCommentFormat.setForeground(QColor("#6A9955"));
        commentStartExpression = QRegularExpression("/\\*");
        commentEndExpression = QRegularExpression("\\*/");

        quotationFormat.setForeground(QColor("#CE9178"));
        rule.pattern = QRegularExpression("\".*\"");
        rule.format = quotationFormat;
        highlightingRules.append(rule);

        functionFormat.setFontItalic(true);
        functionFormat.setForeground(QColor("#DCDCAA"));
        rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
        rule.format = functionFormat;
        highlightingRules.append(rule);
    }

protected:
    void highlightBlock(const QString &text) override {
        for (const HighlightingRule &rule : highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
        setCurrentBlockState(0);
        int startIndex = 0;
        if (previousBlockState() != 1) startIndex = text.indexOf(commentStartExpression);
        while (startIndex >= 0) {
            QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
            int endIndex = match.capturedStart();
            int commentLength = 0;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + match.capturedLength();
            }
            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
        }
    }

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
};

class CodeEditor : public QTextEdit {
public:
    CodeEditor(QWidget *parent = nullptr) : QTextEdit(parent) {
        highlighter = new QscSyntaxHighlighter(this->document());
        QFile file(QCoreApplication::applicationDirPath() + "/IGIAutoComplete.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) autocompleteList.append(line);
            }
        }
        QFile modelFile(QCoreApplication::applicationDirPath() + "/IGIModels.json");
        if (modelFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument doc = QJsonDocument::fromJson(modelFile.readAll());
            modelInfoObj = doc.object();
        }
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, this, &CodeEditor::showCustomContextMenu);
    }

    bool isHexMode = false;
    
protected:
    void keyPressEvent(QKeyEvent *e) override {
        if (isHexMode) {
            if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right || e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) {
                QTextEdit::keyPressEvent(e);
                return;
            }
            if (e->text().isEmpty()) {
                QTextEdit::keyPressEvent(e);
                return;
            }
            if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete || e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                return;
            }
            
            QTextCursor cursor = textCursor();
            int col = cursor.positionInBlock();
            QString text = e->text();
            QChar inputChar = text[0];
            
            if (col >= 60 && col < 76) {
                // Editing ASCII
                char c = inputChar.toLatin1();
                QString hexStr = QString("%1").arg((quint8)c, 2, 16, QChar('0')).toUpper();
                
                int hexPos = 10 + (col - 60) * 3;
                cursor.setPosition(cursor.block().position() + hexPos);
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                cursor.insertText(hexStr);
                
                cursor.setPosition(cursor.block().position() + col);
                cursor.deleteChar();
                cursor.insertText(QString((c >= 32 && c < 127) ? c : '.'));
                
                cursor.setPosition(cursor.block().position() + col + 1);
                setTextCursor(cursor);
            } else if (col >= 10 && col < 58) {
                // Editing HEX
                if (!inputChar.isLetterOrNumber() || !QString("0123456789ABCDEFabcdef").contains(inputChar)) return;
                
                inputChar = inputChar.toUpper();
                cursor.deleteChar();
                cursor.insertText(inputChar);
                
                int byteStartCol = 10 + ((col - 10) / 3) * 3;
                cursor.setPosition(cursor.block().position() + byteStartCol);
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                QString byteHex = cursor.selectedText();
                
                char c = (char)byteHex.toInt(nullptr, 16);
                
                int asciiPos = 60 + (byteStartCol - 10) / 3;
                cursor.setPosition(cursor.block().position() + asciiPos);
                cursor.deleteChar();
                cursor.insertText(QString((c >= 32 && c < 127) ? c : '.'));
                
                cursor.setPosition(cursor.block().position() + col + 1);
                if ((col - 10) % 3 == 1) cursor.movePosition(QTextCursor::Right);
                setTextCursor(cursor);
            }
            return;
        }

        if (e->key() == Qt::Key_Tab) {
            QTextCursor cursor = textCursor();
            cursor.select(QTextCursor::WordUnderCursor);
            QString prefix = cursor.selectedText();
            if (!prefix.isEmpty()) {
                for (const QString &word : autocompleteList) {
                    if (word.startsWith(prefix, Qt::CaseInsensitive) && word != prefix) {
                        cursor.insertText(word);
                        return;
                    }
                }
            }
        }
        QTextEdit::keyPressEvent(e);
    }
    
private:
    void showCustomContextMenu(const QPoint &pt) {
        QMenu *menu = createStandardContextMenu();
        QTextCursor cursor = cursorForPosition(pt);
        cursor.select(QTextCursor::WordUnderCursor);
        QString word = cursor.selectedText();
        
        QRegularExpression re("\\d{3}_\\d{2}_\\d");
        if (re.match(word).hasMatch()) {
            menu->addSeparator();
            QAction *infoAction = menu->addAction("Model Information: " + word);
            connect(infoAction, &QAction::triggered, [this, word]() {
                QString info = "No information found for this model in IGIModels.json.";
                
                // Parse modelInfoObj which might be an array or object
                if (modelInfoObj.contains(word)) {
                    QJsonDocument doc(modelInfoObj[word].toObject());
                    info = doc.toJson(QJsonDocument::Indented);
                } else {
                    // It was an array inside a JSON file but loaded as an array? Wait, I didn't load it as an array!
                    // I will check the JSON directly.
                    QFile modelFile(QCoreApplication::applicationDirPath() + "/IGIModels.json");
                    if (modelFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QJsonDocument fullDoc = QJsonDocument::fromJson(modelFile.readAll());
                        if (fullDoc.isArray()) {
                            QJsonArray arr = fullDoc.array();
                            for (int i = 0; i < arr.size(); ++i) {
                                QJsonObject obj = arr[i].toObject();
                                if (obj["ModelId"].toString() == word) {
                                    info = "Model Name: " + obj["ModelName"].toString();
                                    break;
                                }
                            }
                        } else if (fullDoc.isObject() && fullDoc.object().contains(word)) {
                            info = "Model Name: " + fullDoc.object()[word].toObject()["ModelName"].toString();
                        }
                    }
                }
                QMessageBox::information(this, "Model Info", info);
            });
        }
        menu->exec(mapToGlobal(pt));
        delete menu;
    }

    QscSyntaxHighlighter* highlighter;
    QStringList autocompleteList;
    QJsonObject modelInfoObj;
};

class MainWindow : public QMainWindow {
public:
    MainWindow() {
        setWindowTitle("IGI Game Converter");
        QIcon appIcon(":/igi1conv.ico");
        setWindowIcon(appIcon);
        resize(1200, 800);
        treeView = nullptr;

        fileModel = new QFileSystemModel(this);
        fileModel->setReadOnly(false);
        QString iniPath = QCoreApplication::applicationDirPath() + "/igi1conv.ini";
        QString defaultFolder = "D:/Software/IGI-Game";
        QString lastFolder = QSettings(iniPath, QSettings::IniFormat).value("LastFolder", defaultFolder).toString();
        if (!QDir(lastFolder).exists()) {
            lastFolder = QDir(defaultFolder).exists() ? defaultFolder : QCoreApplication::applicationDirPath();
        }
        fileModel->setRootPath(lastFolder);
        
        // Custom proxy model to handle type names and size formatting
        class MyProxyModel : public QSortFilterProxyModel {
        public:
            MyProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
                if (role == Qt::DisplayRole) {
                    int col = index.column();
                    QFileSystemModel* fsm = qobject_cast<QFileSystemModel*>(sourceModel());
                    if (fsm) {
                        QFileInfo info = fsm->fileInfo(mapToSource(index));
                        if (col == 1) { // Size
                            if (info.isFile()) {
                                qint64 size = info.size();
                                if (size < 1024) return QString::number(size) + " B";
                                if (size < 1024 * 1024) return QString::number(size / 1024.0, 'f', 1) + " KB";
                                return QString::number(size / (1024.0 * 1024.0), 'f', 1) + " MB";
                            }
                            return "";
                        } else if (col == 2) { // Type
                            if (info.isDir()) return "File Folder";
                            QString ext = info.suffix().toLower();
                            if (ext == "mef") return "MEF Binary Model";
                            if (ext == "mex") return "MEX Extended Model";
                            if (ext == "res") return "Resource Archive";
                            if (ext == "spr") return "Sprite Image";
                            if (ext == "fnt") return "Font File";
                            if (ext == "pic") return "Picture Image";
                            if (ext == "tex") return "Texture Image";
                            if (ext == "mtp") return "Material Properties";
                            if (ext == "txt") {
                                // Distinguish text MEF from generic text
                                if (info.fileName().contains(".mef.", Qt::CaseInsensitive))
                                    return "MEF Text Model";
                                return "Text File";
                            }
                            return ext.toUpper() + " File";
                        }
                    }
                }
                return QSortFilterProxyModel::data(index, role);
            }
        };

        proxyModel = new MyProxyModel(this);
        proxyModel->setSourceModel(fileModel);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterKeyColumn(0); // Only filter by file name

        // ─── Model Search Toolbar ───────────────────────────────────────────
        QToolBar* searchToolBar = addToolBar("Model Search");
        searchToolBar->setMovable(false);
        searchToolBar->setVisible(false);
        searchToolBar->setStyleSheet("QToolBar{background:#252525;border-bottom:1px solid #444;spacing:4px;padding:2px 6px;}");
        
        QLabel* lblSearch = new QLabel("🔍 Model:");
        lblSearch->setStyleSheet("color:#ccc;font-size:11px;");
        searchToolBar->addWidget(lblSearch);
        
        QLineEdit* modelSearchBox = new QLineEdit();
        modelSearchBox->setPlaceholderText("Search by name or ID (e.g. 'Jones' or '000_01_1' or '435')...");
        modelSearchBox->setMinimumWidth(380);
        modelSearchBox->setMaximumWidth(500);
        modelSearchBox->setFixedHeight(24);
        modelSearchBox->setStyleSheet("background:#333;color:#eee;border:1px solid #555;border-radius:3px;padding:2px 6px;font-family:Consolas;font-size:11px;");
        searchToolBar->addWidget(modelSearchBox);
        
        QPushButton* btnRefresh = new QPushButton("Refresh");
        btnRefresh->setStyleSheet("background:#444;color:#eee;border:1px solid #555;border-radius:3px;padding:2px 8px;font-size:11px;");
        searchToolBar->addWidget(btnRefresh);
        connect(btnRefresh, &QPushButton::clicked, this, [this]() {
            if (this->fileModel) {
                QString currentPath = this->fileModel->rootPath();
                this->fileModel->setRootPath("");
                this->fileModel->setRootPath(currentPath);
            }
        });
        
        QLabel* modelSearchResult = new QLabel("  Type to search...");
        modelSearchResult->setStyleSheet("color:#888;font-size:11px;font-family:Consolas;min-width:300px;");
        searchToolBar->addWidget(modelSearchResult);
        
        // Load IGIModels.json for bidirectional search
        connect(modelSearchBox, &QLineEdit::textChanged, this, [this, modelSearchResult, iniPath](const QString& query) {
            if (query.trimmed().isEmpty()) {
                modelSearchResult->setText("  Type to search...");
                modelSearchResult->setStyleSheet("color:#888;font-size:11px;font-family:Consolas;min-width:300px;");
                proxyModel->setFilterWildcard("*");
                if (this->treeView) {
                    QString currentRoot = fileModel->rootPath();
                    if (currentRoot.isEmpty()) currentRoot = QDir::currentPath();
                    this->treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(currentRoot)));
                }
                return;
            }
            
            static QJsonArray s_modelsArr;
            static bool s_loaded = false;
            if (!s_loaded) {
                s_loaded = true;
                QFile f(QCoreApplication::applicationDirPath() + "/IGIModels.json");
                if (f.open(QIODevice::ReadOnly)) {
                    s_modelsArr = QJsonDocument::fromJson(f.readAll()).array();
                }
            }
            
            QString q = query.trimmed().toLower();
            QStringList results;
            QString exactMatchId;
            
            for (const QJsonValue& v : s_modelsArr) {
                QJsonObject obj = v.toObject();
                QString id   = obj["ModelId"].toString();
                QString name = obj["ModelName"].toString();
                
                bool exactMatch = (id.toLower() == q || name.toLower() == q);
                bool partialMatch = (id.toLower().contains(q) || name.toLower().contains(q));
                
                if (exactMatch) {
                    exactMatchId = id;
                    results.prepend(QString("%1 → %2").arg(id, name));
                } else if (partialMatch) {
                    if (exactMatchId.isEmpty()) exactMatchId = id; // use first match as fallback
                    results << QString("%1 → %2").arg(id, name);
                }
            }
            
            if (results.isEmpty()) {
                modelSearchResult->setText("  No results for: " + query);
                modelSearchResult->setStyleSheet("color:#ff6666;font-size:11px;font-family:Consolas;min-width:300px;");
                proxyModel->setFilterWildcard("*" + query + "*");
            } else {
                modelSearchResult->setText("  " + results.join("  |  "));
                modelSearchResult->setStyleSheet("color:#66FFAA;font-size:11px;font-family:Consolas;min-width:300px;");
                // Filter file tree to show all files (models + textures) belonging to the match
                QString basePrefix = exactMatchId.split('_').first();
                proxyModel->setFilterWildcard("*" + basePrefix + "*");
                if (this->treeView) {
                    QString currentRoot = fileModel->rootPath();
                    if (currentRoot.isEmpty()) currentRoot = QDir::currentPath();
                    this->treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(currentRoot)));
                }
            }
        });
        
        QShortcut *modelSearchToggle = new QShortcut(QKeySequence("Ctrl+Shift+M"), this);
        connect(modelSearchToggle, &QShortcut::activated, this, [this, searchToolBar, modelSearchBox]() {
            searchToolBar->setVisible(!searchToolBar->isVisible());
            if (searchToolBar->isVisible()) {
                modelSearchBox->setFocus();
                modelSearchBox->selectAll();
            }
        });

        QMenu* fileMenu = menuBar()->addMenu("&File");
        fileMenu->addAction(QIcon::fromTheme("document-open"), "&Open File...", this, [this]() {
            QString filePath = QFileDialog::getOpenFileName(this, "Select File to Open");
            if (!filePath.isEmpty()) {
                QString dir = QFileInfo(filePath).absolutePath();
                fileModel->setRootPath(dir);
                if (this->treeView) {
                    this->treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(dir)));
                }
                loadFile(filePath);
            }
        }, QKeySequence("Ctrl+O"));

        fileMenu->addAction(QIcon::fromTheme("folder-open"), "&Open Folder...", this, [this, iniPath]() {
            QString dir = QFileDialog::getExistingDirectory(this, "Select Workspace Folder");
            if (!dir.isEmpty()) {
                fileModel->setRootPath(dir);
                treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(dir)));
                QSettings(iniPath, QSettings::IniFormat).setValue("LastFolder", dir);
                QDir tempDir(QDir::tempPath() + "/igi_temp_mef");
                if (tempDir.exists()) tempDir.removeRecursively();
                logMessage("[INFO] Workspace changed. Smart cache cleared.");
            }
        }, QKeySequence("Ctrl+Shift+O"));
        
        fileMenu->addAction(QIcon::fromTheme("document-save"), "&Save", this, [this]() {
            if (viewerEdit->isVisible() && !currentFile.isEmpty() && !viewerEdit->isReadOnly()) {
                if (currentExt == "qvm") {
                    QString tempQscPath = QDir::tempPath() + "/igi_temp.qsc";
                    QFile f(tempQscPath);
                    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        f.write(viewerEdit->toPlainText().toUtf8());
                        f.close();
                        QString cmd = qApp->applicationFilePath();
                        QProcess::execute(cmd, QStringList() << "qsc" << "compile" << tempQscPath << "-o" << currentFile);
                        logMessage("[INFO] Compiled QSC to QVM and saved successfully!");
                        QMessageBox::information(this, "Saved", "Compiled and saved QVM successfully!");
                    }
                } else if (viewModeCombo->currentIndex() == 2) { // Hex View
                    QString text = viewerEdit->toPlainText();
                    QByteArray data;
                    QStringList lines = text.split("\n");
                    for (const QString& line : lines) {
                        if (line.isEmpty() || line.startsWith("...")) continue;
                        int pipe = line.indexOf("|");
                        if (pipe == -1) continue;
                        QString hexPart = line.mid(10, pipe - 10);
                        QStringList bytes = hexPart.split(" ", Qt::SkipEmptyParts);
                        for (const QString& b : bytes) {
                            data.append((char)b.toInt(nullptr, 16));
                        }
                    }
                    QFile f(currentFile);
                    if (f.open(QIODevice::WriteOnly)) {
                        f.write(data);
                        logMessage("[INFO] Hex File saved successfully!");
                        QMessageBox::information(this, "Saved", "Hex File saved successfully!");
                    }
                } else {
                    QFile f(currentFile);
                    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        f.write(viewerEdit->toPlainText().toUtf8());
                        logMessage("[INFO] File saved successfully!");
                        QMessageBox::information(this, "Saved", "File saved successfully!");
                    }
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
        editMenu->addAction("Find in Text", this, [this](){
            textSearchWidget->setVisible(!textSearchWidget->isVisible());
            if (textSearchWidget->isVisible()) textSearchBox->setFocus();
            else textSearchWidget->hide();
        }, QKeySequence("Ctrl+F"));
        editMenu->addAction("Select All",  this, [this](){ viewerEdit->selectAll(); },   QKeySequence::SelectAll);
        editMenu->addSeparator();
        editMenu->addAction("Find File", this, [this](){
            fileSearchBox->setVisible(!fileSearchBox->isVisible());
            if (fileSearchBox->isVisible()) fileSearchBox->setFocus();
            else { fileSearchBox->clear(); treeView->setFocus(); }
        }, QKeySequence("Ctrl+Shift+F"));
        editMenu->addAction("Replace...", this, [this](){
            // Simple find/replace dialog
            QDialog dlg(this); dlg.setWindowTitle("Find & Replace");
            QVBoxLayout* vl = new QVBoxLayout(&dlg);
            QHBoxLayout* r1 = new QHBoxLayout(); QLineEdit* find = new QLineEdit(); r1->addWidget(new QLabel("Find:")); r1->addWidget(find);
            QHBoxLayout* r2 = new QHBoxLayout(); QLineEdit* repl = new QLineEdit(); r2->addWidget(new QLabel("Replace:")); r2->addWidget(repl);
            QHBoxLayout* r3 = new QHBoxLayout();
            QPushButton* btnReplace = new QPushButton("Replace"); QPushButton* btnAll = new QPushButton("Replace All"); QPushButton* btnClose = new QPushButton("Close");
            r3->addWidget(btnReplace); r3->addWidget(btnAll); r3->addWidget(btnClose);
            vl->addLayout(r1); vl->addLayout(r2); vl->addLayout(r3);
            connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);
            connect(btnReplace, &QPushButton::clicked, [&](){ viewerEdit->find(find->text()); });
            connect(btnAll, &QPushButton::clicked, [&](){
                QString txt = viewerEdit->toPlainText();
                if (!find->text().isEmpty()) { txt.replace(find->text(), repl->text()); viewerEdit->setPlainText(txt); }
            });
            dlg.exec();
        }, QKeySequence("Ctrl+H"));
        // F2 = rename selected file in tree
        QShortcut* f2Rename = new QShortcut(QKeySequence(Qt::Key_F2), this);
        connect(f2Rename, &QShortcut::activated, this, [this](){
            if (treeView) {
                QModelIndex idx = proxyModel->mapToSource(treeView->currentIndex());
                if (idx.isValid()) treeView->edit(proxyModel->mapFromSource(idx));
            }
        });

        QMenu* viewMenu = menuBar()->addMenu("&View");
        viewMenu->addAction("Auto", [this]() { viewModeCombo->setCurrentIndex(0); });
        viewMenu->addAction("Text", [this]() { viewModeCombo->setCurrentIndex(1); });
        viewMenu->addAction("Hex", [this]() { viewModeCombo->setCurrentIndex(2); });
        viewMenu->addAction("Image View", [this]() { viewModeCombo->setCurrentIndex(3); });
        viewMenu->addAction("3D View", [this]() { viewModeCombo->setCurrentIndex(4); });
        viewMenu->addSeparator();
        auto applyMenuTheme = [iniPath](const QString& name) {
            QSettings(iniPath, QSettings::IniFormat).setValue("Theme", name);
            if (name == "Light") {
                QPalette p;
                p.setColor(QPalette::Window, QColor(240,240,240));
                p.setColor(QPalette::WindowText, Qt::black);
                p.setColor(QPalette::Base, Qt::white);
                p.setColor(QPalette::AlternateBase, QColor(233,233,233));
                p.setColor(QPalette::ToolTipBase, Qt::white);
                p.setColor(QPalette::ToolTipText, Qt::black);
                p.setColor(QPalette::Text, Qt::black);
                p.setColor(QPalette::Button, QColor(240,240,240));
                p.setColor(QPalette::ButtonText, Qt::black);
                p.setColor(QPalette::BrightText, Qt::red);
                p.setColor(QPalette::Link, QColor(0,0,200));
                p.setColor(QPalette::Highlight, QColor(0,120,215));
                p.setColor(QPalette::HighlightedText, Qt::white);
                qApp->setPalette(p);
                qApp->setStyleSheet(
                    "QTextEdit{background:white;color:black;border:1px solid #ccc;}"
                    "QPlainTextEdit{background:white;color:black;border:1px solid #ccc;}"
                    "QTreeView{background:white;color:black;alternate-background-color:#f0f0f0;}"
                    "QLineEdit{background:white;color:black;border:1px solid #aaa;border-radius:3px;}"
                    "QToolBar{background:#ebebeb;border-bottom:1px solid #ccc;spacing:4px;padding:2px 6px;}"
                    "QMenuBar{background:#f0f0f0;color:black;}"
                    "QMenu{background:#f0f0f0;color:black;}"
                    "QStatusBar{background:#f0f0f0;color:black;}"
                    "QLabel{color:black;}"
                    "QSplitter::handle{background:#ccc;}"
                );
            } else if (name == "Solarized") {
                QPalette p;
                p.setColor(QPalette::Window, QColor(0,43,54));
                p.setColor(QPalette::WindowText, QColor(131,148,150));
                p.setColor(QPalette::Base, QColor(7,54,66));
                p.setColor(QPalette::AlternateBase, QColor(0,43,54));
                p.setColor(QPalette::Text, QColor(131,148,150));
                p.setColor(QPalette::Button, QColor(0,43,54));
                p.setColor(QPalette::ButtonText, QColor(131,148,150));
                p.setColor(QPalette::Highlight, QColor(38,139,210));
                p.setColor(QPalette::HighlightedText, Qt::white);
                qApp->setPalette(p);
                qApp->setStyleSheet("");
            } else if (name == "Military") {
                QPalette p;
                p.setColor(QPalette::Window, QColor(30,40,20));
                p.setColor(QPalette::WindowText, QColor(120,200,60));
                p.setColor(QPalette::Base, QColor(15,25,10));
                p.setColor(QPalette::AlternateBase, QColor(30,40,20));
                p.setColor(QPalette::Text, QColor(120,200,60));
                p.setColor(QPalette::Button, QColor(30,40,20));
                p.setColor(QPalette::ButtonText, QColor(120,200,60));
                p.setColor(QPalette::Highlight, QColor(80,140,30));
                p.setColor(QPalette::HighlightedText, Qt::black);
                qApp->setPalette(p);
                qApp->setStyleSheet("");
            } else {
                // Dark
                QPalette p;
                p.setColor(QPalette::Window, QColor(53,53,53));
                p.setColor(QPalette::WindowText, Qt::white);
                p.setColor(QPalette::Base, QColor(25,25,25));
                p.setColor(QPalette::AlternateBase, QColor(53,53,53));
                p.setColor(QPalette::ToolTipBase, Qt::white);
                p.setColor(QPalette::ToolTipText, Qt::white);
                p.setColor(QPalette::Text, Qt::white);
                p.setColor(QPalette::Button, QColor(53,53,53));
                p.setColor(QPalette::ButtonText, Qt::white);
                p.setColor(QPalette::BrightText, Qt::red);
                p.setColor(QPalette::Link, QColor(42,130,218));
                p.setColor(QPalette::Highlight, QColor(42,130,218));
                p.setColor(QPalette::HighlightedText, Qt::black);
                qApp->setPalette(p);
                qApp->setStyleSheet("");
            }
        };
        viewMenu->addAction("Light Theme",    [applyMenuTheme]() { applyMenuTheme("Light"); });
        viewMenu->addAction("Dark Theme",     [applyMenuTheme]() { applyMenuTheme("Dark"); });
        viewMenu->addAction("Solarized Theme",[applyMenuTheme]() { applyMenuTheme("Solarized"); });
        viewMenu->addAction("Military Theme", [applyMenuTheme]() { applyMenuTheme("Military"); });


        QSettings settings(iniPath, QSettings::IniFormat);
        globalLevelMtpPath = settings.value("LevelMTP", "").toString();
        globalLevelDatPath = settings.value("LevelDAT", "").toString();
        globalTextureDir = settings.value("TextureDir", "").toString();
        globalCacheDir = settings.value("CacheDir", QDir::tempPath() + "/igi_temp_mef").toString();
        QString logLevel = settings.value("LOGS_LEVEL", "INFO").toString();

        QMenu* settingsMenu = menuBar()->addMenu("&Settings");
        
        QMenu* levelMenu = settingsMenu->addMenu("&Level");
        levelMenu->addAction("Set Level...", this, [this, iniPath]() {
            QString levelDir = QFileDialog::getExistingDirectory(this, "Select Level Folder (e.g. LEVEL8)", globalLevelDatPath.isEmpty() ? "D:/Software/IGI-Game" : QFileInfo(globalLevelDatPath).absolutePath());
            if (levelDir.isEmpty()) return;
            
            // Auto-resolve DAT, MTP, Textures from the level folder
            QDir dir(levelDir);
            QString levelName = dir.dirName(); // e.g. "LEVEL8"
            
            // Find .DAT
            QStringList dats = dir.entryList(QStringList() << "*.dat" << "*.DAT", QDir::Files);
            if (!dats.isEmpty()) {
                globalLevelDatPath = levelDir + "/" + dats.first();
                QSettings(iniPath, QSettings::IniFormat).setValue("LevelDAT", globalLevelDatPath);
                logMessage("[INFO] Level DAT auto-set: " + globalLevelDatPath);
            }
            
            // Find .MTP
            QStringList mtps = dir.entryList(QStringList() << "*.mtp" << "*.MTP", QDir::Files);
            if (!mtps.isEmpty()) {
                globalLevelMtpPath = levelDir + "/" + mtps.first();
                QSettings(iniPath, QSettings::IniFormat).setValue("LevelMTP", globalLevelMtpPath);
                logMessage("[INFO] Level MTP auto-set: " + globalLevelMtpPath);
            }
            
            // Find TEXTURES subfolder
            QStringList textureDirs = {"TEXTURES", "textures", "Textures"};
            for (const QString& td : textureDirs) {
                if (QDir(levelDir + "/" + td).exists()) {
                    globalTextureDir = levelDir + "/" + td;
                    break;
                }
            }
            // If not found, check for .RES file and auto-extract
            if (globalTextureDir.isEmpty()) {
                QStringList resList = dir.entryList(QStringList() << "*.res" << "*.RES", QDir::Files);
                for (const QString& resName : resList) {
                    if (resName.contains(levelName, Qt::CaseInsensitive) || resName.contains("TEXTURES", Qt::CaseInsensitive)) {
                        globalTextureDir = levelDir + "/TEXTURES";
                        QDir().mkpath(globalTextureDir);
                        QString resPath = levelDir + "/" + resName;
                        logMessage("[INFO] Extracting " + resName + " -> " + globalTextureDir);
                        QProcess::execute(qApp->applicationFilePath(), QStringList() << "res" << "extract" << resPath << "-o" << globalTextureDir);
                        break;
                    }
                }
            }
            if (!globalTextureDir.isEmpty()) {
                QSettings(iniPath, QSettings::IniFormat).setValue("TextureDir", globalTextureDir);
                logMessage("[INFO] Texture Dir auto-set: " + globalTextureDir);
                
                // Extract any .res files inside texture dir
                QDir texDir(globalTextureDir);
                for (const QString& res : texDir.entryList(QStringList() << "*.res" << "*.RES", QDir::Files)) {
                    logMessage("[INFO] Extracting " + res + " from textures dir...");
                    QProcess::execute(qApp->applicationFilePath(), QStringList() << "res" << "extract" << texDir.absoluteFilePath(res) << "-o" << globalTextureDir);
                }
            }
            QSettings(iniPath, QSettings::IniFormat).setValue("LevelPath", levelDir);
            QMessageBox::information(this, "Level Set",
                QString("Level: %1\nDAT: %2\nMTP: %3\nTextures: %4")
                .arg(levelDir, globalLevelDatPath, globalLevelMtpPath, globalTextureDir));
        });

        settingsMenu->addAction("Cache Folder...", this, [this, iniPath]() {
            QString newCache = QFileDialog::getExistingDirectory(this, "Select Temp Cache Folder", globalCacheDir);
            if (!newCache.isEmpty()) {
                globalCacheDir = newCache;
                modelViewer->cacheDir = globalCacheDir;
                QSettings(iniPath, QSettings::IniFormat).setValue("CacheDir", globalCacheDir);
                logMessage("[INFO] Cache Folder set to: " + globalCacheDir);
            }
        });

        auto doClearCache = [this]() {
            QDir dir(globalCacheDir);
            if (!dir.exists()) {
                logMessage("[INFO] Cache folder does not exist: " + globalCacheDir);
                return;
            }
            int files = 0;
            qint64 bytes = 0;
            for (const QFileInfo& fi : QDir(globalCacheDir).entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
                { files++; bytes += fi.size(); }
            // Walk subdirs too
            QDirIterator it(globalCacheDir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) { it.next(); files++; bytes += it.fileInfo().size(); }
            for (const QString& entry : dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
                QFileInfo fi(globalCacheDir + "/" + entry);
                if (fi.isDir()) QDir(fi.absoluteFilePath()).removeRecursively();
                else            QFile::remove(fi.absoluteFilePath());
            }
            logMessage(QString("[INFO] Cache cleared: %1 files removed (%2 KB freed) — %3")
                        .arg(files).arg(bytes / 1024).arg(globalCacheDir));
        };

        settingsMenu->addAction("Clear Cache", this, doClearCache);

        // Small button in the menu-bar corner for quick cache clearing
        QPushButton* clearCacheBtn = new QPushButton("\xf0\x9f\x97\x91 Clear Cache", menuBar());
        clearCacheBtn->setFlat(true);
        clearCacheBtn->setStyleSheet(
            "QPushButton{color:#ccc;background:transparent;border:1px solid #555;"
            "border-radius:3px;padding:2px 8px;font-size:11px;}"
            "QPushButton:hover{background:#3a3a3a;border-color:#888;}"
            "QPushButton:pressed{background:#222;}");
        clearCacheBtn->setToolTip("Clear temporary cache folder");
        connect(clearCacheBtn, &QPushButton::clicked, this, doClearCache);
        menuBar()->setCornerWidget(clearCacheBtn, Qt::TopRightCorner);

        QMenu* logsMenu = settingsMenu->addMenu("&Logs");
        logsMenu->addAction("Enable Logs", this, [this, iniPath]() {
            QSettings(iniPath, QSettings::IniFormat).setValue("LOGS_ENABLED", true);
            logMessage("[INFO] Logs Enabled");
        });
        logsMenu->addAction("Disable Logs", this, [this, iniPath]() {
            QSettings(iniPath, QSettings::IniFormat).setValue("LOGS_ENABLED", false);
            logMessage("[INFO] Logs Disabled");
        });

        QMenu* logsLevelMenu = logsMenu->addMenu("Logs Level");
        logsLevelMenu->addAction("INFO", this, [this, iniPath]() {
            QSettings(iniPath, QSettings::IniFormat).setValue("LOGS_LEVEL", "INFO");
            logMessage("[INFO] LOGS_LEVEL set to INFO");
        });
        logsLevelMenu->addAction("DEBUG", this, [this, iniPath]() {
            QSettings(iniPath, QSettings::IniFormat).setValue("LOGS_LEVEL", "DEBUG");
            logMessage("[INFO] LOGS_LEVEL set to DEBUG");
        });
        logsLevelMenu->addAction("ERROR", this, [this, iniPath]() {
            QSettings(iniPath, QSettings::IniFormat).setValue("LOGS_LEVEL", "ERROR");
            logMessage("[INFO] LOGS_LEVEL set to ERROR");
        });

        QMenu* helpMenu = menuBar()->addMenu("&Help");
        helpMenu->addAction("About", this, [this]() {
            QMessageBox::about(this, "About", "IGI Game Convertor\nVersion 1.7.0\nAuthor: HeavenHM\nDeveloped in C++ with Qt5/Qt6.\nAdvanced Edition with MEF Native Viewer and full CLI integration.");
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
            // First check if 'text' matches a model name to resolve its ID prefix
            static QJsonArray s_modelsArr;
            static bool s_loaded = false;
            if (!s_loaded) {
                s_loaded = true;
                QFile f(QCoreApplication::applicationDirPath() + "/IGIModels.json");
                if (f.open(QIODevice::ReadOnly)) {
                    s_modelsArr = QJsonDocument::fromJson(f.readAll()).array();
                }
            }
            QString exactMatchId;
            QString q = text.trimmed().toLower();
            for (const QJsonValue& v : s_modelsArr) {
                QJsonObject obj = v.toObject();
                QString id   = obj["ModelId"].toString();
                QString name = obj["ModelName"].toString();
                if (id.toLower() == q || name.toLower() == q) { exactMatchId = id; break; }
            }
            
            if (!exactMatchId.isEmpty()) {
                QString basePrefix = exactMatchId.split('_').first();
                proxyModel->setFilterWildcard("*" + basePrefix + "*");
            } else {
                proxyModel->setFilterWildcard("*" + text + "*");
            }
            
            if (this->treeView) {
                QString currentRoot = fileModel->rootPath();
                if (currentRoot.isEmpty()) currentRoot = QDir::currentPath();
                this->treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(currentRoot)));
            }
        });
        treeView = new QTreeView(leftWidget);
        treeView->setModel(proxyModel);
        treeView->setRootIndex(proxyModel->mapFromSource(fileModel->index(lastFolder)));
        treeView->setColumnWidth(0, 250);
        treeView->setSortingEnabled(true);
        treeView->sortByColumn(0, Qt::AscendingOrder);
        treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        treeView->setSelectionMode(QAbstractItemView::ExtendedSelection); // Multi-selection
        treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
        treeView->setDragEnabled(true);
        treeView->setAcceptDrops(true);
        treeView->setDropIndicatorShown(true);
        treeView->setDragDropMode(QAbstractItemView::DragDrop);
        treeView->setDefaultDropAction(Qt::CopyAction);
        leftLayout->addWidget(treeView);

        connect(treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);

        // Right side: Viewer and Controls
        QWidget* rightWidget = new QWidget(splitter);
        QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);

        textSearchWidget = new QWidget();
        QHBoxLayout* textSearchLayout = new QHBoxLayout(textSearchWidget);
        textSearchLayout->setContentsMargins(0,0,0,0);
        textSearchBox = new QLineEdit();
        textSearchBox->setPlaceholderText("Find...");
        textSearchLayout->addWidget(textSearchBox);
        
        QLineEdit* replaceBox = new QLineEdit();
        replaceBox->setPlaceholderText("Replace with...");
        textSearchLayout->addWidget(replaceBox);
        
        QPushButton* btnReplace = new QPushButton("Replace");
        textSearchLayout->addWidget(btnReplace);
        QPushButton* btnReplaceAll = new QPushButton("Replace All");
        textSearchLayout->addWidget(btnReplaceAll);

        textSearchWidget->hide();
        rightLayout->addWidget(textSearchWidget);
        
        connect(btnReplace, &QPushButton::clicked, this, [this, replaceBox]() {
            if (viewerEdit->textCursor().hasSelection()) {
                viewerEdit->textCursor().insertText(replaceBox->text());
            }
            viewerEdit->find(textSearchBox->text());
        });
        
        connect(btnReplaceAll, &QPushButton::clicked, this, [this, replaceBox]() {
            QTextCursor cursor = viewerEdit->textCursor();
            cursor.beginEditBlock();
            cursor.movePosition(QTextCursor::Start);
            viewerEdit->setTextCursor(cursor);
            while (viewerEdit->find(textSearchBox->text())) {
                viewerEdit->textCursor().insertText(replaceBox->text());
            }
            cursor.endEditBlock();
        });

        connect(textSearchBox, &QLineEdit::textChanged, this, [this](const QString& text) {
            viewerEdit->moveCursor(QTextCursor::Start);
            viewerEdit->find(textSearchBox->text());
        });
        
        connect(textSearchBox, &QLineEdit::returnPressed, this, [this]() {
            viewerEdit->find(textSearchBox->text());
        });
        
        QShortcut *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), treeView);
        connect(renameShortcut, &QShortcut::activated, this, [this]() {
            QModelIndex index = treeView->currentIndex();
            QModelIndex srcIndex = proxyModel->mapToSource(index);
            if (srcIndex.isValid()) {
                QString path = fileModel->filePath(srcIndex);
                bool ok;
                QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, QFileInfo(path).fileName(), &ok);
                if (ok && !newName.isEmpty()) {
                    QString newPath = QFileInfo(path).absolutePath() + "/" + newName;
                    if (QFileInfo(path).isDir()) QDir().rename(path, newPath);
                    else QFile::rename(path, newPath);
                }
            }
        });

        QHBoxLayout* viewModeLayout = new QHBoxLayout();
        viewModeLayout->addWidget(new QLabel("Mode:"));
        viewModeCombo = new QComboBox();
        viewModeCombo->addItems({"Auto", "Text", "Hex", "Image View", "3D View"});
        viewModeLayout->addWidget(viewModeCombo);
        viewModeLayout->addStretch();
        rightLayout->addLayout(viewModeLayout);

        viewerEdit = new CodeEditor(this);
        viewerEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        viewerEdit->hide();

        // ── Text editor toolbar ──────────────────────────────────────────────

        imageEditor = new ImageEditor(this);
        imageEditor->hide();

        modelViewer = new ModelViewer();
        modelViewer->cacheDir = globalCacheDir;

        rightLayout->addWidget(viewerEdit, 3);
        rightLayout->addWidget(imageEditor, 3);
        rightLayout->addWidget(modelViewer, 3);

        // Debug Output
        QGroupBox* debugGroup = new QGroupBox("Conversion Debug Output");
        QVBoxLayout* debugLayout = new QVBoxLayout(debugGroup);
        consoleEdit = new QTextEdit();
        consoleEdit->setReadOnly(true);
        consoleEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        consoleEdit->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(consoleEdit, &QTextEdit::customContextMenuRequested, this, [this](const QPoint& pos) {
            QMenu menu;
            menu.addAction("Copy", consoleEdit, &QTextEdit::copy);
            menu.addAction("Select All", consoleEdit, &QTextEdit::selectAll);
            menu.addSeparator();
            menu.addAction("Clear", consoleEdit, &QTextEdit::clear);
            menu.exec(consoleEdit->mapToGlobal(pos));
        });
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
        
        // Multi-select: copy/cut/paste using keyboard on selection
        QShortcut *copySelShortcut = new QShortcut(QKeySequence::Copy, treeView);
        connect(copySelShortcut, &QShortcut::activated, this, [this]() {
            auto indexes = treeView->selectionModel()->selectedRows();
            if (!indexes.isEmpty()) {
                clipboardFilePath = fileModel->filePath(proxyModel->mapToSource(indexes.first()));
                clipboardIsCut = false;
                logMessage(QString("[INFO] Copied %1 item(s) to clipboard").arg(indexes.size()));
            }
        });
        QShortcut *cutSelShortcut = new QShortcut(QKeySequence::Cut, treeView);
        connect(cutSelShortcut, &QShortcut::activated, this, [this]() {
            auto indexes = treeView->selectionModel()->selectedRows();
            if (!indexes.isEmpty()) {
                clipboardFilePath = fileModel->filePath(proxyModel->mapToSource(indexes.first()));
                clipboardIsCut = true;
                logMessage(QString("[INFO] Cut %1 item(s) to clipboard").arg(indexes.size()));
            }
        });
        QShortcut *deleteSelShortcut = new QShortcut(QKeySequence::Delete, treeView);
        connect(deleteSelShortcut, &QShortcut::activated, this, [this]() {
            auto indexes = treeView->selectionModel()->selectedRows();
            if (indexes.isEmpty()) return;
            if (QMessageBox::question(this, "Delete", QString("Delete %1 selected item(s)?").arg(indexes.size())) != QMessageBox::Yes) return;
            for (auto& idx : indexes) {
                QString path = fileModel->filePath(proxyModel->mapToSource(idx));
                if (QFileInfo(path).isDir()) QDir(path).removeRecursively();
                else QFile(path).remove();
            }
        });

        connect(viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (!currentFile.isEmpty()) {
                loadFile(currentFile, index);
            }
        });
        
        hideAllViewers();
    }

    void closeEvent(QCloseEvent *event) override {
        QDir tempDir(QDir::tempPath() + "/igi_temp_mef");
        if (tempDir.exists()) tempDir.removeRecursively();
        QMainWindow::closeEvent(event);
    }

    void logMessage(const QString& msg) {
        if (consoleEdit) {
            consoleEdit->append(msg);
        }
        QString iniPath = QCoreApplication::applicationDirPath() + "/igi1conv.ini";
        if (QSettings(iniPath, QSettings::IniFormat).value("LOGS_ENABLED", true).toBool()) {
            QFile logFile(QCoreApplication::applicationDirPath() + "/igi1conv.log");
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&logFile);
                out << msg << "\n";
            }
        }
    }

private:
    QFileSystemModel* fileModel;
    QSortFilterProxyModel* proxyModel;
    QTreeView* treeView;
    QLineEdit* fileSearchBox;
    QWidget* textSearchWidget;
    QLineEdit* textSearchBox;
    QComboBox* viewModeCombo;
    CodeEditor* viewerEdit;
    ImageEditor* imageEditor;
    ModelViewer* modelViewer;
    QTextEdit* consoleEdit;
    QString currentFile;
    QString currentExt;
    QString clipboardFilePath;
    bool clipboardIsCut = false;
    QString globalLevelMtpPath;
    QString globalLevelDatPath;
    QString globalTextureDir;
    QString globalCacheDir;

    void hideAllViewers() {
        viewerEdit->hide();
        imageEditor->hide();
        modelViewer->hide();
    }

    void showContextMenu(const QPoint& pos) {
        QModelIndex index = treeView->indexAt(pos);
        QModelIndex srcIndex = proxyModel->mapToSource(index);
        if (!srcIndex.isValid()) return;

        QString path = fileModel->filePath(srcIndex);
        QString ext = QFileInfo(path).suffix().toLower();
        bool isDir = fileModel->isDir(srcIndex);

        // Get all selected items
        auto selectedIndexes = treeView->selectionModel()->selectedRows();
        bool multiSelect = selectedIndexes.size() > 1;

        QMenu menu;
        
        // Multi-selection operations
        if (multiSelect) {
            menu.addAction(QString("Selected %1 items").arg(selectedIndexes.size()))->setEnabled(false);
            menu.addSeparator();
            menu.addAction("Delete Selected", [this, selectedIndexes]() {
                if (QMessageBox::question(this, "Delete", QString("Delete %1 selected items?").arg(selectedIndexes.size())) != QMessageBox::Yes) return;
                for (auto& idx : selectedIndexes) {
                    QString p = fileModel->filePath(proxyModel->mapToSource(idx));
                    if (QFileInfo(p).isDir()) QDir(p).removeRecursively();
                    else QFile(p).remove();
                }
            });
            menu.addAction("Copy Selected", [this, selectedIndexes]() {
                clipboardFilePath = fileModel->filePath(proxyModel->mapToSource(selectedIndexes.first()));
                clipboardIsCut = false;
                logMessage(QString("[INFO] Copied %1 item(s)").arg(selectedIndexes.size()));
            });
            menu.addAction("Cut Selected", [this, selectedIndexes]() {
                clipboardFilePath = fileModel->filePath(proxyModel->mapToSource(selectedIndexes.first()));
                clipboardIsCut = true;
                logMessage(QString("[INFO] Cut %1 item(s)").arg(selectedIndexes.size()));
            });
            if (!clipboardFilePath.isEmpty()) {
                menu.addAction("Paste Here", [this, path, isDir]() {
                    QString destDir = isDir ? path : QFileInfo(path).absolutePath();
                    QString dest = destDir + "/" + QFileInfo(clipboardFilePath).fileName();
                    int copyCount = 1;
                    while (QFileInfo::exists(dest) && (!clipboardIsCut || dest != clipboardFilePath)) {
                        QString base = QFileInfo(clipboardFilePath).completeBaseName();
                        QString suf = QFileInfo(clipboardFilePath).suffix();
                        if (!suf.isEmpty()) suf = "." + suf;
                        dest = destDir + "/" + base + (copyCount == 1 ? "_copy" : "_copy" + QString::number(copyCount)) + suf;
                        copyCount++;
                    }
                    if (clipboardIsCut) {
                        QFile::rename(clipboardFilePath, dest);
                        clipboardFilePath = "";
                    } else {
                        QFile::copy(clipboardFilePath, dest);
                    }
                });
            }
            menu.exec(treeView->mapToGlobal(pos));
            return;
        }


        // Single item context menu
        if (isDir) {
            menu.addAction("Pack to .res archive", [this, path]() {
                QString folderName = QFileInfo(path).fileName();
                QString defaultOut = path + "/" + folderName + ".res";
                QString outRes = QFileDialog::getSaveFileName(this, "Save Resource Archive",
                    defaultOut, "Resource Archive (*.res)");
                if (outRes.isEmpty()) return;

                QString prefix = QInputDialog::getText(this, "Archive Prefix",
                    "Enter the internal folder prefix (e.g. 'textures/').\n"
                    "The tool will automatically add 'LOCAL:' for the external path.",
                    QLineEdit::Normal, folderName + "/");
                
                QProcess proc;
                proc.setProgram(qApp->applicationFilePath());
                QStringList args;
                args << "res" << "pack" << path << outRes;
                if (!prefix.isEmpty()) {
                    args << "--prefix" << prefix;
                }
                proc.setArguments(args);
                proc.start();
                proc.waitForFinished(-1);
                if (proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0) {
                    logMessage("[SUCCESS] Packed folder to " + outRes);
                    QMessageBox::information(this, "Packed", "Successfully packed to:\n" + outRes);
                } else {
                    QString err = proc.readAllStandardError();
                    logMessage("[ERROR] Failed to pack folder to " + outRes + "\n" + err);
                    QMessageBox::critical(this, "Pack Failed", "Error packing folder:\n" + err);
                }
            });
            menu.addSeparator();
        }
        if (!isDir) {
            menu.addAction("Open in Native App", [path]() { QDesktopServices::openUrl(QUrl::fromLocalFile(path)); });

            QMenu* viewMenu = menu.addMenu("View As");
            viewMenu->addAction("Text View", [this, path]() { loadFile(path, 1); });
            viewMenu->addAction("Hex View",  [this, path]() { loadFile(path, 2); });
            viewMenu->addAction("Image View",[this, path]() { loadFile(path, 3); });
            viewMenu->addAction("3D View",   [this, path]() { loadFile(path, 4); });
            menu.addSeparator();
        }


        menu.addSeparator();
        menu.addAction("Rename...", [this, path]() {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename File", "New name:", QLineEdit::Normal, QFileInfo(path).fileName(), &ok);
            if (ok && !newName.isEmpty()) {
                QString newPath = QFileInfo(path).absolutePath() + "/" + newName;
                if (QFileInfo(path).isDir()) {
                    QDir().rename(path, newPath);
                } else {
                    QFile::rename(path, newPath);
                }
            }
        });
        menu.addAction("Delete", [this, path]() {
            if (QMessageBox::question(this, "Delete", "Are you sure you want to delete " + QFileInfo(path).fileName() + "?") == QMessageBox::Yes) {
                if (QFileInfo(path).isDir()) {
                    QDir(path).removeRecursively();
                } else {
                    QFile(path).remove();
                }
            }
        });
        menu.addAction("Cut", [this, path]() { clipboardFilePath = path; clipboardIsCut = true; });
        menu.addAction("Copy", [this, path]() { clipboardFilePath = path; clipboardIsCut = false; });
        if (!clipboardFilePath.isEmpty()) {
            menu.addAction("Paste Here", [this, path, isDir]() {
                QString destDir = isDir ? path : QFileInfo(path).absolutePath();
                QString dest = destDir + "/" + QFileInfo(clipboardFilePath).fileName();
                
                int copyCount = 1;
                while (QFileInfo::exists(dest) && (!clipboardIsCut || dest != clipboardFilePath)) {
                    QString base = QFileInfo(clipboardFilePath).completeBaseName();
                    QString ext = QFileInfo(clipboardFilePath).suffix();
                    if (!ext.isEmpty()) ext = "." + ext;
                    if (copyCount == 1) dest = destDir + "/" + base + "_copy" + ext;
                    else dest = destDir + "/" + base + "_copy" + QString::number(copyCount) + ext;
                    copyCount++;
                }

                if (QFileInfo(clipboardFilePath).isDir()) {
                    QProcess::execute("cmd", QStringList() << "/c" << "xcopy" << QDir::toNativeSeparators(clipboardFilePath) << QDir::toNativeSeparators(dest) << "/E" << "/I" << "/H" << "/Y");
                } else {
                    QFile::copy(clipboardFilePath, dest);
                }
                
                if (clipboardIsCut) {
                    if (QFileInfo(clipboardFilePath).isDir()) QDir(clipboardFilePath).removeRecursively();
                    else QFile(clipboardFilePath).remove();
                    clipboardFilePath = "";
                }
            });
        }
        menu.addSeparator();

        if (isDir) {
            bool hasMefs = false;
            QDir dir(path);
            if (!dir.entryList(QStringList() << "*.mef", QDir::Files).isEmpty()) hasMefs = true;
            if (hasMefs) {
                menu.addSeparator();
                menu.addAction("Apply Textures All (in Folder)", [this, path]() {
                    currentFile = path;
                    executeCommand("mef apply-tex-all");
                });
                menu.addAction("Export All (in Folder)", [this, path]() {
                    currentFile = path;
                    executeCommand("mef export-bundle-all");
                });
            }
        } else if (ext == "tex" || ext == "spr" || ext == "pic") {
            menu.addAction("Convert to PNG", [this, path]() { loadFile(path); executeCommand("tex to-png"); });
            menu.addAction("Convert to TGA", [this, path]() { loadFile(path); executeCommand("tex to-tga"); });
            menu.addAction("Info",           [this, path]() { loadFile(path); executeCommand("tex info"); });
            menu.addAction("Decode Batch",   [this, path]() { loadFile(path); executeCommand("tex decode-batch"); });
        } else if (ext == "png" || ext == "tga" || ext == "bmp" || ext == "jpg" || ext == "jpeg") {
            menu.addAction("Convert to TEX", [this, path]() {
                QString newPath = path.left(path.lastIndexOf('.')) + ".tex";
                QImage img(path);
                if (!img.isNull() && imageEditor->saveAsTex(img, newPath)) {
                    logMessage("[INFO] Converted image to TEX: " + newPath);
                    QMessageBox::information(this, "Success", "Converted to " + newPath);
                } else {
                    logMessage("[ERROR] Failed to convert image to TEX: " + path);
                }
            });
        } else if (ext == "qsc" || ext == "qas") {
            menu.addAction("Compile",        [this, path]() { loadFile(path); executeCommand("qsc compile"); });
            menu.addAction("Validate",       [this, path]() { loadFile(path); executeCommand("qsc validate"); });
        } else if (ext == "qvm") {
            menu.addAction("Decompile",      [this, path]() { loadFile(path); executeCommand("qvm decompile"); });
            menu.addAction("Disassemble",    [this, path]() { loadFile(path); executeCommand("qvm disasm"); });
            menu.addAction("Info",           [this, path]() { loadFile(path); executeCommand("qvm info"); });
        } else if (ext == "mef" || ext == "mex") {
            bool isBinary = true;
            QFile f(path);
            if (f.open(QIODevice::ReadOnly)) {
                QByteArray magic = f.read(4);
                if (magic != "ILFF") {
                    isBinary = false;
                }
                f.close();
            }

            QString cmdPrefix = ext;

            if (isBinary) {
                QMenu* infoMenu = menu.addMenu("Details");
                infoMenu->addAction("Info", [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " info"); });
                infoMenu->addAction("Dump", [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " dump"); });

                QMenu* exportMenu = menu.addMenu("Export");
                exportMenu->addAction("Export to Obj",       [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " export-bundle"); });
                if (ext == "mef") {
                    exportMenu->addAction("Export to Mef(Text)",      [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " to-text"); });
                    exportMenu->addAction("Build Rigid Model", [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " build-rigid"); });
                } else {
                    exportMenu->addAction("Export to Mex(Text)",      [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " to-text"); });
                }

                if (ext == "mef") {

                    // Textures submenu
                    QMenu* texMenu = menu.addMenu("Textures");
                    texMenu->addAction("Apply Textures", [this, path]() {
                        currentFile = path;
                        executeCommand("mef apply-tex");
                        QString baseName = QFileInfo(path).completeBaseName();
                        QString tempDir = globalCacheDir + "/bundle/" + baseName + "/";
                        if (QDir(tempDir).exists()) {
                            logMessage("[INFO] Extracted textures to temp folder. Loading native MEF with textures: " + path);
                            viewModeCombo->setCurrentIndex(4);
                            modelViewer->loadModel(path);
                            modelViewer->show();
                        }
                    });
                    texMenu->addAction("Apply Textures (All)", [this, path]() {
                        currentFile = QFileInfo(path).absolutePath();
                        executeCommand("mef apply-tex-all");
                    });
                    texMenu->addAction("Map Textures", [this, path]() {
                        QString baseName = QFileInfo(path).completeBaseName();
                        if (globalLevelDatPath.isEmpty()) {
                            logMessage("[ERROR] No level DAT set. Go to Settings > Level to set one.");
                            return;
                        }
                        QProcess proc;
                        proc.setProgram(qApp->applicationFilePath());
                        proc.setArguments(QStringList() << "dat" << "export" << globalLevelDatPath);
                        proc.start();
                        proc.waitForFinished(10000);
                        QString datOut = proc.readAllStandardOutput();

                        QString report = QString("=== Texture Map: %1 ===\n").arg(baseName);
                        int modelIdx = datOut.indexOf(QString("\"modelName\": \"%1\"").arg(baseName), 0, Qt::CaseInsensitive);
                        if (modelIdx != -1) {
                            int texStart = datOut.indexOf("\"textures\": [", modelIdx);
                            if (texStart != -1) {
                                int texEnd = datOut.indexOf("]", texStart);
                                QString texArr = datOut.mid(texStart + 12, texEnd - texStart - 12);
                                report += "Textures mapped in DAT:\n" + texArr.replace("\"", "").trimmed() + "\n";
                            } else {
                                report += "No textures found in DAT for this model.\n";
                            }
                        } else {
                            report += "(No texture mapping found in DAT for this model)\n";
                        }
                        QProcess mefProc;
                        mefProc.setProgram(qApp->applicationFilePath());
                        mefProc.setArguments(QStringList() << "mef" << "dump" << path);
                        mefProc.start();
                        mefProc.waitForFinished(5000);
                        QString mefOut = mefProc.readAllStandardOutput();
                        report += "\n=== MEF Materials & Attachments ===\n";
                        for (const QString& line : mefOut.split('\n')) {
                            if (line.startsWith("material") || line.startsWith("ATTA") || line.startsWith("attachment"))
                                report += line.trimmed() + "\n";
                        }
                        logMessage(report);
                    });
                }
            } else {
                menu.addAction("Compile to Binary", [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " compile-text"); });
                QMenu* exportMenu = menu.addMenu("Export");
                exportMenu->addAction("Export to Obj", [this, path, cmdPrefix]() { loadFile(path); executeCommand(cmdPrefix + " export"); });
            }
        } else if (ext == "res") {
            menu.addAction("Extract", [this, path]() { loadFile(path); executeCommand("res extract"); });
            menu.addAction("List",    [this, path]() { loadFile(path); executeCommand("res list"); });
            menu.addAction("Unpack",  [this, path]() { loadFile(path); executeCommand("res unpack"); });
        } else if (ext == "mtp") {
            menu.addAction("Convert",       [this, path]() { loadFile(path); executeCommand("mtp to-dat"); });
            menu.addAction("Dump",          [this, path]() { loadFile(path); executeCommand("mtp dump"); });
            menu.addAction("Info",          [this, path]() { loadFile(path); executeCommand("mtp info"); });
        } else if (ext == "dat") {
            menu.addAction("Convert",       [this, path]() { loadFile(path); executeCommand("dat to-mtp"); });
            menu.addAction("Export",        [this, path]() { loadFile(path); executeCommand("dat export"); });
            menu.addAction("Info",          [this, path]() { loadFile(path); executeCommand("dat info"); });
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
            } else if (currentExt == "mef" || currentExt == "mex") {
                bool isBinary = true;
                QFile f(path);
                if (f.open(QIODevice::ReadOnly)) {
                    QByteArray magic = f.read(4);
                    if (magic != "ILFF") isBinary = false;
                    f.close();
                }
                if (isBinary) {
                    if (currentExt == "mef") mode = 4; // 3D
                    else mode = 2; // Hex
                } else {
                    mode = 1; // Text
                }
            } else if (currentExt == "obj") {
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

        if (mode == 4 && currentExt == "mef") {
            // We directly use native MEF viewing. The textures will be found in temp folder if bundled.
        }

        if (mode == 1) { // Text
            QString loadPath = path;
            if (currentExt == "qvm") {
                consoleEdit->append("[INFO] Decompiling QVM to QSC automatically for viewing...");
                loadPath = QDir::tempPath() + "/igi_temp.qsc";
                QString cmd = qApp->applicationFilePath();
                QProcess::execute(cmd, QStringList() << "qvm" << "decompile" << path << "-o" << loadPath);
            }
            QFile file(loadPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                viewerEdit->setPlainText(QString::fromUtf8(file.readAll()));
            }
            viewerEdit->isHexMode = false;
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
            viewerEdit->isHexMode = true;
            viewerEdit->setReadOnly(false);
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
                                // ARGB1555: bit15=A, bits14-10=R, bits9-5=G, bits4-0=B
                                int r = (c >> 10) & 0x1F; r = (r << 3) | (r >> 2);
                                int g = (c >>  5) & 0x1F; g = (g << 3) | (g >> 2);
                                int b =  c        & 0x1F; b = (b << 3) | (b >> 2);
                                qimg.setPixelColor(x, y, QColor(r, g, b, 255));
                            }
                        }
                    }
                    imageEditor->loadImage(path, qimg);
                } else {
                    imageEditor->clear();
                }
            } else {
                QImage img = loadImageSafe(path);
                if (!img.isNull()) {
                    imageEditor->loadImage(path, img);
                } else {
                    imageEditor->clear();
                }
            }
            imageEditor->show();
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
        } else if (cmd == "mef export-bundle") {
            QString target = currentFile;
            QString outBundle = QFileDialog::getExistingDirectory(this, "Select Export Folder", outDir);
            if (outBundle.isEmpty()) return;
            QString datFile = globalLevelDatPath;
            if (datFile.isEmpty()) datFile = QFileDialog::getOpenFileName(this, "Select DAT File", outDir, "DAT Files (*.dat)");
            if (datFile.isEmpty()) return;
            QString texDir = globalTextureDir;
            if (texDir.isEmpty()) texDir = QFileDialog::getExistingDirectory(this, "Select Textures Directory", outDir);
            if (texDir.isEmpty()) return;
            logMessage(QString("[INFO] Exporting Bundle to %1 using DAT: %2").arg(outBundle, datFile));
            args.clear();
            args << "mef" << "bundle" << target << "-o" << outBundle << "--dat" << datFile << "--texdir" << texDir;
        } else if (cmd == "mef apply-tex") {
            QString target = currentFile;
            QString outBundle = globalCacheDir + "/bundle";
            QString datFile = globalLevelDatPath;
            if (datFile.isEmpty()) {
                datFile = QFileDialog::getOpenFileName(this, "Select DAT File for Bundle", outDir, "DAT Files (*.dat)");
            }
            if (datFile.isEmpty()) return;
            QString texDir = globalTextureDir;
            if (texDir.isEmpty()) {
                texDir = QFileDialog::getExistingDirectory(this, "Select Textures Directory for Bundle", outDir);
            }
            if (texDir.isEmpty()) return;
            logMessage(QString("[INFO] Applying Textures using DAT: %1, TexDir: %2").arg(datFile, texDir));
            args.clear();
            args << "mef" << "bundle" << target << "-o" << outBundle << "--dat" << datFile << "--texdir" << texDir << "--no-obj";
        } else if (cmd == "mef apply-tex-all") {
            // Bundle ALL .mef files in the folder one by one
            QString folderPath = currentFile; // must be a directory
            if (!QFileInfo(folderPath).isDir()) folderPath = QFileInfo(folderPath).absolutePath();
            
            QString datFile = globalLevelDatPath;
            if (datFile.isEmpty()) {
                datFile = QFileDialog::getOpenFileName(this, "Select DAT File for Bundle", folderPath, "DAT Files (*.dat)");
            }
            if (datFile.isEmpty()) return;
            QString texDir = globalTextureDir;
            if (texDir.isEmpty()) {
                texDir = QFileDialog::getExistingDirectory(this, "Select Textures Directory for Bundle", folderPath);
            }
            if (texDir.isEmpty()) return;
            
            QString outBundle = globalCacheDir + "/bundle";
            QDir bundleDir(outBundle);
            bundleDir.mkpath(".");
            
            QDir dir(folderPath);
            QStringList mefs = dir.entryList(QStringList() << "*.mef" << "*.MEF", QDir::Files);
            logMessage(QString("[INFO] Apply Textures to All: found %1 MEF files in %2").arg(mefs.size()).arg(folderPath));
            
            QProgressDialog progress("Extracting textures for all MEFs in folder...", "Cancel", 0, mefs.size(), this);
            progress.setWindowModality(Qt::WindowModal);
            progress.setMinimumDuration(0);
            progress.show();
            
            QString firstModel;
            int bundled = 0;
            for (int i = 0; i < mefs.size(); ++i) {
                if (progress.wasCanceled()) {
                    logMessage("[WARN] Apply Textures All was canceled by user.");
                    break;
                }
                const QString& mefName = mefs[i];
                progress.setValue(i);
                progress.setLabelText(QString("Extracting %1 (%2 of %3)...").arg(mefName).arg(i+1).arg(mefs.size()));
                qApp->processEvents();

                QString mefPath = folderPath + "/" + mefName;
                QString mefBase = QFileInfo(mefName).completeBaseName();
                QStringList bargs;
                bargs << "mef" << "bundle" << mefPath << "-o" << outBundle << "--dat" << datFile << "--texdir" << texDir << "--no-obj";
                logMessage(QString("> igi1conv %1").arg(bargs.join(" ")));
                QProcess proc;
                proc.setProgram(qApp->applicationFilePath());
                proc.setArguments(bargs);
                proc.start();
                proc.waitForFinished(30000);
                QString out = proc.readAllStandardOutput();
                QString err = proc.readAllStandardError();
                if (!out.isEmpty()) logMessage(out.trimmed());
                if (!err.isEmpty()) logMessage("ERROR: " + err.trimmed());
                
                if (firstModel.isEmpty()) {
                    firstModel = folderPath + "/" + mefName; // Load the original MEF file!
                }
                bundled++;
            }
            progress.setValue(mefs.size());
            logMessage(QString("[INFO] Apply Textures All complete: %1/%2 models processed").arg(bundled).arg(mefs.size()));
            if (!firstModel.isEmpty()) {
                modelViewer->loadModel(firstModel);
                viewModeCombo->blockSignals(true);
                viewModeCombo->setCurrentIndex(4);
                viewModeCombo->blockSignals(false);
                hideAllViewers();
                modelViewer->show();
                logMessage("[INFO] Loaded first bundled MEF in 3D View: " + firstModel);
            }
            return; // Already ran all sub-processes above

        } else if (cmd == "mef export-bundle-all") {
            QString folderPath = currentFile;
            if (!QFileInfo(folderPath).isDir()) folderPath = QFileInfo(folderPath).absolutePath();
            
            QString outBundle = QFileDialog::getExistingDirectory(this, "Select Export Folder", folderPath);
            if (outBundle.isEmpty()) return;
            
            QString datFile = globalLevelDatPath;
            if (datFile.isEmpty()) datFile = QFileDialog::getOpenFileName(this, "Select DAT File", folderPath, "DAT Files (*.dat)");
            if (datFile.isEmpty()) return;
            QString texDir = globalTextureDir;
            if (texDir.isEmpty()) texDir = QFileDialog::getExistingDirectory(this, "Select Textures Directory", folderPath);
            if (texDir.isEmpty()) return;
            
            QDir dir(folderPath);
            QStringList mefs = dir.entryList(QStringList() << "*.mef" << "*.MEF", QDir::Files);
            logMessage(QString("[INFO] Export All: found %1 MEF files in %2").arg(mefs.size()).arg(folderPath));
            
            QProgressDialog progress("Exporting all MEFs in folder...", "Cancel", 0, mefs.size(), this);
            progress.setWindowModality(Qt::WindowModal);
            progress.setMinimumDuration(0);
            progress.show();
            
            int bundled = 0;
            for (int i = 0; i < mefs.size(); ++i) {
                if (progress.wasCanceled()) {
                    logMessage("[WARN] Export All was canceled by user.");
                    break;
                }
                const QString& mefName = mefs[i];
                progress.setValue(i);
                progress.setLabelText(QString("Exporting %1 (%2 of %3)...").arg(mefName).arg(i+1).arg(mefs.size()));
                qApp->processEvents();

                QString mefPath = folderPath + "/" + mefName;
                QStringList bargs;
                bargs << "mef" << "bundle" << mefPath << "-o" << outBundle << "--dat" << datFile << "--texdir" << texDir;
                logMessage(QString("> igi1conv %1").arg(bargs.join(" ")));
                QProcess proc;
                proc.setProgram(qApp->applicationFilePath());
                proc.setArguments(bargs);
                proc.start();
                proc.waitForFinished(30000);
                QString out = proc.readAllStandardOutput();
                QString err = proc.readAllStandardError();
                if (!out.isEmpty()) logMessage(out.trimmed());
                if (!err.isEmpty()) logMessage("ERROR: " + err.trimmed());
                bundled++;
            }
            progress.setValue(mefs.size());
            logMessage(QString("[INFO] Export-All complete: %1/%2 models bundled").arg(bundled).arg(mefs.size()));
            return; // Already ran all sub-processes above

        } else if (cmd == "mef to-text" || cmd == "mex to-text") {
            QString outTxt = currentFile;
            args.clear();
            if (cmd == "mex to-text") {
                args << "mef" << "dump" << currentFile << "-o" << outTxt;
                logMessage(QString("[INFO] Dumping MEX text %1 to: %2").arg(currentExt.toUpper(), outTxt));
            } else {
                args << "mef" << "to-text" << currentFile << "-o" << outTxt;
                logMessage(QString("[INFO] Exporting text %1 to: %2").arg(currentExt.toUpper(), outTxt));
            }
        } else if (cmd == "mef compile-text" || cmd == "mex compile-text") {
            QString outBin = currentFile;
            args.clear();
            args << "mef" << "compile" << currentFile << "-o" << outBin;
            logMessage(QString("[INFO] Compiling text %1 to binary: %2").arg(currentExt.toUpper(), outBin));
        } else if (cmd == "res unpack") {
            args << currentFile << (outDir + "/" + baseName + "_unpacked");
        } else if (cmd == "mef export" || cmd == "mex export") {
            QString outFolder = QFileDialog::getExistingDirectory(this, "Select Export Folder", outDir);
            if (outFolder.isEmpty()) return;
            args << currentFile << "-o" << (outFolder + "/" + baseName + ".obj");
        } else if (cmd == "mef build-rigid") {
            QString outFolder = QFileDialog::getExistingDirectory(this, "Select Output Folder for Rigid Model", outDir);
            if (outFolder.isEmpty()) return;
            args << currentFile << "-o" << (outFolder + "/" + baseName + ".mef");
            if (!globalLevelDatPath.isEmpty()) args << "--dat" << globalLevelDatPath;
            if (!globalTextureDir.isEmpty())   args << "--texdir" << globalTextureDir;
        } else {
            args << currentFile;
            if (cmd == "qsc compile") args << "-o" << (outDir + "/" + baseName + ".qvm");
            else if (cmd == "qvm decompile") args << "-o" << (outDir + "/" + baseName + ".qsc");
            else if (cmd == "mef dump" || cmd == "mex dump") args << "-o" << (outDir + "/" + baseName + "_dump.txt");
            else if (cmd == "res extract") args << "-o" << outDir;
            else if (cmd == "fnt export") args << "-o" << (outDir + "/" + baseName + ".png");
            else if (cmd == "qvm disasm") args << "-o" << (outDir + "/" + baseName + ".qas");
        }

        if (args.isEmpty()) return;

        QString progressText = "Executing command...";
        if (cmd.endsWith("build-rigid")) {
            progressText = "Completing Build model finding attachments textures.... Please wait";
        }
        QProgressDialog progress(progressText, "Cancel", 0, 0, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        progress.show();
        qApp->processEvents();

        QProcess process;
        process.setProgram(qApp->applicationFilePath());
        process.setArguments(args);
        
        logMessage(QString("> igi1conv %1").arg(args.join(" ")));
        process.start();
        
        while (!process.waitForFinished(100)) {
            qApp->processEvents();
            if (progress.wasCanceled()) {
                logMessage("[WARN] Command was canceled by user.");
                process.kill();
                process.waitForFinished();
                break;
            }
        }

        QString output = process.readAllStandardOutput();
        QString err = process.readAllStandardError();
        
        if (!output.isEmpty()) logMessage(output.trimmed());
        if (!err.isEmpty()) logMessage("ERROR: " + err.trimmed());
        logMessage("--------------------------------------------------");
    }
};

int run_gui() {
    int argc = 1;
    char arg0[] = "igi1conv.exe";
    char* argv[] = { arg0, nullptr };

    QApplication app(argc, argv);
    app.setStyle("Fusion");
    {
        QIcon ico(":/igi1conv.ico");
        if (!ico.isNull()) app.setWindowIcon(ico);
    }
    QString iniPath = QCoreApplication::applicationDirPath() + "/igi1conv.ini";
    QString theme = QSettings(iniPath, QSettings::IniFormat).value("Theme", "Dark").toString();
    
    auto applyTheme = [&](const QString& t) {
        if (t == "Light") {
            QPalette p;
            p.setColor(QPalette::Window, QColor(240,240,240));
            p.setColor(QPalette::WindowText, Qt::black);
            p.setColor(QPalette::Base, Qt::white);
            p.setColor(QPalette::AlternateBase, QColor(233,233,233));
            p.setColor(QPalette::ToolTipBase, Qt::white);
            p.setColor(QPalette::ToolTipText, Qt::black);
            p.setColor(QPalette::Text, Qt::black);
            p.setColor(QPalette::Button, QColor(240,240,240));
            p.setColor(QPalette::ButtonText, Qt::black);
            p.setColor(QPalette::BrightText, Qt::red);
            p.setColor(QPalette::Link, QColor(0,0,200));
            p.setColor(QPalette::Highlight, QColor(0,120,215));
            p.setColor(QPalette::HighlightedText, Qt::white);
            app.setPalette(p);
            app.setStyleSheet(
                "QTextEdit{background:white;color:black;border:1px solid #ccc;}"
                "QPlainTextEdit{background:white;color:black;border:1px solid #ccc;}"
                "QTreeView{background:white;color:black;alternate-background-color:#f0f0f0;}"
                "QLineEdit{background:white;color:black;border:1px solid #aaa;border-radius:3px;}"
                "QToolBar{background:#ebebeb;border-bottom:1px solid #ccc;spacing:4px;padding:2px 6px;}"
                "QMenuBar{background:#f0f0f0;color:black;}"
                "QMenu{background:#f0f0f0;color:black;}"
                "QStatusBar{background:#f0f0f0;color:black;}"
                "QLabel{color:black;}"
                "QSplitter::handle{background:#ccc;}"
            );
        } else if (t == "Solarized") {
            QPalette p;
            p.setColor(QPalette::Window, QColor(0,43,54));
            p.setColor(QPalette::WindowText, QColor(131,148,150));
            p.setColor(QPalette::Base, QColor(7,54,66));
            p.setColor(QPalette::AlternateBase, QColor(0,43,54));
            p.setColor(QPalette::Text, QColor(131,148,150));
            p.setColor(QPalette::Button, QColor(0,43,54));
            p.setColor(QPalette::ButtonText, QColor(131,148,150));
            p.setColor(QPalette::Highlight, QColor(38,139,210));
            p.setColor(QPalette::HighlightedText, Qt::white);
            app.setPalette(p);
            app.setStyleSheet("");
        } else if (t == "Military") {
            QPalette p;
            p.setColor(QPalette::Window, QColor(30,40,20));
            p.setColor(QPalette::WindowText, QColor(120,200,60));
            p.setColor(QPalette::Base, QColor(15,25,10));
            p.setColor(QPalette::AlternateBase, QColor(30,40,20));
            p.setColor(QPalette::Text, QColor(120,200,60));
            p.setColor(QPalette::Button, QColor(30,40,20));
            p.setColor(QPalette::ButtonText, QColor(120,200,60));
            p.setColor(QPalette::Highlight, QColor(80,140,30));
            p.setColor(QPalette::HighlightedText, Qt::black);
            app.setPalette(p);
            app.setStyleSheet("");
        } else {
            // Dark (default)
            QPalette p;
            p.setColor(QPalette::Window, QColor(53,53,53));
            p.setColor(QPalette::WindowText, Qt::white);
            p.setColor(QPalette::Base, QColor(25,25,25));
            p.setColor(QPalette::AlternateBase, QColor(53,53,53));
            p.setColor(QPalette::ToolTipBase, Qt::white);
            p.setColor(QPalette::ToolTipText, Qt::white);
            p.setColor(QPalette::Text, Qt::white);
            p.setColor(QPalette::Button, QColor(53,53,53));
            p.setColor(QPalette::ButtonText, Qt::white);
            p.setColor(QPalette::BrightText, Qt::red);
            p.setColor(QPalette::Link, QColor(42,130,218));
            p.setColor(QPalette::Highlight, QColor(42,130,218));
            p.setColor(QPalette::HighlightedText, Qt::black);
            app.setPalette(p);
            app.setStyleSheet("");
        }
    };
    applyTheme(theme);

    MainWindow window;
    window.show();

    return app.exec();
}
