# Bond SimpleJSON Map Encoding Format Change

## Summary
Changed Bond SimpleJSON encoding to use standard JSON maps with string keys instead of mixed-type arrays.

## Before (Old Format)
Maps were encoded as arrays with alternating key-value pairs:
```json
{
  "myMap": [
    "key1", "value1",
    "key2", "value2",
    123, "value3"
  ]
}
```

## After (New Format)
Maps are now encoded as standard JSON objects:
```json
{
  "myMap": {
    "key1": "value1",
    "key2": "value2", 
    "123": "value3"
  }
}
```

## Key Changes

### C++ Implementation
- **Writer**: `simple_json_writer.h` now uses `{` and `}` for maps instead of `[` and `]`
- **Reader**: `simple_json_reader_impl.h` iterates over JSON object members instead of array elements
- **Key Conversion**: Added `ConvertStringToKey` functions to convert string keys back to original types

### Java Implementation  
- **Writer**: `SimpleJsonWriter.java` tracks container types and writes map keys as JSON field names
- **Reader**: `SimpleJsonReader.java` handles both arrays (legacy) and objects (new format)
- **Type Safety**: Added comprehensive string-to-type conversion methods for all Bond data types

## Benefits
1. **Standard JSON**: Uses native JSON object syntax that all JSON parsers understand
2. **Better Tooling**: JSON viewers, editors, and validators work correctly with maps
3. **Interoperability**: Other systems can easily consume Bond JSON without special handling
4. **Readability**: Much more intuitive format for humans to read and write

## Backward Compatibility
The Java reader implementation can handle both old array format and new object format for smooth migration.

## Implementation Status
- ✅ C++ SimpleJSON writer and reader updated
- ✅ Java SimpleJSON writer updated with container type tracking
- ✅ Java SimpleJSON reader updated with map key conversion
- ✅ All Bond data types supported for map keys
- ⚠️ Compilation testing limited by missing Bond framework dependencies