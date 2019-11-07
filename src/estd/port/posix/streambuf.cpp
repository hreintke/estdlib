#include "../identify_platform.h"

#if defined(ESTD_POSIX) || defined(ESP_OPEN_RTOS)

#include "../../streambuf.h"
#include "streambuf.h"

namespace estd { namespace internal { namespace impl {

//streamsize native_streambuf<char, posix_stream_t, ::std::char_traits<char> >::
streamsize posix_streambuf::
    xsputn(const char_type* s, streamsize count, bool blocking)
{
    return fwrite(s, sizeof(char), count, &this->stream);
}


//template<>
streamsize posix_streambuf::
    xsgetn(char_type* s, streamsize count, bool blocking)
{
    return fread(s, sizeof(char), count, &this->stream);
}

//template<>
int posix_streambuf::sputc(char ch, bool blocking)
{
    return fputc(ch, &this->stream);
}


//template<>
int posix_streambuf::sbumpc(bool blocking)
{
    return fgetc(&this->stream);
}

//template<>
int posix_streambuf::sgetc(bool blocking)
{
    int c = fgetc(&this->stream);
    ungetc(c, &this->stream);
    return c;
}

} } }

#endif
