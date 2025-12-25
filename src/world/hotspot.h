#ifndef HOTSPOT_H
#define HOTSPOT_H

#include <QRect>
#include <QString>

/**
 * @brief The Hotspot class represents a clickable region in a miniwindow.
 *
 * Miniwindow Mouse Events
 *
 * Hotspots are rectangular regions that respond to mouse events. They enable:
 * - Click handlers for buttons and controls
 * - Drag-and-drop for movable windows
 * - Mouseover effects and tooltips
 * - Custom cursors
 *
 * CHotspot class
 */
class Hotspot {
  public:
    Hotspot() : m_Cursor(0), m_Flags(0), m_DragFlags(0)
    {
    }

    // ========== Position ==========
    QRect m_rect; // Hotspot rectangle (miniwindow-relative coordinates)

    // ========== Mouse Event Callbacks ==========
    QString m_sMouseOver;       // Function to call when mouse enters hotspot
    QString m_sCancelMouseOver; // Function to call when mouse leaves hotspot
    QString m_sMouseDown;       // Function to call when mouse button pressed
    QString m_sCancelMouseDown; // Function to call when mouse released outside hotspot
    QString m_sMouseUp;         // Function to call when mouse button released in hotspot

    // ========== Tooltip ==========
    QString m_sTooltipText; // Tooltip text to display

    // ========== Cursor ==========
    qint32 m_Cursor; // Cursor type (0=arrow, 1=hand, etc.)

    // ========== Flags ==========
    qint32 m_Flags; // Hotspot flags

    // ========== Drag-and-Drop Callbacks ==========
    QString m_sMoveCallback;    // Function to call during drag (mouse move)
    QString m_sReleaseCallback; // Function to call when drag ends (mouse release)
    qint32 m_DragFlags;         // Drag-and-drop flags

    // ========== Scroll Wheel Callback ==========
    QString m_sScrollwheelCallback; // Function to call on mouse wheel scroll
};

#endif // HOTSPOT_H
