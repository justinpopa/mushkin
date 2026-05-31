#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/**
 * MUSHclient error codes
 *
 * Source: errors.h from original MUSHclient
 *
 * These integer codes are returned by Lua API functions and must match the
 * original MUSHclient values exactly for plugin compatibility. They are
 * exposed to Lua scripts via the error_code global table.
 *
 * This header is intentionally dependency-free (no Lua, no Qt) so that any
 * translation unit in the project can use symbolic error codes.
 */
enum {
    eOK = 0,                            // no error
    eWorldOpen = 30001,                 // the world is already open
    eWorldClosed = 30002,               // the world is closed, this action cannot be performed
    eNoNameSpecified = 30003,           // no name has been specified where one is required
    eCannotPlaySound = 30004,           // the sound file could not be played
    eTriggerNotFound = 30005,           // the specified trigger name does not exist
    eTriggerAlreadyExists = 30006,      // attempt to add a trigger that already exists
    eTriggerCannotBeEmpty = 30007,      // the trigger "match" string cannot be empty
    eInvalidObjectLabel = 30008,        // the name of this object is invalid
    eScriptNameNotLocated = 30009,      // script name is not in the script file
    eAliasNotFound = 30010,             // the specified alias name does not exist
    eAliasAlreadyExists = 30011,        // attempt to add an alias that already exists
    eAliasCannotBeEmpty = 30012,        // the alias "match" string cannot be empty
    eCouldNotOpenFile = 30013,          // unable to open requested file
    eLogFileNotOpen = 30014,            // log file was not open
    eLogFileAlreadyOpen = 30015,        // log file was already open
    eLogFileBadWrite = 30016,           // bad write to log file
    eTimerNotFound = 30017,             // the specified timer name does not exist
    eTimerAlreadyExists = 30018,        // attempt to add a timer that already exists
    eVariableNotFound = 30019,          // attempt to delete a variable that does not exist
    eCommandNotEmpty = 30020,           // attempt to use SetCommand with a non-empty command window
    eBadRegularExpression = 30021,      // bad regular expression syntax
    eTimeInvalid = 30022,               // time given to AddTimer is invalid
    eBadMapItem = 30023,                // map item contains invalid characters, or is empty
    eNoMapItems = 30024,                // no items in mapper
    eUnknownOption = 30025,             // option name not found
    eOptionOutOfRange = 30026,          // new value for option is out of range
    eTriggerSequenceOutOfRange = 30027, // trigger sequence value invalid
    eTriggerSendToInvalid = 30028,      // where to send trigger text to is invalid
    eTriggerLabelNotSpecified = 30029, // trigger label not specified/invalid for 'send to variable'
    ePluginFileNotFound = 30030,       // file name specified for plugin not found
    eProblemsLoadingPlugin = 30031,    // there was a parsing or other problem loading the plugin
    ePluginCannotSetOption = 30032,    // plugin is not allowed to set this option
    ePluginCannotGetOption = 30033,    // plugin is not allowed to get this option
    eNoSuchPlugin = 30034,             // plugin is not installed
    eNotAPlugin = 30035,               // only a plugin can do this
    eNoSuchRoutine = 30036,            // plugin does not support that routine
    ePluginDoesNotSaveState = 30037,   // plugin does not support saving state
    ePluginCouldNotSaveState = 30038,  // plugin could not save state (eg. no directory)
    ePluginDisabled = 30039,           // plugin is currently disabled
    eErrorCallingPluginRoutine = 30040,  // could not call plugin routine
    eCommandsNestedTooDeeply = 30041,    // calls to "Execute" nested too deeply
    eBadParameter = 30046,               // general problem with a parameter to a script call
    eClipboardEmpty = 30050,             // cannot get (text from the) clipboard
    eFileNotFound = 30051,               // cannot open the specified file
    eAlreadyTransferringFile = 30052,    // already transferring a file
    eNotTransferringFile = 30053,        // not transferring a file
    eNoSuchCommand = 30054,              // there is not a command of that name
    eArrayAlreadyExists = 30055,         // that array already exists
    eArrayDoesNotExist = 30056,          // that array does not exist
    eArrayNotEvenNumberOfValues = 30057, // values to be imported into array are not in pairs
    eImportedWithDuplicates = 30058,     // import succeeded, however some values were overwritten
    eBadDelimiter = 30059,               // import/export delimiter must be a single character
    eSetReplacingExistingValue = 30060,  // array element set, existing value overwritten
    eKeyDoesNotExist = 30061,            // array key does not exist
    eCannotImport = 30062,          // cannot import because cannot find unused temporary character
    eItemInUse = 30063,             // cannot delete trigger/alias/timer because it is executing
    eSpellCheckNotActive = 30064,   // spell checker is not active
    eCannotAddFont = 30065,         // cannot create requested font
    ePenStyleNotValid = 30066,      // invalid settings for pen parameter
    eUnableToLoadImage = 30067,     // bitmap image could not be loaded
    eImageNotInstalled = 30068,     // image has not been loaded into window
    eInvalidNumberOfPoints = 30069, // number of points supplied is incorrect
    eInvalidPoint = 30070,          // point is not numeric
    eHotspotPluginChanged = 30071,  // hotspot processing must all be in same plugin
    eHotspotNotInstalled = 30072,   // hotspot has not been defined for this window
    eNoSuchWindow = 30073,          // requested miniwindow does not exist
    eBrushStyleNotValid = 30074,    // invalid settings for brush parameter
    eNoSuchNotepad = 30075,         // requested notepad does not exist
    eFileNotOpened = 30076,         // could not open file for writing
    eInvalidColourName = 30077,     // invalid color name or format
};

#endif // ERROR_CODES_H
