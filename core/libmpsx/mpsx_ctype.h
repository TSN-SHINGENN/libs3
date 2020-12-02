#ifndef INC_MPSX_CTYPE_H
#define INC_MPSX_CTYPE_H

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

int mpsx_ascii_isprint(const int c);
int mpsx_ascii_isspace(const int c);
int mpsx_ascii_isdigit(const int c);
int mpsx_ascii_isalpha(const int c);
int mpsx_ascii_islower(const int c);
int mpsx_ascii_isupper(const int c);

int mpsx_ascii_toupper(const int c);
int mpsx_ascii_tolower(const int c);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPSX_CTYPE_H */
