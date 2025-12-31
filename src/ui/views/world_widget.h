#ifndef WORLD_WIDGET_H
#define WORLD_WIDGET_H

#include <QWidget>

// Forward declarations
class WorldDocument;
class OutputView;
class InputView;
class QSplitter;
class QVBoxLayout;
class QLabel;
class QMdiSubWindow;

/**
 * WorldWidget - MDI child widget for one MUD connection
 *
 * Combines WorldDocument (data) with OutputView (display) and InputView (input).
 * This is the Qt equivalent of MFC's CChildFrame + CMUSHView + CSendView.
 *
 * Architecture:
 *   WorldWidget
 *     ├─ QSplitter (vertical)
 *     │   ├─ OutputView (top pane) - displays MUD text
 *     │   └─ InputView (bottom pane) - command input
 *     └─ WorldDocument (data model)
 *
 * For , we use placeholder widgets:
 * - QTextEdit for OutputView (will be replaced in )
 *   - QLineEdit for InputView (will be replaced later)
 *
 * Source: childfrm.cpp (CChildFrame class)
 */
class WorldWidget : public QWidget {
    Q_OBJECT

  public:
    explicit WorldWidget(QWidget* parent = nullptr);
    ~WorldWidget() override;

    // Access to document
    WorldDocument* document() const
    {
        return m_document;
    }

    // Access to views
    OutputView* outputView() const
    {
        return m_outputView;
    }
    InputView* inputView() const
    {
        return m_inputView;
    }

    // World state
    QString worldName() const;
    QString serverAddress() const;
    QString filename() const
    {
        return m_filename;
    }
    bool isModified() const
    {
        return m_modified;
    }
    void setModified(bool modified);

    // File operations
    bool loadFromFile(const QString& filename);
    bool saveToFile(const QString& filename);

    // Connection state
    bool isConnected() const
    {
        return m_connected;
    }
    void setConnected(bool connected);

  signals:
    void modifiedChanged(bool modified);
    void connectedChanged(bool connected);
    void windowTitleChanged(const QString& title);
    void notepadRequested(class NotepadWidget* notepad); // Forwarded from WorldDocument

  public slots:
    // Input handling (stub for now)
    void sendCommand();

#ifdef Q_OS_MACOS
    // Update frame visibility based on maximize state
    void updateFrameForWindowState(Qt::WindowStates state);
#endif

    // Connection control
    void connectToMud();
    void disconnectFromMud();

    // Focus management
    void activateInputArea();

    // Command history management
    void previousCommand();
    void nextCommand();
    void repeatLastCommand();
    void clearCommandHistory();

  protected:
    // Event handling for keyboard shortcuts
    void keyPressEvent(QKeyEvent* event) override;

#ifdef Q_OS_MACOS
    // Resize handling for frameless windows
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

  private:
    int getResizeEdges(const QPoint& pos) const;
    Qt::CursorShape cursorForEdges(int edges) const;
#endif

  private:
    void setupUi();
    void updateWindowTitle();
    void updateInfoBar(); // Update info bar appearance from document state

    // Components
    WorldDocument* m_document; // The world data (we own this)
    QSplitter* m_splitter;     // Vertical splitter
    OutputView* m_outputView;  // Custom text display widget
    InputView* m_inputView;    // Custom input widget with history
    QLabel* m_infoBar;         // Script-controllable info bar
#ifdef Q_OS_MACOS
    QWidget* m_titleBar;       // Custom title bar for macOS MDI
    QLabel* m_titleLabel;      // Title label in custom title bar
#endif

    // State
    bool m_modified;    // Has unsaved changes
    bool m_connected;   // Connected to MUD
    QString m_filename; // Current file path (empty for new worlds)

#ifdef Q_OS_MACOS
    // Resize state for frameless windows
    enum ResizeEdge { NoEdge = 0, Left = 1, Right = 2, Top = 4, Bottom = 8 };
    int m_resizeEdges = NoEdge;
    QPoint m_resizeStartPos;
    QRect m_resizeStartGeometry;
    static constexpr int ResizeMargin = 6; // Pixels from edge to trigger resize

    // Drag state for title bar
    bool m_dragging = false;
    QPoint m_dragStartPos;
#endif
};

#endif // WORLD_WIDGET_H
