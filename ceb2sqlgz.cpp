/*
Copyright © 2022 mjk <https://github.com/Yuubi-san>.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along
with this program.  If not, see <https://www.gnu.org/licenses/>.


Additional permission under GNU AGPL version 3 section 7

If you modify this Program, or any covered work, by linking or combining
it with OpenSSL project's OpenSSL library (or a modified version of that
library), containing parts covered by the terms of the OpenSSL or SSLeay
licenses, the licensors of this Program grant you additional permission
to convey the resulting work.  Corresponding Source for a non-source form
of such a combination shall include the source code for the parts of OpenSSL
used as well as that of the covered work.
*/

#include <limits>
#include <chrono>
#include <cuchar>
#include <codecvt>
#include <fstream>
#include <iostream>
#include <termios.h>
#include <openssl/evp.h>
#include "unaesgcm/unaesgcm.hpp"

#if __cpp_lib_char8_t
  using std::u8string;
  using std::u8string_view;
#else
  using u8string      = std::basic_string     <unsigned char>;
  using u8string_view = std::basic_string_view<u8string::value_type>;
#endif

template<typename> struct smaller;
template<> struct smaller<std::uint16_t> { using type = std::uint8_t;  };
template<> struct smaller<std::uint32_t> { using type = std::uint16_t; };
template<> struct smaller<std::uint64_t> { using type = std::uint32_t; };

template<typename T>
static auto readi( std::istream &in )
{
  using S = typename smaller<T>::type;
  const auto
    hi = readi<S>(in),
    lo = readi<S>(in);
  return static_cast<T>( T{hi} << std::numeric_limits<S>::digits | lo );
}

template<typename T>
static auto read( std::istream &in )
{
  static_assert( std::is_standard_layout_v<T> and std::is_trivial_v<T> );
  T pod;
  in.read( reinterpret_cast<char *>(&pod), sizeof pod );
  return pod;
}

template<>
auto readi<std::uint8_t>( std::istream &in )
{
  return read<std::uint8_t>( in );
}

template<>
auto read<u8string>( std::istream &in )
{
  u8string s( readi<std::uint16_t>(in), u8'\0' );
  in.read( reinterpret_cast<char *>(data(s)), static_cast<long>(size(s)) );
  return s;
}

static auto readv( std::istream &in, const std::size_t sz )
{
  std::vector<byte> v( sz );
  in.read( reinterpret_cast<char *>(data(v)), static_cast<long>(size(v)) );
  return v;
}

static auto put_sys_time(
  const std::chrono::system_clock::time_point t, const char *const fmt )
{
  const auto tt = std::chrono::system_clock::to_time_t( t );
  return std::put_time( std::localtime(&tt), fmt );
}

static auto derive_key( const u8string_view passphrase,
  const std::array<byte,bits<128>> salt )
{
  constexpr auto max = unsigned{std::numeric_limits<int>::max()};
  if ( size(passphrase) > max )
    throw std::length_error{"passphrase too long"};
  constexpr auto rounds = 1024;
  std::array<byte,bits<128>> key;
  if ( PKCS5_PBKDF2_HMAC_SHA1(
    reinterpret_cast<const char *>(data(passphrase)),
    static_cast<int>(size(passphrase)),
    data(salt), size(salt),
    rounds,
    size(key), data(key)) )
    return key;
  throw std::runtime_error{"PKCS5_PBKDF2_HMAC_SHA1 failed"};
}


static u8string utf_this_utffing_shit( const std::string_view str )
// aka global_locale_encoding_to_utf8
{
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> wc;
  u8string result;

  auto in_pos = data(str);
  const auto ed = in_pos + size(str);
  std::mbstate_t state{};

  while ( in_pos != ed )
  {
    char32_t cp;
    switch ( const auto n = std::mbrtoc32(&cp, in_pos, ed - in_pos, &state) )
    {
      case 0:
        throw std::invalid_argument{"null character in passphrase"};
      case std::size_t{0}-1:
        throw std::invalid_argument{"illegal byte sequence in passphrase"};
      case std::size_t{0}-2:
        throw std::invalid_argument{"incomplet byte sequence in passphrase"};
      case std::size_t{0}-3:
        #if __cpp_lib_unreachable
          std::unreachable();
        #else
          __builtin_unreachable();
        #endif
      default:
        const auto res = wc.to_bytes(cp);
        result.append(
          reinterpret_cast<const u8string::value_type *>(data(res)),
          size(res) );
        in_pos += n;
    }
  }
  return result;
}


int main( const int argc, const char *const *const argv ) try
{
  using ms = std::chrono::milliseconds;

  if ( argc != 2 )
  {
    std::clog <<"usage: "<< (argc ? argv[0] : "ceb2sqlgz") <<" file.ceb\n";
    return 1;
  }

  std::cin.exceptions( std::ios_base::badbit|std::ios_base::failbit );

  std::string passphrase;
  {
    constexpr auto stdin_fd = 0;

    struct restorer { std::optional<::termios> otios; ~restorer() {
      if ( otios and ::tcsetattr(stdin_fd, TCSANOW, &*otios) )
      {
        const auto e = errno;
        std::clog << "error: couldn't re-enable password echoing: "
          "tcsetattr resulted in errno "<< e <<'\n';
      }
    } } r;

    int err = 0;
    if ( ::termios tios; ::tcgetattr(stdin_fd, &tios) == 0 )
    {
      r.otios = tios;
      tios.c_lflag &= ~decltype(tios.c_lflag){ECHO};
      if ( ::tcsetattr(stdin_fd, TCSANOW, &tios) )
      {
        err = errno;
        std::clog << "warning: couldn't disable password echoing: "
          "tcsetattr resulted in errno "<< err <<'\n';
        r.otios = {};
      }
    }
    else
    {
      err = errno;
      if ( err != ENOTTY )
        std::clog << "warning: can't disable password echoing: "
          "tcgetattr resulted in errno "<< err <<'\n';
    }

    if ( err == 0 )
      std::cerr << "Passphrase plz: ";
    else if ( err != ENOTTY )
      std::cerr << "Passphrase plz (will be echoed): ";
    getline( std::cin, passphrase );
    if ( err == 0 )
      std::cerr << "\nk thx\n";
    else if ( err != ENOTTY )
      std::cerr << "k thx\n";
  }
  std::setlocale(LC_ALL, "");
  const auto u8passphrase = utf_this_utffing_shit( passphrase );
  std::setlocale(LC_ALL, "C");

  std::ifstream in{ argv[1], std::ios_base::binary };
  auto &out = std::cout;

  in.exceptions(
    std::ios_base::badbit|std::ios_base::failbit|std::ios_base::eofbit );

  const auto version  = readi<std::uint32_t>(in);
  if ( version != 1 )
  {
    std::clog << argv[0] <<
      ": error: unsupported CEB version ("<< version <<") or not a CEB file\n";
    return 1;
  }
  const auto producer = read<u8string>(in);
  const auto account  = read<u8string>(in);
  const auto time =
    std::chrono::system_clock::time_point{ms{readi<std::uint64_t>(in)}};
  const auto iv   = readv(in, bits<96>);
  const auto salt = read<std::array<byte,bits<128>>>(in);

  std::clog
    << argv[0] <<": format:   CEB v"<< version <<'\n'
    << argv[0] <<": producer: "<<
      reinterpret_cast<const char *>(producer.c_str()) <<'\n'
    << argv[0] <<": account:  "<<
      reinterpret_cast<const char *>(account.c_str()) <<'\n'
    << argv[0] <<": datetime: "<<
      put_sys_time(time, "%F %T") <<'.'<<
      std::setw(3) << std::setfill('0') <<
      time.time_since_epoch() / ms{1} % 1000 <<
      put_sys_time(time, " %z") <<'\n'
  ;
  dump_hex(std::clog << argv[0] <<": IV:       ", iv) << '\n';
  dump_hex(std::clog << argv[0] <<": salt:     ", salt) << '\n';

  if ( unaesgcm(iv, derive_key(u8passphrase, salt), in, out) )
    return 0;
  else
  {
    std::clog
      << argv[0] <<": authentication failed (wrong password? corrupt file?)\n";
    return 1;
  }
}
catch ( ... )
{
  // Catching only for the sake of unwinding.
  // Let the implementation do the error printing:
  throw;
}
