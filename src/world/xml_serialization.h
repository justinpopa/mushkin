#ifndef XML_SERIALIZATION_H
#define XML_SERIALIZATION_H

#include <QString>

// Forward declaration
class WorldDocument;
class QFile;

// Import flags for selective XML import
// These match the original MUSHclient XML_* constants
enum ImportFlags {
    XML_TRIGGERS = 0x0001,
    XML_ALIASES = 0x0002,
    XML_TIMERS = 0x0004,
    XML_MACROS = 0x0008, // Accelerators/keypad
    XML_VARIABLES = 0x0010,
    XML_COLOURS = 0x0020,
    XML_KEYPAD = 0x0040,
    XML_PRINTING = 0x0080,
    XML_GENERAL = 0x0100, // General world settings

    // Convenience: import all automation items
    XML_ALL = XML_TRIGGERS | XML_ALIASES | XML_TIMERS | XML_MACROS | XML_VARIABLES | XML_COLOURS |
              XML_KEYPAD | XML_PRINTING | XML_GENERAL
};

// XML serialization functions for MUSHclient world files (.mcl)
//
// MUSHclient world files are XML documents that must maintain backward
// compatibility with the original MFC version. The format is:
//
// <?xml version="1.0" encoding="UTF-8"?>
// <!DOCTYPE muclient>
// <muclient>
//   <world name="..." server="..." port="..." ...>
//     <triggers>...</triggers>
//     <aliases>...</aliases>
//   </world>
// </muclient>

namespace XmlSerialization {

// Detects if a file is an XML world file by checking for BOM and XML signatures
// Checks first 500 bytes for:
// - UTF-8 BOM (EF BB BF)
// - UTF-16LE BOM (FF FE)
// - XML signatures: <?xml, <muclient, <world, etc.
//
// Returns true if file appears to be XML format
bool IsArchiveXML(QFile& file);

// Saves a WorldDocument to an XML file
// Writes all configuration options from OptionsTable and AlphaOptionsTable
// Stubs trigger/alias/timer sections (will be fully implemented later)
//
// Returns true on success, false on failure
bool SaveWorldXML(WorldDocument* doc, const QString& filename);

// Loads a WorldDocument from an XML file
// Reads all configuration options and updates document fields
// Skips trigger/alias/timer sections for now (stubbed)
//
// Returns true on success, false on failure
bool LoadWorldXML(WorldDocument* doc, const QString& filename);

// Imports triggers, aliases, timers, variables from an XML string
// Does NOT import world configuration options (name, server, port, etc.)
// Only imports automation elements for migrating scripts
//
// @param doc WorldDocument to import into
// @param xmlString XML content to parse
// @param flags Bitmask of ImportFlags to control what gets imported (default: XML_ALL)
//
// Returns count of items imported (triggers + aliases + timers + variables)
// Returns -1 if XML is invalid or parsing fails
int ImportXML(WorldDocument* doc, const QString& xmlString, int flags = XML_ALL);

// Exports triggers, aliases, timers, variables to an XML string
// Only exports sections specified by flags bitmask
//
// @param doc WorldDocument to export from
// @param flags Bitmask of ImportFlags to control what gets exported
// @param comment Optional comment to include in XML header
//
// Returns XML string, or empty string on failure
QString ExportXML(WorldDocument* doc, int flags = XML_ALL, const QString& comment = QString());

} // namespace XmlSerialization

#endif // XML_SERIALIZATION_H
