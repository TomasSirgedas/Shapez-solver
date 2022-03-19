#include "trace.h"
#include <string>
#include <Windows.h>

namespace std
{
   class trace_streambuf : public basic_streambuf<char, char_traits<char>>
   {
   public:
      trace_streambuf() {}
      int_type overflow( int_type x )
      {
         buffer += (char)x;
         if ( x == 10 )
         {
            cout << buffer;
            ::OutputDebugStringA( buffer.c_str() );
            buffer.clear();
         }
         return 0;
      }
   private:
      string buffer;
   };
   std::trace_streambuf trace_buf;
   std::basic_ostream<char, std::char_traits<char>> trace( &trace_buf );
}