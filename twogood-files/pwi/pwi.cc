/* $Id$ */
#include <fstream>
#include <iostream>
#include <stdint.h>

#define VERBOSE 0

using namespace std;

#define PWI_MAGIC         "{\\pwi"
#define PWI_MAGIC_SIZE    5

#define FONT_ENTRY_SIZE   0x50

#define PARAGRAPH_ENTRY_SIZE  0x8

static void dump(const char *desc, void* data, size_t len)/*{{{*/
{
  uint8_t* buf = (uint8_t*)data;
  size_t i, j;
  char hex[8 * 3 + 1];
  char chr[8 + 1];

  fprintf(stderr, "%s (%d bytes):\n", desc, len);
  for (i = 0; i < len + 7; i += 8) {
    for (j = 0; j < 8; j++) 
      if (j + i >= len) {
        hex[3*j+0] = ' ';
        hex[3*j+1] = ' ';
        hex[3*j+2] = ' ';
        chr[j] = ' ';
      } else {
        uint8_t c = buf[j + i];
        const char *hexchr = "0123456789abcdef";
        hex[3*j+0] = hexchr[(c >> 4) & 0xf];
        hex[3*j+1] = hexchr[c & 0xf];
        hex[3*j+2] = ' ';
        if (c > ' ' && c <= '~')
          chr[j] = c;
        else
          chr[j] = '.';
      }
    hex[8*3] = '\0';
    chr[8] = '\0';
    fprintf(stderr, "  %04x: %s %s\n", i, hex, chr);
  }
}/*}}}*/

static unsigned read16(istream& input)
{
  unsigned value = input.get();
  value |= (unsigned)input.get() << 8;
  return value;
}

void text(istream& input)/*{{{*/
{
  input.seekg(0x4, ios_base::cur);

  /*unsigned plain_text_size      =*/ read16(input);
  unsigned total_size           = read16(input);

  //cerr << "Total size: 0x" << hex << total_size << endl;

  input.seekg(0x14, ios_base::cur);

  uint8_t* data = new uint8_t[total_size];

  unsigned paragraph_data_offset = input.tellg();
  input.read((char*)data, total_size);

  for (unsigned j = 0; j < total_size; j++)
  {
    if (data[j] == 0x00)
    {
      cerr << "Warning, data byte is NULL!" << endl;
    }
    else if (data[j] & 0x80)
    {
      switch (data[j])
      {
        case 0xe5:    // Select font
          j+=2;
          break;

        case 0xe6:    // Font size
          j+=2;
          break;

        case 0xe7:    // Color
          j+=2;
          break;

        case 0xe8:    // Font weight
          j+=2;
          break;

        case 0xe9:    // Italic
          j++;
          break;

        case 0xea:    // Underline
          j++;
          break;

        case 0xeb:    // Strikethrough
          j++;
          break;

        case 0xec:    // Highlight
          j++;
          break;

        case 0xef:    // XXX unknown
          j+=3;
          break;

        case 0xc1:    // Character code
          j++;
          cout << (char)data[j];
          break;

        case 0xc2:    // Unknown
          j+=2;
          break;

        case 0xc4:    // Control code
          j++;
          switch (data[j])
          {
            case 0x00:   // End of paragraph
              break;

            case 0x04:   // Tab character
              cout << '\t';
              break;

            case 0x19:  // XXX unknown
              j+=2;
              break;

            case 0x1b:  // XXX unknown
              j+=6;
              break;

            default:
              cerr << "Unknown control char: 0x" << hex << (unsigned)data[j] <<  
                " at offset 0x" << paragraph_data_offset + j << endl;
              break;
          }
          break;

        case 0xc5:  // Reference to embedded data?
          j+=2;
          break;

        case 0xf1:
          j+=7;
          break;

        default:
          cerr << "Unknown code: 0x" << hex << (unsigned)data[j] << 
            " at offset 0x" << paragraph_data_offset + j << endl;
          abort();
      }
    }
    else
      cout << (char)data[j];
  }
  cout << endl;

  delete[] data;
}/*}}}*/

void drawing(istream& input)
{
  // Skip unknown
  input.seekg(0xc, ios_base::cur);

  unsigned drawing_size = read16(input);

  cerr << "Drawing: 0x" << hex << drawing_size << 
    " bytes at offset 0x" << hex << input.tellg() << endl;
  
  // Skip drawing
  input.seekg(drawing_size, ios_base::cur);
}

void align(istream& input, streampos start_offset)
{
  //cerr << "Start offset: 0x" << hex << start_offset << endl;
  //cerr << "Current offset: 0x" << hex << input.tellg() << endl;
  unsigned align = 4 - ((input.tellg() - start_offset) & 3);
  //cerr << "Align: " << dec << align << endl;
  input.seekg(align, ios_base::cur);
}

int main(int argc, char**argv)
{
  ifstream input(argv[1], ofstream::binary);

  //
  // Header
  //

  char magic[PWI_MAGIC_SIZE];
  input.read(magic, PWI_MAGIC_SIZE);

  if (0 != memcmp(magic, PWI_MAGIC, PWI_MAGIC_SIZE)) 
  {
    cerr << "Not a PWI file" << endl;
    return 1;
  }

  // Skip to font count
  input.seekg(0x13, ios_base::cur);

  unsigned font_count = read16(input);
  //cerr << "Font count: " << dec << font_count << endl;

  // Skip to font table
  input.seekg(0xa, ios_base::cur);

  //cerr << "Font table starts at offset 0x" << hex << input.tellg() << endl;

  // Skip font table
  input.seekg(font_count * FONT_ENTRY_SIZE, ios_base::cur);

  //cerr << "Font table ends at offset 0x" << hex << input.tellg() << endl;

  // Skip to paragraph count 1
  input.seekg(0xba, ios_base::cur);
  unsigned paragraph_count = read16(input);

  // Skip to paragraph count 2
  input.seekg(0x2, ios_base::cur);
  /*unsigned paragraph_count_2 =*/ read16(input);

  // Skip to paragraph count 3
  input.seekg(0x6, ios_base::cur);
  /*unsigned paragraph_count_3 =*/ read16(input);

#if VERBOSE
  cerr << "Paragraph count: " << dec << 
    paragraph_count /*<< " = " << 
                      paragraph_count_2 << " = " << 
                      paragraph_count_3*/ << endl;
#endif

  input.seekg(0x6, ios_base::cur);

  //cerr << "Paragraph index starts at offset 0x" << hex << input.tellg() << endl;

#if 0
  for (unsigned i = 0; i < paragraph_count; i++)
  {
    cerr.fill(' ');
    cerr.width(2);
    cerr << dec << i << ": ";
    cerr.fill('0');
    for (int j = 0; j < 4; j++)
    {
      cerr.width(4);
      cerr << hex << read16(input) << ' ';
    }
    cerr << endl;
  }
#else
  input.seekg(paragraph_count * PARAGRAPH_ENTRY_SIZE, ios_base::cur);
#endif

  //cerr << "Paragraph index ends at offset 0x" << hex << input.tellg() << endl;

  input.seekg(2, ios_base::cur);

  if (0x0 != read16(input))
    cerr << "Not 0x0!" << endl;

  for (unsigned i = 0; i < paragraph_count; i++)
  {
#if VERBOSE
    cerr << "Paragraph " << dec << i << " starts at offset 0x" << hex << input.tellg() << endl;
#endif

    unsigned code = read16(input);

    streampos start_offset = input.tellg();

    switch (code)
    {
      case 0x41:
        text(input);
        
        align(input, start_offset);
        
        code = read16(input);
        if (code != 0x42)
        {
          cerr << "Not 0x42 but 0x" << hex << code << " at offset 0x" 
            << hex << ((unsigned)input.tellg() - 2) << endl;
          abort();
        }
        
        start_offset = input.tellg();
        input.seekg(0xe, ios_base::cur);
        break;

      case 0x43:
        drawing(input);
        break;

      default:
        cerr << "Unknown code 0x" << hex << code << " at 0x" <<
          (input.tellg() - (streampos)2) << endl;
        abort();
    }

    input.seekg(4 - ((input.tellg() - start_offset) & 3), ios_base::cur);

#if VERBOSE
    cerr << "Paragraph " << dec << i << " ends at offset 0x" << hex << input.tellg() << endl;
#endif
  }

  if (0x82 != read16(input))
    cerr << "Not 0x82!" << endl;

#if VERBOSE
  cerr << "Decoding ends at offset 0x" << hex << input.tellg() << endl;
#endif

  return 0;
}
