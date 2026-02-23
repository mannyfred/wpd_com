# wpd_com
Windows Portable Device COM BOF

Supports enumeration and file listing.

Currently supports retrieving extra properties, but not executing commands (too cancerous).

### Usage

```
wpd_com enum [--imp]
wpd_com get_prop <device_index> --prop-guid <guid> --prop-id <id> [--imp] [--access 0xXXXX]
wpd_com ls <device_index> [--path <value>] [--recursion <value>] [--imp] [--access 0xXXXX]
```


Enumerate portable devices, display their functional category, supported commands and properties:
```
>>> wpd_com enum

[*] Number of devices: 1
[0] Friendly name: LePod
[0] Description: iPod
[0] Manufacturer: Apple
[0] Functional Cat. {23F05BBC-15DE-4C2A-A55B-A9AF5CE412EF}
[0] Functional Obj. E:\
[0] Supported CMDs (32)

    GUID: {B7474E91-E7F8-4AD9-B400-AD1A4B58EEEC}
    CMDs: 2 3 4
    GUID: {EF1E43DD-A9ED-4341-8BCC-186192AEA089}
    CMDs: 7 2 3 4 5 6
    GUID: {9E5582E4-0814-44E6-981A-B2998D583804}
    CMDs: 2 4 6 5 3 7
    GUID: {B3A2B22D-A595-4108-BE0A-FC3C965F3D4A}
    CMDs: 2 3 4 5 6 7 8
    GUID: {0CABEC78-6B74-41C6-9216-2639D1FCE356}
    CMDs: 2 3 4 10 11 5 6 7 8 9
[0] Supported Properties (18)

    GUID: {EF6B490D-5CD8-437A-AFFC-DA8B60EE4A3C}
    CMDs: 2
    GUID: {8F052D93-ABCA-4FC5-A5AC-B01DF4DBE598}
    CMDs: 2
    GUID: {EF6B490D-5CD8-437A-AFFC-DA8B60EE4A3C}
    CMDs: 3 5 4 7 6 26 9
    GUID: {26D4979A-E643-4626-9E2B-736DC0C92FDC}
    CMDs: 6 15 8 7 3 12 9 5 4
```
_Returns 0x8002802B when no portable devices found_

List files in specific directory, specify recursion level (default is 2), use impersonation:
```
>>> wpd_com ls 0 --recursion 3 --imp --path E:\iPod_Control\Device

E:\iPod_Control\Device\Radio
E:\iPod_Control\Device\Radio\RadioBuffer
E:\iPod_Control\Device\Radio\Presets_US_FM.plist
E:\iPod_Control\Device\Radio\Locals_US_FM.plist
E:\iPod_Control\Device\Radio\Presets_EU_FM.plist
E:\iPod_Control\Device\Radio\Locals_EU_FM.plist
E:\iPod_Control\Device\iPodSettings.xml
...
```

Get more properties. WPD_DEVICE_PROPERTIES_V1 - WPD_DEVICE_FIRMWARE_VERSION and use impersonation:
```
>>> wpd_com get_prop 0 --imp --prop-guid 26D4979A-E643-4626-9E2B-736DC0C92FDC --prop-id 3

1.70
```
_Returns 0x80004004 when return type is not supported, add support yourself_

---

Functional Categories:
```
WPD_FUNCTIONAL_CATEGORY_DEVICE                  - {08EA466B-E3A4-4336-A1F3-A44D2B5C438C}
WPD_FUNCTIONAL_CATEGORY_STORAGE                 - {23F05BBC-15DE-4C2A-A55B-A9AF5CE412EF}
WPD_FUNCTIONAL_CATEGORY_STILL_IMAGE_CAPTURE     - {613CA327-AB93-4900-B4FA-895BB5874B79}
WPD_FUNCTIONAL_CATEGORY_AUDIO_CAPTURE           - {3F2A1919-C7C2-4A00-855D-F57CF06DEBBB}
WPD_FUNCTIONAL_CATEGORY_VIDEO_CAPTURE           - {E23E5F6B-7243-43AA-8DF1-0EB3D968A918}
WPD_FUNCTIONAL_CATEGORY_SMS                     - {0044A0B1-C1E9-4AFD-B358-A62C6117C9CF}
WPD_FUNCTIONAL_CATEGORY_RENDERING_INFORMATION   - {08600BA4-A7BA-4A01-AB0E-0065D0A356D3}
WPD_FUNCTIONAL_CATEGORY_NETWORK_CONFIGURATION   - {48F4DB72-7C6A-4AB0-9E1A-470E3CDBF26A}
WPD_FUNCTIONAL_CATEGORY_ALL                     - {2D8A6512-A74C-448E-BA8A-F4AC07C49399}
```

[Commands (some examples)](https://learn.microsoft.com/en-us/windows/win32/wpd_sdk/commands):
```
WPD_CATEGORY_OBJECT_ENUMERATION - {B7474E91-E7F8-4AD9-B400-AD1A4B58EEEC}
    WPD_COMMAND_OBJECT_ENUMERATION_START_FIND   - 2
    WPD_COMMAND_OBJECT_ENUMERATION_FIND_NEXT    - 3
    WPD_COMMAND_OBJECT_ENUMERATION_END_FIND     - 4

WPD_CATEGORY_OBJECT_PROPERTIES  - {9E5582E4-0814-44E6-981A-B2998D583804}
    WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED - 2
    WPD_COMMAND_OBJECT_PROPERTIES_GET_ATTRIBUTES- 3
    WPD_COMMAND_OBJECT_PROPERTIES_GET           - 4
    WPD_COMMAND_OBJECT_PROPERTIES_SET           - 5
    WPD_COMMAND_OBJECT_PROPERTIES_GET_ALL       - 6
    WPD_COMMAND_OBJECT_PROPERTIES_DELETE        - 7

WPD_CATEGORY_OBJECT_RESOURCES   - {B3A2B22D-A595-4108-BE0A-FC3C965F3D4A}
    WPD_COMMAND_OBJECT_RESOURCES_WRITE          - 6
    ...

WPD_CATEGORY_OBJECT_MANAGEMENT  - {EF1E43DD-A9ED-4341-8BCC-186192AEA089}
    ...
```

Supported Properties (some examples):
```
WPD_OBJECT_PROPERTIES_V1    - {EF6B490D-5CD8-437A-AFFC-DA8B60EE4A3C}
    WPD_OBJECT_ID                   - 2
    WPD_OBJECT_PARENT_ID            - 3
    WPD_OBJECT_ISHIDDEN             - 9
    WPD_OBJECT_CAN_DELETE           - 26

WPD_DEVICE_PROPERTIES_V1    - {26D4979A-E643-4626-9E2B-736DC0C92FDC}
    WPD_DEVICE_FIRMWARE_VERSION     - 3
    WPD_DEVICE_POWER_LEVEL          - 4
    WPD_DEVICE_POWER_SOURCE         - 5
    WPD_DEVICE_PROTOCOL             - 6
    WPD_DEVICE_MANUFACTURER         - 7
    WPD_DEVICE_MODEL                - 8
    WPD_DEVICE_SERIAL_NUMBER        - 9
    WPD_DEVICE_TYPE                 - 15
    WPD_DEVICE_NETWORK_IDENTIFIER   - 16
```

_Just check `PortableDevice.h`_

### Acknowledgments

[@joaoviictorti]() for random info nuggets