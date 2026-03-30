#include <windows.h>
#include "PortableDevice.h"
#include "PortableDeviceApi.h"


#define SAFE_RELEASE_CPP(ptr)   \
        if ((ptr) != nullptr)   \
        {                       \
            (ptr)->Release();   \
            (ptr) = nullptr;    \
        }


#ifdef DEBUG
#define DEBUG_PRINT_ERR(func, res) { BeaconPrintf(CALLBACK_ERROR, "[!] %d::%s -> 0x%0.8X\n", __LINE__, func, res); }
#else
#define DEBUG_PRINT_ERR(func, res) { ; }
#endif


static const GUID g_IPortableDevice                 = { 0x625e2df8, 0x6392, 0x4cf0, { 0x9a, 0xd1, 0x3c, 0xfa, 0x5f, 0x17, 0x77, 0x5c } };
static const GUID g_PortableDeviceFTM               = { 0xf7c0039a, 0x4762, 0x488a, { 0xb4, 0xb3, 0x76, 0x0e, 0xf9, 0xa1, 0xba, 0x9b } };
static const GUID g_PortableDeviceManager           = { 0x0af10cec, 0x2ecd, 0x4b92, { 0x95, 0x81, 0x34, 0xf6, 0xae, 0x06, 0x37, 0xf3 } };
static const GUID g_IPortableDeviceManager          = { 0xa1567595, 0x4c2f, 0x4574, { 0xa6, 0xfa, 0xec, 0xef, 0x91, 0x7b, 0x9a, 0x40 } };
static const GUID g_PortableDeviceValues            = { 0x0c15d503, 0xd017, 0x47ce, { 0x90, 0x16, 0x7b, 0x3f, 0x97, 0x87, 0x21, 0xcc } };
static const GUID g_IPortableDeviceValues           = { 0x6848f6f2, 0x3155, 0x4f86, { 0xb6, 0xf5, 0x26, 0x3e, 0xee, 0xab, 0x31, 0x43 } };
static const GUID g_PortableDeviceKeyCollection     = { 0xde2d022d, 0x2480, 0x43be, { 0x97, 0xf0, 0xd1, 0xfa, 0x2c, 0xf9, 0x8f, 0x4f } };
static const GUID g_IPortableDeviceKeyCollection    = { 0xdada2357, 0xe0ad, 0x492e, { 0x98, 0xdb, 0xdd, 0x61, 0xc5, 0x3b, 0xa3, 0x53 } };


extern "C" {

#include "beacon.h"

    void go(char*, int);

    WINBASEAPI size_t   __cdecl MSVCRT$wcslen(const wchar_t*);
    WINBASEAPI void     __cdecl MSVCRT$memset(void*, int, size_t);
    WINBASEAPI int      __cdecl MSVCRT$memcmp(const void*, const void*, size_t);
    WINBASEAPI int      __cdecl MSVCRT$_vsnwprintf_s(wchar_t*, size_t, size_t, const wchar_t*, va_list);

    WINBASEAPI HANDLE   WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI BOOL     WINAPI KERNEL32$HeapFree(HANDLE, DWORD, LPVOID);
    WINBASEAPI LPVOID   WINAPI KERNEL32$HeapAlloc(HANDLE, DWORD, SIZE_T);

    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoUninitialize();
    DECLSPEC_IMPORT VOID    WINAPI OLE32$CoTaskMemFree(LPVOID);
    DECLSPEC_IMPORT int     WINAPI OLE32$StringFromGUID2(REFGUID, LPOLESTR, int);
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID, DWORD);
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$PropVariantClear(PROPVARIANT*);
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CreateStreamOnHGlobal(HGLOBAL, BOOL, LPSTREAM*);
}


LPSTREAM lpStream = (LPSTREAM)1;


void BeaconOutputStreamW() {

    STATSTG         ssStreamData;
    SIZE_T          cbSize;
    ULONG           cbRead;
    LARGE_INTEGER   pos;
    LPWSTR          lpwOutput = nullptr;

    if ( lpStream <= (LPSTREAM)1 )
        return;

    if ( FAILED( lpStream->Stat( &ssStreamData, STATFLAG_NONAME ) ) )
        return;
    
    cbSize      = ssStreamData.cbSize.LowPart;
    lpwOutput   = (LPWSTR)KERNEL32$HeapAlloc( KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, cbSize + 1 );

    if ( lpwOutput != nullptr ) {

        pos.QuadPart = 0;

        if ( FAILED( lpStream->Seek( pos, STREAM_SEEK_SET, nullptr ) ) )
            goto _End;

        if ( FAILED( lpStream->Read( lpwOutput, (ULONG)cbSize, &cbRead ) ) )
            goto _End;

        BeaconPrintf( CALLBACK_OUTPUT, "%ls", lpwOutput );
    }

_End:

    SAFE_RELEASE_CPP( lpStream );

    if ( lpwOutput != nullptr ) {
        MSVCRT$memset( lpwOutput, 0, cbSize + 1 );
        KERNEL32$HeapFree( KERNEL32$GetProcessHeap(), 0, lpwOutput );
    }
}


void BeaconPrintToStreamW( LPCWSTR lpwFormat, ... ) {

    va_list argList;
    DWORD   dwWritten;
    wchar_t chBuffer[1024];

    if ( lpStream <= (LPSTREAM)1 ) {

        if ( FAILED( OLE32$CreateStreamOnHGlobal( nullptr, true, &lpStream ) ) )
            goto _End;
    }

    va_start( argList, lpwFormat );
    MSVCRT$memset( chBuffer, 0, sizeof(chBuffer) );

    if ( !MSVCRT$_vsnwprintf_s( chBuffer, _countof(chBuffer), _TRUNCATE, lpwFormat, argList ) ) {
        goto _End;
    }

    lpStream->Write( chBuffer, (ULONG)MSVCRT$wcslen( chBuffer ) * sizeof(wchar_t), &dwWritten );

_End:
    va_end( argList );
}


HRESULT get_device( wchar_t* lpID, bool bImpersonate, ULONG desired_access, IPortableDevice** device ) {

    HRESULT     hr;                 // WPD_CLIENT_INFORMATION_PROPERTIES_V1
    PROPERTYKEY pk_cmd = { .fmtid = { 0x204D9F0C, 0x2292, 0x4080, { 0x9F, 0x42, 0x40, 0x66, 0x4E, 0x70, 0xF8, 0x59 } } };

    IPortableDevice*        portDevice          = nullptr;
    IPortableDeviceValues*  portDeviceValues    = nullptr;

    if ( FAILED( hr = OLE32$CoCreateInstance( g_PortableDeviceValues, nullptr, CLSCTX_INPROC_SERVER, g_IPortableDeviceValues, (void**)&portDeviceValues ) ) ) {
        DEBUG_PRINT_ERR( "CoCreateInstance", hr );
        goto _End;
    }

    if ( FAILED( hr = OLE32$CoCreateInstance( g_PortableDeviceFTM, nullptr, CLSCTX_INPROC_SERVER, g_IPortableDevice, (void**)&portDevice ) ) ) {
        DEBUG_PRINT_ERR( "CoCreateInstance", hr );
        goto _End;
    }

    if ( bImpersonate ) {

        // WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE
        pk_cmd.pid   = 8;

        if ( FAILED( hr = portDeviceValues->SetUnsignedIntegerValue( pk_cmd, SECURITY_IMPERSONATION ) ) ) {
            DEBUG_PRINT_ERR( "IPortableDeviceValues::SetUnsignedIntegerValue", hr );
	        goto _End;
        }

        //BeaconPrintToStreamW( L"[*] Set impersonation\n" );
    }

    if ( desired_access ) {

        // WPD_CLIENT_DESIRED_ACCESS
        pk_cmd.pid   = 9;

        if ( FAILED( hr = portDeviceValues->SetUnsignedIntegerValue( pk_cmd, SECURITY_IMPERSONATION ) ) ) {
            DEBUG_PRINT_ERR( "IPortableDeviceValues::SetUnsignedIntegerValue", hr );
	        goto _End;
        }

        //BeaconPrintToStreamW( L"[*] Set desired_access to 0x%0.8X\n", desired_access );
    }

    if ( FAILED( hr = portDevice->Open( lpID, portDeviceValues ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDevice::Open", hr );
        goto _End;
    }

    *device = portDevice;

_End:
    SAFE_RELEASE_CPP( portDeviceValues );
    return hr;
}


HRESULT get_property( wchar_t* lpID, bool bImpersonate, datap Parser ) {

    HRESULT     hr;
    PROPERTYKEY pk_cmd;
    PROPVARIANT pv  = {0};

    IPortableDevice*                portDevice          = nullptr;
    IPortableDeviceValues*          portDeviceValues    = nullptr;
    IPortableDeviceContent*         portDeviceContent   = nullptr;
    IPortableDeviceProperties*      portDeviceProps     = nullptr;
    IPortableDeviceKeyCollection*   portDeviceColl      = nullptr;

    ULONG access    = BeaconDataInt( &Parser );
    auto  cmd_guid  = reinterpret_cast<GUID*>( BeaconDataExtract( &Parser, nullptr ) );
    auto  cmd_id    = BeaconDataInt( &Parser );

    if ( FAILED( hr = get_device( lpID, bImpersonate, access, &portDevice ) ) ) {
        goto _End;
    }

    if ( FAILED( hr = portDevice->Content( &portDeviceContent ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDevice::Content", hr );
        goto _End;
    }

    if ( FAILED( hr = portDeviceContent->Properties( &portDeviceProps ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDeviceContent::Properties", hr );
        goto _End;
    }

    if ( FAILED( hr = OLE32$CoCreateInstance( g_PortableDeviceKeyCollection, nullptr, CLSCTX_INPROC_SERVER, g_IPortableDeviceKeyCollection, (void**)&portDeviceColl ) ) ) {
        DEBUG_PRINT_ERR( "CoCreateInstance", hr );
        goto _End;
    }

    pk_cmd.fmtid = *cmd_guid;
    pk_cmd.pid   = cmd_id;

    portDeviceColl->Add( pk_cmd );

    if ( FAILED( hr = portDeviceProps->GetValues( WPD_DEVICE_OBJECT_ID, portDeviceColl, &portDeviceValues ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDeviceProperties::GetValues", hr );
        goto _End; 
    }

    if ( FAILED( hr = portDeviceValues->GetValue( pk_cmd, &pv ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDeviceValues::GetValue", hr );
        goto _End; 
    }

    switch ( pv.vt ) {
        case VT_ERROR:
            hr = pv.scode;
            break;
        case VT_LPWSTR:
            BeaconPrintToStreamW( L"%ws", pv.pwszVal );
            break;
        case VT_UI4:
            BeaconPrintToStreamW( L"%lu", pv.ulVal );
            break;
        case VT_UI8:
            BeaconPrintToStreamW( L"%llu", pv.uhVal );
            break;
        case VT_BOOL:
            BeaconPrintToStreamW( L"%d", pv.boolVal );
            break;
        default:
            hr = E_ABORT;
            break;
    }

_End:
    OLE32$PropVariantClear( &pv );
    SAFE_RELEASE_CPP( portDeviceValues );
    SAFE_RELEASE_CPP( portDeviceColl );
    SAFE_RELEASE_CPP( portDeviceProps );
    SAFE_RELEASE_CPP( portDeviceContent );
    SAFE_RELEASE_CPP( portDevice );
    return hr;
}


HRESULT get_listing( wchar_t* lpID, IPortableDeviceContent* content, int max_depth, int current_depth, int* cnt, int threshold ) {

    HRESULT hr;
    IEnumPortableDeviceObjectIDs*   portDeviceObjectIDs = nullptr;

    if ( current_depth > max_depth ) {
        return S_OK;
    }

    if ( FAILED( hr = content->EnumObjects( 0, lpID, nullptr, &portDeviceObjectIDs ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDeviceContent::EnumObjects", hr );
		goto _End;
    }

    while ( true ) {

        wchar_t*    names[4] = { 0 };
        DWORD       fetched  = 0;

        if ( FAILED( hr = portDeviceObjectIDs->Next( 4, names, &fetched ) ) ) {
            DEBUG_PRINT_ERR( "IEnumPortableDeviceObjectIDs::Next", hr );
            break;
        }

        if ( fetched == 0 ) {
            hr = S_OK;
            break;
        }

        for ( int i = 0; i < fetched; i++ ) {

            if ( names[ i ] == nullptr ) {
                continue;
            }

            (*cnt)++;

            if ( *cnt >= threshold ) {
                BeaconOutputStreamW();
                *cnt = 0;
            }

            BeaconPrintToStreamW( L"%ws\n", names[ i ] );

            if ( current_depth < max_depth ) {
                hr = get_listing( names[ i ], content, max_depth, current_depth + 1, cnt, threshold );
            }

            OLE32$CoTaskMemFree( names[ i ] );
            names[ i ] = nullptr;

        }

        if ( hr == S_FALSE ) {
            hr = S_OK;
            break;
        }
    }

_End:
    SAFE_RELEASE_CPP( portDeviceObjectIDs );
    return hr;
}


HRESULT list_files( wchar_t* lpID, bool bImpersonate, datap Parser  ) {

    int                             recursion,
                                    loop_cnt = 0;

    HRESULT                         hr;
    ULONG                           access; 
    wchar_t*                        path                = nullptr;
    IPortableDevice*                portDevice          = nullptr;
    IPortableDeviceContent*         portDeviceContent   = nullptr;
    IEnumPortableDeviceObjectIDs*   portDeviceObjectIDs = nullptr;

    access      = BeaconDataInt( &Parser );
    recursion   = BeaconDataInt( &Parser );
    path        = (wchar_t*)BeaconDataExtract( &Parser, nullptr );

    if ( !path ) {
        path = WPD_DEVICE_OBJECT_ID;
    }

    if ( FAILED( hr = get_device( lpID, bImpersonate, access, &portDevice ) ) ) {
        goto _End;
    }

    if ( FAILED( hr = portDevice->Content( &portDeviceContent ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDevice::Content", hr );
		goto _End;
    }

    hr = get_listing( path, portDeviceContent, recursion, 0, &loop_cnt, 20 );

_End:
    SAFE_RELEASE_CPP( portDeviceObjectIDs );
    SAFE_RELEASE_CPP( portDeviceContent );
    SAFE_RELEASE_CPP( portDevice );
    return hr;
}


void enum_devices( IPortableDeviceManager* deviceManager, wchar_t** lpMemIDs, bool bImpersonate, DWORD cnt ) {

    auto get_info = [ &deviceManager, &lpMemIDs ]( int idx, wchar_t* output, decltype( &IPortableDeviceManager::GetDeviceManufacturer ) fn ) -> void {

        DWORD       name_len = 0;
        wchar_t*    name = nullptr;

        ( deviceManager->*fn )( lpMemIDs[ idx ], nullptr, &name_len );

        if ( name_len > 0 ) {
            name = reinterpret_cast<wchar_t*>( KERNEL32$HeapAlloc( KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, name_len * sizeof(wchar_t) ) );
            ( deviceManager->*fn )( lpMemIDs[ idx ], name, &name_len );
        }

        BeaconPrintToStreamW( L"[%d] %ws: %ws\n", idx, output, name );

        if ( name ) {
            MSVCRT$memset( name, 0, name_len );
            KERNEL32$HeapFree( KERNEL32$GetProcessHeap(), 0, name );
        }
    };

	auto get_capabilities = [ &deviceManager, &lpMemIDs, &bImpersonate ]( int idx ) -> void {

		HRESULT     hr;
		GUID	    last_guid;
        PROPERTYKEY pk_cmd;
		DWORD	    num_funcs   = 0,
				    num_objs    = 0,
				    num_cmds    = 0,
                    num_prop    = 0;

        IUnknown*                               iunknown                = nullptr;
		IPortableDevice*                        portDevice              = nullptr;
        IPortableDeviceValues*                  portCommandValues       = nullptr;
        IPortableDeviceValues*                  portCommandReturn       = nullptr;
		IPortableDeviceCapabilities*            deviceCapabilities      = nullptr;
		IPortableDeviceKeyCollection*           deviceCommands          = nullptr;
        IPortableDeviceKeyCollection*           deviceProperties        = nullptr;
		IPortableDevicePropVariantCollection*   deviceFuncCategories    = nullptr;

        if ( FAILED( hr = get_device( lpMemIDs[ idx ], bImpersonate, 0, &portDevice ) ) ) {
            DEBUG_PRINT_ERR( "get_device", hr );
            goto _End;
        }

		if ( FAILED( hr = portDevice->Capabilities( &deviceCapabilities ) ) ) {
            DEBUG_PRINT_ERR( "IPortableDevice::Capabilities", hr );
			goto _End;
		}

		if ( FAILED( hr = deviceCapabilities->GetFunctionalCategories( &deviceFuncCategories ) ) ) {
            DEBUG_PRINT_ERR( "IPortableDeviceCapabilities::GetFunctionalCategories", hr );
			goto _End;
		}

		deviceFuncCategories->GetCount( &num_funcs );

		for ( int j = 0; j < num_funcs; j++ ) {

			PROPVARIANT pv = { 0 };

			if ( SUCCEEDED( hr = deviceFuncCategories->GetAt( j, &pv ) ) ) {

				if ( pv.vt == VT_CLSID && pv.puuid != nullptr ) {

					wchar_t buf[39];
					IPortableDevicePropVariantCollection* deviceFuncObjs = nullptr;

					OLE32$StringFromGUID2( *pv.puuid, buf, 39 );
                    BeaconPrintToStreamW( L"[%d] Functional Cat. %ws\n", idx, buf );

					if ( SUCCEEDED( hr = deviceCapabilities->GetFunctionalObjects( *pv.puuid, &deviceFuncObjs ) ) ) {

						if ( SUCCEEDED( hr = deviceFuncObjs->GetCount( &num_objs ) ) ) {

							for ( int k = 0; k < num_objs; k++ ) {

								PROPVARIANT pv2 = { 0 };

								if ( SUCCEEDED( hr = deviceFuncObjs->GetAt( k, &pv2 ) ) ) {

									if ( pv2.vt == VT_LPWSTR && pv2.pwszVal != nullptr ) {
                                        BeaconPrintToStreamW( L"[%d] Functional Obj. %ws\n", idx, pv2.pwszVal );
									}
								}

								OLE32$PropVariantClear( &pv2 );
							}
						}
					}

					SAFE_RELEASE_CPP( deviceFuncObjs );
				}
			}

			OLE32$PropVariantClear( &pv );
		}

		if ( FAILED( hr = deviceCapabilities->GetSupportedCommands( &deviceCommands ) ) ) {
            DEBUG_PRINT_ERR( "IPortableDeviceCapabilities::GetSupportedCommands", hr );
			goto _End;
		}

		deviceCommands->GetCount( &num_cmds );
        BeaconPrintToStreamW( L"[%d] Supported CMDs (%d)\n", idx, num_cmds );

		for ( int l = 0; l < num_cmds; l++ ) {

			PROPERTYKEY pk = { 0 };

			if ( SUCCEEDED( hr = deviceCommands->GetAt( l, &pk ) ) ) {

                if ( MSVCRT$memcmp( &last_guid, &pk.fmtid, sizeof(GUID) ) != 0 ) {

					wchar_t buf[39];
					OLE32$StringFromGUID2( pk.fmtid, buf, 39 );
                    BeaconPrintToStreamW( L"\n    GUID: %ws\n    CMDs: %d", buf, pk.pid );
				}
				else {
                    BeaconPrintToStreamW( L" %d", pk.pid );
				}

                if ( l == num_cmds - 1 ) {
                    BeaconPrintToStreamW( L"\n" );
                }

				last_guid = pk.fmtid;
			}
		}

        last_guid = { 0 };

        if ( FAILED( hr = OLE32$CoCreateInstance( g_PortableDeviceValues, nullptr, CLSCTX_INPROC_SERVER, g_IPortableDeviceValues, (void**)&portCommandValues ) ) ) {
            DEBUG_PRINT_ERR( "CoCreateInstance", hr );
		    goto _End;
	    }

        // WPD_PROPERTY_COMMON_COMMAND_CATEGORY
        pk_cmd.fmtid = { 0xF0422A9C, 0x5DC8, 0x4440, { 0xB5, 0xBD, 0x5D, 0xF2, 0x88, 0x35, 0x65, 0x8A } };
        pk_cmd.pid   = 1001;

        // WPD_CATEGORY_OBJECT_PROPERTIES
        portCommandValues->SetGuidValue( pk_cmd, { 0x9E5582E4, 0x0814, 0x44E6, { 0x98, 0x1A, 0xB2, 0x99, 0x8D, 0x58, 0x38, 0x04 } } );

        // WPD_PROPERTY_COMMON_COMMAND_ID
        pk_cmd.pid   = 1002;

        // WPD_COMMAND_OBJECT_PROPERTIES_GET_SUPPORTED
        portCommandValues->SetUnsignedIntegerValue( pk_cmd, 2 );

        // WPD_PROPERTY_OBJECT_PROPERTIES_OBJECT_ID
        pk_cmd.fmtid = { 0x9E5582E4, 0x0814, 0x44E6, { 0x98, 0x1A, 0xB2, 0x99, 0x8D, 0x58, 0x38, 0x04 } };
        pk_cmd.pid   = 1001;
        portCommandValues->SetStringValue( pk_cmd, WPD_DEVICE_OBJECT_ID );

        if ( FAILED( hr = portDevice->SendCommand( 0, portCommandValues, &portCommandReturn ) ) ) {
            DEBUG_PRINT_ERR( "IPortableDevice::SendCommand", hr );
            goto _End;
        }

        // WPD_PROPERTY_COMMON_HRESULT
        pk_cmd.fmtid = { 0xF0422A9C, 0x5DC8, 0x4440, { 0xB5, 0xBD, 0x5D, 0xF2, 0x88, 0x35, 0x65, 0x8A } };
        pk_cmd.pid   = 1003;
        portCommandReturn->GetErrorValue( pk_cmd, &hr );
        if ( FAILED( hr ) ) {
            goto _End;
        }

        // WPD_PROPERTY_OBJECT_PROPERTIES_PROPERTY_KEYS
        pk_cmd.fmtid = { 0x9E5582E4, 0x0814, 0x44E6, { 0x98, 0x1A, 0xB2, 0x99, 0x8D, 0x58, 0x38, 0x04 } };
        pk_cmd.pid   = 1002;

        if ( FAILED( hr = portCommandReturn->GetIUnknownValue( pk_cmd, &iunknown ) ) || !iunknown ) {
            DEBUG_PRINT_ERR( "IPortableDeviceValues::GetIUnknownValue", hr );
            goto _End;
        }

        if ( FAILED( hr = iunknown->QueryInterface( g_IPortableDeviceKeyCollection, (void**)&deviceProperties ) ) ) {
            DEBUG_PRINT_ERR( "IUnknown::QueryInterface", hr );
            goto _End;
        }

        deviceProperties->GetCount( &num_prop );
        BeaconPrintToStreamW( L"[%d] Supported Properties (%d)\n", idx, num_prop );

        for ( int m = 0; m < num_prop; m++ ) {

            PROPERTYKEY pk = { 0 };

            if ( SUCCEEDED( hr = deviceProperties->GetAt( m, &pk ) ) ) {

                if ( MSVCRT$memcmp( &last_guid, &pk.fmtid, sizeof(GUID) ) != 0 ) {

                    wchar_t buf[39];
                    OLE32$StringFromGUID2( pk.fmtid, buf, 39 );
                    BeaconPrintToStreamW( L"\n    GUID: %ws\n    CMDs: %d", buf, pk.pid );
                }
                else {
                    BeaconPrintToStreamW( L" %d", pk.pid );
                }

                if ( m == num_prop - 1 ) {
                    BeaconPrintToStreamW( L"\n" );
                }

                last_guid = pk.fmtid;
            }
        }

_End:
        if ( FAILED( hr ) ) {
            BeaconPrintToStreamW( L"[!] Capabilities: 0x%0.8X\n", hr );
        }
        SAFE_RELEASE_CPP( deviceProperties );
        SAFE_RELEASE_CPP( iunknown );
        SAFE_RELEASE_CPP( portCommandValues );
        SAFE_RELEASE_CPP( portCommandReturn );
		SAFE_RELEASE_CPP( deviceCapabilities );
		SAFE_RELEASE_CPP( deviceCommands );
		SAFE_RELEASE_CPP( portDevice );
	};

    BeaconPrintToStreamW( L"[*] Number of devices: %d\n", cnt );

    for ( int i = 0; i < cnt; i++ ) {
        get_info( i, L"Description", &IPortableDeviceManager::GetDeviceDescription );
        get_info( i, L"Friendly name", &IPortableDeviceManager::GetDeviceFriendlyName );
        get_info( i, L"Manufacturer", &IPortableDeviceManager::GetDeviceManufacturer );
        get_capabilities( i );
        BeaconOutputStreamW();
    }
}


void go(char* args, int len) {

    HRESULT     hr;
    DWORD       cnt;
    int         cmd         = 0;
    bool        bInit       = false,
                bImp        = false;
    wchar_t**   lpMemIDs    = nullptr;

    IPortableDeviceManager* deviceManager = nullptr;

    datap Parser;
    BeaconDataParse( &Parser, args, len );

    cmd     = BeaconDataInt( &Parser );
    bImp    = (bool)BeaconDataInt( &Parser );

    if ( SUCCEEDED( hr = OLE32$CoInitializeEx( nullptr, COINIT_MULTITHREADED ) ) ) {
        bInit = true;
    } else {
        goto _End;
    }

    if ( FAILED( hr = OLE32$CoCreateInstance( g_PortableDeviceManager, nullptr, CLSCTX_INPROC_SERVER, g_IPortableDeviceManager, (void**)&deviceManager ) ) ) {
        DEBUG_PRINT_ERR( "CoCreateInstance", hr );
		goto _End;
	}

    if ( FAILED( hr = deviceManager->GetDevices( nullptr, &cnt ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDeviceManager::GetDevices", hr );
		goto _End;
	}

    if ( cnt == 0 ) {
        hr = TYPE_E_ELEMENTNOTFOUND;
        goto _End;
    }

    lpMemIDs = reinterpret_cast<wchar_t**>( KERNEL32$HeapAlloc( KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, cnt * sizeof(wchar_t*) ) );

    if ( FAILED( hr = deviceManager->GetDevices( lpMemIDs, &cnt ) ) ) {
        DEBUG_PRINT_ERR( "IPortableDeviceManager::GetDevices", hr );
		goto _End;
	}

    switch ( cmd ) {
        case 2:
            hr = list_files( lpMemIDs[ BeaconDataInt( &Parser ) ], bImp, Parser );
            break;
        case 4:
            enum_devices( deviceManager, lpMemIDs, bImp, cnt );
            break;
        case 8:
            hr = get_property( lpMemIDs[ BeaconDataInt( &Parser ) ], bImp, Parser );
            break;
        default:
            break;
    }

    for ( int i = 0; i < cnt; i++ ) {
        OLE32$CoTaskMemFree( lpMemIDs[ i ] );
    }


_End:
    SAFE_RELEASE_CPP( deviceManager );

    if ( lpMemIDs ) {
        KERNEL32$HeapFree( KERNEL32$GetProcessHeap(), 0, lpMemIDs );
    }

    if ( FAILED( hr ) ) {
        BeaconPrintToStreamW( L"[!] 0x%0.8X\n", hr );
    }

    BeaconOutputStreamW();

    if ( bInit ) {
        OLE32$CoUninitialize();
    }
}
