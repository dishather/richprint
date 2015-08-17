// Decode and print Rich header from executable files.

#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>

using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::map;
using std::string;
using std::istringstream;

namespace {

//
typedef map<unsigned long, string> StrMap;

//
unsigned short readWord( std::fstream &file, std::streampos pos )
{
    file.seekg( pos );
    unsigned short res = 0;
    unsigned char buf[2];
    file.read( reinterpret_cast<char*>( buf ), 2 );
    res = (unsigned short)buf[0] |
        (unsigned short)buf[1] << 8;
    return res;
}

//
unsigned long readDword( std::fstream &file, std::streampos pos )
{
    file.seekg( pos );
    unsigned long res = 0;
    unsigned char buf[4];
    file.read( reinterpret_cast<char*>( buf ), 4 );
    res = (unsigned short)buf[0] |
        (unsigned short)buf[1] << 8 |
        (unsigned short)buf[2] << 16 |
        (unsigned short)buf[3] << 24;
    return res;
}

//
char const *getMachineType( unsigned short machineId )
{
    char const *machine = "Unknown";
    // from https://msdn.microsoft.com/en-us/windows/hardware/gg463119.aspx
    switch( machineId )
    {
        case 0x8664:
            machine = "x64";
            break;
        case 0x14c:
            machine = "x32";
            break;
        case 0x1d3:
            machine = "Matsushita AM33";
            break;
        case 0x1c0:
            machine = "ARM LE";
            break;
        case 0x1c4:
            machine = "ARMv7+ Thumb";
            break;
        case 0xaa64:
            machine = "ARMv8 64bit";
            break;
        case 0xebc:
            machine = "EFI bytecode";
            break;
        case 0x200:
            machine = "Intel Itanium";
            break;
        case 0x9041:
            machine = "Mitsubishi M32R LE";
            break;
        case 0x266:
            machine = "MIPS16";
            break;
        case 0x366:
            machine = "MIPS w/FPU";
            break;
        case 0x466:
            machine = "MIPS16 w/FPU";
            break;
        case 0x1f0:
            machine = "PowerPC LE";
            break;
        case 0x1f1:
            machine = "PowerPC w/FPU";
            break;
        case 0x166:
            machine = "MIPS LE";
            break;
        case 0x1a2:
            machine = "Hitachi SH3";
            break;
        case 0x1a3:
            machine = "Hitachi SH3 DSP";
            break;
        case 0x1a6:
            machine = "Hitachi SH4";
            break;
        case 0x1a8:
            machine = "Hitachi SH5";
            break;
        case 0x1c2:
            machine = "ARM or Thumb";
            break;
        case 0x169:
            machine = "MIPS LE WCE v2";
            break;
    }
    return machine;
}

//
void decodeRichheader(
    std::fstream   &file,
    std::streampos  start,
    std::streampos  endp,
    unsigned long   key,
    StrMap const   &descriptions )
{
    std::streampos cur = start + std::streampos( 16 );
    endp -= std::streampos( 8 );
    cout << "@comp.id   id version count   description" << endl;
    while( cur < endp )
    {
        unsigned long ver = readDword( file, cur ) ^ key;
        unsigned long id = ver >> 16;
        unsigned long minver = ver & 0xFFFF;
        cout << std::hex << std::setw( 8 ) << std::setfill( '0' ) << ver <<
            " " << std::setw( 4 ) << std::setfill( ' ' ) << id <<
            " " << std::dec << std::setw( 6 ) << minver << " ";
        cur += std::streampos( 4 );
        cout << std::setw( 5 ) << ( readDword( file, cur ) ^ key );
        cur += std::streampos( 4 );

        // any description?
        StrMap::const_iterator i = descriptions.find( ver );
        if( i != descriptions.end() )
        {
            cout << ' ' << (*i).second;
        }
        cout << endl;
    }
}

//
void getRichheader( char const *fname, StrMap const &descriptions )
{
    std::fstream file( fname, ios::in | ios::binary );
    if(! file.good() )
    {
        cerr << "Failed to open file " << fname << endl;
        return;
    }

    cout << "Processing " << fname << endl;

    // Check MZ header.
    unsigned short mz = readWord( file, 0 );
    if( mz != 0x5a4d )
    {
        cerr << "No MZ header - not an executable." << endl;
        cerr << "Magic is: " << std::hex << mz << endl;
        return;
    }
    // cout << "Found MZ header." << endl;

    // Get metrics from header.
    unsigned short num_relocs   = readWord( file, 6 );
    unsigned short header_para  = readWord( file, 8 );
    if( header_para < 4 )
    {
        cerr << "Too few paragraphs in DOS header: " << header_para
            << ", not a PE executable." << endl;
        return;
    }

    // cout << "Size of DOS header: " << header_para << " paragraphs." << endl;

    unsigned short reloc_offset = readWord( file, 0x18 );
    unsigned short pe_offset    = readWord( file, 0x3c );
    if( pe_offset < header_para * 16 )
    {
        cerr << "PE offset is too small: " << pe_offset
            << ", not a PE executable." << endl;
        return;
    }
    // Check PE signature.
    unsigned long t = readDword( file, pe_offset );
    if( t != 0x4550 )
    {
        cerr << "No PE header signature: " << std::hex << t
            << ", not a PE executable." << endl;
        return;
    }

    // Define if executable is 32 or 64-bit
    cout << "Target machine: " <<
        getMachineType( readWord( file, pe_offset + 4 ) ) << endl;

    // Calculate the offset of the DOS stub.
    std::streampos dosexe_offset = reloc_offset;
    // If we have relocations in dos stub (unlikely, but why not?), add them
    // up too.
    if( num_relocs > 0 )
    {
        dosexe_offset += 4 * num_relocs;
    }
    // Align on paragraph boundary.
    if( dosexe_offset % 16 )
    {
        dosexe_offset += 16 - ( dosexe_offset % 16 );
    }

    // Now we have dosexe_offset where stub begins, and pe_offset where all
    // extra data must end. Seek the Rich header.
    std::streampos roffset = 0;
    for (std::streampos i = dosexe_offset; i < pe_offset; i += 4 )
    {
        t = readDword( file, i );
        if( t == 0x68636952 )
        {
            roffset = i + std::streampos( 4 );
            break;
        }
    }
    if( roffset == std::streampos ( 0 ) )
    {
        cerr << "Rich header not found." << endl;
        return;
    }

    unsigned long key = readDword( file, roffset );
    // cout << "Found Rich header signature. Key is 0x" << std::hex << key <<
    //    endl;

    std::streampos doffset = 0;
    for (std::streampos i = dosexe_offset; i < pe_offset; i += 4 )
    {
        t = readDword( file, i ) ^ key;
        if( t == 0x536E6144 )
        {
            doffset = i;
            break;
        }
    }
    if( doffset == std::streampos ( 0 ) )
    {
        cerr << "Rich header's DanS token not found." << endl;
        return;
    }
    roffset += 4; // skip CRC aka Key.

    if( roffset > pe_offset )
    {
        cerr << "Calculated end offset runs into PE header: 0x" << std::hex
            << roffset << endl;
        return;
    }

    // cout << "Found DanS token at offset 0x"<< std::hex << doffset
        // << ", end offset is 0x" << roffset << "." << endl;

    decodeRichheader( file, doffset, roffset, key, descriptions );
}

//
void loadDescriptions( char const *fname, StrMap &descriptions )
{
    std::fstream file( fname, ios::in );
    if(! file.good() )
    {
        return;
    }
    // Read file line by line.
    while( !file.eof() )
    {
        string str;
        getline( file, str );
        if( str.length() > 8 && str[0] != '#' )
        {
            istringstream is( str );
            unsigned long id;
            is >> std::hex >> id;
            string desc;
            is.get(); // skip space
            getline( is, desc );
            // sanity check: do not allow duplicate comp.id's
            if( descriptions.find( id ) != descriptions.end() )
            {
                cerr << "!!! Duplicate comp.id:\n" << std::hex << std::setw(8)
                    << std::setfill( '0' ) << id << ' ' << descriptions[id]
                    << endl;
                cerr << std::hex << std::setw(8) << std::setfill( '0' ) << id
                    << ' ' << desc << endl;
                continue;
            }
            descriptions[id] = desc;
        }
    }
}

} // anonymous namespace ends

//
int main( int argc, char *argv[] )
{
    if( argc < 2 )
    {
        cout << "Rich header decoder. Usage:\n\n" << argv[0] << " file ..."
            << endl;
        cout << "\nRich headers can be found in executable files, DLLs, "
            "and other binary files\ncreated by Microsoft linker." << endl;
        return 0;
    }

    StrMap descriptions;
    loadDescriptions( "comp_id.txt", descriptions );

    for( int i = 1; i < argc; ++i )
    {
        getRichheader( argv[i], descriptions );
    }
}
